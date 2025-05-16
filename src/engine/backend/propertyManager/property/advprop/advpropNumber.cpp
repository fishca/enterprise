#include "advpropNumber.h"
#include "backend/propertyManager/property/variant/numberVariant.h"

// -----------------------------------------------------------------------
// wxPGSourceDataProperty
// -----------------------------------------------------------------------

wxPG_IMPLEMENT_PROPERTY_CLASS(wxNumberProperty, wxNumericProperty, TextCtrl)

wxNumberProperty::wxNumberProperty(const wxString& label,
	const wxString& strName,
	const number_t& value)
	: wxNumericProperty(label, strName)
{
	m_precision = -1;
	SetValue(
		new wxVariantDataNumber(value)
	);
}

wxNumberProperty::~wxNumberProperty() {}

wxString wxNumberProperty::ValueToString(wxVariant& variant,
	int argFlags) const
{
	wxString text;
	if (!variant.IsNull()) {
		wxVariantDataNumber* numberVariant =
			dynamic_cast<wxVariantDataNumber*>(variant.GetData());
		const number_t& value = numberVariant->GetNumber();
		text = value.ToString();
	}
	return text;
}

bool wxNumberProperty::StringToValue(wxVariant& variant, const wxString& text, int argFlags) const
{
	number_t value;
	wxVariantDataNumber* numberVariant =
		dynamic_cast<wxVariantDataNumber*>(variant.GetData());
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
	else if (argFlags & wxPG_REPORT_ERROR) {
	}
	return false;
}

bool
wxNumberProperty::ValidateValue(wxVariant& variant,
	wxPGValidationInfo& validationInfo) const
{
	number_t fpv = 0.0f;
	wxVariantDataNumber* numberVariant = dynamic_cast<wxVariantDataNumber*>(variant.GetData());
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
	number_t v = 0.0f;
	wxVariantDataNumber* numberVariant = dynamic_cast<wxVariantDataNumber*>(value.GetData());
	if (numberVariant != nullptr) {
		v = numberVariant->GetNumber();
	}
	double step = m_spinStep.GetDouble();
	v += (step * stepScale);
	DoNumericValidation(v, nullptr, mode);
	numberVariant->SetNumber(v);
	return value;
}