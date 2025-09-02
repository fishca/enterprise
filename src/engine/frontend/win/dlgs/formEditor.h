#ifndef __FORM_EDITOR_H__
#define __FORM_EDITOR_H__

#include <wx/dialog.h>
#include <wx/button.h>
#include <wx/sizer.h>
#include <wx/treectrl.h>

#include <wx/propgrid/propgrid.h>
#include <wx/propgrid/manager.h>

#include <map>

#include "frontend/visualView/ctrl/form.h"
#include "frontend/visualView/visual.h"

///////////////////////////////////////////////////////////////////////////////
/// Class CDialogFormEditor
///////////////////////////////////////////////////////////////////////////////

#pragma region __commands_h__

class CFormEditorCmd {
	bool m_executed;
public:

	CFormEditorCmd() : m_executed(false) {}

	void Execute() {
		if (!m_executed) {
			DoExecute();
			m_executed = true;
		}
	}

protected:

	/**
	 * Ejecuta el comando.
	 */
	virtual void DoExecute() = 0;
};

#pragma endregion 

class FRONTEND_API CDialogFormEditor : public wxDialog {
public:

	CDialogFormEditor(CValueForm* valueForm);

protected:

	IValueFrame* GetSelectedObject() const { return m_selectedControl; }

	/**
	* Gracias a que podemos asociar un objeto a cada item, esta clase nos va
	* a facilitar obtener el objeto (IValueFrame) asociado a un item para
	* seleccionarlo pinchando en el item.
	 */
	class CDialogFormEditorObjectTreeItemData : public wxTreeItemData {
	public:
		CDialogFormEditorObjectTreeItemData(IValueFrame* obj) : m_object(obj) {}
		IValueFrame* GetObject() { return m_object; }
	private:
		IValueFrame* m_object = nullptr;
	};

	/**
	* Menu popup asociado a cada item del arbol.
	*
	* Este objeto ejecuta los comandos incluidos en el menu referentes al objeto
	* seleccionado.
	*/
	class CDialogFormEditorItemPopupMenu : public wxMenu {
	public:

		int GetSelectedID() const { return m_selID; }

		CDialogFormEditorItemPopupMenu(CDialogFormEditor* parent, IValueFrame* obj);

		void OnUpdateEvent(wxUpdateUIEvent& e);
		void OnMenuEvent(wxCommandEvent& event);

	protected:

		CDialogFormEditor* m_handler = nullptr;
		IValueFrame* m_object = nullptr;
		int m_selID;

		wxDECLARE_EVENT_TABLE();
	};

	/**
	 * Crea el arbol completamente.
	 */
	void CreateTree();
	void RebuildTree() {

		m_treeControl->Freeze();

		// Clear the old tree and map
		m_treeControl->DeleteAllItems();
		m_listItem.clear();

		if (m_owner != nullptr) {

			wxTreeItemId dummy;
			AddChildren(m_owner, dummy, true);

			// Expand items that were previously expanded
			RestoreItemStatus(m_owner);
		}

		m_treeControl->Thaw();
	}

	void AddChildren(IValueFrame* child, const wxTreeItemId& parent, bool is_root = false);

	int GetImageIndex(const wxString& name) const {
		auto it = m_iconIdx.find(name);
		if (it != m_iconIdx.end()) {
			return it->second;
		}
		return wxNOT_FOUND; //default icon
	}

	void UpdateItem(const wxTreeItemId& id, IValueFrame* obj) {

		// mostramos el nombre
		wxString class_name(obj->GetClassName());
		wxString obj_name(obj->GetControlName());

		wxString text = obj->GetControlCaption();

		// actualizamos el item
		m_treeControl->SetItemText(id, obj->GetControlCaption());

		if (m_owner != nullptr && obj == GetSelectedObject()) {
			m_treeControl->EnsureVisible(id);
			m_treeControl->SelectItem(id);
			m_treeControl->SetItemBold(id);
		}
	}

	void RestoreItemStatus(IValueFrame* obj) {
		std::map< IValueFrame*, wxTreeItemId>::iterator item_it = m_listItem.find(obj);
		if (item_it != m_listItem.end()) {
			wxTreeItemId id = item_it->second;

			if (obj->GetExpanded()) {
				m_treeControl->Expand(id);
			}
		}
		unsigned int i, count = obj->GetChildCount();
		for (i = 0; i < count; i++) {
			RestoreItemStatus(obj->GetChild(i));
		}
	}

	IValueFrame* GetObjectFromTreeItem(const wxTreeItemId& item) {
		if (item.IsOk()) {
			wxTreeItemData* item_data = m_treeControl->GetItemData(item);
			if (item_data) {
				IValueFrame* obj(((CDialogFormEditorObjectTreeItemData*)item_data)->GetObject());
				return obj;
			}
		}

		return nullptr;
	}

	//events
	void OnUpdateEvent(wxUpdateUIEvent& e);
	void OnMenuEvent(wxCommandEvent& e);
	void OnButtonEvent(wxCommandEvent& e);

	void OnSelChanged(wxTreeEvent& event);
	void OnRightClick(wxTreeEvent& event);
	void OnBeginDrag(wxTreeEvent& event);
	void OnEndDrag(wxTreeEvent& event);
	void OnKeyDown(wxTreeEvent& event);

	void OnPropertyGridChanging(wxPropertyGridEvent& event);
	void OnPropertyGridItemSelected(wxPropertyGridEvent& event);

	//special 
	void MovePosition(IValueFrame* obj, bool right, unsigned int num = 1);

private:

	void ExecuteCommand(CFormEditorCmd* cmd) {
		m_cmdArray.emplace_back(cmd);
	}

	CValueForm* m_owner;
	IValueFrame* m_selectedControl;

	std::vector<std::shared_ptr<CFormEditorCmd>> m_cmdArray;

	std::map<IValueFrame*, wxTreeItemId> m_listItem;
	std::map<wxPGProperty*, IProperty*> m_pgArray;

	std::map<wxString, int> m_iconIdx;

	wxAuiToolBar* m_mainToolBar;
	wxAuiToolBarItem* m_toolUpItem;
	wxAuiToolBarItem* m_toolDownItem;

	wxImageList* m_iconList = nullptr;
	wxTreeCtrl* m_treeControl;

	wxPropertyGridManager* m_propertyGridManager;
	wxPropertyGridPage* m_propertyGridPage;

	wxStdDialogButtonSizer* m_sdbSizer;
	wxTreeItemId m_draggedItem;

	wxButton* m_sdbSizerOK;
	wxButton* m_sdbSizerApply;
	wxButton* m_sdbSizerCancel;

	wxDECLARE_EVENT_TABLE();
};

#endif 