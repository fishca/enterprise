////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : spreadsheet object
////////////////////////////////////////////////////////////////////////////

#include "metaSpreadsheetObject.h"
#include "backend/metaData.h"

wxIMPLEMENT_ABSTRACT_CLASS(IValueMetaObjectSpreadsheet, IValueMetaObject);

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectSpreadsheet, IValueMetaObjectSpreadsheet);
wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectCommonSpreadsheet, IValueMetaObjectSpreadsheet);

//***********************************************************************
//*                           read & save events                        *
//***********************************************************************

bool IValueMetaObjectSpreadsheet::OnBeforeRunMetaObject(int flags)
{
	return IValueMetaObject::OnBeforeRunMetaObject(flags);
}

bool IValueMetaObjectSpreadsheet::OnAfterCloseMetaObject()
{
	return IValueMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                           Spreadsheet                               *
//***********************************************************************

bool CValueMetaObjectSpreadsheet::LoadData(CMemoryReader& reader)
{
	return m_propertyTemplate->LoadData(reader);
}

bool CValueMetaObjectSpreadsheet::SaveData(CMemoryWriter& writer)
{
	return m_propertyTemplate->SaveData(writer);
}

//***********************************************************************
//*                       Common Spreadsheet							*
//***********************************************************************

bool CValueMetaObjectCommonSpreadsheet::LoadData(CMemoryReader& reader)
{
	return m_propertyTemplate->LoadData(reader);
}

bool CValueMetaObjectCommonSpreadsheet::SaveData(CMemoryWriter& writer)
{
	return m_propertyTemplate->SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectSpreadsheet, "template", g_metaTemplateCLSID);
METADATA_TYPE_REGISTER(CValueMetaObjectCommonSpreadsheet, "commonTemplate", g_metaCommonTemplateCLSID);
