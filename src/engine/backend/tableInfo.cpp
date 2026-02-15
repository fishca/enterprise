////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : table model information
////////////////////////////////////////////////////////////////////////////

#include "tableInfo.h"

wxIMPLEMENT_ABSTRACT_CLASS(IValueModel, CValue);
wxIMPLEMENT_ABSTRACT_CLASS(IValueModel::IValueModelColumnCollection, CValue);
wxIMPLEMENT_ABSTRACT_CLASS(IValueModel::IValueModelColumnCollection::IValueModelColumnInfo, CValue);
wxIMPLEMENT_ABSTRACT_CLASS(IValueModel::IValueModelReturnLine, CValue);

wxIMPLEMENT_ABSTRACT_CLASS(IValueTable, IValueModel);
wxIMPLEMENT_ABSTRACT_CLASS(IValueTree, IValueModel);

IValueModel::IValueModel()
	: CValue(eValueTypes::TYPE_VALUE), 
	m_modelProvider(nullptr), 
	m_srcNotifier(nullptr), 
	m_refreshModel(false)
{
	m_modelProvider = new CDataViewModelProvider(this);
	//m_modelProvider->IncRef(); // always one 
}

IValueModel::~IValueModel()
{
	m_modelProvider->DecRef();
}

wxDataViewItem IValueModel::GetSelection() const
{
	if (m_srcNotifier == nullptr)
		return wxDataViewItem(nullptr);
	return m_srcNotifier->GetSelection();
}

void IValueModel::RowValueStartEdit(const wxDataViewItem& item, unsigned int col)
{
	if (m_srcNotifier == nullptr)
		return;
	m_srcNotifier->StartEditing(item, col);
}

IValueModel::CActionCollection IValueModel::GetActionCollection(const form_identifier_t& formType)
{
	CActionCollection action(this);

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

	return action;
}

void IValueModel::ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm)
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
				CallRefreshModel(wxDataViewItem(nullptr), m_srcNotifier != nullptr ? m_srcNotifier->GetCountPerPage() : defaultCountPerPage);
			}
			break;
		case eFilterByColumn:
		{
			const wxDataViewItem& item = GetSelection();
			if (!item.IsOk())
				break;
			wxDataViewColumn* dataView = m_srcNotifier != nullptr ? m_srcNotifier->GetCurrentColumn() : nullptr;
			if (dataView != nullptr) {
				CValue retValue; GetValueByMetaID(item, dataView->GetModelColumn(), retValue);
				m_filterRow.SetFilterByID(dataView->GetModelColumn(), retValue);
			}
			CallRefreshModel(wxDataViewItem(nullptr), m_srcNotifier != nullptr ? m_srcNotifier->GetCountPerPage() : defaultCountPerPage);
			break;
		}
		case eFilterClear:
			m_filterRow.ResetFilter();
			CallRefreshModel(wxDataViewItem(nullptr), m_srcNotifier != nullptr ? m_srcNotifier->GetCountPerPage() : defaultCountPerPage);
			break;
	}
}

///////////////////////////////////////////////////////////////////////////////////////

bool IValueModel::ShowFilter()
{
	if (m_srcNotifier == nullptr)
		return false;
	return m_srcNotifier->ShowFilter(m_filterRow);
}

///////////////////////////////////////////////////////////////////////////////////////

IValueModel::IValueModelColumnCollection::IValueModelColumnInfo::IValueModelColumnInfo() :
	CValue(eValueTypes::TYPE_VALUE, true), m_methodHelper(new CMethodHelper())
{
}

IValueModel::IValueModelColumnCollection::IValueModelColumnInfo::~IValueModelColumnInfo()
{
	wxDELETE(m_methodHelper);
}

enum Prop {
	enColumnName,
	enColumnTypes,
	enColumnCaption,
	enColumnWidth
};

void IValueModel::IValueModelColumnCollection::IValueModelColumnInfo::PrepareNames() const
{
	m_methodHelper->ClearHelper();

	m_methodHelper->AppendProp(wxT("Name"));
	m_methodHelper->AppendProp(wxT("Types"));
	m_methodHelper->AppendProp(wxT("Caption"));
	m_methodHelper->AppendProp(wxT("Width"));
}

bool IValueModel::IValueModelColumnCollection::IValueModelColumnInfo::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	switch (lPropNum)
	{
		case enColumnName:
			pvarPropVal = GetColumnName();
			return true;
		case enColumnTypes:
			pvarPropVal = CValue::CreateAndPrepareValueRef<CValueTypeDescription>(GetColumnType());
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

IValueModel::IValueModelColumnCollection::IValueModelColumnInfo* IValueModel::IValueModelColumnCollection::GetColumnByID(unsigned int col) const
{
	for (unsigned int idx = 0; idx < GetColumnCount(); idx++) {
		IValueModelColumnInfo* columnInfo = GetColumnInfo(idx);
		wxASSERT(columnInfo);
		if (col == columnInfo->GetColumnID())
			return columnInfo;
	}

	return nullptr;
}



IValueModel::IValueModelColumnCollection::IValueModelColumnInfo* IValueModel::IValueModelColumnCollection::GetColumnByName(const wxString& colName) const
{
	for (unsigned int idx = 0; idx < GetColumnCount(); idx++) {
		IValueModelColumnInfo* columnInfo = GetColumnInfo(idx);
		wxASSERT(columnInfo);
		if (stringUtils::CompareString(colName, columnInfo->GetColumnName()))
			return columnInfo;
	}

	return nullptr;
}