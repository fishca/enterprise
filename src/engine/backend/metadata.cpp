////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaData 
////////////////////////////////////////////////////////////////////////////

#include "metaData.h"

#include "backend/compiler/enumFactory.h"
#include "backend/metaCollection/partial/contextManager.h"
#include "backend/system/systemManager.h"
#include "backend/debugger/debugServer.h"
#include "backend/debugger/debugClient.h"

#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/databaseErrorCodes.h"

#include "backend/appData.h"

//**************************************************************************************************
//*                                          IMetaData											   *
//**************************************************************************************************

//ID's 
meta_identifier_t IMetaData::GenerateNewID() const
{
	IMetaObject* commonObject = GetCommonMetaObject();
	wxASSERT(commonObject);
	meta_identifier_t id = commonObject->GetMetaID() + 1;
	DoGenerateNewID(id, commonObject);
	return id;
}

void IMetaData::DoGenerateNewID(meta_identifier_t& id, IMetaObject* top) const
{
	for (unsigned int idx = 0; idx < top->GetChildCount(); idx++) {
		IMetaObject* child = top->GetChild(idx);
		wxASSERT(child);
		meta_identifier_t newID = child->GetMetaID() + 1;
		if (newID > id) {
			id = newID;
		}
		DoGenerateNewID(id, child);
	}
}

IMetaObject* IMetaData::CreateMetaObject(const class_identifier_t& clsid, IMetaObject* parent, bool runObject)
{
	wxASSERT(clsid != 0);

	CValue* ppParams[] = { parent };
	IMetaObject* newMetaObject = nullptr;

	try {
		newMetaObject = CValue::CreateAndConvertObjectRef<IMetaObject>(clsid, ppParams, 1);
		newMetaObject->IncrRef();
	}
	catch (...) {
		return nullptr;
	}

	if (newMetaObject != nullptr) {

		newMetaObject->SetName(
			GetNewName(clsid, parent, newMetaObject->GetClassName())
		);

		//always create meta object
		bool success = newMetaObject->OnCreateMetaObject(this, runObject ? newObjectFlag : pasteObjectFlag);

		//first initialization
		if (!success || !newMetaObject->OnLoadMetaObject(this)) {
			if (parent != nullptr) {
				parent->RemoveChild(newMetaObject);
			}
			wxDELETE(newMetaObject);
			return nullptr;
		}

		//and running initialization
		if (runObject && (!success || !newMetaObject->OnBeforeRunMetaObject(newObjectFlag))) {
			if (parent != nullptr) {
				parent->RemoveChild(newMetaObject);
			}
			wxDELETE(newMetaObject);
			return nullptr;
		}

		Modify(true);

		if (runObject && (!success || !newMetaObject->OnAfterRunMetaObject(newObjectFlag))) {
			if (parent != nullptr) {
				parent->RemoveChild(newMetaObject);
			}
			wxDELETE(newMetaObject);
			return nullptr;
		}
	}

	return newMetaObject;
}

wxString IMetaData::GetNewName(const class_identifier_t& clsid, IMetaObject* parent, const wxString& strPrefix, bool forConstructor)
{
	unsigned int countRec = forConstructor ?
		0 : 1;

	wxString currPrefix = strPrefix.Length() > 0 ?
		strPrefix : wxT("newItem");

	wxString newName = forConstructor ?
		wxString::Format(wxT("%s"), currPrefix) :
		wxString::Format(wxT("%s%d"), currPrefix, countRec);

	while (forConstructor ||
		countRec > 0) {

		bool foundedName = false;

		if (parent != nullptr) {

			for (unsigned int idx = 0; idx < parent->GetChildCount(); idx++) {

				auto child = parent->GetChild(idx);
				if (clsid != child->GetClassType())
					continue;

				if (!parent->FilterChild(child->GetClassType()))
					continue;

				if (child->IsDeleted())
					continue;

				if (stringUtils::CompareString(newName, child->GetName())) {
					foundedName = true;
					break;
				}
			}
		}

		if (!foundedName)
			break;

		newName = wxString::Format(wxT("%s%d"), currPrefix, ++countRec);
	}

	return newName;
}

bool IMetaData::RenameMetaObject(IMetaObject* metaObject, const wxString& newName)
{
	bool foundedName = false;

	for (const auto object : GetAnyArrayObject(metaObject->GetClassType())) {
		if (object->GetParent() != metaObject->GetParent())
			continue;
		if (object != metaObject &&
			stringUtils::CompareString(newName, object->GetName())) {
			foundedName = true;
			break;
		}
	}

	if (foundedName) return false;

	if (metaObject->OnRenameMetaObject(newName)) {
		Modify(true); return true;
	}
	return false;
}

void IMetaData::RemoveMetaObject(IMetaObject* object, IMetaObject* parent)
{
	if (object->OnAfterCloseMetaObject()) {
		if (object->OnDeleteMetaObject()) {
			object->MarkAsDeleted();
			for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {
				auto child = object->GetChild(idx);
				if (!object->FilterChild(child->GetClassType()))
					continue;
				RemoveMetaObject(child, object);
			}
		}
		object->OnReloadMetaObject();
		Modify(true);
	}
}