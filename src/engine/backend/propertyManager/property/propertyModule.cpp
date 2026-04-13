#include "propertyModule.h"
#include "backend/propertyManager/property/variant/variantModule.h"

#define chunkForm 0x023456543

////////////////////////////////////////////////////////////////////////

wxVariantData* CPropertyModule::CreateVariantData()
{
	return new wxVariantDataModule();
}

////////////////////////////////////////////////////////////////////////

wxString CPropertyModule::GetValueAsString() const
{
	return get_cell_variant<wxVariantDataModule>()->GetModuleText();
}

void CPropertyModule::SetValue(const wxString& val)
{
	get_cell_variant<wxVariantDataModule>()->SetModuleText(val);
}

////////////////////////////////////////////////////////////////////////

//base property for "module"
bool CPropertyModule::SetDataValue(const CValue& varPropVal)
{
	return false;
}

bool CPropertyModule::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = GetName();
	return true;
}

bool CPropertyModule::LoadData(CMemoryReader& reader)
{
	CPropertyModule::SetValue(reader.r_stringZ());
	return true;
}

bool CPropertyModule::SaveData(CMemoryWriter& writer)
{
	writer.w_stringZ(GetValueAsString());
	return true;
}