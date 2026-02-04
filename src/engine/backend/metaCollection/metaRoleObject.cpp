#include "metaRoleObject.h"

//***********************************************************************
//*                            RoleObject                               *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectRole, IValueMetaObject);

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

CValueMetaObjectRole::CValueMetaObjectRole(const wxString& name, const wxString& synonym, const wxString& comment) :
	IValueMetaObject(name, synonym, comment)
{
}

//***********************************************************************
//*                             event object                            *
//***********************************************************************

bool CValueMetaObjectRole::OnCreateMetaObject(IMetaData* metaData, int flags)
{
	return IValueMetaObject::OnCreateMetaObject(metaData, flags);
}

bool CValueMetaObjectRole::OnBeforeRunMetaObject(int flags)
{
	return IValueMetaObject::OnBeforeRunMetaObject(flags);
}

bool CValueMetaObjectRole::OnAfterCloseMetaObject()
{
	return IValueMetaObject::OnAfterCloseMetaObject();
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectRole, "role", g_metaRoleCLSID);