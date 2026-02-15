////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : dataprocessor window
////////////////////////////////////////////////////////////////////////////

#include "dataProcessorWnd.h"
#include "frontend/mainFrame/mainFrame.h"
#include "frontend/docView/docManager.h"
#include "backend/appData.h"

#define	objectFormsName _("Forms")
#define	objectModulesName _("Modules")
#define	objectTemplatesName _("Templates")
#define objectAttributesName _("Attributes")
#define objectTablesName _("Tables")
#define objectEnumerationsName _("Enums")

//***********************************************************************
//*                         metaData                                    * 
//***********************************************************************

void CDataProcessorTree::ActivateItem(const wxTreeItemId& item)
{
	IValueMetaObject* currObject = GetMetaObject(item);

	if (currObject == nullptr)
		return;

	OpenFormMDI(currObject);
}

IValueMetaObject* CDataProcessorTree::NewItem(const class_identifier_t& clsid, IValueMetaObject* parent, bool runObject)
{
	return m_metaData->CreateMetaObject(clsid, parent, runObject);
}

IValueMetaObject* CDataProcessorTree::CreateItem(bool showValue)
{
	const wxTreeItemId& item = GetSelectionIdentifier();
	if (!item.IsOk()) return nullptr;

	IValueMetaObject* createdObject = NewItem(
		GetClassIdentifier(),
		GetMetaIdentifier()
	);

	if (createdObject != nullptr) {

		IPropertyObject* prev_selected = objectInspector->GetSelectedObject();

		if (showValue) { OpenFormMDI(createdObject); }
		UpdateToolbar(createdObject, FillItem(createdObject, item,
			prev_selected == objectInspector->GetSelectedObject(), false));
		for (auto& doc : docManager->GetDocumentsVector()) {
			CMetaDocument* metaDoc = wxDynamicCast(doc, CMetaDocument);
			//if (metaDoc != nullptr) metaDoc->UpdateAllViews();
		}
	}

	m_metaTreeCtrl->RefreshSelectedItem();
	return createdObject;
}

wxTreeItemId CDataProcessorTree::FillItem(IValueMetaObject* metaItem, const wxTreeItemId& item, bool select, bool scroll)
{
	m_metaTreeCtrl->Freeze();

	wxTreeItemId createdItem = nullptr;
	if (metaItem->GetClassType() == g_metaTableCLSID) {
		createdItem = AppendGroupItem(item, g_metaAttributeCLSID, metaItem);
	}
	else {
		createdItem = AppendItem(item, metaItem);
	}

	//Advanced mode
	if (metaItem->GetClassType() == g_metaTableCLSID) {

		CValueMetaObjectTableData* metaItemRecord = dynamic_cast<CValueMetaObjectTableData*>(metaItem);
		wxASSERT(metaItemRecord);

		for (auto attribute : metaItemRecord->GetAttributeArrayObject()) {
			if (attribute->IsDeleted())
				continue;
			if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
				continue;
			AppendItem(createdItem, attribute);
		}
	}

	m_metaTreeCtrl->InvalidateBestSize();
	m_metaTreeCtrl->SetEvtHandlerEnabled(select);
	m_metaTreeCtrl->SelectItem(createdItem);
	m_metaTreeCtrl->SetEvtHandlerEnabled(true);
	m_metaTreeCtrl->Expand(createdItem);

	m_metaTreeCtrl->Thaw();

	if (scroll)
		m_metaTreeCtrl->ScrollTo(createdItem);

	return createdItem;
}

void CDataProcessorTree::EditItem()
{
	wxTreeItemId selection = m_metaTreeCtrl->GetSelection();

	if (!selection.IsOk())
		return;

	IValueMetaObject* m_currObject = GetMetaObject(selection);

	if (!m_currObject)
		return;

	OpenFormMDI(m_currObject);
}

void CDataProcessorTree::RemoveItem()
{
	wxTreeItemId selection = m_metaTreeCtrl->GetSelection();

	if (!selection.IsOk())
		return;

	wxTreeItemIdValue m_cookie;
	wxTreeItemId hItem = m_metaTreeCtrl->GetFirstChild(selection, m_cookie);

	while (hItem)
	{
		EraseItem(hItem);
		hItem = m_metaTreeCtrl->GetNextChild(hItem, m_cookie);
	}

	IValueMetaObject* metaObject = GetMetaObject(selection);
	wxASSERT(metaObject);
	EraseItem(selection);
	m_metaData->RemoveMetaObject(metaObject);

	//Delete item from tree
	m_metaTreeCtrl->Delete(selection);

	for (auto& doc : docManager->GetDocumentsVector()) {
		CMetaDocument* metaDoc = wxDynamicCast(doc, CMetaDocument);
		if (metaDoc != nullptr) metaDoc->UpdateAllViews();
	}

	const wxTreeItemId& nextSelection = m_metaTreeCtrl->GetFocusedItem();

	if (nextSelection.IsOk()) {
		UpdateToolbar(GetMetaObject(nextSelection), nextSelection);
	}

	//update choice if need
	UpdateChoiceSelection();
}

