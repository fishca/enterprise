#include "userInfo.h"

#include "appData.h"   // db_query macro, user_table
#include "databaseLayer/databaseLayer.h"
#include "databaseLayer/databaseErrorCodes.h"
#include "fileSystem/fs.h"
#include "guid.h"

namespace {

// Chunk tags inside binaryData (DB) and the per-record blob (export buffer).
// Same numeric values for both — the on-disk format is shared so import
// from old configurations keeps working.
constexpr unsigned int eBlockPswd = 0x0234530;
constexpr unsigned int eBlockRole = 0x0234540;
constexpr unsigned int eBlockLang = 0x0234550;

void ReadPasswordChunk(const wxMemoryBuffer& buffer, ibUserInfo& info)
{
	ibReaderMemory reader(buffer);
	info.m_strUserGuid     = reader.r_stringZ();
	info.m_strUserName     = reader.r_stringZ();
	info.m_strUserFullName = reader.r_stringZ();
	info.m_strUserPassword = reader.r_stringZ();
}

void ReadRoleChunk(const wxMemoryBuffer& buffer, ibUserInfo& info)
{
	ibReaderMemory reader(buffer);
	const unsigned int count = reader.r_u32();
	info.m_roleArray.reserve(count);
	for (unsigned int idx = 0; idx < count; idx++) {
		ibUserInfo::ibUserRole entry;
		entry.m_strRoleGuid = reader.r_stringZ();
		entry.m_miRoleId    = reader.r_s32();
		info.m_roleArray.emplace_back(std::move(entry));
	}
}

void ReadLanguageChunk(const wxMemoryBuffer& buffer, ibUserInfo& info)
{
	ibReaderMemory reader(buffer);
	info.m_strLanguageGuid = reader.r_stringZ();
	info.m_strLanguageCode = reader.r_stringZ();
}

wxMemoryBuffer WritePasswordChunk(const ibUserInfo& info)
{
	ibWriterMemory writer;
	writer.w_stringZ(info.m_strUserGuid);
	writer.w_stringZ(info.m_strUserName);
	writer.w_stringZ(info.m_strUserFullName);
	writer.w_stringZ(info.m_strUserPassword);
	return writer.buffer();
}

wxMemoryBuffer WriteRoleChunk(const ibUserInfo& info)
{
	ibWriterMemory writer;
	writer.w_u32(info.m_roleArray.size());
	for (const auto& role : info.m_roleArray) {
		writer.w_stringZ(role.m_strRoleGuid);
		writer.w_s32(role.m_miRoleId);
	}
	return writer.buffer();
}

wxMemoryBuffer WriteLanguageChunk(const ibUserInfo& info)
{
	ibWriterMemory writer;
	writer.w_stringZ(info.m_strLanguageGuid);
	writer.w_stringZ(info.m_strLanguageCode);
	return writer.buffer();
}

// Common: populate identity columns from a sys_user result row, then
// crack open the binaryData blob into the same chunks Serialize/Deserialize
// use. Caller has already advanced the cursor onto the row of interest.
void FillFromRow(ibDatabaseResultSet* row, ibUserInfo& info)
{
	info.m_strUserGuid     = row->GetResultString(wxT("guid"));
	info.m_strUserName     = row->GetResultString(wxT("name"));
	info.m_strUserFullName = row->GetResultString(wxT("fullName"));

	wxMemoryBuffer buffer;
	row->GetResultBlob(wxT("binaryData"), buffer);
	ibReaderMemory reader(buffer);

	wxMemoryBuffer chunk;
	if (reader.r_chunk(eBlockPswd, chunk)) ReadPasswordChunk(chunk, info);
	if (reader.r_chunk(eBlockRole, chunk)) ReadRoleChunk    (chunk, info);
	if (reader.r_chunk(eBlockLang, chunk)) ReadLanguageChunk(chunk, info);
}

} // namespace

ibUserInfo ibUserInfo::Read(const ibGuid& userGuid)
{
	ibUserInfo info;
	if (!userGuid.isValid())
		return info;

	ibStatementGuard stmt(db_query,
		db_query->PrepareStatement(wxT("SELECT * FROM %s WHERE guid = ?;"), user_table));
	if (stmt)
		stmt->SetParamString(1, userGuid.str());

	ibResultSetGuard result(db_query,
		stmt ? stmt->RunQueryWithResults() : nullptr);

	if (result && result->Next())
		FillFromRow(result.get(), info);

	return info;
}

