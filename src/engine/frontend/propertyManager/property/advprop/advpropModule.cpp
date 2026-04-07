#include "advpropModule.h"
#include "advpropHyperLink.h"

#include "backend/propertyManager/property/propertyModule.h"
#include "frontend/propertyManager/property/private/prop.h"

// register frontend property 
class ibPropertyModuleLoader
{
public:
	ibPropertyModuleLoader()
	{
		ibPG_IMPLEMENT_PROPERTY_CALLBACK(ibPGHyperLinkProperty, ibPropertyModule::ms_propertyModule);
	}
}g_moduleLoader;