void CDataProcessorTree::EraseItem(const wxTreeItemId& item)
{
	IValueMetaObject* const metaObject = GetMetaObject(item);
	for (auto& doc : docManager->GetDocumentsVector()) {
		CMetaDocument* metaDoc = wxDynamicCast(doc, CMetaDocument);
		if (metaDoc != nullptr && metaObject == metaDoc->GetMetaObject()) {
			metaDoc->DeleteAllViews();
		}
	}
}

void CDataProcessorTree::SelectItem()
{
	if (appData->GetAppMode() != eRunMode::eDESIGNER_MODE) return;
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();
	IValueMetaObject* metaObject = GetMetaObject(selection);
	UpdateToolbar(metaObject, selection);
	objectInspector->SelectObject(metaObject);
}

void CDataProcessorTree::PropertyItem()
{
	if (appData->GetAppMode() != eRunMode::eDESIGNER_MODE) return;
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();
	IValueMetaObject* metaObject = GetMetaObject(selection);
	UpdateToolbar(metaObject, selection);
	if (!objectInspector->IsShownInspector())
		objectInspector->ShowInspector();
	objectInspector->SelectObject(metaObject);
}

void CDataProcessorTree::Collapse()
{
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();
	CTreeData* data =
		dynamic_cast<CTreeData*>(m_metaTreeCtrl->GetItemData(selection));
	if (data != nullptr)
		data->m_expanded = false;
}

void CDataProcessorTree::Expand()
{
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();
	CTreeData* data =
		dynamic_cast<CTreeData*>(m_metaTreeCtrl->GetItemData(selection));
	if (data != nullptr)
		data->m_expanded = true;
}

void CDataProcessorTree::UpItem()
{
	if (appData->GetAppMode() != eRunMode::eDESIGNER_MODE)
		return;

	m_metaTreeCtrl->Freeze();
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();
	const wxTreeItemId& nextItem = m_metaTreeCtrl->GetPrevSibling(selection);
	IValueMetaObject* metaObject = GetMetaObject(selection);
	if (metaObject != nullptr && nextItem.IsOk()) {
		const wxTreeItemId& parentItem = m_metaTreeCtrl->GetItemParent(nextItem);
		wxTreeItemIdValue coockie; wxTreeItemId nextId = m_metaTreeCtrl->GetFirstChild(parentItem, coockie);
		size_t pos = 0;
		do {
			if (nextId == nextItem)
				break;
			nextId = m_metaTreeCtrl->GetNextChild(nextId, coockie); pos++;
		} while (nextId.IsOk());
		IValueMetaObject* parentObject = metaObject->GetParent();
		IValueMetaObject* nextObject = GetMetaObject(nextItem);
		if (parentObject->ChangeChildPosition(metaObject, parentObject->GetChildPosition(nextObject))) {
			wxTreeItemId newId = m_metaTreeCtrl->InsertItem(parentItem,
				pos + 2,
				m_metaTreeCtrl->GetItemText(nextItem),
				m_metaTreeCtrl->GetItemImage(nextItem),
				m_metaTreeCtrl->GetItemImage(nextItem),
				m_metaTreeCtrl->GetItemData(nextItem)
			);

			auto tree = m_metaTreeCtrl;
			std::function<void(CDataProcessorTreeCtrl*, const wxTreeItemId&, const wxTreeItemId&)> swap = [&swap](CDataProcessorTreeCtrl* tree, const wxTreeItemId& dst, const wxTreeItemId& src) {
				wxTreeItemIdValue coockie; wxTreeItemId nextId = tree->GetFirstChild(dst, coockie);
				while (nextId.IsOk()) {
					wxTreeItemId newId = tree->AppendItem(src,
						tree->GetItemText(nextId),
						tree->GetItemImage(nextId),
						tree->GetItemImage(nextId),
						tree->GetItemData(nextId)
					);
					if (tree->HasChildren(nextId)) {
						swap(tree, nextId, newId);
					}
					tree->SetItemData(nextId, nullptr);
					nextId = tree->GetNextChild(nextId, coockie);
				}
				};

			swap(tree, nextItem, newId);

			m_metaTreeCtrl->SetItemData(nextItem, nullptr);
			m_metaTreeCtrl->Delete(nextItem);

			//m_metaTreeCtrl->Expand(newId);
		}
	}
	m_metaTreeCtrl->Thaw();
}

