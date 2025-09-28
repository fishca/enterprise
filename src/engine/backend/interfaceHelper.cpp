#include "interfaceHelper.h"

#define interfaceBlock 0x200020

bool IInterfaceObject::LoadInterface(CMemoryReader& dataReader)
{
	wxMemoryBuffer buf;
	if (dataReader.r_chunk(interfaceBlock, buf)) {
		std::shared_ptr<CMemoryReader> dataInterfaceReader(new CMemoryReader(buf));
		unsigned int countInterface = dataInterfaceReader->r_u32(); m_interfaces.clear();
		for (unsigned int idx = 0; idx < countInterface; idx++) {
			m_interfaces.emplace(dataInterfaceReader->r_s32());
		}
		return true;
	}

	return true;
}

bool IInterfaceObject::SaveInterface(CMemoryWriter& dataWritter)
{
	CMemoryWriter dataRoleWritter;
	dataRoleWritter.w_u32(m_interfaces.size());
	for (auto id : m_interfaces) {
		dataRoleWritter.w_s32(id); // interface id
	}
	dataWritter.w_chunk(interfaceBlock, dataRoleWritter.buffer());
	return true;
}
