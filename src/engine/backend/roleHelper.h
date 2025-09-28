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
	wxString m_roleName;
	wxString m_roleLabel;
	IAccessObject* m_owner; // pointer to the owner object
	bool m_defValue;  // handler function name
private:
	void InitRole(IAccessObject* obj, const bool& value = true);
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
public:

	bool GetDefValue() const { return m_defValue; }

	wxString GetName() const { return m_roleName; }
	IAccessObject* GetRoleObject() const { return m_owner; }
	wxString GetLabel() const { return m_roleLabel.IsEmpty() ? m_roleName : m_roleLabel; }

	friend class IAccessObject;
};

#include "backend/fileSystem/fs.h"

class BACKEND_API IAccessObject {
public:

	virtual ~IAccessObject();

	bool AccessRight(const wxString& roleName) const { return true; }
	bool AccessRight(const wxString& roleName, const wxString& userName) const { return AccessRight(roleName); }
	bool AccessRight(const CRole* role) const { return AccessRight(role->GetName()); }
	bool AccessRight(const CRole* role, const wxString& userName) const { return AccessRight(role->GetName(), userName); }
	bool AccessRight(const CRole* role, const meta_identifier_t& id) const;
	bool AccessRight(const wxString& roleName, const meta_identifier_t& id) const {
		if (!roleName.IsEmpty()) {
			auto& it = std::find_if(m_roles.begin(), m_roles.end(),
				[roleName](const std::pair<wxString, CRole*>& pair) {return roleName == pair.first; }
			);
			if (it != m_roles.end()) return AccessRight(it->second, id);
		}
		return false;
	}

	bool SetRight(const CRole* role, const meta_identifier_t& id, const bool& val);
	bool SetRight(const wxString& roleName, const meta_identifier_t& id, const bool& val) {
		if (!roleName.IsEmpty()) {
			auto& it = std::find_if(m_roles.begin(), m_roles.end(),
				[roleName](const std::pair<wxString, CRole*> pair) {return roleName == pair.first; }
			);
			if (it != m_roles.end()) return SetRight(it->second, id, val);
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

	std::map<meta_identifier_t,
		std::map<wxString, bool>
	> m_valRoles;

	std::vector<std::pair<wxString, CRole*>> m_roles;
};

#endif