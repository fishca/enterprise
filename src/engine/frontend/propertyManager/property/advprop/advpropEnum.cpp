#include "advpropEnum.h"

#include "backend/propertyManager/property/propertyEnum.h"
#include "frontend/propertyManager/property/private/prop.h"

// register frontend property 
class ibPropertyEnumLoader
{
public:
	ibPropertyEnumLoader()
	{
		ibPG_IMPLEMENT_PROPERTY_CALLBACK(wxEnumProperty, ibPropertyEnumBase::ms_propertyEnum);
	}
}g_enumLoader;