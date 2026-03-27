////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : enum unit
////////////////////////////////////////////////////////////////////////////

#include "enumUnit.h"

ibValueEnumerationWrapper::ibValueEnumerationWrapper(bool createInstance) :
	ibValue(ibValueTypes::TYPE_VALUE, true), m_methodHelper(nullptr)
{
	if (createInstance) {
		m_methodHelper = new ibValueMethodHelper();
	}
}

ibValueEnumerationWrapper::~ibValueEnumerationWrapper()
{
	wxDELETE(m_methodHelper);
}

void ibValueEnumerationWrapper::PrepareNames() const
{
	if (m_methodHelper != nullptr) {
		m_methodHelper->ClearHelper();
		for (auto &obj : m_listEnumStr) {
			m_methodHelper->AppendProp(obj);
		}
	}
}