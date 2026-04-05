#include "advpropNumber.h"
#include "backend/propertyManager/property/propertyNumber.h"
#include "backend/propertyManager/property/variant/variantNumber.h"
#include "frontend/propertyManager/property/private/prop.h"

// -----------------------------------------------------------------------
// wxPGSourceDataProperty
// -----------------------------------------------------------------------

wxPG_IMPLEMENT_PROPERTY_CLASS(wxNumberProperty, wxNumericProperty, TextCtrl)

// register frontend property 
class ibPropertyNumberLoader
{
public:
	ibPropertyNumberLoader()
	{
		ibPG_IMPLEMENT_PROPERTY_CALLBACK(wxNumberProperty, ibPropertyNumber::ms_propertyNumber);
		ibPG_IMPLEMENT_PROPERTY_CALLBACK(wxIntProperty, ibPropertyInteger::ms_propertyInteger);
		ibPG_IMPLEMENT_PROPERTY_CALLBACK(wxUIntProperty, ibPropertyUInteger::ms_propertyUInteger);
	}
}g_numberLoader;

wxNumberProperty::wxNumberProperty(const wxString& label,
	const wxString& strName,
	const ibNumber& value)
	: wxNumericProperty(label, strName)
{
	m_precision = -1;
	SetValue(
		new ibVariantDataNumber(value)
	);
}

wxNumberProperty::~wxNumberProperty() {}

wxString wxNumberProperty::ValueToString(wxVariant& variant,
	wxPGPropValFormatFlags flags) const
{
	wxString text;
	if (!variant.IsNull()) {
		ibVariantDataNumber* numberVariant =
			dynamic_cast<ibVariantDataNumber*>(variant.GetData());
		const ibNumber& value = numberVariant->GetNumber();
		text = value.ToString();
	}
	return text;
}

bool wxNumberProperty::StringToValue(wxVariant& variant, const wxString& text, wxPGPropValFormatFlags flags) const
{
	ibNumber value;
	ibVariantDataNumber* numberVariant =
		dynamic_cast<ibVariantDataNumber*>(variant.GetData());
	wxASSERT(numberVariant);
	if (text.empty()) {
		numberVariant->SetNumber(0.0f);
		return true;
	}

	bool res = value.FromString(text.ToStdWstring()) == 0;
	if (res) {
		if (numberVariant->GetNumber() != value) {
			numberVariant->SetNumber(value);
			return true;
		}
	}
	else if (!!(flags & wxPGPropValFormatFlags::ReportError)) {
	}
	return false;
}

bool
wxNumberProperty::ValidateValue(wxVariant& variant,
	wxPGValidationInfo& validationInfo) const
{
	ibNumber fpv = 0.0f;
	ibVariantDataNumber* numberVariant = dynamic_cast<ibVariantDataNumber*>(variant.GetData());
	if (numberVariant != nullptr) {
		fpv = numberVariant->GetNumber();
	}
	return DoNumericValidation(fpv, &validationInfo, wxPG_PROPERTY_VALIDATION_ERROR_MESSAGE);
}

bool wxNumberProperty::DoSetAttribute(const wxString& strName, wxVariant& variant)
{
	if (strName == wxPG_FLOAT_PRECISION) {
		m_precision = variant.GetLong();
		return true;
	}
	return wxNumericProperty::DoSetAttribute(strName, variant);
}

wxValidator*
wxNumberProperty::GetClassValidator()
{
#if wxUSE_VALIDATORS
	WX_PG_DOGETVALIDATOR_ENTRY()

		wxValidator* validator = new wxNumericPropertyValidator(
			wxNumericPropertyValidator::Float);

	WX_PG_DOGETVALIDATOR_EXIT(validator)
#else
	return nullptr;
#endif
}

wxValidator* wxNumberProperty::DoGetValidator() const
{
	return GetClassValidator();
}

wxVariant wxNumberProperty::AddSpinStepValue(long stepScale) const
{
	int mode = m_spinWrap ? wxPG_PROPERTY_VALIDATION_WRAP
		: wxPG_PROPERTY_VALIDATION_SATURATE;
	wxVariant value = GetValue();
	ibNumber v = 0.0f;
	ibVariantDataNumber* numberVariant = dynamic_cast<ibVariantDataNumber*>(value.GetData());
	if (numberVariant != nullptr) {
		v = numberVariant->GetNumber();
	}
	double step = m_spinStep.GetDouble();
	v += (step * stepScale);
	DoNumericValidation(v, nullptr, mode);
	numberVariant->SetNumber(v);
	return value;
}