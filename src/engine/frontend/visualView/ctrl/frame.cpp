////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : base control
////////////////////////////////////////////////////////////////////////////

#include "control.h"
#include "form.h"
#include "backend/compiler/procUnit.h"

wxIMPLEMENT_ABSTRACT_CLASS(IValueFrame, CValue);

//*************************************************************************
//*                          ValueControl		                          *
//*************************************************************************

IValueFrame::IValueFrame() : CValue(eValueTypes::TYPE_VALUE),
m_methodHelper(new CMethodHelper()), m_valEventContainer(nullptr), m_controlId(0), m_controlGuid(Guid::newGuid())
{
	m_valEventContainer = CValue::CreateAndConvertObjectValueRef<CValueEventContainer>(this);
	m_valEventContainer->IncrRef();
}

IValueFrame::~IValueFrame()
{
	m_valEventContainer->DecrRef();
	wxDELETE(m_methodHelper);
}

wxString IValueFrame::GetClassName() const
{
	const class_identifier_t& clsid = GetClassType();
	if (clsid == 0)
		return _("Not founded in wxClassInfo!");
	IAbstractTypeCtor* typeCtor = CValue::GetAvailableCtor(clsid);
	if (typeCtor != nullptr) {
		return typeCtor->GetClassName();
	}
	return _("Not founded in wxClassInfo!");
}

wxString IValueFrame::GetObjectTypeName() const
{
	const class_identifier_t& clsid = GetClassType();
	if (clsid == 0)
		return _("Not founded in wxClassInfo!");
	IControlTypeCtor* typeCtor = dynamic_cast<IControlTypeCtor*>(CValue::GetAvailableCtor(clsid));
	if (typeCtor != nullptr) {
		return typeCtor->GetTypeControlName();
	}
	return _("Not founded in wxClassInfo!");
}

#define	headerBlock 0x012230
#define	dataBlock 0x012250
#define	childBlock 0x012270

bool IValueFrame::IsEditable() const
{
	const CValueForm* handler = GetOwnerForm();
	if (handler != nullptr)
		return handler->IsEditable();
	return false;
}

bool IValueFrame::LoadControl(const IMetaObjectForm* metaForm, CMemoryReader& dataReader)
{
	//Save meta version 
	const version_identifier_t& version = dataReader.r_u32(); //reserved 

	//Load unique guid 
	const Guid& class_guid = dataReader.r_stringZ();

	//Load meta id
	m_controlId = dataReader.r_u32();

	//Load standart fields
	SetControlName(dataReader.r_stringZ());

	//default value 
	m_expanded = dataReader.r_u8();

	return LoadData(dataReader);
}

bool IValueFrame::SaveControl(const IMetaObjectForm* metaForm, CMemoryWriter& dataWritter, bool copy_form)
{
	//Save meta version 
	dataWritter.w_u32(version_oes_last); //reserved 

	//Save unique guid
	dataWritter.w_stringZ(wxNewUniqueGuid);

	//Save meta id 
	dataWritter.w_u32(m_controlId);

	//Save standart fields
	dataWritter.w_stringZ(GetControlName());

	//default value 
	dataWritter.w_u8(m_expanded);

	return SaveData(dataWritter);
}

#define propBlock 0x00023456
#define eventBlock 0x00023457

bool IValueFrame::PasteProperty(CMemoryReader& reader)
{
	std::shared_ptr <CMemoryReader>propReader(reader.open_chunk(propBlock));
	if (propReader != nullptr) {
		for (u64 iter_pos = 0; ; iter_pos++) {
			std::shared_ptr <CMemoryReader>propDataReader(propReader->open_chunk(iter_pos));
			if (propDataReader == nullptr)
				break;
			IProperty* prop = GetProperty(propDataReader->r_stringZ());
			if (prop != nullptr && !prop->PasteData(*propDataReader))
				return false;
		}
	}
	std::shared_ptr <CMemoryReader>eventReader(reader.open_chunk(eventBlock));
	if (eventReader != nullptr) {
		for (u64 iter_pos = 0; ; iter_pos++) {
			std::shared_ptr <CMemoryReader>eventDataReader(eventReader->open_chunk(iter_pos));
			if (eventDataReader == nullptr)
				break;
			IEvent* event = GetEvent(eventDataReader->r_stringZ());
			if (event != nullptr && !event->PasteData(*eventDataReader))
				return false;
		};
	}

	return true;
}

