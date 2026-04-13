#include "advpropSize.h"

// -----------------------------------------------------------------------
// wxPGSizeProperty
// -----------------------------------------------------------------------
#if wxCHECK_VERSION(3, 1, 0)
wxPG_IMPLEMENT_PROPERTY_CLASS(wxPGSizeProperty, wxPGProperty, TextCtrl)
#else
WX_PG_IMPLEMENT_PROPERTY_CLASS(wxPGSizeProperty, wxPGProperty, wxSize, const wxSize&, TextCtrl)
#endif

wxPGSizeProperty::wxPGSizeProperty(const wxString& label,
	const wxString& strName,
	const wxSize& value) : wxPGProperty(label, strName)
{
	DoSetValue(value);
	AddPrivateChild(new wxIntProperty(_("Width"), wxT("width"), value.x));
	AddPrivateChild(new wxIntProperty(_("Height"), wxT("height"), value.y));
}

void wxPGSizeProperty::RefreshChildren()
{
	if (GetChildCount() < 2) return;

	const wxSize& size = wxSizeRefFromVariant(m_value);

	Item(0)->SetValue((long)size.x);
	Item(1)->SetValue((long)size.y);
}

wxVariant wxPGSizeProperty::ChildChanged(wxVariant& thisValue, const int childIndex,
	wxVariant& childValue) const {
	wxSize size;
	size << thisValue;
	const int val = childValue.GetLong();
	switch (childIndex)
	{
	case 0:
		size.x = val;
		break;
	case 1:
		size.y = val;
		break;
	}
	wxVariant newVariant;
	newVariant << size;
	return newVariant;
}