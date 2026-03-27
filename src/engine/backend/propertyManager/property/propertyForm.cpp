#include "propertyForm.h"
#include "backend/propertyManager/property/variant/variantForm.h"

#define chunkForm 0x023456543

////////////////////////////////////////////////////////////////////////

wxVariantData* ibPropertyForm::CreateVariantData()
{
	return new ibVariantDataForm();
}

////////////////////////////////////////////////////////////////////////

wxString ibPropertyForm::GetValueAsString() const
{
	return get_cell_variant<ibVariantDataForm>()->GetModuleText();
}

wxMemoryBuffer& ibPropertyForm::GetValueAsMemoryBuffer() const
{
	return get_cell_variant<ibVariantDataForm>()->GetFormData();
}

void ibPropertyForm::SetValue(const wxString& val)
{
	get_cell_variant<ibVariantDataForm>()->SetModuleText(val);
}

void ibPropertyForm::SetValue(const wxMemoryBuffer& val)
{
	get_cell_variant<ibVariantDataForm>()->SetFormData(val);
}

////////////////////////////////////////////////////////////////////////

//base property for "form"
bool ibPropertyForm::SetDataValue(const ibValue& varPropVal)
{
	return false;
}

bool ibPropertyForm::GetDataValue(ibValue& pvarPropVal) const
{
	pvarPropVal = GetName();
	return true;
}

bool ibPropertyForm::LoadData(ibReaderMemory& reader)
{
	reader.r_chunk(chunkForm, GetValueAsMemoryBuffer());
	ibPropertyForm::SetValue(reader.r_stringZ());
	return true;
}

bool ibPropertyForm::SaveData(ibWriterMemory& writer)
{
	writer.w_chunk(chunkForm, GetValueAsMemoryBuffer());
	writer.w_stringZ(GetValueAsString());
	return true;
}

//////////////////////////////////////////////////////////

#include "backend/metaCollection/metaFormObject.h"

bool ibPropertyForm::PasteData(ibReaderMemory& reader)
{
	ibValueMetaObjectFormBase* metaForm = dynamic_cast<ibValueMetaObjectFormBase*>(m_owner);
	if (metaForm == nullptr) return false;
	reader.r_chunk(chunkForm, GetValueAsMemoryBuffer());
	ibPropertyForm::SetValue(reader.r_stringZ());	
	return metaForm->PasteFormData();
}

bool ibPropertyForm::CopyData(ibWriterMemory& writer)
{
	ibValueMetaObjectFormBase* metaForm = dynamic_cast<ibValueMetaObjectFormBase*>(m_owner);
	if (metaForm == nullptr) return false; 
	writer.w_chunk(chunkForm, metaForm->CopyFormData());
	writer.w_stringZ(GetValueAsString());
	return true;
}

//////////////////////////////////////////////////////////
