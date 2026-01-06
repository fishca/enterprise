#include "metaInterfaceObject.h"

//***********************************************************************
//*                            IntrfaceObject                           *
//***********************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CMetaObjectInterface, IMetaObject);

//***********************************************************************
//*                           Metamodule                                *
//***********************************************************************

CMetaObjectInterface::CMetaObjectInterface(const wxString& name, const wxString& synonym, const wxString& comment) :
	IMetaObject(name, synonym, comment)
{
}

#include "backend/metadata.h"

bool CMetaObjectInterface::GetInterfaceItemArrayObject(EInterfaceCommandSection cmdSection,
	std::vector<IMetaObject*>& array) const
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

bool CMetaObjectInterface::LoadData(CMemoryReader& reader)
{
	return m_propertyPicture->LoadData(reader);
}

bool CMetaObjectInterface::SaveData(CMemoryWriter& writer)
{
	return m_propertyPicture->SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

METADATA_TYPE_REGISTER(CMetaObjectInterface, "interface", g_metaInterfaceCLSID);