void CDataProcessorTree::DownItem()
{
	if (appData->GetAppMode() != eRunMode::eDESIGNER_MODE)
		return;

	m_metaTreeCtrl->Freeze();
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();
	const wxTreeItemId& prevItem = m_metaTreeCtrl->GetNextSibling(selection);
	IValueMetaObject* metaObject = GetMetaObject(selection);
	if (metaObject != nullptr && prevItem.IsOk()) {
		const wxTreeItemId& parentItem = m_metaTreeCtrl->GetItemParent(prevItem);
		wxTreeItemIdValue coockie; wxTreeItemId nextId = m_metaTreeCtrl->GetFirstChild(parentItem, coockie);
		size_t pos = 0;
		do {
			if (nextId == prevItem)
				break;
			nextId = m_metaTreeCtrl->GetNextChild(nextId, coockie); pos++;
		} while (nextId.IsOk());
		IValueMetaObject* parentObject = metaObject->GetParent();
		IValueMetaObject* prevObject = GetMetaObject(prevItem);
		if (parentObject->ChangeChildPosition(metaObject, parentObject->GetChildPosition(prevObject))) {
			wxTreeItemId newId = m_metaTreeCtrl->InsertItem(parentItem,
				pos - 1,
				m_metaTreeCtrl->GetItemText(prevItem),
				m_metaTreeCtrl->GetItemImage(prevItem),
				m_metaTreeCtrl->GetItemImage(prevItem),
				m_metaTreeCtrl->GetItemData(prevItem)
			);

			auto tree = m_metaTreeCtrl;
			std::function<void(CDataProcessorTreeCtrl*, const wxTreeItemId&, const wxTreeItemId&)> swap = [&swap](CDataProcessorTreeCtrl* tree, const wxTreeItemId& dst, const wxTreeItemId& src) {
				wxTreeItemIdValue coockie; wxTreeItemId nextId = tree->GetFirstChild(dst, coockie);
				while (nextId.IsOk()) {
					wxTreeItemId newId = tree->AppendItem(src,
						tree->GetItemText(nextId),
						tree->GetItemImage(nextId),
						tree->GetItemImage(nextId),
						tree->GetItemData(nextId)
					);
					if (tree->HasChildren(nextId)) {
						swap(tree, nextId, newId);
					}
					tree->SetItemData(nextId, nullptr);
					nextId = tree->GetNextChild(nextId, coockie);
				}
				};

			swap(tree, prevItem, newId);

			m_metaTreeCtrl->SetItemData(prevItem, nullptr);
			m_metaTreeCtrl->Delete(prevItem);

			//m_metaTreeCtrl->Expand(newId);
		}
	}
	m_metaTreeCtrl->Thaw();
}

void CDataProcessorTree::SortItem()
{
	if (appData->GetAppMode() != eRunMode::eDESIGNER_MODE)
		return;
	m_metaTreeCtrl->Freeze();
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();
	IValueMetaObject* prevObject = GetMetaObject(selection);
	if (prevObject != nullptr && selection.IsOk()) {
		const wxTreeItemId& parentItem =
			m_metaTreeCtrl->GetItemParent(selection);
		if (parentItem.IsOk()) {
			m_metaTreeCtrl->SortChildren(parentItem);
		}
	}
	m_metaTreeCtrl->Thaw();
}

void CDataProcessorTree::CommandItem(unsigned int id)
{
	if (appData->GetAppMode() != eRunMode::eDESIGNER_MODE)
		return;
	wxTreeItemId sel = m_metaTreeCtrl->GetSelection();
	IValueMetaObject* metaObject = GetMetaObject(sel);
	if (!metaObject)
		return;
	metaObject->ProcessCommand(id);
}

#include "frontend/artProvider/artProvider.h"

