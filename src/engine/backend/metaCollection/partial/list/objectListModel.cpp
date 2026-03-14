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
		attr.SetBackgroundColour(wxColour(214, 239, 252));
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////

bool CValueTreeDataObjectFolderRef::SetParentTopItem(const wxDataViewExtItem& item)
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

wxDataViewExtItem CValueTreeDataObjectFolderRef::GetParentTopItem() const
{
	std::function<void(wxValueTreeListNode*, wxValueTreeListNode*&, const CGuid&)> findGuid =
		[&findGuid](wxValueTreeListNode* parent, wxValueTreeListNode*& foundedNode, const CGuid& guid)
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
		return wxDataViewExtItem(foundedNode);

	return wxDataViewExtItem(nullptr);
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