#include "tableView.h"

// ---------------------------------------------------------
// wxDataViewExtItemAttr
// ---------------------------------------------------------

wxFont wxDataViewExtItemAttr::GetEffectiveFont(const wxFont& font) const
{
	if (!HasFont())
		return font;

	wxFont f(font);
	if (GetBold())
		f.MakeBold();
	if (GetItalic())
		f.MakeItalic();
	if (GetStrikethrough())
		f.MakeStrikethrough();
	return f;
}


// ---------------------------------------------------------
// wxDataViewExtModelNotifier
// ---------------------------------------------------------

bool wxDataViewExtModelNotifier::ItemsAdded(const wxDataViewExtItem& parent, const wxDataViewExtItemArray& items)
{
	size_t count = items.GetCount();
	size_t i;
	for (i = 0; i < count; i++)
		if (!ItemAdded(parent, items[i])) return false;

	return true;
}

bool wxDataViewExtModelNotifier::ItemsDeleted(const wxDataViewExtItem& parent, const wxDataViewExtItemArray& items)
{
	size_t count = items.GetCount();
	size_t i;
	for (i = 0; i < count; i++)
		if (!ItemDeleted(parent, items[i])) return false;

	return true;
}

bool wxDataViewExtModelNotifier::ItemsChanged(const wxDataViewExtItemArray& items)
{
	size_t count = items.GetCount();
	size_t i;
	for (i = 0; i < count; i++)
		if (!ItemChanged(items[i])) return false;

	return true;
}

// ---------------------------------------------------------
// wxDataViewExtModel
// ---------------------------------------------------------

wxDataViewExtModel::wxDataViewExtModel()
{
}

wxDataViewExtModel::~wxDataViewExtModel()
{
	wxDataViewExtModelNotifiers::const_iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		delete* iter;
	}
}

bool wxDataViewExtModel::ItemAdded(const wxDataViewExtItem& parent, const wxDataViewExtItem& item)
{
	bool ret = true;

	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		wxDataViewExtModelNotifier* notifier = *iter;
		if (!notifier->ItemAdded(parent, item))
			ret = false;
	}

	return ret;
}

bool wxDataViewExtModel::ItemDeleted(const wxDataViewExtItem& parent, const wxDataViewExtItem& item)
{
	bool ret = true;

	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		wxDataViewExtModelNotifier* notifier = *iter;
		if (!notifier->ItemDeleted(parent, item))
			ret = false;
	}

	return ret;
}

bool wxDataViewExtModel::ItemChanged(const wxDataViewExtItem& item)
{
	bool ret = true;

	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		wxDataViewExtModelNotifier* notifier = *iter;
		if (!notifier->ItemChanged(item))
			ret = false;
	}

	return ret;
}

bool wxDataViewExtModel::ItemsAdded(const wxDataViewExtItem& parent, const wxDataViewExtItemArray& items)
{
	bool ret = true;

	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		wxDataViewExtModelNotifier* notifier = *iter;
		if (!notifier->ItemsAdded(parent, items))
			ret = false;
	}

	return ret;
}

bool wxDataViewExtModel::ItemsDeleted(const wxDataViewExtItem& parent, const wxDataViewExtItemArray& items)
{
	bool ret = true;

	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		wxDataViewExtModelNotifier* notifier = *iter;
		if (!notifier->ItemsDeleted(parent, items))
			ret = false;
	}

	return ret;
}

bool wxDataViewExtModel::ItemsChanged(const wxDataViewExtItemArray& items)
{
	bool ret = true;

	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		wxDataViewExtModelNotifier* notifier = *iter;
		if (!notifier->ItemsChanged(items))
			ret = false;
	}

	return ret;
}

bool wxDataViewExtModel::ValueChanged(const wxDataViewExtItem& item, unsigned int col)
{
	bool ret = true;

	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		wxDataViewExtModelNotifier* notifier = *iter;
		if (!notifier->ValueChanged(item, col))
			ret = false;
	}

	return ret;
}

bool wxDataViewExtModel::Cleared()
{
	bool ret = true;

	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		wxDataViewExtModelNotifier* notifier = *iter;
		if (!notifier->Cleared())
			ret = false;
	}

	return ret;
}

#pragma region __table_notifier__h__

unsigned int wxDataViewExtModel::GetCurrentModelColumn(int view_id) const
{
	if (m_notifiers.size() > 0)
		return m_notifiers[view_id]->GetCurrentModelColumn();
	return 0;
}

void wxDataViewExtModel::StartEditing(const wxDataViewExtItem& item, unsigned int col, int view_id) const
{
	if (m_notifiers.size() > 0)
		m_notifiers[view_id]->StartEditing(item, col);
}

