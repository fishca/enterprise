#ifndef _ATTRIBUTE_CONTROL_H__
#define _ATTRIBUTE_CONTROL_H__

#include "frontend/frontend.h"

///////////////////////////////////////////////////////////////////////////
#include "backend/backend_type.h"
#include "backend/compiler/value.h"
///////////////////////////////////////////////////////////////////////////

class BACKEND_API IMetaData;

class BACKEND_API IValueMetaObject;
class BACKEND_API IValueMetaObjectAttribute;
class BACKEND_API IValueMetaObjectGenericData;
class BACKEND_API CValueMetaObjectTableData;

class BACKEND_API ISourceDataObject;

///////////////////////////////////////////////////////////////////////////
class FRONTEND_API IControlFrame;
///////////////////////////////////////////////////////////////////////////

#include "backend/srcObject.h"

class FRONTEND_API ITypeControlFactory : public IBackendTypeSourceFactory {
public:

	//////////////////////////////////////////////////
	virtual IValueMetaObjectAttribute* GetSourceAttributeObject() const = 0;
	//////////////////////////////////////////////////

	static bool SimpleChoice(IControlFrame* ownerValue, const class_identifier_t& clsid, wxWindow* parent);

	static bool QuickChoice(IControlFrame* ownerValue, const class_identifier_t& clsid, wxWindow* parent);
	static void QuickChoice(IControlFrame* controlValue, CValue& newValue, wxWindow* parent, const wxString& strData);

	static class_identifier_t ShowSelectType(IMetaData* metadata, const CTypeDescription& typeDescription);

	//////////////////////////////////////////////////

	enum eSelectMode GetSelectMode() const;
	
	//Create value by selected type
	virtual CValue CreateValue() const;
	virtual CValue* CreateValueRef() const;

	//Get data type 
	virtual class_identifier_t GetDataType() const;
};

#endif