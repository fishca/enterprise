////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metaData 
////////////////////////////////////////////////////////////////////////////

#include "metaData.h"

//**************************************************************************************************
//*                                          ibMetaData											   *
//**************************************************************************************************

//ID's 
ibMetaID ibMetaData::GenerateNewID() const
{
	ibValueMetaObject* commonObject = GetCommonMetaObject();
	wxASSERT(commonObject);
	ibMetaID id = commonObject->GetMetaID() + 1;
	DoGenerateNewID(id, commonObject);
	return id;
}

void ibMetaData::DoGenerateNewID(ibMetaID& id, ibValueMetaObject* top) const
{
	for (unsigned int idx = 0; idx < top->GetChildCount(); idx++) {
		ibValueMetaObject* child = top->GetChild(idx);
		wxASSERT(child);
		ibMetaID newID = child->GetMetaID() + 1;
		if (newID > id) {
			id = newID;
		}
		DoGenerateNewID(id, child);
	}
}

ibValueMetaObject* ibMetaData::CreateMetaObject(const ibClassID& clsid, ibValueMetaObject* parent, bool runObject)
{
	wxASSERT(clsid != 0);

	ibValue* ppParams[] = { parent };
	ibValueMetaObject* newMetaObject = nullptr;

	try {
		newMetaObject = ibValue::CreateAndConvertObjectRef<ibValueMetaObject>(clsid, ppParams, 1);
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

wxString ibMetaData::GetNewName(const ibClassID& clsid, ibValueMetaObject* parent, const wxString& strPrefix, bool forConstructor)
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

bool ibMetaData::RenameMetaObject(ibValueMetaObject* metaObject, const wxString& newName)
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

void ibMetaData::RemoveMetaObject(ibValueMetaObject* object, ibValueMetaObject* parent)
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