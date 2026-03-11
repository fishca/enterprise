////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : list model 
////////////////////////////////////////////////////////////////////////////

#include "objectList.h"

//***********************************************************************************
//*                                  Model                                          *
//***********************************************************************************

void CValueListDataObjectEnumRef::GetValueByRow(wxVariant& variant,
	const wxDataViewExtItem& row, unsigned int col) const
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool CValueListDataObjectEnumRef::SetValueByRow(const wxVariant& variant,
	const wxDataViewExtItem& row, unsigned int col)
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void CValueListDataObjectRef::GetValueByRow(wxVariant& variant,
	const wxDataViewExtItem& row, unsigned int col) const
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool CValueListDataObjectRef::SetValueByRow(const wxVariant& variant,
	const wxDataViewExtItem& row, unsigned int col)
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void CValueTreeDataObjectFolderRef::GetValueByRow(wxVariant& variant,
	const wxDataViewExtItem& item, unsigned int col) const
{
	wxValueTreeNode* node = GetViewData<wxValueTreeNode>(item);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool CValueTreeDataObjectFolderRef::SetValueByRow(const wxVariant& variant,
	const wxDataViewExtItem& item, unsigned int col)
{
	wxValueTreeNode* node = GetViewData<wxValueTreeNode>(item);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CValueTreeDataObjectFolderRef::GetAttrByRow(const wxDataViewExtItem& item,
	unsigned int col, wxDataViewExtItemAttr& attr) const
{
	wxValueTreeNode* node = GetViewData<wxValueTreeNode>(item);
	if (node == nullptr)
		return false;
	CValue isFolder = false;
	node->GetValue(*m_metaObject->GetDataIsFolder(), isFolder);
	if (isFolder.GetBoolean())
		attr.SetBackgroundColour(wxColour(224, 212, 190));
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void CValueListRegisterObject::GetValueByRow(wxVariant& variant,
	const wxDataViewExtItem& row, unsigned int col) const
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool CValueListRegisterObject::SetValueByRow(const wxVariant& variant,
	const wxDataViewExtItem& row, unsigned int col)
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}