////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko 
//	Description : module information for CValue 
////////////////////////////////////////////////////////////////////////////

#include "moduleInfo.h"

IModuleDataObject::IModuleDataObject() :
	m_compileModule(nullptr),
	m_procUnit(nullptr)
{
}

IModuleDataObject::IModuleDataObject(CCompileModule* compileCode) :
	m_compileModule(compileCode), m_procUnit(nullptr)
{
}

IModuleDataObject::~IModuleDataObject()
{
	wxDELETE(m_compileModule);
	wxDELETE(m_procUnit);
}
