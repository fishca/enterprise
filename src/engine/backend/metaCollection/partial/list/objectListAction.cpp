////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : list actionData 
////////////////////////////////////////////////////////////////////////////

#include "objectList.h"

enum
{
	eChooseValue = 25,
	eMarkAsDelete = 26,
	eAddFolder = 27,
};

CValueListDataObjectEnumRef::CActionCollection CValueListDataObjectEnumRef::GetActionCollection(const form_identifier_t& formType)
{
	CActionCollection actionData(this);

	if (m_choiceMode)
		actionData.AddAction(wxT("Select"), _("Select"), g_picSelectCLSID, true, eChooseValue);

	const CActionCollection& data =
		IValueTable::GetActionCollection(formType);

	for (unsigned int idx = 0; idx < data.GetCount(); idx++) {
		const action_identifier_t& id = data.GetID(idx);
		if (id > 0) {
			actionData.AddAction(
				data.GetNameByID(id),
				data.GetCaptionByID(id),
				data.GetPictureByID(id),
				data.IsCreatePictureAndText(id),
				id
			);
		}
		else {
			actionData.AddSeparator();
		}
	}

	return actionData;
}

void CValueListDataObjectEnumRef::ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm)
{
	switch (lNumAction)
	{
		case eChooseValue:
			ChooseValue(srcForm);
			break;
		default:
			IValueTable::ExecuteAction(lNumAction, srcForm);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CValueListDataObjectRef::CActionCollection CValueListDataObjectRef::GetActionCollection(const form_identifier_t& formType)
{
	CActionCollection actionData(this);

	if (m_choiceMode)
		actionData.AddAction(wxT("Select"), _("Select"), g_picSelectCLSID, true, eChooseValue);

	if (m_choiceMode)
		actionData.AddSeparator();

	const CActionCollection& data =
		IValueTable::GetActionCollection(formType);

	for (unsigned int idx = 0; idx < data.GetCount(); idx++) {
		const action_identifier_t& id = data.GetID(idx);
		if (id > 0) {
			actionData.AddAction(
				data.GetNameByID(id),
				data.GetCaptionByID(id),
				data.GetPictureByID(id), 
				data.IsCreatePictureAndText(id),
				id
			);
		}
		else {
			actionData.AddSeparator();
		}
	}

	actionData.InsertAction(3, wxT("MarkAsDelete"), _("Mark as delete"), g_picMarkAsDeleteCLSID, true, eMarkAsDelete);

	return actionData;
}

void CValueListDataObjectRef::ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm)
{
	switch (lNumAction)
	{
		case eMarkAsDelete:
			MarkAsDeleteValue();
			break;
		case eChooseValue:
			ChooseValue(srcForm);
			break;
		default:
			IValueTable::ExecuteAction(lNumAction, srcForm);
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CValueListDataObjectRef::CActionCollection CValueTreeDataObjectFolderRef::GetActionCollection(const form_identifier_t& formType)
{
	CActionCollection actionData(this);

	if (m_choiceMode) {
		actionData.AddAction(wxT("Select"), _("Select"), g_picSelectCLSID, true, eChooseValue);
		actionData.AddSeparator();
	}

	if (m_listMode == LIST_FOLDER || m_listMode == LIST_ITEM || m_listMode == LIST_ITEM_FOLDER) {
		actionData.AddAction(wxT("AddFolder"), _("Add folder"), g_picAddFolderCLSID, true, eAddFolder);
	}

	CActionCollection data =
		IValueTree::GetActionCollection(formType);

	if (m_listMode == LIST_FOLDER)
		data.RemoveAction(eAddValue); //add

	for (unsigned int idx = 0; idx < data.GetCount(); idx++) {
		const action_identifier_t& id = data.GetID(idx);
		if (id > 0) {
			actionData.AddAction(
				data.GetNameByID(id),
				data.GetCaptionByID(id),
				data.GetPictureByID(id),
				data.IsCreatePictureAndText(id),
				id
			);
		}
		else {
			actionData.AddSeparator();
		}
	}

	if (m_choiceMode) {
		actionData.InsertAction(5, wxT("MarkAsDelete"), _("Mark as delete"), g_picMarkAsDeleteCLSID, true, eMarkAsDelete);
	}
	else {
		actionData.InsertAction(4, wxT("MarkAsDelete"), _("Mark as delete"), g_picMarkAsDeleteCLSID, true, eMarkAsDelete);
	}

	return actionData;
}

void CValueTreeDataObjectFolderRef::ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm)
{
	switch (lNumAction)
	{
		case eAddFolder:
			AddFolderValue();
			break;
		case eMarkAsDelete:
			MarkAsDeleteValue();
			break;
		case eChooseValue:
			ChooseValue(srcForm);
			break;
		default:
			IValueTree::ExecuteAction(lNumAction, srcForm);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////

CValueListRegisterObject::CActionCollection CValueListRegisterObject::GetActionCollection(const form_identifier_t& formType)
{
	return IValueTable::GetActionCollection(formType);
}

void CValueListRegisterObject::ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm)
{
	IValueTable::ExecuteAction(lNumAction, srcForm);
}