bool wxDataViewExtModel::ShowFilter(CFilterRow& filter, int view_id)
{
	if (m_notifiers.size() > 0)
		return m_notifiers[view_id]->ShowFilter(filter);
	return false;
}

bool wxDataViewExtModel::ShowViewMode(int view_id)
{
	if (m_notifiers.size() > 0)
		return m_notifiers[view_id]->ShowViewMode();
	return false;
}

void wxDataViewExtModel::Select(const wxDataViewExtItem& item, int view_id) const
{
	if (m_notifiers.size() > 0)
		return m_notifiers[view_id]->Select(item);
}

int wxDataViewExtModel::GetCountPerPage(int view_id) const
{
	if (m_notifiers.size() > 0)
		return m_notifiers[view_id]->GetCountPerPage();
	return 0;
}

wxDataViewExtItem wxDataViewExtModel::GetSelection(int view_id) const
{
	if (m_notifiers.size() > 0)
		return m_notifiers[view_id]->GetSelection();
	return wxDataViewExtItem(nullptr);
}

int wxDataViewExtModel::GetSelections(wxDataViewExtItemArray& sel, int view_id) const
{
	if (m_notifiers.size() > 0)
		return m_notifiers[view_id]->GetSelections(sel);
	return 0;
}

#pragma endregion 

bool wxDataViewExtModel::BeforeReset()
{
	bool ret = true;

	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		wxDataViewExtModelNotifier* notifier = *iter;
		if (!notifier->BeforeReset())
			ret = false;
	}

	return ret;
}

bool wxDataViewExtModel::AfterReset()
{
	bool ret = true;

	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		wxDataViewExtModelNotifier* notifier = *iter;
		if (!notifier->AfterReset())
			ret = false;
	}

	return ret;
}

void wxDataViewExtModel::Resort()
{
	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		wxDataViewExtModelNotifier* notifier = *iter;
		notifier->Resort();
	}
}

void wxDataViewExtModel::AddNotifier(wxDataViewExtModelNotifier* notifier)
{
	m_notifiers.push_back(notifier);
	notifier->SetOwner(this);
}

void wxDataViewExtModel::RemoveNotifier(wxDataViewExtModelNotifier* notifier)
{
	wxDataViewExtModelNotifiers::iterator iter;
	for (iter = m_notifiers.begin(); iter != m_notifiers.end(); ++iter)
	{
		if (*iter == notifier)
		{
			delete notifier;
			m_notifiers.erase(iter);

			// Skip the assert below.
			return;
		}
	}

	wxFAIL_MSG(wxS("Removing non-registered notifier"));
}

int wxDataViewExtModel::Compare(const wxDataViewExtItem& item1, const wxDataViewExtItem& item2,
	unsigned int column, bool ascending) const
{
	wxVariant value1, value2;

	// Avoid calling GetValue() for the cells that are not supposed to have any
	// value, this might be unexpected.
	if (HasValue(item1, column))
		GetValue(value1, item1, column);
	if (HasValue(item2, column))
		GetValue(value2, item2, column);

	if (!ascending)
	{
		wxVariant temp = value1;
		value1 = value2;
		value2 = temp;
	}

	if (value1.GetType() == wxT("string"))
	{
		wxString str1 = value1.GetString();
		wxString str2 = value2.GetString();
		int res = str1.Cmp(str2);
		if (res)
			return res;
	}
	else if (value1.GetType() == wxT("long"))
	{
		long l1 = value1.GetLong();
		long l2 = value2.GetLong();
		if (l1 < l2)
			return -1;
		else if (l1 > l2)
			return 1;
	}
	else if (value1.GetType() == wxT("double"))
	{
		double d1 = value1.GetDouble();
		double d2 = value2.GetDouble();
		if (d1 < d2)
			return -1;
		else if (d1 > d2)
			return 1;
	}
#if wxUSE_DATETIME
	else if (value1.GetType() == wxT("datetime"))
	{
		wxDateTime dt1 = value1.GetDateTime();
		wxDateTime dt2 = value2.GetDateTime();
		if (dt1.IsEarlierThan(dt2))
			return -1;
		if (dt2.IsEarlierThan(dt1))
			return 1;
	}
#endif // wxUSE_DATETIME
	else if (value1.GetType() == wxT("bool"))
	{
		bool b1 = value1.GetBool();
		bool b2 = value2.GetBool();

		if (b1 != b2)
			return b1 ? 1 : -1;
	}
	else
	{
		int res = DoCompareValues(value1, value2);
		if (res != 0)
			return res;
	}


	// items must be different
	wxUIntPtr id1 = wxPtrToUInt(item1.GetID()),
		id2 = wxPtrToUInt(item2.GetID());

	return ascending ? id1 - id2 : id2 - id1;
}