////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaobject
////////////////////////////////////////////////////////////////////////////

#include "metaObject.h"
#include "backend/appData.h"

#include "backend/metaData.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"

wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObject, CValue);

#define metaBlock 0x200222
#define helpBlock 0x200224

//*****************************************************************************************
//*                                  MetaObject                                           *
//*****************************************************************************************

void IValueMetaObject::ResetGuid()
{
	m_metaGuid = wxNewUniqueGuid;
}

void IValueMetaObject::ResetId()
{
	if (m_metaData != nullptr) {
		m_metaId = m_metaData->GenerateNewID();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

IBackendMetadataTree* IValueMetaObject::GetMetaDataTree() const
{
	return m_metaData ? m_metaData->GetMetaTree() : nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool IValueMetaObject::BuildNewName()
{
	const wxString& strName = GetName(); bool foundedName = false;
	std::vector<IValueMetaObject*> array;
	if (m_parent != nullptr && m_parent->FillArrayObjectByFilter(array, { GetClassType() })) {
		for (const auto object : array) {
			if (object->GetParent() != GetParent())
				continue;
			if (object != this &&
				stringUtils::CompareString(strName, object->GetName())) {
				foundedName = true;
				break;
			}
		}
	}

	if (foundedName) {
		const wxString& metaPrevName = m_propertyName->GetValueAsString();
		size_t length = metaPrevName.length();
		while (length >= 0 && stringUtils::IsDigit(metaPrevName[--length]));
		const wxString& metaName = m_metaData->GetNewName(GetClassType(), GetParent(), metaPrevName.Left(length + 1));
		SetName(metaName);
		const wxString& metaPrevSynonym = m_propertySynonym->GetValueAsString();
		const wxString& metaSynonym = metaPrevSynonym.Length() > 0 ? stringUtils::GenerateSynonym(metaName) : wxEmptyString;
		SetSynonym(metaSynonym);
	}

	return !foundedName;
}

/////////////////////////////////////////////////////////////////////////////////////////

IValueMetaObject::IValueMetaObject(const wxString& strName, const wxString& synonym, const wxString& comment) : CValue(eValueTypes::TYPE_VALUE, true),
m_methodHelper(new CMethodHelper()), m_metaData(nullptr), m_metaFlags(metaDefaultFlag), m_metaId(0)
{
	m_propertyName->SetValue(strName);
	//m_propertySynonym->SetValue(CBackendLocalization::CreateLocalizationRawLocText(synonym));
	m_propertySynonym->SetValue(synonym);
	m_propertyComment->SetValue(comment);
}

IValueMetaObject::~IValueMetaObject()
{
	wxDELETE(m_methodHelper);
}

bool IValueMetaObject::LoadMeta(CMemoryReader& dataReader)
{
	//Save meta version 
	(void)dataReader.r_u32(); //reserved 

	//Load unique guid 
	wxString strGuid;
	dataReader.r_stringZ(strGuid);
	m_metaGuid = strGuid;

	//Load meta id
	m_metaId = dataReader.r_u32();

	//Load standart fields
	m_propertyName->LoadData(dataReader);
	m_propertySynonym->LoadData(dataReader);
	m_propertyComment->LoadData(dataReader);

	//special info deleted 
	if (dataReader.r_u8()) {
		MarkAsDeleted();
	}

	//load interface 
	if (!LoadInterface(dataReader))
		return false;

	//load roles 
	if (!LoadRole(dataReader))
		return false;

	//load meta 
	wxMemoryBuffer meta_buffer;
	if (!dataReader.r_chunk(metaBlock, meta_buffer))
		return false;

	CMemoryReader metaObjectReader(meta_buffer);
	metaObjectReader.r_u32(); //reserved flags
	if (!LoadData(metaObjectReader))
		return false;

	//load help 
	wxMemoryBuffer help_buffer;
	if (!dataReader.r_chunk(helpBlock, help_buffer))
		return false;
	
	CMemoryReader helpReader(help_buffer);
	m_strHelpContent = helpReader.r_stringZ();
	return true;
}

bool IValueMetaObject::SaveMeta(CMemoryWriter& dataWritter)
{
	//save meta version 
	dataWritter.w_u32(version_oes_last); //reserved 

	//save unique guid
	dataWritter.w_stringZ(m_metaGuid);

	//save meta id 
	dataWritter.w_u32(m_metaId);

	//save standart fields
	m_propertyName->SaveData(dataWritter);
	m_propertySynonym->SaveData(dataWritter);
	m_propertyComment->SaveData(dataWritter);

	//special info deleted
	dataWritter.w_u8(IsDeleted());

	//save interface 
	if (!SaveInterface(dataWritter))
		return false;

	//save roles 
	if (!SaveRole(dataWritter))
		return false;

	//save meta 
	CMemoryWriter metaObjectWritter;
	metaObjectWritter.w_u32(0); //reserved flags
	if (!SaveData(metaObjectWritter))
		return false;

	dataWritter.w_chunk(metaBlock, metaObjectWritter.buffer());

	//save help 
	CMemoryWriter helpWritter;
	helpWritter.w_stringZ(m_strHelpContent);
	dataWritter.w_chunk(helpBlock, helpWritter.buffer());
	return true;
}

bool IValueMetaObject::LoadMetaObject(IMetaData* metaData, CMemoryReader& dataReader)
{
	m_metaData = metaData;

	if (!LoadMeta(dataReader))
		return false;

	if (!OnLoadMetaObject(metaData))
		return false;

	return true;
}

bool IValueMetaObject::SaveMetaObject(IMetaData* metaData, CMemoryWriter& dataWritter, int flags)
{
	bool saveToFile = (flags & saveToFileFlag) != 0;

	if (m_metaData != metaData)
		return false;

	if (!SaveMeta(dataWritter))
		return false;

	if (!saveToFile &&
		!OnSaveMetaObject(flags)) {
		return false;
	}

	return true;
}

bool IValueMetaObject::DeleteMetaObject(IMetaData* metaData)
{
	if (m_metaData != metaData)
		return false;

	return DeleteData();
}

bool IValueMetaObject::CreateMetaTable(IMetaDataConfiguration* srcMetaData, int flags)
{
	return CreateAndUpdateTableDB(srcMetaData, nullptr, flags);
}

bool IValueMetaObject::UpdateMetaTable(IMetaDataConfiguration* srcMetaData, IValueMetaObject* srcMetaObject)
{
	return CreateAndUpdateTableDB(srcMetaData, srcMetaObject, updateMetaTable);
}

bool IValueMetaObject::DeleteMetaTable(IMetaDataConfiguration* srcMetaData)
{
	return CreateAndUpdateTableDB(srcMetaData, this, deleteMetaTable);
}

bool IValueMetaObject::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	GenerateGuid();
	wxASSERT(metaData);
	m_metaId = metaData->GenerateNewID();
	m_metaData = metaData;
#ifdef DEBUG  
	wxLogDebug(wxT("* Create metaData object %s with id %i"),
		GetClassName(), GetMetaID()
	);
#endif
	return true;
}

bool IValueMetaObject::OnLoadMetaObject(IMetaData* metaData)
{
	m_metaData = metaData;
	return true;
}

bool IValueMetaObject::OnDeleteMetaObject()
{
	return true;
}

bool IValueMetaObject::OnAfterCloseMetaObject()
{
	IBackendMetadataTree* const metaTree = m_metaData->GetMetaTree();
	if (metaTree != nullptr)
		metaTree->CloseMetaObject(this);
	return true;
}

#pragma region interface_h
void IValueMetaObject::DoSetInterface(const meta_identifier_t& id, const bool& val)
{
	m_metaData->Modify(true);
}
#pragma endregion
#pragma region role_h 
void IValueMetaObject::DoSetRight(const CRole* role, const bool& val)
{
	m_metaData->Modify(true);
}
#pragma endregion

bool IValueMetaObject::IsFullAccess() const
{
	if (appData->DesignerMode())
		return true;

	return m_metaData->IsFullAccess();
}

CUserRoleInfo IValueMetaObject::GetUserRoleInfo() const
{
	CUserRoleInfo roleInfo;
	for (auto role : appData->GetUserRoleArray())
		roleInfo.m_arrayRole.emplace_back(role.m_miRoleId);
	return roleInfo;
}

#define	headerBlock 0x002330
#define	dataBlock 0x002350
#define	childBlock 0x002370

bool IValueMetaObject::Init()
{
	// always false
	return false;
}

bool IValueMetaObject::Init(CValue** paParams, const long lSizeArray)
{
	if (lSizeArray < 1)
		return false;

	IValueMetaObject* parent = nullptr;
	if (paParams[0]->ConvertToValue(parent)) {
		const class_identifier_t& clsid = GetClassType();
		if (parent != nullptr) {
			SetParent(parent);
			parent->AddChild(this);
		}
		return parent != nullptr ?
			parent->FilterChild(clsid) : true;
	}

	return false;
}

bool IValueMetaObject::IsEditable() const
{
	if (!IsEnabled() || IsDeleted())
		return false;

	IBackendMetadataTree* const metaTree = m_metaData->GetMetaTree();
	if (metaTree != nullptr)
		return metaTree->IsEditable();

	return m_parent != nullptr ?
		m_parent->IsEditable() : true;
}

bool IValueMetaObject::CompareObject(const IValueMetaObject* compareObject) const
{
#pragma region _compare_fill_h_
	class CControlComparator {
	public:

		static bool CompareObject(
			const IValueMetaObject* compareObject1,
			const IValueMetaObject* compareObject2
		)
		{
			if (compareObject2 == nullptr)
				return false;

			if (compareObject1->GetClassType() != compareObject2->GetClassType())
				return false;

			if (compareObject1->GetMetaID() != compareObject2->GetMetaID())
				return false;

			for (unsigned int idx = 0; idx < compareObject1->GetPropertyCount(); idx++) {

				const IProperty* propDst = compareObject1->GetProperty(idx);
				wxASSERT(propDst);

				const IProperty* propSrc = compareObject2->GetProperty(propDst->GetName());

				if (propSrc == nullptr)
					return false;

				if (propDst->GetValue() != propSrc->GetValue())
					return false;
			}

			return (compareObject1->GetPropertyCount() == compareObject2->GetPropertyCount() && compareObject1->GetEventCount() == compareObject2->GetEventCount()) &&
				compareObject1->GetChildCount() == compareObject2->GetChildCount();
		}
	};

#pragma endregion 

	return CControlComparator::CompareObject(this, compareObject);
}

bool IValueMetaObject::CopyObject(CMemoryWriter& writer) const
{
#pragma region _copy_guard_h_

	class CControlCopyGuard {

		static void Generate(const IValueMetaObject* copyObject) {
			for (unsigned int idx = 0; idx < copyObject->GetChildCount(); idx++)
				Generate(copyObject->GetChild(idx));
			copyObject->m_metaCopyGuid = wxNewUniqueGuid;
		}

		static void Erase(const IValueMetaObject* copyObject) {
			for (unsigned int idx = 0; idx < copyObject->GetChildCount(); idx++)
				Erase(copyObject->GetChild(idx));
			copyObject->m_metaCopyGuid = wxNullGuid;
		}

	public:

		CControlCopyGuard(const IValueMetaObject* copyObject) : m_copyObject(copyObject) { Generate(m_copyObject); }
		~CControlCopyGuard() { Erase(m_copyObject); }

	protected:
		const IValueMetaObject* m_copyObject = nullptr;
	};

	CControlCopyGuard controlCopyGuard(this);

#pragma endregion 

	wxASSERT(m_metaCopyGuid.isValid());

#pragma region _copy_fill_h_

	class CControlMemoryWriter {
	public:

		static bool CopyObject(const IValueMetaObject* copyObject, CMemoryWriter& writer)
		{
			CMemoryWriter writerHeaderMemory;
			writerHeaderMemory.w_s32(copyObject->m_metaData->GetVersion());
			writerHeaderMemory.w_stringZ(copyObject->m_metaCopyGuid);
			writer.w_chunk(headerBlock, writerHeaderMemory.pointer(), writerHeaderMemory.size());

			CMemoryWriter writerChildMemory;

			for (const auto object : copyObject->m_children) {

				if (!copyObject->FilterChild(object->GetClassType()))
					continue;
				if (object->IsDeleted())
					continue;
				CMemoryWriter writerMemory;
				if (!CopyObject(object, writerMemory))
					return false;

				writerChildMemory.w_chunk(object->GetClassType(), writerMemory.pointer(), writerMemory.size());
			}

			writer.w_chunk(childBlock, writerChildMemory.pointer(), writerChildMemory.size());

			CMemoryWriter writerDataMemory;
			
			if (!copyObject->CopyProperty(writerDataMemory))
				return false;

			if (!copyObject->SaveInterface(writerDataMemory))
				return false;

			if (!copyObject->SaveRole(writerDataMemory))
				return false;

			writer.w_chunk(dataBlock, writerDataMemory.pointer(), writerDataMemory.size());
			return true;
		}
	};

#pragma endregion 

	return CControlMemoryWriter::CopyObject(this, writer);
}

bool IValueMetaObject::PasteObject(CMemoryReader& reader)
{
#pragma region _paste_fill_h_

	class CControlMemoryReader {

		static bool PasteObject(IValueMetaObject* pasteObject, CMemoryReader& reader)
		{
			IMetaData* metaData = pasteObject->GetMetaData();

			std::shared_ptr <CMemoryReader>readerHeaderMemory(reader.open_chunk(headerBlock));

			/*const version_identifier_t& version =*/ readerHeaderMemory->r_s32();
			pasteObject->m_metaGuid = readerHeaderMemory->r_stringZ();

			//and running initialization
			if (!pasteObject->OnBeforeRunMetaObject(pasteObjectFlag))
				return false;

			std::shared_ptr <CMemoryReader> readerChildMemory(reader.open_chunk(childBlock));
			if (readerChildMemory != nullptr) {
				CMemoryReader* prevReaderMemory = nullptr;
				do {
					class_identifier_t clsid = 0;
					CMemoryReader* readerMemory = readerChildMemory->open_chunk_iterator(clsid, &*prevReaderMemory);
					if (readerMemory == nullptr)
						break;
					if (clsid > 0) {
						IValueMetaObject* metaObject = metaData->CreateMetaObject(clsid, pasteObject, false);
						if (metaObject != nullptr && !PasteObject(metaObject, *readerMemory)) {
							wxDELETE(metaObject);
							return false;
						}
					}
					prevReaderMemory = readerMemory;
				} while (true);
			}

			std::shared_ptr <CMemoryReader>readerDataMemory(reader.open_chunk(dataBlock));

			if (!pasteObject->PasteProperty(*readerDataMemory))
				return false;

			pasteObject->BuildNewName();

			pasteObject->LoadInterface(*readerDataMemory);
			pasteObject->LoadRole(*readerDataMemory);

			if (!pasteObject->OnAfterRunMetaObject(pasteObjectFlag))
				return false;

			return true;
		}

	public:

		static bool PasteAndRunObject(IValueMetaObject* pasteObject, CMemoryReader& reader)
		{
			IMetaData* metaData = pasteObject->GetMetaData();

			std::shared_ptr <CMemoryReader>readerHeaderMemory(reader.open_chunk(headerBlock));

			/*const version_identifier_t& version =*/ readerHeaderMemory->r_s32();
			/*pasteObject->m_metaGuid =*/ readerHeaderMemory->r_stringZ();

			//and running initialization
			if (!pasteObject->OnBeforeRunMetaObject(pasteObjectFlag))
				return false;

			std::shared_ptr <CMemoryReader> readerChildMemory(reader.open_chunk(childBlock));
			if (readerChildMemory != nullptr) {
				CMemoryReader* prevReaderMemory = nullptr;
				do {
					class_identifier_t clsid = 0;
					CMemoryReader* readerMemory = readerChildMemory->open_chunk_iterator(clsid, &*prevReaderMemory);
					if (readerMemory == nullptr)
						break;
					if (clsid > 0) {
						IValueMetaObject* metaObject = metaData->CreateMetaObject(clsid, pasteObject, false);
						if (metaObject != nullptr && !PasteObject(metaObject, *readerMemory)) {
							wxDELETE(metaObject);
							return false;
						}
					}
					prevReaderMemory = readerMemory;
				} while (true);
			}

			std::shared_ptr <CMemoryReader>readerDataMemory(reader.open_chunk(dataBlock));

			if (!pasteObject->PasteProperty(*readerDataMemory))
				return false;

			pasteObject->BuildNewName();

			pasteObject->LoadInterface(*readerDataMemory);
			pasteObject->LoadRole(*readerDataMemory);

			if (!pasteObject->OnAfterRunMetaObject(pasteObjectFlag))
				return false;

			return true;
		}
	};

#pragma endregion 

	return CControlMemoryReader::PasteAndRunObject(this, reader);
}

bool IValueMetaObject::ChangeChildPosition(IValueMetaObject* object, unsigned int pos)
{
	m_metaData->Modify(true);
	return IPropertyObjectHelper::ChangeChildPosition(object, pos);
}

wxString IValueMetaObject::GetModuleName() const
{
	IValueMetaObject* parent = GetParent();
	//wxASSERT(parent);
	if (parent != nullptr) {
		return parent->GetName() + wxT(": ") + GetName();
	}
	return GetName();
}

wxString IValueMetaObject::GetFullName() const
{
	wxString strFullName;

	IValueMetaObject* parent = GetParent();

	if (parent != nullptr && g_metaModuleCLSID != GetClassType() && g_metaManagerCLSID != GetClassType())
		strFullName = strFullName + GetClassName() + '.' + GetName();
	else
		strFullName = GetName();

	while (parent != nullptr) {
		if (g_metaCommonMetadataCLSID != parent->GetClassType()) {
			strFullName = parent->GetClassName() + '.' +
				parent->GetName() + '.' +
				strFullName;
		}
		parent = parent->GetParent();
	}

	return strFullName;
}

wxString IValueMetaObject::GetFileName() const
{
	return m_metaData->GetFileName();
}

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

void IValueMetaObject::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	for (unsigned idx = 0; idx < IPropertyObject::GetPropertyCount(); idx++) {
		IProperty* property = IPropertyObject::GetProperty(idx);
		if (property == nullptr) continue;
		m_methodHelper->AppendProp(property->GetName(), true, false, idx);
	}
}

bool IValueMetaObject::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	IProperty* property = GetPropertyByIndex(lPropNum);
	if (property != nullptr) return property->SetDataValue(varPropVal);
	return false;
}

bool IValueMetaObject::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const IProperty* property = GetPropertyByIndex(lPropNum);
	if (property != nullptr) return property->GetDataValue(pvarPropVal);
	return false;
}

CRestructureInfo s_restructureInfo;