#include "form.h"
#include "frontend/visualView/ctrl/sizers.h"

inline wxString GetClassType(const wxString& className)
{
	IControlTypeCtor* objectSingle =
		dynamic_cast<IControlTypeCtor*>(CValue::GetAvailableCtor(className));
	wxASSERT(objectSingle);
	return objectSingle->GetTypeControlName();
}

inline void SetDefaultLayoutProperties(CValueSizerItem* sizerItem)
{
	IValueFrame* child = sizerItem->GetChild(0);
	const wxString& obj_type = child->GetObjectTypeName();

	if (obj_type == wxT("sizer")) {
		sizerItem->SetBorder(0);
		sizerItem->SetFlagBorder(wxALL);
		sizerItem->SetFlagState(wxEXPAND);
	}
	else if (child->GetClassName() == wxT("splitter") ||
		child->GetClassName() == wxT("spacer")) {
		sizerItem->SetProportion(1);
		sizerItem->SetFlagState(wxEXPAND);
	}
	else if (child->GetClassName() == wxT("staticline")) {
		sizerItem->SetFlagBorder(wxALL);
		sizerItem->SetFlagState(wxEXPAND);
	}
	else if (child->GetClassName() == wxT("toolbar")) {
		sizerItem->SetBorder(0);
		sizerItem->SetFlagBorder(wxALL);
		sizerItem->SetFlagState(wxEXPAND);
	}
	else if (child->GetClassName() == wxT("notebook")) {
		sizerItem->SetProportion(1);
		sizerItem->SetFlagBorder(wxALL);
		sizerItem->SetFlagState(wxEXPAND);
	}
	else if (obj_type == wxT("widget") ||
		obj_type == wxT("statusbar")) {
		sizerItem->SetProportion(0);
		sizerItem->SetFlagBorder(wxALL);
		sizerItem->SetFlagState(wxSHRINK);
	}
	else if (obj_type == wxT("container")) {
		sizerItem->SetProportion(1);
		sizerItem->SetFlagBorder(wxALL);
		sizerItem->SetFlagState(wxEXPAND);
	}
}

IValueFrame* CValueForm::NewObject(const class_identifier_t& clsid, IValueFrame* controlParent, const CValue& generateId)
{
	if (CValue::IsRegisterCtor(clsid)) {
		IValueFrame* newControl = nullptr;
		CValue* ppParams[] = { this, controlParent, const_cast<CValue*>(&generateId) };
		try {
			newControl = CValue::CreateAndConvertObjectRef< IValueFrame>(clsid, ppParams, 3);
			newControl->IncrRef();
		}
		catch (...) {
			return nullptr;
		}
		return newControl;
	}
	return nullptr;
}

#include "frontend/visualView/ctrl/toolBar.h"

void CValueForm::ResolveNameConflict(IValueFrame* control)
{
	class CResolveNameConflict {

		// Save the original name for use later.
		static wxString GetOriginalName(const IValueFrame* control) {

			wxString originalName = control->GetControlName();
			if (!originalName.IsEmpty()) {
				size_t length = originalName.length();
				while (length >= 0 && stringUtils::IsDigit(originalName[--length]));
				originalName = originalName.Left(length + 1);
			}
			else {
				const IValueFrame* parentControl = control->GetParent();
				if (parentControl != nullptr && g_controlToolBarItemCLSID == control->GetClassType()) {
					originalName = parentControl->GetControlName() + wxT("_") + control->GetClassName();
				}
				else if (parentControl != nullptr && g_controlToolBarSeparatorCLSID == control->GetClassType()) {
					originalName = parentControl->GetControlName() + wxT("_") + control->GetClassName();
				}
				else {
					originalName = control->GetClassName();
				}
			}
			return originalName;
		}

	public:

		static void BuildNameSet(IValueFrame* object, CValueForm* top) {

			wxString originalName = GetOriginalName(object); // Save the original name for use later.
			wxString generateName = originalName; // The name that gets incremented.

			if (object->GetComponentType() != COMPONENT_TYPE_SIZERITEM) {
				// comprobamos si hay conflicto
				unsigned int index = 0; bool founded_name = false;
				do {
					for (auto& valueControl : top->m_listControl) {
						if (0 == valueControl->GetControlID()) continue;
						if (object == valueControl) continue;
						if (stringUtils::CompareString(generateName, valueControl->GetControlName())) {
							founded_name = true;
							break;
						}
						founded_name = false;
					}
					if (founded_name) generateName = wxString::Format(wxT("%s%i"), originalName, ++index);
				} while (founded_name);
			}

			object->SetControlName(generateName);
		};
	};

	// el nombre no puede estar repetido dentro del mismo form
	CResolveNameConflict::BuildNameSet(control, control->GetOwnerForm());
}

