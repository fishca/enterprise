////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaobject
////////////////////////////////////////////////////////////////////////////

#include "metaObject.h"
#include "backend/appData.h"

#include "backend/metaData.h"
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"

wxIMPLEMENT_ABSTRACT_CLASS(IMetaObject, CValue);

#define metaBlock 0x200222

//*****************************************************************************************
//*                                  MetaObject                                           *
//*****************************************************************************************

void IMetaObject::ResetGuid()
{
	m_metaGuid = wxNewUniqueGuid;
}

void IMetaObject::ResetId()
{
	if (m_metaData != nullptr) {
		m_metaId = m_metaData->GenerateNewID();
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

IBackendMetadataTree* IMetaObject::GetMetaDataTree() const
{
	return m_metaData ? m_metaData->GetMetaTree() : nullptr;
}

/////////////////////////////////////////////////////////////////////////////////////////

bool IMetaObject::BuildNewName()
{
	bool foundedName = false;
	for (const auto object : m_metaData->GetMetaObject(GetClassType())) {
		if (object->GetParent() != GetParent())
			continue;
		if (object != this &&
			stringUtils::CompareString(GetName(), object->GetName())) {
			foundedName = true;
			break;
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

IMetaObject::IMetaObject(const wxString& strName, const wxString& synonym, const wxString& comment) : CValue(eValueTypes::TYPE_VALUE, true),
m_methodHelper(new CMethodHelper()), m_metaData(nullptr), m_metaFlags(metaDefaultFlag), m_metaId(0)
{
	m_propertyName->SetValue(strName);
	m_propertySynonym->SetValue(synonym);
	m_propertyComment->SetValue(comment);
}

IMetaObject::~IMetaObject()
{
	wxDELETE(m_methodHelper);
}

bool IMetaObject::LoadMeta(CMemoryReader& dataReader)
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
	wxMemoryBuffer buffer_chunk;
	if (!dataReader.r_chunk(metaBlock, buffer_chunk))
		return false;

	CMemoryReader dataObjectReader(buffer_chunk);
	dataObjectReader.r_u32(); //reserved flags
	if (!LoadData(dataObjectReader))
		return false;

	return true;
}

bool IMetaObject::SaveMeta(CMemoryWriter& dataWritter)
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
	CMemoryWriter dataObjectWritter;
	dataObjectWritter.w_u32(0); //reserved flags
	if (!SaveData(dataObjectWritter))
		return false;

	dataWritter.w_chunk(metaBlock, dataObjectWritter.buffer());
	return true;
}

bool IMetaObject::LoadMetaObject(IMetaData* metaData, CMemoryReader& dataReader)
{
	m_metaData = metaData;

	if (!LoadMeta(dataReader))
		return false;

	if (!OnLoadMetaObject(metaData))
		return false;

	return true;
}

bool IMetaObject::SaveMetaObject(IMetaData* metaData, CMemoryWriter& dataWritter, int flags)
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

bool IMetaObject::DeleteMetaObject(IMetaData* metaData)
{
	if (m_metaData != metaData)
		return false;

	return DeleteData();
}

bool IMetaObject::CreateMetaTable(IMetaDataConfiguration* srcMetaData, int flags)
{
	return CreateAndUpdateTableDB(srcMetaData, nullptr, flags);
}

bool IMetaObject::UpdateMetaTable(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject)
{
	return CreateAndUpdateTableDB(srcMetaData, srcMetaObject, updateMetaTable);
}

bool IMetaObject::DeleteMetaTable(IMetaDataConfiguration* srcMetaData)
{
	return CreateAndUpdateTableDB(srcMetaData, this, deleteMetaTable);
}

bool IMetaObject::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	GenerateGuid();
	wxASSERT(metaData);
	m_metaId = metaData->GenerateNewID();
	m_metaData = metaData;
#ifdef DEBUG  
	wxLogDebug("* Create metaData object %s with id %i",
		GetClassName(), GetMetaID()
	);
#endif
	return true;
}

bool IMetaObject::OnLoadMetaObject(IMetaData* metaData)
{
	m_metaData = metaData;
	return true;
}

bool IMetaObject::OnDeleteMetaObject()
{
	return true;
}

bool IMetaObject::OnAfterCloseMetaObject()
{
	IBackendMetadataTree* const metaTree = m_metaData->GetMetaTree();
	if (metaTree != nullptr)
		metaTree->CloseMetaObject(this);
	return true;
}

#pragma region interface_h
void IMetaObject::DoSetInterface(const meta_identifier_t& id, const bool& val)
{
	m_metaData->Modify(true);
}
#pragma endregion
#pragma region role_h 
void IMetaObject::DoSetRight(const CRole* role, const bool& val)
{
	m_metaData->Modify(true);
}
#pragma endregion

#define	headerBlock 0x002330
#define	dataBlock 0x002350
#define	childBlock 0x002370

bool IMetaObject::Init()
{
	// always false
	return false;
}

bool IMetaObject::Init(CValue** paParams, const long lSizeArray)
{
	if (lSizeArray < 1)
		return false;

	IMetaObject* parent = nullptr;
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

bool IMetaObject::IsEditable() const
{
	if (!IsEnabled() || IsDeleted())
		return false;

	IBackendMetadataTree* const metaTree = m_metaData->GetMetaTree();
	if (metaTree != nullptr)
		return metaTree->IsEditable();

	return m_parent != nullptr ?
		m_parent->IsEditable() : true;
}

bool IMetaObject::CompareObject(IMetaObject* compareObject) const
{
#pragma region _compare_fill_h_
	class CControlComparator {
	public:

		static bool CompareObject(
			const IMetaObject* compareObject1,
			const IMetaObject* compareObject2
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

bool IMetaObject::CopyObject(CMemoryWriter& writer) const
{
#pragma region _copy_guard_h_

	class CControlCopyGuard {

		static void Generate(const IMetaObject* copyObject) {
			for (unsigned int idx = 0; idx < copyObject->GetChildCount(); idx++)
				Generate(copyObject->GetChild(idx));
			copyObject->m_metaCopyGuid = wxNewUniqueGuid;
		}

		static void Erase(const IMetaObject* copyObject) {
			for (unsigned int idx = 0; idx < copyObject->GetChildCount(); idx++)
				Erase(copyObject->GetChild(idx));
			copyObject->m_metaCopyGuid = wxNullGuid;
		}

	public:

		CControlCopyGuard(const IMetaObject* copyObject) : m_copyObject(copyObject) { Generate(m_copyObject); }
		~CControlCopyGuard() { Erase(m_copyObject); }

	protected:
		const IMetaObject* m_copyObject = nullptr;
	};

	CControlCopyGuard controlCopyGuard(this);

#pragma endregion 

	wxASSERT(m_metaCopyGuid.isValid());

#pragma region _copy_fill_h_

	class CControlMemoryWriter {
	public:

		static bool CopyObject(const IMetaObject* copyObject, CMemoryWriter& writer)
		{
			CMemoryWriter writterHeaderMemory;
			writterHeaderMemory.w_s32(copyObject->m_metaData->GetVersion());
			writterHeaderMemory.w_stringZ(copyObject->m_metaCopyGuid);
			writer.w_chunk(headerBlock, writterHeaderMemory.pointer(), writterHeaderMemory.size());

			CMemoryWriter writterChildMemory;

			for (const auto object : copyObject->m_children) {

				if (!copyObject->FilterChild(object->GetClassType()))
					continue;
				if (object->IsDeleted())
					continue;
				CMemoryWriter writterMemory;
				if (!CopyObject(object, writterMemory))
					return false;

				writterChildMemory.w_chunk(object->GetClassType(), writterMemory.pointer(), writterMemory.size());
			}

			writer.w_chunk(childBlock, writterChildMemory.pointer(), writterChildMemory.size());

			CMemoryWriter writterDataMemory;
			if (!copyObject->CopyProperty(writterDataMemory))
				return false;

			writer.w_chunk(dataBlock, writterDataMemory.pointer(), writterDataMemory.size());
			return true;
		}
	};

#pragma endregion 

	return CControlMemoryWriter::CopyObject(this, writer);
}

bool IMetaObject::PasteObject(CMemoryReader& reader)
{
#pragma region _paste_fill_h_

	class CControlMemoryReader {

		static bool PasteObject(IMetaObject* pasteObject, CMemoryReader& reader)
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
						IMetaObject* metaObject = metaData->CreateMetaObject(clsid, pasteObject, false);
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

			if (!pasteObject->OnAfterRunMetaObject(pasteObjectFlag))
				return false;

			return true;
		}

	public:

		static bool PasteAndRunObject(IMetaObject* pasteObject, CMemoryReader& reader)
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
						IMetaObject* metaObject = metaData->CreateMetaObject(clsid, pasteObject, false);
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

			if (!pasteObject->OnAfterRunMetaObject(pasteObjectFlag))
				return false;

			return true;
		}
	};

#pragma endregion 

	return CControlMemoryReader::PasteAndRunObject(this, reader);
}

bool IMetaObject::ChangeChildPosition(IMetaObject* object, unsigned int pos)
{
	m_metaData->Modify(true);
	return IPropertyObjectHelper::ChangeChildPosition(object, pos);
}

wxString IMetaObject::GetModuleName() const
{
	IMetaObject* parent = GetParent();
	//wxASSERT(parent);
	if (parent != nullptr) {
		return parent->GetName() + wxT(": ") + GetName();
	}
	return GetName();
}

wxString IMetaObject::GetFullName() const
{
	wxString strFullName;

	IMetaObject* parent = GetParent();

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

wxString IMetaObject::GetFileName() const
{
	return m_metaData->GetFileName();
}

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

void IMetaObject::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	for (unsigned idx = 0; idx < IPropertyObject::GetPropertyCount(); idx++) {
		IProperty* property = IPropertyObject::GetProperty(idx);
		if (property == nullptr) continue;
		m_methodHelper->AppendProp(property->GetName(), true, false, idx);
	}
}

bool IMetaObject::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	IProperty* property = GetPropertyByIndex(lPropNum);
	if (property != nullptr) return property->SetDataValue(varPropVal);
	return false;
}

bool IMetaObject::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const IProperty* property = GetPropertyByIndex(lPropNum);
	if (property != nullptr) return property->GetDataValue(pvarPropVal);
	return false;
}

CRestructureInfo s_restructureInfo;