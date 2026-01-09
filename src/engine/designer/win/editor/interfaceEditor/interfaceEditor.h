#ifndef __INTERFACE_EDITOR_H__
#define __INTERFACE_EDITOR_H__

#include <wx/treectrl.h>
#include <wx/splitter.h>

#include "backend/metadataConfiguration.h"

#include "frontend/win/theme/luna_toolbarart.h"
#include "frontend/win/ctrls/checktree.h"

class CInterfaceEditor : public wxWindow {

	wxTreeItemId m_treeMETADATA;
	wxTreeItemId m_treeCOMMON; //special tree

	wxTreeItemId m_treeFORMS;

	wxTreeItemId m_treeCONSTANTS;

	wxTreeItemId m_treeCATALOGS;
	wxTreeItemId m_treeDOCUMENTS;
	wxTreeItemId m_treeDATAPROCESSORS;
	wxTreeItemId m_treeREPORTS;
	wxTreeItemId m_treeINFORMATION_REGISTERS;
	wxTreeItemId m_treeACCUMULATION_REGISTERS;

	IMetaObject* m_metaInterface;

	class wxTreeItemMetaData : public wxTreeItemData {
		IInterfaceObject* m_metaObject; //тип элемента
	public:
		wxTreeItemMetaData(IInterfaceObject* metaObject) : m_metaObject(metaObject) {}
		IInterfaceObject* GetMetaObject() const { return m_metaObject; }
	};

	wxCheckTree* m_interfaceCtrl;

protected:

	void OnCheckItem(wxTreeEvent& event);

private:

	wxTreeItemId AppendGroupItem(const wxTreeItemId& parent,
		const class_identifier_t& clsid, const wxString& name = wxEmptyString) const {
		const IAbstractTypeCtor* typeCtor = CValue::GetAvailableCtor(clsid);
		wxASSERT(typeCtor);
		wxImageList* imageList = m_interfaceCtrl->GetImageList();
		wxASSERT(imageList);
		const int imageIndex = imageList->Add(typeCtor->GetClassIcon());
		return m_interfaceCtrl->AppendItem(parent, name.IsEmpty() ? typeCtor->GetClassName() : name, imageIndex, imageIndex, nullptr);
	}

	wxTreeItemId AppendItem(const wxTreeItemId& parent,
		IMetaObject* metaObject) const {
		wxImageList* imageList = m_interfaceCtrl->GetImageList();
		wxASSERT(imageList);
		const int imageIndex = imageList->Add(metaObject->GetIcon());
		wxTreeItemId createItem = m_interfaceCtrl->AppendItem(parent, metaObject->GetName(), imageIndex, imageIndex, new wxTreeItemMetaData(metaObject));
		m_interfaceCtrl->SetItemState(createItem,
			metaObject->IsSetInterface(m_metaInterface->GetMetaID()) ?
			metaObject->IsEditable() ? wxCheckTree::CHECKED : wxCheckTree::CHECKED_DISABLED :
			metaObject->IsEditable() ? wxCheckTree::UNCHECKED : wxCheckTree::UNCHECKED_DISABLED
		);

		return createItem;
	}

	void InitInterface();
	void ClearInterface();

	void FillData();

public:

	void RefreshInterface() {
		ClearInterface();
		FillData();
	}

	void SetReadOnly(bool readOnly = true) {
		m_interfaceCtrl->Enable(!readOnly);
	}

	CInterfaceEditor(wxWindow* parent,
		wxWindowID winid = wxID_ANY,
		IMetaObject* metaObject = nullptr
	);
};

#endif 