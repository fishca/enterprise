////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko 
//	Description : module information for ibValue 
////////////////////////////////////////////////////////////////////////////

#include "moduleInfo.h"

ibModuleDataObject::ibModuleDataObject() :
	m_compileModule(nullptr),
	m_procUnit(nullptr)
{
}

ibModuleDataObject::ibModuleDataObject(ibCompileModule* compileCode) :
	m_compileModule(compileCode), m_procUnit(nullptr)
{
}

ibModuleDataObject::~ibModuleDataObject()
{
	wxDELETE(m_compileModule);
	wxDELETE(m_procUnit);
}
