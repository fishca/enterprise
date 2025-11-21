#ifndef __SRC_OBJECT_H__
#define __SRC_OBJECT_H__

//********************************************************************************************
//*                                     Defines                                              *
//********************************************************************************************

class BACKEND_API IMetaObjectCompositeData;

//********************************************************************************************
//*								      ISourceObject											 *
//********************************************************************************************

class ISourceObject {
public:

	virtual ~ISourceObject() {}
	
	//get metaData from object 
	virtual IMetaObjectCompositeData* GetSourceMetaObject() const = 0;
	
	//Get ref class 
	virtual class_identifier_t GetSourceClassType() const = 0;

	//Get presentation 
	virtual wxString GetSourceCaption() const = 0;
};

//********************************************************************************************
//*								      ITabularObject										 *
//********************************************************************************************

class ITabularObject {
public:

	virtual ~ITabularObject() {}

	//get metaData from object 
	virtual IMetaObjectCompositeData* GetSourceMetaObject() const = 0;

	//Get ref class 
	virtual class_identifier_t GetSourceClassType() const = 0;
};

#endif 