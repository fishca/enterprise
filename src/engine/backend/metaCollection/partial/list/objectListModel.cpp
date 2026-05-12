////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : list model 
////////////////////////////////////////////////////////////////////////////

#include "objectList.h"

//***********************************************************************************
//*                                  Model                                          *
//***********************************************************************************

void ibValueListDataObjectEnumRef::GetValueByRow(wxVariant& variant,
	const ibDataViewItem& row, unsigned int col) const
{
	ibValueTableRow* node = GetViewData<ibValueTableRow>(row);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool ibValueListDataObjectEnumRef::SetValueByRow(const wxVariant& variant,
	const ibDataViewItem& row, unsigned int col)
{
	ibValueTableRow* node = GetViewData<ibValueTableRow>(row);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void ibValueListDataObjectRef::GetValueByRow(wxVariant& variant,
	const ibDataViewItem& row, unsigned int col) const
{
	ibValueTableRow* node = GetViewData<ibValueTableRow>(row);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool ibValueListDataObjectRef::SetValueByRow(const wxVariant& variant,
	const ibDataViewItem& row, unsigned int col)
{
	ibValueTableRow* node = GetViewData<ibValueTableRow>(row);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void ibValueModelTreeDataObjectFolderRef::GetValueByRow(wxVariant& variant,
	const ibDataViewItem& item, unsigned int col) const
{
	ibValueTreeNode* node = GetViewData<ibValueTreeNode>(item);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool ibValueModelTreeDataObjectFolderRef::SetValueByRow(const wxVariant& variant,
	const ibDataViewItem& item, unsigned int col)
{
	ibValueTreeNode* node = GetViewData<ibValueTreeNode>(item);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ibValueModelTreeDataObjectFolderRef::GetAttrByRow(const ibDataViewItem& item,
	unsigned int col, ibDataViewItemAttr& attr) const
{
	ibValueTreeNode* node = GetViewData<ibValueTreeNode>(item);
	if (node == nullptr)
		return false;
	const ibValue& isFolder =
		node->GetTableValue(*m_metaObject->GetDataIsFolder());
	if (isFolder.GetBoolean())
		attr.SetBackgroundColour(wxColour(214, 239, 252));
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void ibValueListRegisterObject::GetValueByRow(wxVariant& variant,
	const ibDataViewItem& row, unsigned int col) const
{
	ibValueTableRow* node = GetViewData<ibValueTableRow>(row);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool ibValueListRegisterObject::SetValueByRow(const wxVariant& variant,
	const ibDataViewItem& row, unsigned int col)
{
	ibValueTableRow* node = GetViewData<ibValueTableRow>(row);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}