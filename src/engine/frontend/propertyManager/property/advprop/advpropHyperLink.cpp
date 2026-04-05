#include "advpropHyperLink.h"

#include "frontend/propertyManager/property/private/prop.h"
#include "frontend/propertyManager/propertyEditor.h"

// -----------------------------------------------------------------------
// wxPGHyperLinkProperty
// -----------------------------------------------------------------------

wxPG_IMPLEMENT_PROPERTY_CLASS(wxPGHyperLinkProperty, wxPGProperty, HyperLink)

#include "backend/metaCollection/metaObject.h"

wxPGHyperLinkProperty::wxPGHyperLinkProperty(ibPropertyObject* ownerProperty, const wxString& label,
	const wxString& name, const wxVariant& value) : wxPGProperty(label, name), m_ownerProperty(ownerProperty) {

	wxPGProperty::SetFlagRecursively(wxPGFlags::ReadOnly, true);
	//wxPGProperty::SetFlagRecursively(wxPGFlags::Hidden, true);
	wxPGProperty::SetFlagRecursively(wxPGFlags::NoEditor, true);

	wxPGProperty::SetValue(wxVariant(false, wxT("hyperLink_clicked")));
}

wxPGHyperLinkProperty::~wxPGHyperLinkProperty()
{
}

wxString wxPGHyperLinkProperty::ValueToString( wxVariant& value, wxPGPropValFormatFlags flags ) const
{
	return _("Open");
}

bool wxPGHyperLinkProperty::StringToValue(wxVariant& variant,
	const wxString& text,
	wxPGPropValFormatFlags flags) const
{
	return false;
}

#include "backend/metadata.h"

void wxPGHyperLinkProperty::OnSetValue()
{
	if (wxT("hyperLink_clicked") == m_value.GetName() && m_value.GetBool()) {
		ibValueMetaObject* metaObject = dynamic_cast<ibValueMetaObject*>(m_ownerProperty);
		if (metaObject != nullptr) {
			wxTheApp->CallAfter(
				[metaObject]() {
					ibMetaData* metaData = metaObject->GetMetaData();
					if (metaData != nullptr) {
						ibBackendMetadataTree* metaTree = metaData->GetMetaTree();
						if (metaTree != nullptr) metaTree->OpenFormMDI(metaObject);
					}
				}
			);
		}
	}
}

void wxPGHyperLinkProperty::RefreshChildren()
{
	wxPGProperty::Enable(true);
}
