////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder
//	Description : visual editor 
////////////////////////////////////////////////////////////////////////////

#include "visualEditor.h"

#include "frontend/mainFrame/mainFrame.h"
#include "frontend/mainFrame/objinspect/objinspect.h"

#include "backend/propertyManager/propertyManager.h"



///////////////////////////////////////////////////////////////////////////////
// Comandos
///////////////////////////////////////////////////////////////////////////////

class BaseVisualCmd : public ibVisualEditorCmd
{
public:
	BaseVisualCmd(ibVisualEditorNotebook::ibVisualEditor* visualData = nullptr) :
		m_visualData(visualData) {
	}

	virtual void Execute() {
		ibVisualEditorCmd::Execute();
	}
	virtual void Restore() {
		ibVisualEditorCmd::Restore();
	}
protected:
	ibVisualEditorNotebook::ibVisualEditor* m_visualData;
};

/** Command for expanding an object in the object tree */

class ExpandObjectCmd : public BaseVisualCmd
{
	ibValueFrame* m_object = nullptr;
	bool m_expand;

public:

	ExpandObjectCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibValueFrame* object, bool expand);

protected:

	virtual void DoExecute() override;
	virtual void DoRestore() override;
};

/**
* Comando para insertar un objeto en el arbol.
*/

class InsertObjectCmd : public BaseVisualCmd
{
	ibValueFrame* m_parent = nullptr;
	ibValueFrame* m_object = nullptr;
	int m_pos;
	ibValueFrame* m_oldSelected;
	bool m_firstCreated;

public:

	InsertObjectCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibValueFrame* object, ibValueFrame* parent, int pos = -1, bool firstCreated = true);

protected:

	void GenerateId();
	void ResetId();

	virtual void DoExecute() override;
	virtual void DoRestore() override;
};

/**
* Comando para borrar un objeto.
*/

class RemoveObjectCmd : public BaseVisualCmd,
	public wxEvtHandler
{
	ibValueFrame* m_parent = nullptr;
	ibValueFrame* m_object = nullptr;
	int m_oldPos;
	ibValueFrame* m_oldSelected = nullptr;

public:

	RemoveObjectCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibValueFrame* object);
	~RemoveObjectCmd();

protected:

	void GenerateId();
	void ResetId();

	void RemoveObject();

	virtual void DoExecute() override;
	virtual void DoRestore() override;
};

/**
* Comando para modificar una propiedad.
*/

class ModifyPropertyCmd : public BaseVisualCmd
{
	ibProperty* m_property;
	wxVariant m_oldValue, m_newValue;

public:

	ModifyPropertyCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibProperty* prop, const wxVariant& oldValue, const wxVariant& newValue);

protected:
	virtual void DoExecute() override;
	virtual void DoRestore() override;
};

/**
* BaseVisualCmd for modifying an event
*/

class ModifyEventCmd : public BaseVisualCmd
{
	IEvent* m_event = nullptr;
	wxVariant m_oldValue, m_newValue;

public:
	ModifyEventCmd(ibVisualEditorNotebook::ibVisualEditor* data, IEvent* event, const wxVariant& oldValue, const wxVariant& newValue);

protected:
	virtual void DoExecute() override;
	virtual void DoRestore() override;
};

/**
* Comando para mover de posicion un objeto.
*/

class ShiftChildCmd : public BaseVisualCmd
{
	ibValueFrame* m_object = nullptr;
	int m_oldPos, m_newPos;

public:
	ShiftChildCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibValueFrame* object, int pos);

protected:
	virtual void DoExecute() override;
	virtual void DoRestore() override;
};

/**
* CutObjectCmd ademas de eliminar el objeto del arbol se asegura
* de eliminar la referencia "clipboard" deshacer el cambio.
*/

class CutObjectCmd : public BaseVisualCmd,
	public wxEvtHandler

