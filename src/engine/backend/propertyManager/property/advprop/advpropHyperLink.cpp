#include "advpropHyperLink.h"
#include "backend/propertyManager/propertyEditor.h"

// -----------------------------------------------------------------------
// wxPGHyperLinkProperty
// -----------------------------------------------------------------------

wxPG_IMPLEMENT_PROPERTY_CLASS(wxPGHyperLinkProperty, wxPGProperty, HyperLink)

wxPGHyperLinkProperty::wxPGHyperLinkProperty(IPropertyObject* ownerProperty, const wxString& label,
	const wxString& name, const wxVariant& value) : wxPGProperty(label, name), m_ownerProperty(ownerProperty) {

	wxPGProperty::SetFlagRecursively(wxPGPropertyFlags::wxPG_PROP_READONLY, true);
	//wxPGProperty::SetFlagRecursively(wxPGPropertyFlags::wxPG_PROP_HIDDEN, true);
	wxPGProperty::SetFlagRecursively(wxPGPropertyFlags::wxPG_PROP_NOEDITOR, true);

	wxPGProperty::SetValue(wxVariant(false, wxT("hyperLink_clicked")));
}

wxPGHyperLinkProperty::~wxPGHyperLinkProperty()
{
}

wxString wxPGHyperLinkProperty::ValueToString(wxVariant& value, int argFlags) const
{
	return _("Open");
}

bool wxPGHyperLinkProperty::StringToValue(wxVariant& variant,
	const wxString& text,
	int argFlags) const
{
	return false;
}

#include "backend/metadata.h"
#include "backend/metaCollection/metaObject.h"

void wxPGHyperLinkProperty::OnSetValue()
{
	if (wxT("hyperLink_clicked") == m_value.GetName() && m_value.GetBool()) {
		IValueMetaObject* metaObject = dynamic_cast<IValueMetaObject*>(m_ownerProperty);
		if (metaObject != nullptr) {
			wxTheApp->CallAfter(
				[metaObject]() {
					IMetaData* metaData = metaObject->GetMetaData();
					if (metaData != nullptr) {
						IBackendMetadataTree* metaTree = metaData->GetMetaTree();
						if (metaTree != nullptr) metaTree->OpenFormMDI(metaObject);
					}
				}
			);
		}
	}
}
