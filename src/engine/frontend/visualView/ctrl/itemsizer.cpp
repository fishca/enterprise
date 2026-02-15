#include "sizer.h"
#include "frontend/visualView/pageWindow.h"
#include "form.h"

wxIMPLEMENT_DYNAMIC_CLASS(CValueSizerItem, IValueSizer)

//************************************************************************************
//*                            Support item                                          *
//************************************************************************************

inline wxObject* GetParentFormVisualEditor(IVisualHost* visualEdit, IValueFrame* object)
{
	IValueFrame* parent = object->GetParent();
	wxASSERT(parent);

	wxObject* wxparent_object = visualEdit->GetWxObject(parent);
	if (parent->GetClassName() == wxT("notebookPage")) {
		CPanelPage* objPage =
			dynamic_cast<CPanelPage*>(wxparent_object);
		return objPage != nullptr ? objPage->GetSizer() : nullptr;
	}

	return wxparent_object;
}

inline wxObject* GetChildFormVisualEditor(IVisualHost* visualEdit, wxObject* wxobject, unsigned int childIndex)
{
	IValueFrame* obj = visualEdit->GetObjectBase(wxobject);
	if (childIndex >= obj->GetChildCount())
		return nullptr;
	return visualEdit->GetWxObject(obj->GetChild(childIndex));
}

//************************************************************************************
//*                            ValueSizerItem                                        *
//************************************************************************************

CValueSizerItem::CValueSizerItem() : IValueFrame()
{
}

void CValueSizerItem::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool firstÑreated)
{
	IValueFrame* object = visualHost->GetObjectBase(wxobject);

	// Get parent sizer
	wxSizer* sizer = wxDynamicCast(GetParentFormVisualEditor(visualHost, object), wxSizer);

	// Get child window
	wxObject* child = GetChildFormVisualEditor(visualHost, wxobject, 0);
	if (nullptr == child) {
		wxLogError(wxT("The SizerItem component has no child - this should not be possible!"));
		return;
	}

	// Get IObject for property access
	CValueSizerItem* obj = wxDynamicCast(object, CValueSizerItem);

	// Add the child ( window or sizer ) to the sizer
	wxWindow* windowChild = wxDynamicCast(child, wxWindow);
	wxSizer* sizerChild = wxDynamicCast(child, wxSizer);

	if (windowChild != nullptr) {
		sizer->Detach(windowChild);
		sizer->Add(windowChild,
			obj->GetProportion(),
			obj->GetFlagBorder() | obj->GetFlagState(),
			obj->GetBorder());

		windowChild->Layout();
	}
	else if (sizerChild != nullptr) {
		sizer->Detach(sizerChild);
		sizer->Add(sizerChild,
			obj->GetProportion(),
			obj->GetFlagBorder() | obj->GetFlagState(),
			obj->GetBorder());

		sizerChild->Layout();
	}
}

void CValueSizerItem::OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost)
{
	IValueFrame* object = visualHost->GetObjectBase(wxobject);

	// Get parent sizer
	wxSizer* sizer = wxDynamicCast(GetParentFormVisualEditor(visualHost, object), wxSizer);

	// Get child window
	wxObject* child = GetChildFormVisualEditor(visualHost, wxobject, 0);
	if (nullptr == child) {
		wxLogError(wxT("The SizerItem component has no child - this should not be possible!"));
		return;
	}

	// Get IObject for property access
	CValueSizerItem* obj = wxDynamicCast(object, CValueSizerItem);

	// Add the child ( window or sizer ) to the sizer
	wxWindow* windowChild = wxDynamicCast(child, wxWindow);
	wxSizer* sizerChild = wxDynamicCast(child, wxSizer);

	IValueFrame* parentControl = GetParent(); int idx = wxNOT_FOUND;

	for (unsigned int i = 0; i < parentControl->GetChildCount(); i++) {
		IValueFrame* child = parentControl->GetChild(i);
		if (m_controlId == child->GetControlID()) {
			idx = i;
			break;
		}
	}

	if (windowChild != nullptr) {

		if (idx == wxNOT_FOUND) {
			sizer->Detach(windowChild);
			sizer->Add(windowChild,
				obj->GetProportion(),
				obj->GetFlagBorder() | obj->GetFlagState(),
				obj->GetBorder());
		}
		else {
			sizer->Detach(windowChild);
			sizer->Insert(idx, windowChild,
				obj->GetProportion(),
				obj->GetFlagBorder() | obj->GetFlagState(),
				obj->GetBorder());
		}

	}
	else if (sizerChild != nullptr) {

		if (idx == wxNOT_FOUND) {
			sizer->Detach(sizerChild);
			sizer->Add(sizerChild,
				obj->GetProportion(),
				obj->GetFlagBorder() | obj->GetFlagState(),
				obj->GetBorder());
		}
		else {
			sizer->Detach(sizerChild);
			sizer->Insert(idx, sizerChild,
				obj->GetProportion(),
				obj->GetFlagBorder() | obj->GetFlagState(),
				obj->GetBorder());
		}
	}

	const IValueFrame* parent = object->GetParent();
	if (parent->GetClassName() == wxT("notebookPage")) {
		wxObject* wxparent_object = visualHost->GetWxObject(parent);
		CPanelPage* objPage =
			dynamic_cast<CPanelPage*>(wxparent_object);
		objPage->Layout();
	}
}

