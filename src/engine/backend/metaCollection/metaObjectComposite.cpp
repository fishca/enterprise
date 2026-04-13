#include "metaObjectComposite.h"
#include "backend/metadata.h"

wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectCompositeData, IValueMetaObject);

//***********************************************************************
//*							IValueMetaObjectCompositeData				        *
//***********************************************************************

const IMetaValueTypeCtor* IValueMetaObjectCompositeData::GetTypeCtor(const eCtorMetaType& refType) const
{
	return m_metaData->GetTypeCtor(this, refType);
}