bool IValueFrame::CopyProperty(CMemoryWriter& writter) const
{
	CMemoryWriter propWritter;
	for (unsigned int idx = 0; idx < GetPropertyCount(); idx++) {
		IProperty* prop = GetProperty(idx);
		wxASSERT(prop);
		CMemoryWriter propDataWritter;
		propDataWritter.w_stringZ(prop->GetName());
		if (!prop->CopyData(propDataWritter))
			return false;
		propWritter.w_chunk(idx, propDataWritter.pointer(), propDataWritter.size());
	}

	writter.w_chunk(propBlock, propWritter.pointer(), propWritter.size());

	CMemoryWriter eventWritter;
	for (unsigned int idx = 0; idx < GetEventCount(); idx++) {
		IEvent* event = GetEvent(idx);
		wxASSERT(event);
		CMemoryWriter eventDataWritter;
		eventDataWritter.w_stringZ(event->GetName());
		if (!event->CopyData(eventDataWritter))
			return false;
		eventWritter.w_chunk(idx, eventDataWritter.pointer(), eventDataWritter.size());
	}

	writter.w_chunk(eventBlock, eventWritter.pointer(), eventWritter.size());
	return true;
}

//*******************************************************************

bool IValueFrame::Init()
{
	// always false
	return false;
}

bool IValueFrame::Init(CValue** paParams, const long lSizeArray)
{
	if (lSizeArray < 2)
		return false;
	CValueForm* ownerForm = nullptr;
	IValueFrame* controlParent = nullptr;
	if (paParams[0]->ConvertToValue(ownerForm) &&
		paParams[1]->ConvertToValue(controlParent)) {
		if (controlParent != nullptr) {
			controlParent->AddChild(this);
			SetParent(controlParent);
		}
		//SetReadOnly(ownerForm ? ownerForm->IsEditable() : true);
		SetOwnerForm(ownerForm);
		ownerForm->ResolveNameConflict(this);
		if (paParams[2]->GetBoolean()) {
			GenerateGuid();
			GenerateNewID();
		}
		return true;
	}
	return false;
}

//*******************************************************************

IValueFrame* IValueFrame::CreatePasteObject(const CMemoryReader& reader,
	CValueForm* dstForm, IValueFrame* dstParent)
{
	std::shared_ptr <CMemoryReader>readerHeaderMemory(reader.open_chunk(headerBlock));
	if (readerHeaderMemory == nullptr)
		return nullptr;

	const version_identifier_t& version = readerHeaderMemory->r_s32();
	const class_identifier_t& clsid = readerHeaderMemory->r_u64();

	return dstForm->NewObject(clsid, dstParent);
}

//*******************************************************************

bool IValueFrame::CopyObject(CMemoryWriter& writer) const
{
	CMemoryWriter writterHeaderMemory;
	writterHeaderMemory.w_s32(0); //reserved
	writterHeaderMemory.w_u64(GetClassType()); //get class type 
	writer.w_chunk(headerBlock, writterHeaderMemory.pointer(), writterHeaderMemory.size());
	CMemoryWriter writterDataMemory;
	if (!CopyProperty(writterDataMemory))
		return false;
	writer.w_chunk(dataBlock, writterDataMemory.pointer(), writterDataMemory.size());
	CMemoryWriter writterChildMemory;
	for (unsigned int idx = 0; idx < GetChildCount(); idx++) {
		CMemoryWriter writterMemory;
		IValueFrame* obj = GetChild(idx);
		if (!obj->CopyObject(writterMemory))
			return false;
		writterChildMemory.w_chunk(obj->GetClassType(), writterMemory.pointer(), writterMemory.size());
	}
	writer.w_chunk(childBlock, writterChildMemory.pointer(), writterChildMemory.size());
	return true;
}

