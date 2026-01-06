#ifndef __INTERFACE_HELPER_H__
#define __INTERFACE_HELPER_H__

#include "backend_core.h"
#include "backend/fileSystem/fs.h"

//********************************************************************************************
//*										 Sybsystem - interface								 *
//********************************************************************************************

class BACKEND_API IInterfaceObject {
public:

	virtual ~IInterfaceObject() {}

	void SetInterface(const meta_identifier_t& id, const bool& set = true) {
	
		if (set) m_interfaces.emplace(id);
		else m_interfaces.erase(id);

		DoSetInterface(id, set);
	}

	bool IsSetInterface(const meta_identifier_t& id) const {
		return m_interfaces.find(id) != m_interfaces.end();
	}

protected:

	virtual void DoSetInterface(const meta_identifier_t& id, const bool& set = true) {}

	//load & save subsystem in metaobject 
	bool LoadInterface(CMemoryReader& reader);
	bool SaveInterface(CMemoryWriter& writer = CMemoryWriter()) const;

	std::set<meta_identifier_t> m_interfaces;
};

#endif