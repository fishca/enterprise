#include "metaObjectComposite.h"
#include "backend/metadata.h"

wxIMPLEMENT_ABSTRACT_CLASS(ibValueMetaObjectCompositeData, ibValueMetaObject);

//***********************************************************************
//*							ibValueMetaObjectCompositeData				        *
//***********************************************************************

const ibCtorMetaValueType* ibValueMetaObjectCompositeData::GetTypeCtor(const ibCtorMetaType& refType) const
{
	return m_metaData->GetTypeCtor(this, refType);
}