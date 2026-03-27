#include "propertyModule.h"
#include "backend/propertyManager/property/variant/variantModule.h"

#define chunkForm 0x023456543

////////////////////////////////////////////////////////////////////////

wxVariantData* ibPropertyModule::CreateVariantData()
{
	return new ibVariantDataModule();
}

////////////////////////////////////////////////////////////////////////

wxString ibPropertyModule::GetValueAsString() const
{
	return get_cell_variant<ibVariantDataModule>()->GetModuleText();
}

void ibPropertyModule::SetValue(const wxString& val)
{
	get_cell_variant<ibVariantDataModule>()->SetModuleText(val);
}

////////////////////////////////////////////////////////////////////////

//base property for "module"
bool ibPropertyModule::SetDataValue(const ibValue& varPropVal)
{
	return false;
}

bool ibPropertyModule::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = GetName();
	return true;
}

bool ibPropertyModule::LoadData(ibReaderMemory& reader)
{
	ibPropertyModule::SetValue(reader.r_stringZ());
	return true;
}

bool ibPropertyModule::SaveData(ibWriterMemory& writer)
{
	writer.w_stringZ(GetValueAsString());
	return true;
}