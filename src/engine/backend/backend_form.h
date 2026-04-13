#ifndef _FRAME_INT_H__
#define _FRAME_INT_H__

#include "backend/uniqueKey.h"

///////////////////////////////////////////////////
class BACKEND_API IBackendValueForm;
///////////////////////////////////////////////////

class BACKEND_API IBackendMetaDocument {
public:
	virtual ~IBackendMetaDocument() {}
	virtual const class IValueMetaObject* GetMetaObject() const = 0;
};

class BACKEND_API IBackendControlFrame {
public:
#if !wxUSE_EXTENDED_RTTI
	virtual wxClassInfo* GetClassInfo() const;
	// RTTI information, usually declared by wxDECLARE_DYNAMIC_CLASS() or
	// similar, but done manually for the hierarchy root. Note that it's public
	// for compatibility reasons, but shouldn't be accessed directly.
	static wxClassInfo ms_classInfo;
#endif
	virtual ~IBackendControlFrame() {}

	virtual bool GetControlValue(CValue& pvarControlVal) const = 0;
	virtual CGuid GetControlGuid() const = 0;

	virtual IBackendValueForm* GetBackendForm() const { return nullptr; }

	// Get reference class 
	virtual class_identifier_t GetClassType() const = 0;

	// Counter reference
	virtual void ControlIncrRef() = 0;
	virtual void ControlDecrRef() = 0;
};

class BACKEND_API IBackendValueForm : public IBackendValue {
public:

#pragma region _frontend_call_h__

	// Form entry creator 
	static IBackendValueForm* CreateNewForm(const class IValueMetaObjectForm* creator = nullptr, IBackendControlFrame* ownerControl = nullptr,
		class ISourceDataObject* srcObject = nullptr, const CUniqueKey& formGuid = wxNullUniqueKey);

	static CUniqueKey CreateFormUniqueKey(IBackendControlFrame* ownerControl,
		ISourceDataObject* sourceObject, const CUniqueKey& formGuid);

	static IBackendValueForm* FindFormByUniqueKey(IBackendControlFrame* ownerControl,
		ISourceDataObject* sourceObject, const CUniqueKey& formGuid);

	static IBackendValueForm* FindFormByUniqueKey(const CUniqueKey& guid);
	static IBackendValueForm* FindFormByControlUniqueKey(const CUniqueKey& guid);
	static IBackendValueForm* FindFormBySourceUniqueKey(const CUniqueKey& guid);

	static bool UpdateFormUniqueKey(const CUniquePairKey& guid);

#pragma endregion 

	///////////////////////////////////////////////////////////////////////////
	virtual ~IBackendValueForm() {}
	///////////////////////////////////////////////////////////////////////////

	virtual bool LoadForm(const wxMemoryBuffer& formData) = 0;
	virtual wxMemoryBuffer SaveForm() = 0;

	///////////////////////////////////////////////////////////////////////////

	virtual ISourceDataObject* GetSourceObject() const = 0;
	virtual const IValueMetaObjectForm* GetFormMetaObject() const = 0;

	///////////////////////////////////////////////////////////////////////////

	virtual void BuildForm(const form_identifier_t& formType) = 0;
	virtual bool InitializeFormModule() = 0;

	//notify
	virtual void NotifyCreate(const CValue& vCreated) = 0;
	virtual void NotifyChange(const CValue& vChanged) = 0;
	virtual void NotifyDelete(const CValue& vChanged) = 0;

	virtual void NotifyChoice(CValue& vSelected) = 0;

	//form event
	virtual void ActivateForm() = 0;
	virtual void UpdateForm() = 0;
	virtual bool CloseForm(bool force = false) = 0;
	virtual void HelpForm() = 0;

	virtual bool GenerateForm(class IValueRecordDataObjectRef* obj) const = 0;
	virtual void ShowForm(IBackendMetaDocument* doc = nullptr, bool createContext = true) = 0;

	//set & get modify 
	virtual void Modify(bool modify = true) = 0;
	virtual bool IsModified() const = 0;

	//shown form 
	virtual bool IsShown() const = 0;

	//support close form
	virtual void CloseOnChoice(bool close = true) = 0;
	virtual bool IsCloseOnChoice() const = 0;

	virtual void CloseOnOwnerClose(bool close = true) = 0;
	virtual bool IsCloseOnOwnerClose() const = 0;
};

namespace formWrapper {
	namespace inl {
		inline CValue* cast_value(IBackendControlFrame* form) {
			return dynamic_cast<CValue*>(form);
		}
		inline CValue* cast_value(IBackendValue* form) {
			return form ? form->GetImplValueRef() : nullptr;
		}
	}
};

#endif