{
	// necesario para consultar/modificar el objeto "clipboard"
	ibValueFrame* m_parent = nullptr;
	ibValueFrame* m_object = nullptr;
	int m_oldPos;
	ibValueFrame* m_oldSelected = nullptr;

	bool m_needEvent;

public:

	CutObjectCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibValueFrame* object, bool force);
	~CutObjectCmd();

protected:

	void GenerateId();
	void ResetId();

	void RemoveObject();

	virtual void DoExecute() override;
	virtual void DoRestore() override;
};

///////////////////////////////////////////////////////////////////////////////
// Implementacion de los Comandos
///////////////////////////////////////////////////////////////////////////////

ExpandObjectCmd::ExpandObjectCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibValueFrame* object, bool expand) : BaseVisualCmd(data),
m_object(object), m_expand(expand)
{
}

void ExpandObjectCmd::DoExecute()
{
	m_object->SetExpanded(m_expand);
}

void ExpandObjectCmd::DoRestore()
{
	m_object->SetExpanded(!m_expand);
}

InsertObjectCmd::InsertObjectCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibValueFrame* object, ibValueFrame* parent, int pos, bool firstCreated) : BaseVisualCmd(data),
m_parent(parent), m_object(object), m_pos(pos), m_firstCreated(firstCreated)
{
	m_oldSelected = data->GetSelectedObject();

	if (m_parent) {
		m_parent->RemoveChild(m_object);
		m_object->SetParent(nullptr);
	}

	ResetId();
}

void InsertObjectCmd::GenerateId()
{
	std::function<void(ibValueFrame*)> reset = [&reset](ibValueFrame* object) {
		wxASSERT(object);
		for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {
			reset(object->GetChild(idx));
		}
		if (object->GetControlID() == 0) {
			object->GenerateNewID();
		}
		};
	reset(m_object);
}

void InsertObjectCmd::ResetId()
{
	std::function<void(ibValueFrame*)> reset = [&reset](ibValueFrame* object) {
		wxASSERT(object);
		for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {
			reset(object->GetChild(idx));
		}
		if (object->GetControlID() != 0) {
			object->SetControlID(0);
		}
		};
	reset(m_object);
}

void InsertObjectCmd::DoExecute()
{
	m_visualData->Modify(true);

	if (m_parent != nullptr) {
		m_parent->AddChild(m_object);
		m_object->SetParent(m_parent);
	}

	GenerateId();

	if (m_pos >= 0) {
		m_parent->ChangeChildPosition(m_object, m_pos);
	}

	ibValueFrame* obj = m_object;
	while (obj && obj->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
		if (obj->GetChildCount() > 0) {
			obj = obj->GetChild(0);
		}
		else return;
	}

	//create control in visual editor
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor =
		m_visualData->GetVisualEditor();
	
	wxASSERT(visualEditor);
	visualEditor->CreateControl(m_object, nullptr, m_firstCreated);

	ibValueForm *valueForm = visualEditor->GetValueForm();
	if (valueForm != nullptr) valueForm->PrepareNames();

	//select object
	m_visualData->SelectObject(obj, false, false);
}

void InsertObjectCmd::DoRestore()
{
	m_visualData->Modify(true);

	//remove control in visual editor
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor =
		m_visualData->GetVisualEditor();

	visualEditor->RemoveControl(m_object);

	m_parent->RemoveChild(m_object);
	m_object->SetParent(nullptr);

	ResetId();

	ibValueForm* valueForm = visualEditor->GetValueForm();
	if (valueForm != nullptr) valueForm->PrepareNames();

	m_visualData->SelectObject(m_oldSelected);
}

//-----------------------------------------------------------------------------

RemoveObjectCmd::RemoveObjectCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibValueFrame* object) : BaseVisualCmd(data)
{
	m_object = object;
	m_parent = object->GetParent();
	m_oldPos = m_parent->GetChildPosition(object);
	m_oldSelected = data->GetSelectedObject();
}

RemoveObjectCmd::~RemoveObjectCmd()
{
}