#include "backend/metaData.h"

IMetaData* CValueSizerItem::GetMetaData() const
{
	const IValueMetaObjectForm* metaFormObject = m_formOwner ?
		m_formOwner->GetFormMetaObject() :
		nullptr;

	//for form buider
	if (metaFormObject == nullptr) {
		ISourceDataObject* srcValue = m_formOwner ?
			m_formOwner->GetSourceObject() :
			nullptr;
		if (srcValue != nullptr) {
			IValueMetaObjectGenericData* metaValue = srcValue->GetSourceMetaObject();
			wxASSERT(metaValue);
			return metaValue->GetMetaData();
		}
	}

	return metaFormObject ?
		metaFormObject->GetMetaData() :
		nullptr;
}

#include "backend/metaCollection/metaFormObject.h"

form_identifier_t CValueSizerItem::GetTypeForm() const
{
	if (!m_formOwner) {
		wxASSERT(m_formOwner);
		return 0;
	}

	const IValueMetaObjectForm* metaFormObj =
		m_formOwner->GetFormMetaObject();
	wxASSERT(metaFormObj);

	return metaFormObj->GetTypeForm();
}

CProcUnit* CValueSizerItem::GetFormProcUnit() const
{
	if (!m_formOwner) {
		wxASSERT(m_formOwner);
		return nullptr;
	}

	return m_formOwner->GetProcUnit();
}

//**********************************************************************************
//*                                    Data										   *
//**********************************************************************************

bool CValueSizerItem::LoadData(CMemoryReader& reader)
{
	//m_propertyProportion->SetValue(reader.r_s32());
	//m_propertyFlagBorder->SetValue(reader.r_s64());
	//m_propertyFlagState->SetValue(reader.r_s64());
	//m_propertyBorder->SetValue(reader.r_s32());

	m_propertyProportion->LoadData(reader);
	//m_propertyFlagBorder->LoadData(reader);

	m_propertyFlagBorderLeft->LoadData(reader);
	m_propertyFlagBorderRight->LoadData(reader);
	m_propertyFlagBorderTop->LoadData(reader);
	m_propertyFlagBorderBottom->LoadData(reader);

	m_propertyFlagState->LoadData(reader);
	m_propertyBorder->LoadData(reader);

	return IValueFrame::LoadData(reader);
}

bool CValueSizerItem::SaveData(CMemoryWriter& writer)
{
	//writer.w_s32(m_propertyProportion->GetValueAsInteger());
	//writer.w_s64(m_propertyFlagBorder->GetValueAsInteger());
	//writer.w_s64(m_propertyFlagState->GetValueAsInteger());
	//writer.w_s32(m_propertyBorder->GetValueAsInteger());

	m_propertyProportion->SaveData(writer);
	//m_propertyFlagBorder->SaveData(writer);

	m_propertyFlagBorderLeft->SaveData(writer);
	m_propertyFlagBorderRight->SaveData(writer);
	m_propertyFlagBorderTop->SaveData(writer);
	m_propertyFlagBorderBottom->SaveData(writer);

	m_propertyFlagState->SaveData(writer);
	m_propertyBorder->SaveData(writer);

	return IValueFrame::SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

S_CONTROL_TYPE_REGISTER(CValueSizerItem, "SizerItem", "Sizer", string_to_clsid("CT_SIZR"));