void CDataProcessorTree::PrepareContextMenu(wxMenu* defaultMenu, const wxTreeItemId& item)
{
	IValueMetaObject* metaObject = GetMetaObject(item);

	if (metaObject
		&& !metaObject->PrepareContextMenu(defaultMenu))
	{
		wxMenuItem* menuItem = defaultMenu->Append(ID_METATREE_NEW, _("New"));
		menuItem->SetBitmap(wxArtProvider::GetBitmap(wxART_ADD, wxART_FRONTEND, wxSize(16, 16)));
		menuItem->Enable(!m_bReadOnly);
		menuItem = defaultMenu->Append(ID_METATREE_EDIT, _("Edit"));
		menuItem->SetBitmap(wxArtProvider::GetBitmap(wxART_EDIT, wxART_FRONTEND, wxSize(16, 16)));
		menuItem = defaultMenu->Append(ID_METATREE_DELETE, _("Delete"));
		menuItem->SetBitmap(wxArtProvider::GetBitmap(wxART_DELETE, wxART_FRONTEND, wxSize(16, 16)));
		menuItem->Enable(!m_bReadOnly);
		defaultMenu->AppendSeparator();
		menuItem = defaultMenu->Append(ID_METATREE_PROPERTY, _("Properties"));
		menuItem->SetBitmap(wxArtProvider::GetBitmap(wxART_PROPERTY, wxART_SERVICE));
	}
	else if (!metaObject) {
		wxMenuItem* menuItem = defaultMenu->Append(ID_METATREE_NEW, _("New"));
		menuItem->SetBitmap(wxArtProvider::GetBitmap(wxART_ADD, wxART_FRONTEND, wxSize(16, 16)));
		menuItem->Enable(!m_bReadOnly);
	}
}

void CDataProcessorTree::UpdateToolbar(IValueMetaObject* obj, const wxTreeItemId& item)
{
	m_metaTreeToolbar->EnableTool(ID_METATREE_NEW, item != m_metaTreeCtrl->GetRootItem() && !m_bReadOnly);
	m_metaTreeToolbar->EnableTool(ID_METATREE_EDIT, obj != nullptr && item != m_metaTreeCtrl->GetRootItem());
	m_metaTreeToolbar->EnableTool(ID_METATREE_DELETE, obj != nullptr && item != m_metaTreeCtrl->GetRootItem() && !m_bReadOnly);

	m_metaTreeToolbar->EnableTool(ID_METATREE_UP, obj != nullptr && item != m_metaTreeCtrl->GetRootItem() && !m_bReadOnly);
	m_metaTreeToolbar->EnableTool(ID_METATREE_DOWM, obj != nullptr && item != m_metaTreeCtrl->GetRootItem() && !m_bReadOnly);
	m_metaTreeToolbar->EnableTool(ID_METATREE_SORT, obj != nullptr && item != m_metaTreeCtrl->GetRootItem() && !m_bReadOnly);

	m_metaTreeToolbar->Refresh();
}

void CDataProcessorTree::UpdateChoiceSelection()
{
	m_defaultFormValue->Clear();
	m_defaultFormValue->AppendString(_("<not selected>"));

	CValueMetaObjectDataProcessor* commonMetadata = m_metaData->GetDataProcessor();
	wxASSERT(commonMetadata);

	int defSelection = 0;

	for (auto metaForm : commonMetadata->GetFormArrayObject()) {
		if (CValueMetaObjectDataProcessor::eFormDataProcessor != metaForm->GetTypeForm())
			continue;
		int selection_id = m_defaultFormValue->Append(metaForm->GetName(), reinterpret_cast<void*>(metaForm->GetMetaID()));
		if (commonMetadata->GetDefFormObject() == metaForm->GetMetaID()) {
			defSelection = selection_id;
		}
	}

	m_defaultFormValue->SetSelection(defSelection);
	m_defaultFormValue->SendSelectionChangedEvent(wxEVT_CHOICE);
}

bool CDataProcessorTree::RenameMetaObject(IValueMetaObject* obj, const wxString& sNewName)
{
	wxTreeItemId curItem = m_metaTreeCtrl->GetSelection();

	if (!curItem.IsOk())
		return false;

	if (m_metaData->RenameMetaObject(obj, sNewName)) {

		CMetaDocument* currDocument = GetDocument(obj);

		if (currDocument) {
			currDocument->SetTitle(obj->GetClassName() + wxT(": ") + sNewName);
			currDocument->OnChangeFilename(true);
		}

		//update choice if need
		UpdateChoiceSelection();

		m_metaTreeCtrl->SetItemText(curItem, sNewName);
		return true;
	}

	return false;
}

void CDataProcessorTree::InitTree()
{
	m_treeDATAPROCESSORS = AppendRootItem(g_metaDataProcessorCLSID, _("dataProcessor"));
	//Список аттрибутов 
	m_treeATTRIBUTES = AppendGroupItem(m_treeDATAPROCESSORS, g_metaAttributeCLSID, objectAttributesName);
	//список табличных частей 
	m_treeTABLES = AppendGroupItem(m_treeDATAPROCESSORS, g_metaTableCLSID, objectTablesName);
	//Формы
	m_treeFORM = AppendGroupItem(m_treeDATAPROCESSORS, g_metaFormCLSID, objectFormsName);
	//Таблицы
	m_treeTEMPLATES = AppendGroupItem(m_treeDATAPROCESSORS, g_metaTemplateCLSID, objectTablesName);
}