void RemoveObjectCmd::GenerateId()
{
	std::function<void(ibValueFrame*)> reset = [&reset](ibValueFrame* object) {
		wxASSERT(object);
		for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {
			reset(object->GetChild(idx));
		}
		if (object->GetControlID() == 0) {
			object->GenerateNewID();
		}
		};
	reset(m_object);
}

void RemoveObjectCmd::ResetId()
{
	std::function<void(ibValueFrame*)> reset = [&reset](ibValueFrame* object) {
		wxASSERT(object);
		for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {
			reset(object->GetChild(idx));
		}
		if (object->GetControlID() != 0) {
			object->SetControlID(0);
		}
		};
	reset(m_object);
}

void RemoveObjectCmd::RemoveObject()
{
	//remove control in visual editor
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor = m_visualData->GetVisualEditor();

	m_parent->AddChild(m_object);
	m_object->SetParent(m_parent);

	visualEditor->RemoveControl(m_object);

	m_parent->RemoveChild(m_object);
	m_object->SetParent(nullptr);

	ResetId();

	ibValueForm* valueForm = visualEditor->GetValueForm();
	if (valueForm != nullptr) valueForm->PrepareNames();
}

void RemoveObjectCmd::DoExecute()
{
	m_visualData->Modify(true);

	ibValueFrame* obj = m_object;
	while (obj && obj->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
		if (obj->GetChildCount() > 0) {
			obj = obj->GetChild(0);
		}
		else return;
	}

	if (m_object->GetParent() != nullptr) {
		m_parent->RemoveChild(m_object);
		m_object->SetParent(nullptr);
	}

	m_visualData->DetermineObjectToSelect(m_parent, m_oldPos);

	wxEvtHandler::CallAfter(&RemoveObjectCmd::RemoveObject);
}

void RemoveObjectCmd::DoRestore()
{
	m_visualData->Modify(true);

	if (m_object->GetParent() == nullptr) {
		m_parent->AddChild(m_object);
		m_object->SetParent(m_parent);
	}

	ibValueFrame* obj = m_object;
	while (obj && obj->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
		if (obj->GetChildCount() > 0) {
			obj = obj->GetChild(0);
		}
		else return;
	}

	GenerateId();

	// restauramos la posicion
	m_parent->ChangeChildPosition(m_object, m_oldPos);
	m_visualData->SelectObject(m_oldSelected, true, false);

	//create control in visual editor
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor =
		m_visualData->GetVisualEditor();

	visualEditor->CreateControl(m_object);

	ibValueForm* valueForm = visualEditor->GetValueForm();
	if (valueForm != nullptr) valueForm->PrepareNames();
}

//-----------------------------------------------------------------------------

ModifyPropertyCmd::ModifyPropertyCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibProperty* prop, const wxVariant& oldValue, const wxVariant& newValue) : BaseVisualCmd(data),
m_property(prop), m_oldValue(prop), m_newValue(newValue)
{
}

void ModifyPropertyCmd::DoExecute()
{
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor = m_visualData->GetVisualEditor();

	// Get the ibValueFrame from the event
	ibValueFrame* control = dynamic_cast<ibValueFrame*>(m_property->GetPropertyObject());
	wxASSERT(control);

	m_property->SetValue(m_newValue);
	m_visualData->Modify(true);

	if (g_controlFormCLSID == control->GetClassType()) {
		visualEditor->UpdateVisualHost();
	}
	else {
		visualEditor->UpdateControl(control);
	}
}

void ModifyPropertyCmd::DoRestore()
{
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor = m_visualData->GetVisualEditor();
	
	// Get the ibValueFrame from the event
	ibValueFrame* control = dynamic_cast<ibValueFrame*>(m_property->GetPropertyObject());
	wxASSERT(control);
	
	m_property->SetValue(m_oldValue);

	if (g_controlFormCLSID == control->GetClassType()) {
		visualEditor->UpdateVisualHost();
	}
	else {
		visualEditor->UpdateControl(control);
	}

	m_visualData->Modify(true);

	objectInspector->SelectObject(control);
}

