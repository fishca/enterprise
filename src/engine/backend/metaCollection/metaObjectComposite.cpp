#include "metaObjectComposite.h"
#include "backend/metadata.h"

wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectCompositeData, IMetaObject);

//***********************************************************************
//*							IMetaObjectCompositeData				        *
//***********************************************************************

const IMetaValueTypeCtor* IMetaObjectCompositeData::GetTypeCtor(const eCtorMetaType& refType) const
{
	return m_metaData->GetTypeCtor(this, refType);
}