ibUserInfo ibUserInfo::Read(const wxString& userName)
{
	ibUserInfo info;
	if (userName.IsEmpty())
		return info;

	ibStatementGuard stmt(db_query,
		db_query->PrepareStatement(wxT("SELECT * FROM %s WHERE name = ?;"), user_table));
	if (stmt)
		stmt->SetParamString(1, userName);

	ibResultSetGuard result(db_query,
		stmt ? stmt->RunQueryWithResults() : nullptr);

	if (result && result->Next())
		FillFromRow(result.get(), info);

	return info;
}

bool ibUserInfo::HasAny()
{
	ibResultSetGuard result(db_query,
		db_query->RunQueryWithResults(wxT("SELECT name FROM %s;"), user_table));

	if (!result) return false;
	return result->Next();
}

std::vector<ibUserInfo::Brief> ibUserInfo::ListAll()
{
	ibResultSetGuard result(db_query,
		db_query->RunQueryWithResults(wxT("SELECT guid, name, fullName FROM %s;"), user_table));

	std::vector<Brief> list;
	if (!result)
		return list;

	while (result->Next()) {
		Brief entry;
		entry.m_strUserGuid     = result->GetResultString(wxT("guid"));
		entry.m_strUserName     = result->GetResultString(wxT("name"));
		entry.m_strUserFullName = result->GetResultString(wxT("fullName"));
		list.emplace_back(std::move(entry));
	}

	return list;
}

bool ibUserInfo::Save(const ibUserInfo& info)
{
	ibStatementGuard stmt(db_query,
		db_query->GetDatabaseLayerType() != DATABASELAYER_FIREBIRD
			? db_query->PrepareStatement(
				wxT("INSERT INTO %s (guid, name, fullName, changed, dataSize, binaryData) VALUES(?, ?, ?, ?, ?, ?) ON CONFLICT (guid) DO UPDATE SET guid = excluded.guid, name = excluded.name, fullName = excluded.fullName, changed = excluded.changed, dataSize = excluded.dataSize, binaryData = excluded.binaryData; "), user_table)
			: db_query->PrepareStatement(
				wxT("UPDATE OR INSERT INTO %s (guid, name, fullName, changed, dataSize, binaryData) VALUES(?, ?, ?, ?, ?, ?) MATCHING (guid);"), user_table)
	);

	if (!stmt)
		return false;

	stmt->SetParamString(1, info.m_strUserGuid);
	stmt->SetParamString(2, info.m_strUserName);
	stmt->SetParamString(3, info.m_strUserFullName);
	stmt->SetParamDate  (4, wxDateTime::Now());

	ibWriterMemory writer;
	writer.w_chunk(eBlockPswd, WritePasswordChunk(info));
	writer.w_chunk(eBlockRole, WriteRoleChunk    (info));
	writer.w_chunk(eBlockLang, WriteLanguageChunk(info));

	stmt->SetParamNumber(5, writer.size());
	stmt->SetParamBlob  (6, writer.pointer(), writer.size());

	const int result = stmt->RunQuery();
	return result != DATABASE_LAYER_QUERY_RESULT_ERROR;
}

void ibUserInfo::Serialize(ibWriterMemory& writer) const
{
	writer.w_stringZ(m_strUserGuid);
	writer.w_stringZ(m_strUserName);
	writer.w_stringZ(m_strUserFullName);
	writer.w_chunk(eBlockPswd, WritePasswordChunk(*this));
	writer.w_chunk(eBlockRole, WriteRoleChunk    (*this));
	writer.w_chunk(eBlockLang, WriteLanguageChunk(*this));
}

ibUserInfo ibUserInfo::Deserialize(ibReaderMemory& reader)
{
	ibUserInfo info;
	info.m_strUserGuid     = reader.r_stringZ();
	info.m_strUserName     = reader.r_stringZ();
	info.m_strUserFullName = reader.r_stringZ();

	wxMemoryBuffer chunk;
	if (reader.r_chunk(eBlockPswd, chunk)) ReadPasswordChunk(chunk, info);
	if (reader.r_chunk(eBlockRole, chunk)) ReadRoleChunk    (chunk, info);
	if (reader.r_chunk(eBlockLang, chunk)) ReadLanguageChunk(chunk, info);
	return info;
}