//-----------------------------------------------------------------------------

ModifyEventCmd::ModifyEventCmd(ibVisualEditorNotebook::ibVisualEditor* data, IEvent* event, const wxVariant& oldValue, const wxVariant& newValue) : BaseVisualCmd(data),
m_event(event), m_oldValue(oldValue), m_newValue(newValue)
{
}

void ModifyEventCmd::DoExecute()
{
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor = m_visualData->GetVisualEditor();

	// Get the ibValueFrame from the event
	ibValueFrame* control = dynamic_cast<ibValueFrame*>(m_event->GetPropertyObject());
	wxASSERT(control);

	m_event->SetValue(m_newValue);
	m_visualData->Modify(true);

	if (g_controlFormCLSID == control->GetClassType()) {
		visualEditor->UpdateVisualHost();
	}
	else {
		visualEditor->UpdateControl(control);
	}
}

void ModifyEventCmd::DoRestore()
{
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor = m_visualData->GetVisualEditor();

	// Get the ibValueFrame from the event
	ibValueFrame* control = dynamic_cast<ibValueFrame*>(m_event->GetPropertyObject());
	wxASSERT(control);

	m_event->SetValue(m_oldValue);
	m_visualData->Modify(true);

	if (g_controlFormCLSID == control->GetClassType()) {
		visualEditor->UpdateVisualHost();
	}
	else {
		visualEditor->UpdateControl(control);
	}
}

//-----------------------------------------------------------------------------

ShiftChildCmd::ShiftChildCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibValueFrame* object, int pos) : BaseVisualCmd(data)
{
	m_object = object;
	ibValueFrame* parent = object->GetParent();

	assert(parent);

	m_oldPos = parent->GetChildPosition(object);
	m_newPos = pos;
}

void ShiftChildCmd::DoExecute()
{
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor =
		m_visualData->GetVisualEditor();

	if (m_oldPos != m_newPos) {
		ibValueFrame* parent(m_object->GetParent());
		parent->ChangeChildPosition(m_object, m_newPos);

		visualEditor->UpdateControl(m_object);
	}

	m_visualData->Modify(true);
}

void ShiftChildCmd::DoRestore()
{
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor =
		m_visualData->GetVisualEditor();

	if (m_oldPos != m_newPos) {
		ibValueFrame* parent(m_object->GetParent());
		parent->ChangeChildPosition(m_object, m_oldPos);

		visualEditor->UpdateControl(m_object);
	}

	m_visualData->Modify(true);
}

//-----------------------------------------------------------------------------

CutObjectCmd::CutObjectCmd(ibVisualEditorNotebook::ibVisualEditor* data, ibValueFrame* object, bool force) : BaseVisualCmd(data), m_needEvent(!force)
{
	m_object = object;
	m_parent = object->GetParent();
	m_oldPos = m_parent->GetChildPosition(object);
	m_oldSelected = data->GetSelectedObject();
}

CutObjectCmd::~CutObjectCmd()
{
}

void CutObjectCmd::GenerateId()
{
	std::function<void(ibValueFrame*)> reset = [&reset](ibValueFrame* object) {
		wxASSERT(object);
		for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {
			reset(object->GetChild(idx));
		}
		if (object->GetControlID() == 0) {
			object->GenerateNewID();
		}
		};
	reset(m_object);
}

void CutObjectCmd::ResetId()
{
	std::function<void(ibValueFrame*)> reset = [&reset](ibValueFrame* object) {
		wxASSERT(object);
		for (unsigned int idx = 0; idx < object->GetChildCount(); idx++) {
			reset(object->GetChild(idx));
		}
		if (object->GetControlID() != 0) {
			object->SetControlID(0);
		}
		};
	reset(m_object);
}

