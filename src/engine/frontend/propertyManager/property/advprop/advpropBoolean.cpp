#include "advpropBoolean.h"

#include <wx/propgrid/advprops.h>

#include "backend/propertyManager/property/propertyBoolean.h"
#include "frontend/propertyManager/property/private/prop.h"

// register frontend property 
class ibPropertyBooleanLoader
{
public:
	ibPropertyBooleanLoader()
	{
		ibPG_IMPLEMENT_PROPERTY_CALLBACK(wxBoolProperty, ibPropertyBoolean::ms_propertyBoolean);
	}
}g_boolLoader;