IValueFrame* CValueForm::CreateObject(const wxString& className, IValueFrame* controlParent)
{
	IValueFrame* object = nullptr;
	wxString classType = ::GetClassType(className);

	if (controlParent) {
		bool sizer = false;

		if (classType == wxT("form")) sizer = true;
		else if (classType == wxT("sizer")) sizer = controlParent->GetObjectTypeName() == wxT("sizer") || controlParent->GetObjectTypeName() == wxT("form") ? false : true;

		//FIXME! Esto es un parche para evitar crear los tipos menubar,statusbar y
		//toolbar en un form que no sea wxFrame.
		//Hay que modificar el conjunto de tipos para permitir tener varios tipos
		//de forms (como childType de project), pero hay mucho código no válido
		//para forms que no sean de tipo "form". Dicho de otra manera, hay
		//código que dependen del nombre del tipo, cosa que hay que evitar.
		if (controlParent->GetObjectTypeName() == wxT("form") && controlParent->GetClassName() != wxT("clientForm") &&
			(classType == wxT("statusbar") ||
				classType == wxT("menubar") ||
				classType == wxT("ribbonbar") ||
				classType == wxT("toolbar")))

			return nullptr; // tipo no válido

		// No menu dropdown for wxToolBar until wx 2.9 :(
		if (controlParent->GetObjectTypeName() == wxT("tool"))
		{
			IValueFrame* gParent = controlParent->GetParent();

			if (
				(gParent->GetClassName() == wxT("toolbar")) &&
				(className == wxT("menu"))
				)
				return nullptr; // not a valid type
		}

		if (controlParent->GetObjectTypeName() == wxT("toolbar"))
		{
			if (classType == wxT("tool") /*|| classType == wxT("widget")*/)
			{
				object = NewObject(className, controlParent);
				//set enabled item
				//object->SetReadOnly(controlParent->IsEditable());
			}
		}
		else if (controlParent->GetObjectTypeName() == wxT("notebook"))
		{
			if (classType == wxT("notebookPage")) {
				object = NewObject(className, controlParent);
				//set enabled item
				//object->SetReadOnly(controlParent->IsEditable());
			}
		}
		else if (controlParent->GetObjectTypeName() == wxT("container")
			&& controlParent->GetClassName() == wxT("tablebox"))
		{
			if (classType == wxT("tableboxColumn"))
			{
				object = NewObject(className, controlParent);
				//set enabled item
				//object->SetReadOnly(controlParent->IsEditable());
			}
		}
		else if (controlParent->GetObjectTypeName() == wxT("notebookPage"))
		{
			CValueSizerItem* sizerItem = NewObject<CValueSizerItem>("sizerItem", controlParent);
			IValueFrame* obj = NewObject(className, sizerItem);

			if (controlParent) {
				//sizerItem->SetReadOnly(controlParent->IsEditable());
			}

			// la siguiente condición debe cumplirse siempre
			// ya que un item debe siempre contener a otro objeto
			if (obj) {
				//set enabled item
				//obj->SetReadOnly(sizerItem->IsEditable());

				// sizerItem es un tipo de objeto reservado, para que el uso sea
				// más práctico se asignan unos valores por defecto en función
				// del tipo de objeto creado
				if (sizerItem->IsSubclassOf(wxT("sizerItem"))) {
					SetDefaultLayoutProperties(sizerItem);
				}

				object = sizerItem;
			}
		}
		else if (controlParent->GetComponentType() == COMPONENT_TYPE_FRAME ||
			controlParent->GetComponentType() == COMPONENT_TYPE_SIZER ||
			controlParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM)
		{
			CValueSizerItem* sizerItem = NewObject< CValueSizerItem>("sizerItem", controlParent);
			IValueFrame* obj = NewObject(className, sizerItem);

			//if (controlParent) {
			//	sizerItem->SetReadOnly(controlParent->IsEditable());
			//}

			// la siguiente condición debe cumplirse siempre
			// ya que un item debe siempre contener a otro objeto
			if (obj) {
				//set enabled item
				//obj->SetReadOnly(sizerItem->IsEditable());
				// sizerItem es un tipo de objeto reservado, para que el uso sea
				// más práctico se asignan unos valores por defecto en función
				// del tipo de objeto creado
				if (sizerItem->IsSubclassOf(wxT("sizerItem"))) {
					SetDefaultLayoutProperties(sizerItem);
				}

				object = sizerItem;
			}
		}
	}
	else // controlParent == nullptr;
	{
		object = NewObject(className, nullptr);
	}

	return object;
}

#include <wx/clipbrd.h>

#define	copyBlock 0x022290

bool CValueForm::CopyObject(IValueFrame* srcControl, bool copyOnPaste)
{
	if (srcControl->GetComponentType() == COMPONENT_TYPE_FRAME)
		return false;

	// Write some control to clipboard
	if (wxTheClipboard->Open()) {

		CMemoryWriter writterMemory;

		CMemoryWriter writterCopyMemory;
		writterCopyMemory.w_u8(copyOnPaste);

		writterMemory.w_chunk(copyBlock, writterCopyMemory.pointer(), writterCopyMemory.size());

		if (srcControl->CopyObject(writterMemory)) {
			// create an RTF data object
			wxCustomDataObject* pdo = new wxCustomDataObject(oes_clipboard_frame);
			pdo->SetData(writterMemory.size(), writterMemory.pointer()); // the +1 is used to force copy of the \0 character
			// tell clipboard about our RTF
			wxTheClipboard->SetData(pdo);
		}

		wxTheClipboard->Close();
	}

	return false;
}

IValueFrame* CValueForm::PasteObject(CValueForm* dstForm, IValueFrame* dstParent)
{
	IValueFrame* clipboard = nullptr;

	if (wxTheClipboard->Open() && wxTheClipboard->IsSupported(oes_clipboard_frame)) {
		wxCustomDataObject data(oes_clipboard_frame);
		if (wxTheClipboard->GetData(data)) {

			CMemoryReader readerMemory(data.GetData(), data.GetDataSize());

			std::shared_ptr <CMemoryReader>readerCopyMemory(readerMemory.open_chunk(copyBlock));
			const bool copyOnPaste = readerCopyMemory->r_u8();

			clipboard = IValueFrame::CreatePasteObject(readerMemory, dstForm, dstParent);

			if (clipboard == nullptr)
				return nullptr;

			if (!clipboard->PasteObject(readerMemory)) wxDELETE(clipboard);

			if (!copyOnPaste) wxTheClipboard->Clear();
		}
		wxTheClipboard->Close();
	}

	return clipboard;
}