void CutObjectCmd::RemoveObject()
{
	//remove control in visual editor
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor =
		m_visualData->GetVisualEditor();

	m_parent->AddChild(m_object);
	m_object->SetParent(m_parent);

	visualEditor->RemoveControl(m_object);

	//remove control in visual editor
	m_parent->RemoveChild(m_object);
	m_object->SetParent(nullptr);
}

void CutObjectCmd::DoExecute()
{
	m_visualData->Modify(true);

	if (!m_needEvent) {
		//remove control in visual editor
		ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor =
			m_visualData->GetVisualEditor();
		visualEditor->RemoveControl(m_object);
	}

	ibValueFrame* obj = m_object;
	while (obj && obj->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
		if (obj->GetChildCount() > 0) {
			obj = obj->GetChild(0);
		}
		else return;
	}

	//remove control in visual editor
	m_parent->RemoveChild(m_object);
	m_object->SetParent(nullptr);

	//determine object to select
	m_visualData->DetermineObjectToSelect(m_parent, m_oldPos);

	//reset id, guid
	ResetId();

	if (m_needEvent) {
		wxEvtHandler::CallAfter(&CutObjectCmd::RemoveObject);
	}
}

void CutObjectCmd::DoRestore()
{
	m_visualData->Modify(true);

	// reubicamos el objeto donde estaba
	m_parent->AddChild(m_object);
	m_object->SetParent(m_parent);

	ibValueFrame* obj = m_object;
	while (obj && obj->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
		if (obj->GetChildCount() > 0) {
			obj = obj->GetChild(0);
		}
		else return;
	}

	//generate new id
	GenerateId();

	//change child position
	m_parent->ChangeChildPosition(m_object, m_oldPos);

	// restauramos el clipboard
	//m_visualData->SetClipboardObject(nullptr);
	m_visualData->SelectObject(m_oldSelected, true, false);

	//create control in visual editor
	ibVisualEditorNotebook::ibVisualEditor::ibVisualEditorHost* visualEditor =
		m_visualData->GetVisualEditor();

	visualEditor->CreateControl(m_object, m_parent);
}

//-----------------------------------------------------------------------------

ibValueFrame* ibVisualEditorNotebook::ibVisualEditor::CreateObject(const wxString& name)
{
	ibValueFrame* obj = nullptr;
	wxASSERT(m_valueForm);
	try
	{
		//LogDebug("[ibApplicationData::CreateObject] New " + name );
		ibValueFrame* old_selected = GetSelectedObject();
		ibValueFrame* parent = old_selected;

		if (parent)
		{
			bool created = false;

			// Para que sea mas practico, si el objeto no se puede crear debajo
			// del objeto seleccionado vamos a intentarlo en el padre del seleccionado
			// y seguiremos subiendo hasta que ya no podamos crear el objeto.

			while (parent && !created)
			{
				// ademas, el objeto se insertara a continuacion del objeto seleccionado
				obj = m_valueForm->CreateObject(_STDSTR(name), parent);

				if (obj)
				{
					int pos = CalcPositionOfInsertion(GetSelectedObject(), parent);

					Execute(new InsertObjectCmd(this, obj, parent, pos));
					created = true;
				}
				else
				{
					// lo vamos a seguir intentando con el padre, pero cuidado, el padre
					// no puede ser un item!
					parent = parent->GetParent();

					while (parent && parent->GetComponentType() == COMPONENT_TYPE_SIZERITEM)
						parent = parent->GetParent();
				}
			}
		}

		// Seleccionamos el objeto, si este es un item entonces se selecciona
		// el objeto contenido. ?Tiene sentido tener un item debajo de un item?
		while (obj && obj->GetComponentType() == COMPONENT_TYPE_SIZERITEM)
			obj = (obj->GetChildCount() > 0 ? obj->GetChild(0) : nullptr);

		NotifyObjectCreated(obj);

		if (obj)
		{
			SelectObject(obj, true, true);
		}
		else
		{
			SelectObject(old_selected, true, true);
		}
	}
	catch (const std::exception& ex)
	{
		wxLogError(ex.what());
	}

	return obj;
}