void CDataProcessorTree::ActivateTree()
{
	if (m_metaData != nullptr)
		objectInspector->SelectObject(GetMetaObject(m_metaTreeCtrl->GetSelection()));
}

void CDataProcessorTree::ClearTree()
{
	for (auto& doc : docManager->GetDocumentsVector()) {
		const CMetaDocument* metaDoc = wxDynamicCast(doc, CMetaDocument);
		const IValueMetaObject* metaObject = metaDoc->GetMetaObject();
		if (metaObject != nullptr && this == metaObject->GetMetaDataTree()) {
			doc->DeleteAllViews();
		}
	}

	//disable event
	m_metaTreeCtrl->SetEvtHandlerEnabled(false);

	//delete all child item
	if (m_treeATTRIBUTES.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeATTRIBUTES);
	if (m_treeTABLES.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeTABLES);
	if (m_treeFORM.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeFORM);
	if (m_treeTEMPLATES.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeTEMPLATES);

	//delete all items
	m_metaTreeCtrl->DeleteAllItems();

	//Initialize tree
	InitTree();

	//enable event
	m_metaTreeCtrl->SetEvtHandlerEnabled(true);
}

void CDataProcessorTree::FillData()
{
	CValueMetaObjectDataProcessor* commonMetadata = m_metaData->GetDataProcessor();
	wxASSERT(commonMetadata);
	m_metaTreeCtrl->SetItemText(m_treeDATAPROCESSORS, commonMetadata->GetName());
	m_metaTreeCtrl->SetItemData(m_treeDATAPROCESSORS, new wxTreeItemMetaData(commonMetadata));

	//set value data
	m_nameValue->SetValue(commonMetadata->GetName());
	m_synonymValue->SetValue(commonMetadata->GetSynonym());
	m_commentValue->SetValue(commonMetadata->GetComment());

	//set default form value 
	m_defaultFormValue->Clear();

	//append default value 
	m_defaultFormValue->AppendString(_("<not selected>"));

	//Список аттрибутов 
	for (auto attribute : commonMetadata->GetAttributeArrayObject()) {
		if (attribute->IsDeleted())
			continue;
		if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
			continue;
		AppendItem(m_treeATTRIBUTES, attribute);
	}

	//Список табличных частей 
	for (auto metaTable : commonMetadata->GetTableArrayObject()) {
		if (metaTable->IsDeleted())
			continue;
		const wxTreeItemId& hItem = AppendGroupItem(m_treeTABLES, g_metaAttributeCLSID, metaTable);
		for (auto attribute : metaTable->GetAttributeArrayObject()) {
			if (attribute->IsDeleted())
				continue;
			if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
				continue;
			AppendItem(hItem, attribute);
		}
	}

	//Формы
	for (auto metaForm : commonMetadata->GetFormArrayObject()) {
		if (metaForm->IsDeleted())
			continue;
		AppendItem(m_treeFORM, metaForm);
	}

	//Таблицы
	for (auto metaTemplates : commonMetadata->GetTemplateArrayObject()) {
		if (metaTemplates->IsDeleted())
			continue;
		const wxTreeItemId& hItem = AppendItem(m_treeTEMPLATES, metaTemplates);
	}

	//update choice selection
	UpdateChoiceSelection();

	//set init flag
	m_initialized = true;

	//set modify 
	Modify(m_metaData->IsModified());

	//update toolbar 
	UpdateToolbar(nullptr, m_treeATTRIBUTES);
}

bool CDataProcessorTree::Load(CMetaDataDataProcessor* metaData)
{
	ClearTree();
	m_metaData = metaData;
	m_metaTreeCtrl->Freeze();
	FillData(); //Fill all data from metaData
	m_metaData->SetMetaTree(this);
	m_metaTreeCtrl->SelectItem(m_treeATTRIBUTES);
	m_metaTreeCtrl->ExpandAll();
	m_metaTreeCtrl->Thaw();
	return true;
}

bool CDataProcessorTree::Save()
{
	CValueMetaObjectDataProcessor* commonMetadata = m_metaData->GetDataProcessor();
	wxASSERT(commonMetadata);

	commonMetadata->SetName(m_nameValue->GetValue());
	commonMetadata->SetSynonym(m_synonymValue->GetValue());
	commonMetadata->SetComment(m_commentValue->GetValue());

	wxASSERT(m_metaData);

	if (m_metaData->IsModified())
		return m_metaData->SaveDatabase();

	return false;
}