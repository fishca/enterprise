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
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool ibValueListDataObjectEnumRef::SetValueByRow(const wxVariant& variant,
	const ibDataViewItem& row, unsigned int col)
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void ibValueListDataObjectRef::GetValueByRow(wxVariant& variant,
	const ibDataViewItem& row, unsigned int col) const
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool ibValueListDataObjectRef::SetValueByRow(const wxVariant& variant,
	const ibDataViewItem& row, unsigned int col)
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////

void ibValueModelTreeDataObjectFolderRef::GetValueByRow(wxVariant& variant,
	const ibDataViewItem& item, unsigned int col) const
{
	wxValueTreeNode* node = GetViewData<wxValueTreeNode>(item);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool ibValueModelTreeDataObjectFolderRef::SetValueByRow(const wxVariant& variant,
	const ibDataViewItem& item, unsigned int col)
{
	wxValueTreeNode* node = GetViewData<wxValueTreeNode>(item);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ibValueModelTreeDataObjectFolderRef::GetAttrByRow(const ibDataViewItem& item,
	unsigned int col, ibDataViewItemAttr& attr) const
{
	wxValueTreeNode* node = GetViewData<wxValueTreeNode>(item);
	if (node == nullptr)
		return false;
	const ibValue& isFolder =
		node->GetTableValue(*m_metaObject->GetDataIsFolder());
	if (isFolder.GetBoolean())
		attr.SetBackgroundColour(wxColour(214, 239, 252));
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ibValueModelTreeDataObjectFolderRef::SetParentTopItem(const ibDataViewItem& item)
{
	if (!item.IsOk())
	{
		m_topParentGuid.reset();
		return true;
	}

	wxValueTreeListNode* node = GetViewData<wxValueTreeListNode>(item);
	if (node == nullptr)
		return false;

	m_topParentGuid = node->GetGuid();
	return true;
}

ibDataViewItem ibValueModelTreeDataObjectFolderRef::GetParentTopItem() const
{
	std::function<void(wxValueTreeListNode*, wxValueTreeListNode*&, const ibGuid&)> findGuid =
		[&findGuid](wxValueTreeListNode* parent, wxValueTreeListNode*& foundedNode, const ibGuid& guid)
		{
			if (guid == parent->GetGuid()) { foundedNode = parent; return; }
			else if (foundedNode != nullptr) { return; }

			for (unsigned int n = 0; n < parent->GetChildCount(); n++) {
				wxValueTreeListNode* child = dynamic_cast<wxValueTreeListNode*>(parent->GetChild(n));
				if (child != nullptr)
					findGuid(child, foundedNode, guid);
				if (foundedNode != nullptr) break;
			}
		};

	wxValueTreeListNode* foundedNode = nullptr;
	for (unsigned int c = 0; c < GetRoot()->GetChildCount(); c++) {
		wxValueTreeListNode* child = dynamic_cast<wxValueTreeListNode*>(GetRoot()->GetChild(c));
		if (child != nullptr) findGuid(child, foundedNode, m_topParentGuid);
		if (foundedNode != nullptr) break;
	}

	if (foundedNode != nullptr)
		return ibDataViewItem(foundedNode);

	return ibDataViewItem(nullptr);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

void ibValueListRegisterObject::GetValueByRow(wxVariant& variant,
	const ibDataViewItem& row, unsigned int col) const
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return;
	node->GetValue(col, variant);
}

bool ibValueListRegisterObject::SetValueByRow(const wxVariant& variant,
	const ibDataViewItem& row, unsigned int col)
{
	wxValueTableRow* node = GetViewData<wxValueTableRow>(row);
	if (node == nullptr)
		return false;
	return node->SetValue(col, variant);
}