#include "advpropPoint.h"

// -----------------------------------------------------------------------
// wxPGPointProperty
// -----------------------------------------------------------------------

#if wxCHECK_VERSION(3, 1, 0)
wxPG_IMPLEMENT_PROPERTY_CLASS(wxPGPointProperty, wxPGProperty, TextCtrl)
#else
WX_PG_IMPLEMENT_PROPERTY_CLASS(wxPGPointProperty, wxPGProperty, wxPoint, const wxPoint&, TextCtrl)
#endif

wxPGPointProperty::wxPGPointProperty(const wxString& label,
	const wxString& strName,
	const wxPoint& value) : wxPGProperty(label, strName)
{
	DoSetValue(value);
	AddPrivateChild(new wxIntProperty(_("X"), wxT("x"), value.x));
	AddPrivateChild(new wxIntProperty(_("Y"), wxT("y"), value.y));
}

wxPGPointProperty::~wxPGPointProperty() {}

void wxPGPointProperty::RefreshChildren()
{
	if (GetChildCount() < 2) return;

	const wxPoint& point = wxPointRefFromVariant(m_value);

	Item(0)->SetValue((long)point.x);
	Item(1)->SetValue((long)point.y);
}

wxVariant wxPGPointProperty::ChildChanged(wxVariant& thisValue, const int childIndex,
	wxVariant& childValue) const {
	wxPoint point;
	point << thisValue;
	int val = childValue.GetLong();
	switch (childIndex)
	{
		case 0:
			point.x = val;
			break;
		case 1:
			point.y = val;
			break;
	}
	wxVariant newVariant;
	newVariant << point;
	return newVariant;
}