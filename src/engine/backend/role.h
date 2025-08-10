#ifndef __role_h__
#define __role_h__

#include "backend_core.h"

//*******************************************************************************
class BACKEND_API IMetaObject;
//*******************************************************************************

class CRole {
	wxString m_roleName;
	wxString m_roleLabel;
	IMetaObject* m_owner; // pointer to the owner object
	bool m_defValue;  // handler function name
	friend class IMetaObject;
private:
	void InitRole(IMetaObject* metaObject, const bool& value = true);
protected:
	CRole(IMetaObject* metaObject, const wxString& roleName, const wxString& roleLabel,
		const bool& value = true) :
		m_roleName(roleName),
		m_roleLabel(roleLabel),
		m_owner(metaObject),
		m_defValue(value)
	{
		InitRole(metaObject, value);
	}
public:

	bool GetDefValue() const { return m_defValue; }

	wxString GetName() const { return m_roleName; }
	IMetaObject* GetRoleObject() const { return m_owner; }
	wxString GetLabel() const { return m_roleLabel.IsEmpty() ? m_roleName : m_roleLabel; }
};

#endif