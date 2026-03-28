#ifndef _META_CTOR_H__
#define _META_CTOR_H__

//metaobject register document, form, etc ... 
template <class T>
class ibCtorMetaType : public ibCtorValueTypeBase {

public:

	ibCtorMetaType(const wxString& className, const ibClassID& clsid) :ibCtorValueTypeBase(className, CLASSINFO(T), clsid) {}

	virtual wxIcon GetClassIcon() const { return T::GetIconGroup(); }
	virtual ibCtorObjectType GetObjectTypeCtor() const { return ibCtorObjectType::ibCtorObjectType_object_metadata; }

	virtual void CallEvent(ibCtorObjectTypeEvent event) {
		if (event == ibCtorObjectTypeEvent::ibCtorObjectTypeEvent_Register)
			T::OnRegisterObject(GetClassName(), this);
		else if (event == ibCtorObjectTypeEvent::ibCtorObjectTypeEvent_UnRegister)
			T::OnUnRegisterObject(GetClassName());
	}

	virtual ibValue* CreateObject() const { return new T(); }
};

#define METADATA_TYPE_REGISTER(class_info, class_name, clsid)\
GENERATE_REGISTER(wxT(class_name), wxMAKE_UNIQUE_NAME(s_cs_reg_m_), new ibCtorMetaType<class_info>(wxT(class_name), clsid))

#endif