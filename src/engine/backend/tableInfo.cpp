////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : table model information
////////////////////////////////////////////////////////////////////////////

#include "tableInfo.h"

wxIMPLEMENT_ABSTRACT_CLASS(ibValueModel, ibValue);
wxIMPLEMENT_ABSTRACT_CLASS(ibValueModel::ibValueModelColumnCollection, ibValue);
wxIMPLEMENT_ABSTRACT_CLASS(ibValueModel::ibValueModelColumnCollection::ibValueModelColumnInfo, ibValue);
wxIMPLEMENT_ABSTRACT_CLASS(ibValueModel::ibValueModelReturnLine, ibValue);

wxIMPLEMENT_ABSTRACT_CLASS(ibValueModelTable, ibValueModel);
wxIMPLEMENT_ABSTRACT_CLASS(ibValueModelTree, ibValueModel);

ibValueModel::ibValueModel()
	: ibValue(ibValueTypes::TYPE_VALUE),
	m_modelProvider(nullptr),
	m_refreshModel(false)
{
	m_modelProvider = new ibDataViewModelProvider(this);
	//m_modelProvider->IncRef(); // always one 
}

ibValueModel::~ibValueModel()
{
	m_modelProvider->DecRef();
}

ibDataViewItem ibValueModel::GetSelection() const
{
	if (m_modelProvider == nullptr)
		return ibDataViewItem(nullptr);
	return m_modelProvider->GetSelection();
}

void ibValueModel::RowValueStartEdit(const ibDataViewItem& item, unsigned int col)
{
	if (m_modelProvider == nullptr)
		return;
	m_modelProvider->StartEditing(item, col);
}

ibValueModel::ibActionCollection ibValueModel::GetActionCollection(const ibFormID& formType)
{
	ibActionCollection action(this);

	if (UseStandartCommand()) {
		action.AddAction(wxT("Add"), _("Add"), g_picAddCLSID, true, eAddValue);
		action.AddAction(wxT("Copy"), _("Copy"), g_picCopyCLSID, false, eCopyValue);
		action.AddAction(wxT("Edit"), _("Edit"), g_picEditCLSID, false, eEditValue);
		action.AddAction(wxT("Delete"), _("Delete"), g_picDeleteCLSID, false, eDeleteValue);
	}

	if (UseFilter()) {
		if (UseStandartCommand()) action.AddSeparator();
		action.AddAction(wxT("Filter"), _("Filter"), g_picFilterCLSID, false, eFilter);
		action.AddAction(wxT("FilterByColumn"), _("Filter by column"), g_picFilterSetCLSID, false, eFilterByColumn);
		action.AddAction(wxT("FilterClear"), _("Filter clear"), g_picFilterClearCLSID, false, eFilterClear);
	}

	if (UseViewMode()) {
		if (UseStandartCommand() || UseFilter()) action.AddSeparator();
		action.AddAction(wxT("ViewMode"), _("View mode"), g_picHierarchyCLSID, false, eViewMode);
	}

	return action;
}

void ibValueModel::ExecuteAction(const ibActionID& lNumAction, ibBackendValueForm* srcForm)
{
	switch (lNumAction)
	{
	case eAddValue:
		AddValue();
		break;
	case eCopyValue:
		CopyValue();
		break;
	case eEditValue:
		EditValue();
		break;
	case eDeleteValue:
		DeleteValue();
		break;
	case eFilter:
		if (ShowFilter()) {
			CallRefreshModel(ibDataViewItem(nullptr), m_modelProvider != nullptr ? m_modelProvider->GetCountPerPage() : defaultCountPerPage);
		}
		break;
	case eFilterByColumn:
	{
		const ibDataViewItem& item = GetSelection();
		if (!item.IsOk())
			break;
		if (m_modelProvider != nullptr) {
			ibValue retValue; GetValueByMetaID(item, m_modelProvider->GetCurrentModelColumn(), retValue);
			m_filterRow.SetFilterByID(m_modelProvider->GetCurrentModelColumn(), retValue);
		}
		CallRefreshModel(ibDataViewItem(nullptr), m_modelProvider != nullptr ? m_modelProvider->GetCountPerPage() : defaultCountPerPage);
		break;
	}
	case eFilterClear:
		m_filterRow.ResetFilter();
		CallRefreshModel(ibDataViewItem(nullptr), m_modelProvider != nullptr ? m_modelProvider->GetCountPerPage() : defaultCountPerPage);
		break;
	case eViewMode:
		ShowViewMode();
		break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////

bool ibValueModel::ShowFilter()
{
	if (m_modelProvider == nullptr)
		return false;
	return m_modelProvider->ShowFilter(m_filterRow);
}

bool ibValueModel::ShowViewMode()
{
	if (m_modelProvider == nullptr)
		return false;
	return m_modelProvider->ShowViewMode();
}

///////////////////////////////////////////////////////////////////////////////////////

ibValueModel::ibValueModelColumnCollection::ibValueModelColumnInfo::ibValueModelColumnInfo() :
	ibValue(ibValueTypes::TYPE_VALUE, true), m_methodHelper(new ibValueMethodHelper())
{
}

ibValueModel::ibValueModelColumnCollection::ibValueModelColumnInfo::~ibValueModelColumnInfo()
{
	wxDELETE(m_methodHelper);
}

enum Prop {
	enColumnName,
	enColumnTypes,
	enColumnCaption,
	enColumnWidth
};

void ibValueModel::ibValueModelColumnCollection::ibValueModelColumnInfo::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendProp(wxT("Name"));
	m_methodHelper->AppendProp(wxT("Types"));
	m_methodHelper->AppendProp(wxT("Caption"));
	m_methodHelper->AppendProp(wxT("Width"));
}

bool ibValueModel::ibValueModelColumnCollection::ibValueModelColumnInfo::GetPropVal(const long lPropNum, ibValue& pvarPropVal)
{
	switch (lPropNum)
	{
	case enColumnName:
		pvarPropVal = GetColumnName();
		return true;
	case enColumnTypes:
		pvarPropVal = ibValue::CreateAndPrepareValueRef<ibValueTypeDescription>(GetColumnType());
		return true;
	case enColumnCaption:
		pvarPropVal = GetColumnCaption();
		return true;
	case enColumnWidth:
		pvarPropVal = GetColumnWidth();
		return true;
	}

	return false;
}

ibValueModel::ibValueModelColumnCollection::ibValueModelColumnInfo* ibValueModel::ibValueModelColumnCollection::GetColumnByID(unsigned int col) const
{
	for (unsigned int idx = 0; idx < GetColumnCount(); idx++) {
		ibValueModelColumnInfo* columnInfo = GetColumnInfo(idx);
		wxASSERT(columnInfo);
		if (col == columnInfo->GetColumnID())
			return columnInfo;
	}

	return nullptr;
}

ibValueModel::ibValueModelColumnCollection::ibValueModelColumnInfo* ibValueModel::ibValueModelColumnCollection::GetColumnByName(const wxString& colName) const
{
	for (unsigned int idx = 0; idx < GetColumnCount(); idx++) {
		ibValueModelColumnInfo* columnInfo = GetColumnInfo(idx);
		wxASSERT(columnInfo);
		if (stringUtils::CompareString(colName, columnInfo->GetColumnName()))
			return columnInfo;
	}

	return nullptr;
}