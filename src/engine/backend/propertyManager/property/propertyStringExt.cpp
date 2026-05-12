#include "propertyString.h"

//base property for "caption" - for translate 
bool ibPropertyTString::SetDataValue(const ibValue& varPropVal)
{
	ibPropertyStringBase::SetValue(
		ibBackendLocalization::CreateLocalizationRawLocText(varPropVal.GetString())
	);
	return true;
}

bool ibPropertyTString::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = ibPropertyTString::GetValueAsTranslateString();
	return true;
}