void ibVisualEditorNotebook::ibVisualEditor::InsertObject(ibValueFrame* obj, ibValueFrame* parent)
{
	Execute(new InsertObjectCmd(this, obj, parent));
	NotifyObjectCreated(obj);
}

void ibVisualEditorNotebook::ibVisualEditor::CopyObject(ibValueFrame* obj)
{
	// Make a copy of the object on the clipboard, otherwise
	// modifications to the object after the copy will also
	// be made on the clipboard.
	ibValueFrame* objParent = obj->GetParent();
	wxASSERT(m_valueForm);
	ibValueForm::CopyObject(
		objParent != nullptr && objParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM ?
		objParent : obj
	);
}

#include <wx/clipbrd.h>

bool ibVisualEditorNotebook::ibVisualEditor::PasteObject(ibValueFrame* dstObject)
{
	wxASSERT(m_valueForm);

	if (!m_valueForm->IsEditable())
		return false; 

	try {

		// si no se ha podido crear el objeto vamos a intentar crearlo colgado
		// del padre de "parent" y ademas vamos a insertarlo en la posicion
		// siguiente a "parent"
		ibValueFrame* objParent =
			dstObject != nullptr && dstObject->GetComponentType() == COMPONENT_TYPE_SIZERITEM ? dstObject->GetParent() : dstObject;

		ibValueFrame* clipboard = ibValueForm::PasteObject(m_valueForm, objParent);
		if (clipboard == nullptr)
			return false;

		// Remove parent/child relationship from clipboard object
		ibValueFrame* clipParent = clipboard->GetParent();
		if (clipParent) {
			clipParent->RemoveChild(clipboard);
			clipboard->SetParent(nullptr);
		}

		// si no se ha podido crear el objeto vamos a intentar crearlo colgado
		// del padre de "parent" y ademas vamos a insertarlo en la posicion
		// siguiente a "parent"
		ibValueFrame* parentObject = dstObject;
		if (parentObject->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
			parentObject = parentObject->GetParent();
		}

		if (clipboard->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
			clipboard = clipboard->GetChild(0);
		}

		ibValueFrame* obj = nullptr;
		while (obj == nullptr) {
			obj = m_valueForm->CreateObject(_STDSTR(clipboard->GetClassName()), parentObject);
			if (obj != nullptr) {
				if (parentObject) {
					obj->SetParent(nullptr);
					parentObject->RemoveChild(obj);
				}
				ibValueFrame* clipParent = clipboard->GetParent();
				if (clipParent && clipParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
					clipboard = clipParent;
				}
			}
			else {
				parentObject = parentObject->GetParent();
				if (parentObject->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
					parentObject = parentObject->GetParent();
				}
			}
			if (parentObject == nullptr)
				break;
		}

		if (!obj) {
			return false;
		}

		ibValueFrame* aux = obj;

		while (aux && aux != clipboard)
			aux = (aux->GetChildCount() > 0 ? aux->GetChild(0) : nullptr);

		int pos = CalcPositionOfInsertion(dstObject, parentObject);

		if (aux && aux != obj) {
			// sustituimos aux por clipboard
			ibValueFrame* auxParent = aux->GetParent();
			auxParent->RemoveChild(aux);
			aux->SetParent(nullptr);

			auxParent->AddChild(clipboard);
			clipboard->SetParent(auxParent);
		}
		else {
			obj = clipboard;
		}

		// y finalmente insertamos en el arbol
		Execute(new InsertObjectCmd(this, obj, parentObject, pos));
		NotifyObjectCreated(obj);

		// vamos a mantener seleccionado el nuevo objeto creado
		// pero hay que tener en cuenta que es muy probable que el objeto creado
		// sea un "item"
		while (obj && obj->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
			assert(obj->GetChildCount() > 0);
			obj = obj->GetChild(0);
		}

		SelectObject(obj, true, true);
	}
	catch (const std::exception& ex)
	{
		wxLogError(ex.what());
		return false;
	}

	return true;
}

