#include "propertyString.h"

//base property for "caption" - for translate 
bool CPropertyTString::SetDataValue(const CValue& varPropVal)
{
	IPropertyString::SetValue(
		CBackendLocalization::CreateLocalizationRawLocText(varPropVal.GetString())
	);
	return true;
}

bool CPropertyTString::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = CPropertyTString::GetValueAsTranslateString();
	return true;
}