bool IValueFrame::PasteObject(CMemoryReader& reader)
{
	CValueForm* valueForm = GetOwnerForm();
	if (valueForm == nullptr) return false;

	std::shared_ptr <CMemoryReader>readerHeaderMemory(reader.open_chunk(headerBlock));

	const version_identifier_t& version = readerHeaderMemory->r_s32(); //reserved
	const class_identifier_t& clsid = readerHeaderMemory->r_u64();

	std::shared_ptr <CMemoryReader> readerChildMemory(reader.open_chunk(childBlock));
	if (readerChildMemory != nullptr) {
		CMemoryReader* prevReaderMemory = nullptr;
		do {
			class_identifier_t founded_clsid = 0;
			CMemoryReader* readerMemory = readerChildMemory->open_chunk_iterator(founded_clsid, &*prevReaderMemory);
			if (readerMemory == nullptr)
				break;
			if (founded_clsid > 0) {
				IValueFrame* valueFrame = valueForm->NewObject(founded_clsid, this, false);
				if (valueFrame != nullptr && !valueFrame->PasteObject(*readerMemory))
					return false;
			}
			prevReaderMemory = readerMemory;
		} while (true);
	}

	std::shared_ptr <CMemoryReader>readerDataMemory(reader.open_chunk(dataBlock));
	if (!PasteProperty(*readerDataMemory))
		return false;

	valueForm->ResolveNameConflict(this);
	return true;
}

//*******************************************************************

#include "backend/metaData.h"
#include "backend/objCtor.h"

bool IValueFrame::HasQuickChoice() const {
	IMetaData* metaData = GetMetaData();
	if (metaData == nullptr)
		return false;
	CValue selValue; GetControlValue(selValue);
	IAbstractTypeCtor* so = metaData->GetAvailableCtor(selValue.GetClassType());
	if (so != nullptr && so->GetObjectTypeCtor() == eCtorObjectType_object_primitive) {
		return true;
	}
	else if (so != nullptr && so->GetObjectTypeCtor() == eCtorObjectType_object_meta_value) {
		IMetaValueTypeCtor* meta_so = dynamic_cast<IMetaValueTypeCtor*>(so);
		if (meta_so != nullptr) {
			IMetaObjectRecordDataRef* metaObject = dynamic_cast<IMetaObjectRecordDataRef*>(meta_so->GetMetaObject());
			if (metaObject != nullptr)
				return metaObject->HasQuickChoice();
		}
	}
	return false;
}

//*******************************************************************

IBackendValueForm* IValueFrame::GetBackendForm() const
{
	return GetOwnerForm();
}

//*******************************************************************

CVisualDocument* IValueFrame::GetVisualDocument() const
{
	CValueForm* const valueForm = GetOwnerForm();
	if (valueForm == nullptr)
		return nullptr;
	return valueForm->GetVisualDocument();
}

IVisualEditorNotebook* IValueFrame::FindVisualEditor() const
{
	return IVisualEditorNotebook::FindEditorByForm(GetOwnerForm());
}

//*******************************************************************
//*                          Runtime                                *
//*******************************************************************

#include "frontend/visualView/visualHost.h"

wxObject* IValueFrame::GetWxObject() const
{
	CValueForm* const valueForm = GetOwnerForm();
	if (valueForm != nullptr) {

		CVisualDocument* const visualDoc = valueForm->GetVisualDocument();
		if (visualDoc != nullptr) {

			const CVisualView* visualView = visualDoc->GetFirstView();
			const IVisualHost* visualHost = visualView ?
				visualView->GetVisualHost() : nullptr;

			if (visualHost == nullptr)
				return nullptr;

			return visualHost->GetWxObject((IValueFrame*)this);

		}
		else if (g_visualHostContext != nullptr) {  		

			//if run designer form search in own visualHost 
			const IVisualHost* visualHost =
				g_visualHostContext->GetVisualHost();

			return visualHost->GetWxObject((IValueFrame*)this);
		}
	}

	return nullptr;
}

