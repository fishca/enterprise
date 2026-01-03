#ifndef __ROLE_HELPER_H__
#define __ROLE_HELPER_H__

#include "backend_core.h"

//********************************************************************************************
//*                                     Defines                                              *
//********************************************************************************************

class BACKEND_API IAccessObject;

//********************************************************************************************
//*										 Role												 *
//********************************************************************************************

class BACKEND_API CRole {
public:

	wxString GetName() const { return m_roleName; }
	wxString GetLabel() const { return m_roleLabel.IsEmpty() ? m_roleName : m_roleLabel; }

	IAccessObject* GetRoleObject() const { return m_owner; }

	bool GetDefValue() const { return m_defValue; }

protected:

	CRole(IAccessObject* metaObject, const wxString& roleName, const wxString& roleLabel,
		const bool& value = true) :
		m_roleName(roleName),
		m_roleLabel(roleLabel),
		m_owner(metaObject),
		m_defValue(value)
	{
		InitRole(metaObject, value);
	}

private:

	friend class IAccessObject;

	void InitRole(IAccessObject* obj, const bool& value = true);

	wxString m_roleName;
	wxString m_roleLabel;
	IAccessObject* m_owner; // pointer to the owner object
	bool m_defValue;  // handler function name
};

struct CUserRoleInfo {

	CUserRoleInfo() {}
	CUserRoleInfo(const role_identifier_t& id) : m_arrayRole({ id }) {}
	CUserRoleInfo(const std::vector<role_identifier_t>& array) : m_arrayRole(array) {}

	bool IsSetRole() const { return m_arrayRole.size() > 0; }

	std::vector<role_identifier_t> m_arrayRole;
};

#include "backend/fileSystem/fs.h"

class BACKEND_API IAccessObject {
public:

	virtual ~IAccessObject();

	bool AccessRight(const CRole* role) const { return AccessRight(role, GetUserRoleInfo()); }
	bool AccessRight(const wxString& strRoleName) const { return AccessRight(strRoleName, GetUserRoleInfo()); }

	bool AccessRight(const CRole* role, const CUserRoleInfo& roleInfo) const;
	bool AccessRight(const wxString& strRoleName, const CUserRoleInfo& roleInfo) const {
		if (!strRoleName.IsEmpty()) {
			auto iterator = std::find_if(m_roles.begin(), m_roles.end(),
				[strRoleName](const auto& pair) { return stringUtils::CompareString(strRoleName, pair.first); });
			if (iterator != m_roles.end()) return AccessRight(iterator->second, roleInfo);
		}
		return false;
	}

	bool SetRight(const CRole* role, const role_identifier_t& id, const bool& val);
	bool SetRight(const wxString& strRoleName, const role_identifier_t& id, const bool& val) {
		if (!strRoleName.IsEmpty()) {
			auto iterator = std::find_if(m_roles.begin(), m_roles.end(),
				[strRoleName](const auto& pair) { return stringUtils::CompareString(strRoleName, pair.first); });
			if (iterator != m_roles.end()) return SetRight(iterator->second, id, val);
		}
		return false;
	}

	/**
	* Obtiene la propiedad identificada por el nombre.
	*
	* @note Notar que no existe el método SetProperty, ya que la modificación
	*       se hace a través de la referencia.
	*/
	CRole* GetRole(const wxString& nameParam) const;
	CRole* GetRole(unsigned int idx) const; // throws ...;

	/**
	* Obtiene el número de propiedades del objeto.
	*/
	unsigned int GetRoleCount() const {
		return (unsigned int)m_roles.size();
	}

	friend class CRole;

protected:

	virtual bool DoAccessRight(const CRole* role) const { return true; }
	virtual void DoSetRight(const CRole* role, const bool& set) {}

	//Create user info
	virtual CUserRoleInfo GetUserRoleInfo() const = 0;

	//load & save role in metaobject 
	bool LoadRole(CMemoryReader& reader);
	bool SaveRole(CMemoryWriter& writer = CMemoryWriter());

	/**
	* Añade una propiedad al objeto.
	*
	* Este método será usado por el registro de descriptores para crear la
	* instancia del objeto.
	* Los objetos siempre se crearán a través del registro de descriptores.
	*/
	void AddRole(CRole* value);

	template <typename... Args>
	inline CRole* CreateRole(Args&&... args) {
		return new CRole(this, std::forward<Args>(args)...);
	}

	std::map<role_identifier_t,
		std::map<wxString, bool>
	> m_valRoles;

	std::vector<std::pair<wxString, CRole*>> m_roles;
};

#endif