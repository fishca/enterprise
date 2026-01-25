////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : spreadsheet object
////////////////////////////////////////////////////////////////////////////

#include "metaSpreadsheetObject.h"
#include "backend/metaData.h"

wxIMPLEMENT_ABSTRACT_CLASS(IMetaObjectSpreadsheet, IMetaObject);

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectSpreadsheet, IMetaObjectSpreadsheet);
wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectCommonSpreadsheet, IMetaObjectSpreadsheet);

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IMetaObjectSpreadsheet::OnBeforeRunMetaObject(int flags)
{
	return IMetaObject::OnBeforeRunMetaObject(flags);
}

bool IMetaObjectSpreadsheet::OnAfterCloseMetaObject()
{
	return IMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                           Spreadsheet                               *
//***********************************************************************

bool CMetaObjectSpreadsheet::LoadData(CMemoryReader& reader)
{
	return m_propertyTemplate->LoadData(reader);
}

bool CMetaObjectSpreadsheet::SaveData(CMemoryWriter& writer)
{
	return m_propertyTemplate->SaveData(writer);
}

//***********************************************************************
//*                       Common Spreadsheet							*
//***********************************************************************

bool CMetaObjectCommonSpreadsheet::LoadData(CMemoryReader& reader)
{
	return m_propertyTemplate->LoadData(reader);
}

bool CMetaObjectCommonSpreadsheet::SaveData(CMemoryWriter& writer)
{
	return m_propertyTemplate->SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectSpreadsheet, "template", g_metaTemplateCLSID);
METADATA_TYPE_REGISTER(CMetaObjectCommonSpreadsheet, "commonTemplate", g_metaCommonTemplateCLSID);