#include "backend/metaCollection/partial/commonObject.h"

//****************************************************************************
//*                              Support methods                             *
//****************************************************************************

void IValueFrame::PrepareNames() const
{
	m_methodHelper->ClearHelper();
	for (unsigned int idx = 0; idx < IPropertyObject::GetPropertyCount(); idx++) {
		IProperty* property = IPropertyObject::GetProperty(idx);
		if (property == nullptr)
			continue;
		m_methodHelper->AppendProp(property->GetName(), idx, eProperty);
	}
	//if we have sizerItem then call him  
	IValueFrame* sizeritem = GetParent();
	if (sizeritem != nullptr && sizeritem->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
		for (unsigned int idx = 0; idx < sizeritem->GetPropertyCount(); idx++) {
			IProperty* property = sizeritem->GetProperty(idx);
			if (property == nullptr)
				continue;
			m_methodHelper->AppendProp(property->GetName(), idx, eSizerItem);
		}
	}
	m_methodHelper->AppendProp(wxT("events"), true, false, 0, eEvent);
	if (m_valEventContainer != nullptr) {
		m_valEventContainer->PrepareNames();
	}
}

bool IValueFrame::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eProperty) {
		unsigned int idx = m_methodHelper->GetPropData(lPropNum);
		IProperty* property = GetPropertyByIndex(idx);
		if (property != nullptr)
			property->SetDataValue(varPropVal);
	}
	else if (lPropAlias == eSizerItem) {
		//if we have sizerItem then call him savepropery 
		IValueFrame* sizerItem = GetParent();
		if (sizerItem != nullptr &&
			sizerItem->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
			IProperty* property = sizerItem->GetPropertyByIndex(lPropNum);
			if (property != nullptr)
				property->SetDataValue(varPropVal);
		}
	}

	CValueForm* ownerForm = GetOwnerForm();
	if (ownerForm == nullptr)
		return false;

	CVisualDocument* visualDoc = ownerForm->GetVisualDocument();
	if (visualDoc == nullptr)
		return false;

	CVisualHost* visualView = visualDoc->GetFirstView() ?
		visualDoc->GetFirstView()->GetVisualHost() : nullptr;

	if (visualView == nullptr)
		return false;

	wxObject* wxobject = visualView->GetWxObject(this);

	if (wxobject != nullptr) {
		wxWindow* wxparent = nullptr;
		Update(wxobject, visualView);
		IValueFrame* nextParent = GetParent();
		while (wxparent == nullptr && nextParent != nullptr) {
			if (nextParent->GetComponentType() == COMPONENT_TYPE_WINDOW) {
				wxObject* wxobject = visualView->GetWxObject(nextParent);
				wxparent = dynamic_cast<wxWindow*>(wxobject);
				break;
			}
			nextParent = nextParent->GetParent();
		}
		if (wxparent == nullptr) wxparent = visualView->GetBackgroundWindow();
		OnUpdated(wxobject, wxparent, visualView);
		if (wxparent != nullptr) wxparent->Layout();
	}
	return true;
}

bool IValueFrame::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eSizerItem) {
		//if we have sizerItem then call him savepropery 
		IValueFrame* sizerItem = GetParent();
		if (sizerItem != nullptr &&
			sizerItem->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
			unsigned int idx = m_methodHelper->GetPropData(lPropNum);
			IProperty* property = sizerItem->GetPropertyByIndex(idx);
			if (property != nullptr)
				return property->GetDataValue(pvarPropVal);
			return false;
		}
	}
	else if (lPropAlias == eEvent) {
		pvarPropVal = m_valEventContainer;
		return true;
	}
	else {
		unsigned int idx = m_methodHelper->GetPropData(lPropNum);
		IProperty* property = GetPropertyByIndex(idx);
		if (property != nullptr)
			return property->GetDataValue(pvarPropVal);
		return false;
	}

	return false;
}