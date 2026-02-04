#include "metaInterfaceObject.h"

//***********************************************************************
//*                            IntrfaceObject                           *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueMetaObjectInterface, IValueMetaObject);

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

CValueMetaObjectInterface::CValueMetaObjectInterface(const wxString& name, const wxString& synonym, const wxString& comment) :
	IValueMetaObject(name, synonym, comment)
{
}

#include "backend/metadata.h"

bool CValueMetaObjectInterface::GetInterfaceItemArrayObject(EInterfaceCommandSection cmdSection,
	std::vector<IValueMetaObject*>& array) const
{
	for (const auto object : m_metaData->GetAnyArrayObject()) {

		const EInterfaceCommandSection& object_type = object->GetCommandSection();

		//create + list 
		if (object->IsSetInterface(m_metaId) && ((EInterfaceCommandSection_Combined == object_type &&
			(cmdSection == EInterfaceCommandSection::EInterfaceCommandSection_Default || cmdSection == EInterfaceCommandSection::EInterfaceCommandSection_Create)) || cmdSection == object_type))
		{
			array.emplace_back(object);
		}
	}

	return array.size() > 0;
}

bool CValueMetaObjectInterface::LoadData(CMemoryReader& reader)
{
	return m_propertyPicture->LoadData(reader);
}

bool CValueMetaObjectInterface::SaveData(CMemoryWriter& writer)
{
	return m_propertyPicture->SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CValueMetaObjectInterface, "interface", g_metaInterfaceCLSID);