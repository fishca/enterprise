#include "propertyForm.h"
#include "backend/propertyManager/property/variant/formVariant.h"

#define chunkForm 0x023456543

////////////////////////////////////////////////////////////////////////

wxVariantData* CPropertyForm::CreateVariantData()
{
	return new wxVariantDataForm();
}

////////////////////////////////////////////////////////////////////////

wxString CPropertyForm::GetValueAsString() const
{
	return get_cell_variant<wxVariantDataForm>()->GetModuleText();
}

wxMemoryBuffer& CPropertyForm::GetValueAsMemoryBuffer() const
{
	return get_cell_variant<wxVariantDataForm>()->GetFormData();
}

void CPropertyForm::SetValue(const wxString& val)
{
	get_cell_variant<wxVariantDataForm>()->SetModuleText(val);
}

void CPropertyForm::SetValue(const wxMemoryBuffer& val)
{
	get_cell_variant<wxVariantDataForm>()->SetFormData(val);
}

////////////////////////////////////////////////////////////////////////

//base property for "form"
bool CPropertyForm::SetDataValue(const CValue& varPropVal)
{
	return false;
}

bool CPropertyForm::GetDataValue(CValue& pvarPropVal) const
{
	pvarPropVal = GetName();
	return true;
}

bool CPropertyForm::LoadData(CMemoryReader& reader)
{
	reader.r_chunk(chunkForm, GetValueAsMemoryBuffer());
	CPropertyForm::SetValue(reader.r_stringZ());
	return true;
}

bool CPropertyForm::SaveData(CMemoryWriter& writer)
{
	writer.w_chunk(chunkForm, GetValueAsMemoryBuffer());
	writer.w_stringZ(GetValueAsString());
	return true;
}

//////////////////////////////////////////////////////////

#include "backend/metaCollection/metaFormObject.h"

bool CPropertyForm::PasteData(CMemoryReader& reader)
{
	IMetaObjectForm* metaForm = dynamic_cast<IMetaObjectForm*>(m_owner);
	if (metaForm == nullptr) return false;
	reader.r_chunk(chunkForm, GetValueAsMemoryBuffer());
	CPropertyForm::SetValue(reader.r_stringZ());
	return true;
}

bool CPropertyForm::CopyData(CMemoryWriter& writer)
{
	IMetaObjectForm* metaForm = dynamic_cast<IMetaObjectForm*>(m_owner);
	if (metaForm == nullptr) return false; 
	writer.w_chunk(chunkForm, metaForm->CopyFormData());
	writer.w_stringZ(GetValueAsString());
	return true;
}

//////////////////////////////////////////////////////////