void ibVisualEditorNotebook::ibVisualEditor::ExpandObject(ibValueFrame* obj, bool expand)
{
	Execute(new ExpandObjectCmd(this, obj, expand));

	// collapse also all children ...
	PropagateExpansion(obj, expand, !expand);
	NotifyObjectExpanded(obj);
}

void ibVisualEditorNotebook::ibVisualEditor::RemoveObject(ibValueFrame* obj)
{
	DoRemoveObject(obj, false);
}

void ibVisualEditorNotebook::ibVisualEditor::CutObject(ibValueFrame* obj, bool force)
{
	ibValueFrame* objParent = obj->GetParent();
	wxASSERT(m_valueForm);
	ibValueForm::CopyObject(
		objParent != nullptr && objParent->GetComponentType() == COMPONENT_TYPE_SIZERITEM ?
		objParent : obj, false
	);

	DoRemoveObject(obj, true, force);
}

bool ibVisualEditorNotebook::ibVisualEditor::SelectObject(ibValueFrame* obj, bool force, bool notify)
{
	if ((obj == objectInspector->GetSelectedObject()) && !force)
		return false;

	m_visualEditor->SetObjectSelect(obj); m_selObj = obj;

	if (notify) {
		NotifyObjectSelected(obj, force);
	}

	objectInspector->SelectObject(obj, this);
	return true;
}

/////////////////////////////////////////////////////////////////////////////////////

void ibVisualEditorNotebook::ibVisualEditor::MovePosition(ibValueFrame* obj, unsigned int toPos)
{
	Execute(new ShiftChildCmd(this, obj, toPos));
	NotifyEditorRefresh();
	SelectObject(obj, true);
}

void ibVisualEditorNotebook::ibVisualEditor::MovePosition(ibValueFrame* obj, bool right, unsigned int num)
{
	ibValueFrame* noItemObj = obj;
	ibValueFrame* parent = obj->GetParent();

	if (parent != nullptr) {
		// Si el objeto está incluido dentro de un item hay que desplazar
		// el item

		while (parent && parent->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
			obj = parent;
			parent = obj->GetParent();
		}

		unsigned int pos = parent->GetChildPosition(obj);

		// nos aseguramos de que los límites son correctos
		unsigned int children_count = parent->GetChildCount();

		if ((right && num + pos < children_count) ||
			(!right && (num <= pos))) {
			pos = (right ? pos + num : pos - num);
			Execute(new ShiftChildCmd(this, obj, pos));
			NotifyEditorRefresh();
			SelectObject(noItemObj, true);
		}
	}
}

void ibVisualEditorNotebook::ibVisualEditor::ScrollToObject(ibValueFrame* obj)
{
	m_visualEditor->ScrollToObject(obj);
}

/////////////////////////////////////////////////////////////////////////////////////

void ibVisualEditorNotebook::ibVisualEditor::ModifyProperty(ibProperty* prop, const wxVariant& oldValue, const wxVariant& newValue)
{
	ibPropertyObject* object = prop->GetPropertyObject();
	if (oldValue != newValue) {
		Execute(new ModifyPropertyCmd(this, prop, oldValue, newValue));
		NotifyPropertyModified(prop);
	}
}

void ibVisualEditorNotebook::ibVisualEditor::ModifyEvent(IEvent* evt, const wxVariant& oldValue, const wxVariant& newValue)
{
	ibPropertyObject* object = evt->GetPropertyObject();
	if (oldValue != newValue) {
		Execute(new ModifyEventCmd(this, evt, oldValue, newValue));
		NotifyEventModified(evt);
	}
}

/////////////////////////////////////////////////////////////////////////////////////

void ibVisualEditorNotebook::ibVisualEditor::PropagateExpansion(ibValueFrame* obj, bool expand, bool up)
{
	if (obj != nullptr) {
		if (up) {
			ibValueFrame* child = nullptr;
			for (unsigned int i = 0; i < obj->GetChildCount(); i++) {
				child = obj->GetChild(i);
				Execute(new ExpandObjectCmd(this, child, expand));
				PropagateExpansion(child, expand, up);
			}
		}
		else
		{
			PropagateExpansion(obj->GetParent(), expand, up);
			Execute(new ExpandObjectCmd(this, obj, expand));
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////

void ibVisualEditorNotebook::ibVisualEditor::DoRemoveObject(ibValueFrame* obj, bool cutObject, bool force)
{
	// Note:
	//  When removing an object it is important that the "item" objects
	// are not left behind
	ibValueFrame* parent = obj->GetParent();
	ibValueFrame* deleted_obj = obj;

	if (parent) {
		// Get the top item
		while (parent && parent->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
			obj = parent;
			parent = obj->GetParent();
		}

		if (!cutObject) {
			if (!deleted_obj->CanDeleteControl()) {
				return;
			}
		}

		NotifyObjectRemoved(deleted_obj);

		if (cutObject) {
			Execute(new CutObjectCmd(this, obj, force));
		}
		else {
			Execute(new RemoveObjectCmd(this, obj));
		}

		SelectObject(GetSelectedObject(), true, true);
	}
	else {
		if (obj->GetClassType() != g_controlFormCLSID) {
			assert(false);
		}
	}
}

void ibVisualEditorNotebook::ibVisualEditor::DetermineObjectToSelect(ibValueFrame* parent, unsigned int pos)
{
	// get position of next control or last control
	ibValueFrame* objToSelect = nullptr;
	unsigned int count = parent->GetChildCount();
	if (0 == count) {
		objToSelect = parent;
	}
	else {
		pos = (pos < count ? pos : count - 1);
		objToSelect = parent->GetChild(pos);
	}

	while (objToSelect && objToSelect->GetComponentType() == COMPONENT_TYPE_SIZERITEM) {
		objToSelect = objToSelect->GetChild(0);
	}

	SelectObject(objToSelect);
}

/////////////////////////////////////////////////////////////////////////////////////

void ibVisualEditorNotebook::ibVisualEditor::Undo()
{
	m_cmdProc->Undo();
	m_document->Modify(!m_cmdProc->IsAtSavePoint());
	NotifyEditorRefresh();
	NotifyObjectSelected(GetSelectedObject());
}

void ibVisualEditorNotebook::ibVisualEditor::Redo()
{
	m_cmdProc->Redo();
	m_document->Modify(!m_cmdProc->IsAtSavePoint());
	NotifyEditorRefresh();
	NotifyObjectSelected(GetSelectedObject());
}

#include <wx/clipbrd.h>

bool ibVisualEditorNotebook::ibVisualEditor::CanPasteObject() const
{
	ibValueFrame* obj = GetSelectedObject();
	if (obj != nullptr) {
		bool canPasteObject = wxTheClipboard->Open() &&
			wxTheClipboard->IsSupported(oes_clipboard_frame);
		wxTheClipboard->Close();
		return canPasteObject;
	}
	return false;
}

bool ibVisualEditorNotebook::ibVisualEditor::CanCopyObject() const
{
	ibValueFrame* obj = GetSelectedObject();
	if (obj && obj->GetClassType() != g_controlFormCLSID)
		return true;
	return false;
}

int ibVisualEditorNotebook::ibVisualEditor::CalcPositionOfInsertion(ibValueFrame* selected, ibValueFrame* parent)
{
	int pos = wxNOT_FOUND;
	if (parent && selected) {
		ibValueFrame* parentSelected = selected->GetParent();
		while (parentSelected && parentSelected != parent) {
			selected = parentSelected;
			parentSelected = selected->GetParent();
		}
		if (parentSelected && parentSelected == parent) {
			pos = parent->GetChildPosition(selected) + 1;
		}
	}
	return pos;
}
