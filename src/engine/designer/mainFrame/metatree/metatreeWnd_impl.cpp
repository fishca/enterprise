////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : metatree window
////////////////////////////////////////////////////////////////////////////

#include "metatreeWnd.h"
#include "frontend/mainFrame/objinspect/objinspect.h"
#include "frontend/docView/docManager.h"
#include "backend/appData.h"

#define metadataName _("metadata")
#define commonName _("common")

#define commonModulesName _("common modules")
#define commonFormsName _("common forms")
#define commonTemplatesName _("common templates")

#define interfacesName _("interfaces")
#define rolesName _("roles")
#define picturesName _("pictures")
#define languagesName _("languages")

#define constantsName _("constants")

#define catalogsName _("catalogs")
#define documentsName _("documents")
#define enumerationsName _("enumerations")
#define dataProcessorName _("data processors")
#define reportsName _("reports")
#define informationRegisterName _("information Registers")
#define accumulationRegisterName _("accumulation Registers")

#define	objectFormsName _("forms")
#define	objectModulesName _("modules")
#define	objectTemplatesName _("templates")
#define objectAttributesName _("attributes")
#define objectDimensionsName _("dimensions")
#define objectResourcesName _("resources")

#define objectTablesName _("tables")
#define objectEnumerationsName _("enums")

//***********************************************************************
//*								metadata                                * 
//***********************************************************************

#include "frontend/mainFrame/mainFrame.h"
#include "frontend/win/dlgs/formSelector/formSelector.h"

form_identifier_t IMetaDataTree::SelectFormType(CValueMetaObjectForm* metaObject) const
{
	IValueMetaObjectGenericData* parent = wxDynamicCast(
		metaObject->GetParent(), IValueMetaObjectGenericData
	);

	CDialogSelectTypeForm dlg(parent, metaObject);
	CFormTypeList optList = parent->GetFormType();
	for (unsigned int idx = 0; idx < optList.GetItemCount(); idx++) {
		dlg.AppendTypeForm(optList.GetItemName(idx), optList.GetItemLabel(idx), optList.GetItemId(idx));
	}

	dlg.CreateSelector();
	return dlg.ShowModal();
}

void IMetaDataTree::Activate()
{
	if (m_docParent == nullptr) {
		unsigned int count_doc = 0;
		for (auto doc : docManager->GetDocumentsVector()) count_doc++;
		if (count_doc <= 1) SetFocus();
	}
}

void IMetaDataTree::Modify(bool modify)
{
	if (m_docParent != nullptr) {
		m_docParent->Modify(modify);
	}
	else {
		mainFrame->Modify(modify);
	}
}

bool IMetaDataTree::OpenFormMDI(IValueMetaObject* obj)
{
	CMetaDocument* foundedDoc = GetDocument(obj);
	//not found in the list of existing ones
	if (foundedDoc == nullptr) {
		foundedDoc = docManager->OpenFormMDI(obj, m_docParent, m_bReadOnly ? wxDOC_READONLY : wxDOC_NEW);
		//So there was no suitable template!
		if (foundedDoc != nullptr)
			return true;
	}
	else {
		wxWindow* docWindow = foundedDoc->GetDocumentWindow();
		if (docWindow != nullptr) docWindow->Raise();
		return true;
	}

	return false;
}

bool IMetaDataTree::OpenFormMDI(IValueMetaObject* obj, IBackendMetaDocument*& doc)
{
	CMetaDocument* foundedDoc = GetDocument(obj);

	//not found in the list of existing ones
	if (foundedDoc == nullptr) {
		foundedDoc = docManager->OpenFormMDI(obj, m_docParent, m_bReadOnly ? wxDOC_READONLY : wxDOC_NEW);
		//So there was no suitable template!
		if (foundedDoc != nullptr) {
			doc = foundedDoc;
			return true;
		}
	}
	else {
		wxWindow* docWindow = foundedDoc->GetDocumentWindow();
		if (docWindow != nullptr) docWindow->Raise();
		doc = foundedDoc;
		return true;
	}

	return false;
}

bool IMetaDataTree::CloseFormMDI(IValueMetaObject* obj)
{
	CMetaDocument* foundedDoc = GetDocument(obj);

	//not found in the list of existing ones
	if (foundedDoc != nullptr) {
		objectInspector->SelectObject(obj, this);
		if (foundedDoc->Close()) {
			// Delete the child document by deleting all its views.
			return foundedDoc->DeleteAllViews();
		}
	}

	return false;
}

CMetaDocument* IMetaDataTree::GetDocument(IValueMetaObject* obj) const
{
	for (auto& doc : docManager->GetDocumentsVector()) {
		CMetaDocument* metaDoc = wxDynamicCast(doc, CMetaDocument);
		if (metaDoc != nullptr && obj == metaDoc->GetMetaObject()) {
			return metaDoc;
		}
		else if (metaDoc != nullptr) {
			for (auto& child_doc : metaDoc->GetChild()) {
				CMetaDocument* child_metaDoc = wxDynamicCast(child_doc, CMetaDocument);
				if (child_metaDoc != nullptr && obj == child_metaDoc->GetMetaObject()) {
					return child_metaDoc;
				}
			}
		}
	}
	return nullptr;
}

void IMetaDataTree::EditModule(const CGuid& moduleName, int lineNumber, bool setRunLine)
{
	const IMetaData* metaData = GetMetaData();
	if (metaData == nullptr)
		return;

	IValueMetaObject* metaObject = metaData->FindAnyObjectByFilter(moduleName, true);

	if (metaObject == nullptr || metaObject->IsDeleted())
		return;

	if (m_bReadOnly)
		return;

	CMetaDocument* foundedDoc = GetDocument(metaObject);

	//not found in the list of existing ones
	if (foundedDoc == nullptr)
		foundedDoc = docManager->OpenFormMDI(metaObject, m_docParent, m_bReadOnly ? wxDOC_READONLY : wxDOC_NEW);

	IValueModuleDocument* moduleDoc = static_cast<IValueModuleDocument*>(foundedDoc);
	if (moduleDoc != nullptr) moduleDoc->SetCurrentLine(lineNumber, setRunLine);
}

//***********************************************************************
//*								 metaData                               * 
//***********************************************************************

void CMetadataTree::ActivateItem(const wxTreeItemId& item)
{
	IValueMetaObject* currObject = GetMetaObject(item);
	if (currObject == nullptr)
		return;

	OpenFormMDI(currObject);
}

IValueMetaObject* CMetadataTree::NewItem(const class_identifier_t& clsid, IValueMetaObject* parent, bool runObject)
{
	return m_metaData->CreateMetaObject(clsid, parent, runObject);
}

IValueMetaObject* CMetadataTree::CreateItem(bool showValue)
{
	const wxTreeItemId& item = GetSelectionIdentifier();
	if (!item.IsOk()) return nullptr;

	Freeze();

	IValueMetaObject* createdObject = NewItem(
		GetClassIdentifier(),
		GetMetaIdentifier()
	);

	if (createdObject != nullptr) {

		IPropertyObject* oldSelection = objectInspector->GetSelectedObject();
		if (showValue) { OpenFormMDI(createdObject); }
		UpdateToolbar(createdObject,
			FillItem(createdObject, item, oldSelection == objectInspector->GetSelectedObject(), false));
	}

	Thaw();

	m_metaTreeCtrl->RefreshSelectedItem();
	return createdObject;
}

wxTreeItemId CMetadataTree::FillItem(IValueMetaObject* metaItem, const wxTreeItemId& item, bool select, bool scroll)
{
	m_metaTreeCtrl->Freeze();

	wxTreeItemId createdItem = nullptr;
	if (metaItem->GetClassType() == g_metaTableCLSID) {
		createdItem = AppendGroupItem(item, g_metaAttributeCLSID, metaItem);
	}
	else if (metaItem->GetClassType() == g_metaInterfaceCLSID) {
		createdItem = AppendGroupItem(item, g_metaInterfaceCLSID, metaItem);
	}
	else {
		createdItem = AppendItem(item, metaItem);
	}

	//Advanced mode
	if (metaItem->GetClassType() == g_metaCatalogCLSID) AddCatalogItem(metaItem, createdItem);
	else if (metaItem->GetClassType() == g_metaDocumentCLSID) AddDocumentItem(metaItem, createdItem);
	else if (metaItem->GetClassType() == g_metaEnumerationCLSID) AddEnumerationItem(metaItem, createdItem);
	else if (metaItem->GetClassType() == g_metaDataProcessorCLSID) AddDataProcessorItem(metaItem, createdItem);
	else if (metaItem->GetClassType() == g_metaReportCLSID) AddReportItem(metaItem, createdItem);
	else if (metaItem->GetClassType() == g_metaInformationRegisterCLSID) AddInformationRegisterItem(metaItem, createdItem);
	else if (metaItem->GetClassType() == g_metaAccumulationRegisterCLSID) AddAccumulationRegisterItem(metaItem, createdItem);

	else if (metaItem->GetClassType() == g_metaTableCLSID) {

		CValueMetaObjectTableData* metaItemRecord = metaItem->ConvertToType<CValueMetaObjectTableData>();
		wxASSERT(metaItemRecord);

		for (auto attribute : metaItemRecord->GetAttributeArrayObject()) {
			if (attribute->IsDeleted())
				continue;
			if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
				continue;
			AppendItem(createdItem, attribute);
		}
	}
	else if (metaItem->GetClassType() == g_metaInterfaceCLSID) {
		CValueMetaObjectInterface* metaItemRecord = metaItem->ConvertToType<CValueMetaObjectInterface>();
		for (auto object : metaItemRecord->GetInterfaceArrayObject()) {
			if (object->IsDeleted())
				continue;
			AppendItem(createdItem, object);
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

void CMetadataTree::EditItem()
{
	wxTreeItemId selection = m_metaTreeCtrl->GetSelection();
	if (!selection.IsOk())
		return;
	IValueMetaObject* m_currObject = GetMetaObject(selection);
	if (!m_currObject)
		return;

	OpenFormMDI(m_currObject);
}

void CMetadataTree::RemoveItem()
{
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();

	if (!selection.IsOk())
		return;

	wxTreeItemIdValue m_cookie;
	wxTreeItemId hItem = m_metaTreeCtrl->GetFirstChild(selection, m_cookie);

	while (hItem) {
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

	const wxTreeItemId nextSelection = m_metaTreeCtrl->GetFocusedItem();
	if (nextSelection.IsOk()) {
		UpdateToolbar(GetMetaObject(nextSelection), nextSelection);
	}
}

void CMetadataTree::EraseItem(const wxTreeItemId& item)
{
	IValueMetaObject* const metaObject = GetMetaObject(item);
	for (auto& doc : docManager->GetDocumentsVector()) {
		CMetaDocument* metaDoc = wxDynamicCast(doc, CMetaDocument);
		if (metaDoc != nullptr && metaObject != nullptr && metaObject == metaDoc->GetMetaObject()) {
			metaDoc->DeleteAllViews();
		}
	}
}

void CMetadataTree::SelectItem()
{
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();
	IValueMetaObject* metaObject = GetMetaObject(selection);
	UpdateToolbar(metaObject, selection);

	if (appData->GetAppMode() != eRunMode::eDESIGNER_MODE)
		return;

	objectInspector->SelectObject(metaObject);
}

void CMetadataTree::PropertyItem()
{
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();
	IValueMetaObject* metaObject = GetMetaObject(selection);
	UpdateToolbar(metaObject, selection);

	if (appData->GetAppMode() != eRunMode::eDESIGNER_MODE)
		return;

	if (!objectInspector->IsShownInspector())
		objectInspector->ShowInspector();

	objectInspector->SelectObject(metaObject);
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CMetadataTree::Collapse()
{
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();
	CTreeData* data =
		dynamic_cast<CTreeData*>(m_metaTreeCtrl->GetItemData(selection));
	if (data != nullptr)
		data->m_expanded = false;
}

void CMetadataTree::Expand()
{
	const wxTreeItemId& selection = m_metaTreeCtrl->GetSelection();
	CTreeData* data =
		dynamic_cast<CTreeData*>(m_metaTreeCtrl->GetItemData(selection));
	if (data != nullptr)
		data->m_expanded = true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CMetadataTree::UpItem()
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
			std::function<void(CMetaTreeCtrl*, const wxTreeItemId&, const wxTreeItemId&)> swap = [&swap](CMetaTreeCtrl* tree, const wxTreeItemId& dst, const wxTreeItemId& src) {
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

void CMetadataTree::DownItem()
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
			std::function<void(CMetaTreeCtrl*, const wxTreeItemId&, const wxTreeItemId&)> swap = [&swap](CMetaTreeCtrl* tree, const wxTreeItemId& dst, const wxTreeItemId& src) {
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

void CMetadataTree::SortItem()
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

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#include "backend/metadataDataProcessor.h"
#include "backend/metadataReport.h"

void CMetadataTree::InsertItem()
{
	IValueMetaObject* commonMetaObject = m_metaData->GetCommonMetaObject(); wxTreeItemId hSelItem = m_metaTreeCtrl->GetSelection();

	if (hSelItem == m_treeDATAPROCESSORS) {

		wxFileDialog openFileDialog(this, _("Open data processor file"), "", "",
			_("Data processor files (*.edp)|*.edp"), wxFD_OPEN | wxFD_FILE_MUST_EXIST);

		if (openFileDialog.ShowModal() == wxID_CANCEL)
			return;     // the user changed idea...

		//create main metaObject
		CMetaDataDataProcessor metadataDataProcessor(m_metaData);

		if (metadataDataProcessor.LoadFromFile(openFileDialog.GetPath())) {
			m_metaTreeCtrl->Freeze();
			CValueMetaObjectDataProcessor* dataProcessor = metadataDataProcessor.GetDataProcessor();
			wxASSERT(dataProcessor);
			const wxTreeItemId& createdItem = AppendItem(hSelItem, dataProcessor);
			AddDataProcessorItem(dataProcessor, createdItem);
			m_metaTreeCtrl->SelectItem(createdItem);
			dataProcessor->IncrRef();
			m_metaTreeCtrl->Thaw();
		}
	}
	else {
		wxFileDialog openFileDialog(this, _("Open report file"), "", "",
			"report files (*.erp)|*.erp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

		if (openFileDialog.ShowModal() == wxID_CANCEL)
			return;     // the user changed idea...

		CMetaDataReport metadataReport(m_metaData);

		if (metadataReport.LoadFromFile(openFileDialog.GetPath())) {
			m_metaTreeCtrl->Freeze();
			CValueMetaObjectReport* report = metadataReport.GetReport();
			wxASSERT(report);
			const wxTreeItemId& createdItem = AppendItem(hSelItem, report);
			AddReportItem(report, createdItem);
			m_metaTreeCtrl->SelectItem(createdItem);
			report->IncrRef();
			m_metaTreeCtrl->Thaw();
		}
	}

	m_metaData->Modify(true);
}

void CMetadataTree::ReplaceItem()
{
	wxTreeItemId hSelItem = m_metaTreeCtrl->GetSelection();
	IValueMetaObject* currentMetaObject = GetMetaObject(m_metaTreeCtrl->GetSelection());

	if (currentMetaObject->GetClassType() == g_metaDataProcessorCLSID) {

		wxFileDialog openFileDialog(this, _("Open data processor file"), "", "",
			"data processor files (*.edp)|*.edp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

		if (openFileDialog.ShowModal() == wxID_CANCEL)
			return;     // the user changed idea...

		CMetaDataDataProcessor metadataDataProcessor(m_metaData);
		if (metadataDataProcessor.LoadFromFile(openFileDialog.GetPath())) {
			m_metaTreeCtrl->Freeze();
			CValueMetaObjectDataProcessor* metaObject = metadataDataProcessor.GetDataProcessor();
			wxTreeItemData* itemData = m_metaTreeCtrl->GetItemData(hSelItem);
			if (itemData != nullptr) {
				CTreeDataMetaItem* metaItem = dynamic_cast<CTreeDataMetaItem*>(itemData);
				if (metaItem != nullptr)
					metaItem->m_metaObject = metaObject;
			}
			m_metaData->RemoveMetaObject(currentMetaObject);
			m_metaTreeCtrl->SetItemText(hSelItem, metaObject->GetName());
			m_metaTreeCtrl->DeleteChildren(hSelItem);
			AddDataProcessorItem(metaObject, hSelItem);
			m_metaTreeCtrl->Thaw();
		}
	}
	else
	{
		wxFileDialog openFileDialog(this, _("Open report file"), "", "",
			"report files (*.erp)|*.erp", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

		if (openFileDialog.ShowModal() == wxID_CANCEL)
			return;     // the user changed idea...

		CValueMetaObjectReport* newReport = dynamic_cast<CValueMetaObjectReport*>(
			currentMetaObject
			);

		wxASSERT(newReport);

		CMetaDataReport metadataDataProcessor(m_metaData);
		if (metadataDataProcessor.LoadFromFile(openFileDialog.GetPath())) {
			m_metaTreeCtrl->Freeze();
			CValueMetaObjectReport* metaObject = metadataDataProcessor.GetReport();
			wxTreeItemData* itemData = m_metaTreeCtrl->GetItemData(hSelItem);
			if (itemData != nullptr) {
				CTreeDataMetaItem* metaItem = dynamic_cast<CTreeDataMetaItem*>(itemData);
				if (metaItem != nullptr)
					metaItem->m_metaObject = metaObject;
			}
			m_metaData->RemoveMetaObject(currentMetaObject);
			m_metaTreeCtrl->SetItemText(hSelItem, newReport->GetName());
			m_metaTreeCtrl->DeleteChildren(hSelItem);
			AddReportItem(newReport, hSelItem);
			m_metaTreeCtrl->Thaw();
		}
	}

	m_metaData->Modify(true);
}

void CMetadataTree::SaveItem()
{
	IValueMetaObject* currentMetaObject = GetMetaObject(m_metaTreeCtrl->GetSelection());

	if (currentMetaObject->GetClassType() == g_metaDataProcessorCLSID) {

		wxFileDialog saveFileDialog(this, _("Open data processor file"), "", "",
			"data processor files (*.edp)|*.edp", wxFD_SAVE);

		saveFileDialog.SetFilename(m_metaTreeCtrl->GetItemText(m_metaTreeCtrl->GetSelection()));

		if (saveFileDialog.ShowModal() == wxID_CANCEL)
			return;     // the user changed idea...

		CValueMetaObjectDataProcessor* newDataProcessor = dynamic_cast<CValueMetaObjectDataProcessor*>(
			currentMetaObject
			);
		wxASSERT(newDataProcessor);
		CMetaDataDataProcessor metadataDataProcessor(m_metaData, newDataProcessor);
		metadataDataProcessor.SaveToFile(saveFileDialog.GetPath());
	}
	else {
		wxFileDialog saveFileDialog(this, _("Open report file"), "", "",
			"report files (*.erp)|*.erp", wxFD_SAVE);

		saveFileDialog.SetFilename(m_metaTreeCtrl->GetItemText(m_metaTreeCtrl->GetSelection()));

		if (saveFileDialog.ShowModal() == wxID_CANCEL)
			return;     // the user changed idea...

		CValueMetaObjectReport* newDataProcessor = dynamic_cast<CValueMetaObjectReport*>(
			currentMetaObject
			);
		wxASSERT(newDataProcessor);
		CMetaDataReport metadataDataProcessor(m_metaData, newDataProcessor);
		metadataDataProcessor.SaveToFile(saveFileDialog.GetPath());
	}
}

void CMetadataTree::CommandItem(unsigned int id)
{
	if (appData->GetAppMode() != eRunMode::eDESIGNER_MODE)
		return;

	IValueMetaObject* metaObject = GetMetaObject(m_metaTreeCtrl->GetSelection());

	if (!metaObject)
		return;

	metaObject->ProcessCommand(id);
}

void CMetadataTree::PrepareReplaceMenu(wxMenu* defaultMenu)
{
	wxMenuItem* menuItem = defaultMenu->Append(ID_METATREE_REPLACE, _("Replace data processor, report..."));
	menuItem->Enable(!m_bReadOnly);
	menuItem = defaultMenu->Append(ID_METATREE_SAVE, _("Save data processor, report..."));
	defaultMenu->AppendSeparator();
}

#include "frontend/artProvider/artProvider.h"

void CMetadataTree::PrepareContextMenu(wxMenu* defaultMenu, const wxTreeItemId& item)
{
	IValueMetaObject* metaObject = GetMetaObject(item);

	if (metaObject
		&& !metaObject->PrepareContextMenu(defaultMenu))
	{
		if (g_metaDataProcessorCLSID == metaObject->GetClassType()
			|| g_metaReportCLSID == metaObject->GetClassType()) {
			PrepareReplaceMenu(defaultMenu);
		}

		wxMenuItem* menuItem = nullptr;

		menuItem = defaultMenu->Append(ID_METATREE_NEW, _("New"));
		menuItem->SetBitmap(CBackendPicture::GetPicture(g_picAddCLSID));
		menuItem->Enable(!m_bReadOnly);
		menuItem = defaultMenu->Append(ID_METATREE_EDIT, _("Edit"));
		menuItem->SetBitmap(CBackendPicture::GetPicture(g_picEditCLSID));
		menuItem = defaultMenu->Append(ID_METATREE_DELETE, _("Remove"));
		menuItem->SetBitmap(CBackendPicture::GetPicture(g_picDeleteCLSID));
		menuItem->Enable(!m_bReadOnly);
		defaultMenu->AppendSeparator();
		menuItem = defaultMenu->Append(ID_METATREE_PROPERTY, _("Properties"));
		menuItem->SetBitmap(wxArtProvider::GetBitmap(wxART_PROPERTY, wxART_SERVICE));
	}
	else if (!metaObject && item != m_treeCOMMON) {
		wxMenuItem* menuItem = defaultMenu->Append(ID_METATREE_NEW, _("New"));
		menuItem->SetBitmap(CBackendPicture::GetPicture(g_picAddCLSID));
		menuItem->Enable(!m_bReadOnly);

		if (item == m_treeDATAPROCESSORS
			|| item == m_treeREPORTS) {
			defaultMenu->AppendSeparator();
			wxMenuItem* menuItem = defaultMenu->Append(ID_METATREE_INSERT, _("Insert data processor, report..."));
			menuItem->Enable(!m_bReadOnly);
		}
	}
	else if (item == m_treeMETADATA) {
		defaultMenu->AppendSeparator();
		wxMenuItem* menuItem = defaultMenu->Append(ID_METATREE_PROPERTY, _("Properties"));
		menuItem->SetBitmap(wxArtProvider::GetBitmap(wxART_PROPERTY, wxART_SERVICE));
	}
}

void CMetadataTree::UpdateToolbar(IValueMetaObject* obj, const wxTreeItemId& item)
{
	m_metaTreeToolbar->EnableTool(ID_METATREE_NEW, item != m_metaTreeCtrl->GetRootItem() && !m_bReadOnly && item != m_treeCOMMON);
	m_metaTreeToolbar->EnableTool(ID_METATREE_EDIT, obj != nullptr && item != m_metaTreeCtrl->GetRootItem());
	m_metaTreeToolbar->EnableTool(ID_METATREE_DELETE, obj != nullptr && item != m_metaTreeCtrl->GetRootItem() && !m_bReadOnly);

	m_metaTreeToolbar->EnableTool(ID_METATREE_UP, obj != nullptr && item != m_metaTreeCtrl->GetRootItem() && !m_bReadOnly);
	m_metaTreeToolbar->EnableTool(ID_METATREE_DOWM, obj != nullptr && item != m_metaTreeCtrl->GetRootItem() && !m_bReadOnly);
	m_metaTreeToolbar->EnableTool(ID_METATREE_SORT, obj != nullptr && item != m_metaTreeCtrl->GetRootItem() && !m_bReadOnly);

	m_metaTreeToolbar->Refresh();
}

bool CMetadataTree::RenameMetaObject(IValueMetaObject* metaObject, const wxString& newName)
{
	wxTreeItemId curItem = m_metaTreeCtrl->GetSelection();

	if (!curItem.IsOk()) {
		return false;
	}

	if (m_metaData->RenameMetaObject(metaObject, newName)) {
		CMetaDocument* currDocument = GetDocument(metaObject);
		if (currDocument != nullptr) {
			currDocument->SetTitle(metaObject->GetClassName() + wxT(": ") + newName);
			currDocument->OnChangeFilename(true);
		}
		m_metaTreeCtrl->SetItemText(curItem, newName);
		return true;
	}
	return false;
}

#include "backend/metaCollection/partial/commonObject.h"

void CMetadataTree::AddInterfaceItem(IValueMetaObject* metaObject, const wxTreeItemId& hParentID)
{
	CValueMetaObjectInterface* metaObjectValue = metaObject->ConvertToType<CValueMetaObjectInterface>();
	wxASSERT(metaObject);

	for (auto commonInterface : metaObjectValue->GetInterfaceArrayObject()) {

		if (commonInterface->IsDeleted())
			continue;

		//const wxString& strName = commonInterface->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AddInterfaceItem(commonInterface,
			AppendGroupItem(hParentID, g_metaInterfaceCLSID, commonInterface));
	}
}

void CMetadataTree::AddCatalogItem(IValueMetaObject* metaObject, const wxTreeItemId& hParentID)
{
	IValueMetaObjectRecordDataRef* metaObjectValue = metaObject->ConvertToType<IValueMetaObjectRecordDataRef>();
	wxASSERT(metaObject);

	//Список аттрибутов 	
	const wxTreeItemId& hAttributes = AppendGroupItem(hParentID, g_metaAttributeCLSID, objectAttributesName);
	for (auto attribute : metaObjectValue->GetAttributeArrayObject()) {

		if (attribute->IsDeleted())
			continue;

		if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
			continue;

		//const wxString strName = attribute->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hAttributes, attribute);
	}

	//список табличных частей 
	const wxTreeItemId& hTables = AppendGroupItem(hParentID, g_metaTableCLSID, objectTablesName);
	for (auto metaTable : metaObjectValue->GetTableArrayObject()) {

		if (metaTable->IsDeleted())
			continue;

		//const wxString strName = metaTable->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		const wxTreeItemId& hItem = AppendGroupItem(hTables, g_metaAttributeCLSID, metaTable);

		for (auto attribute : metaTable->GetAttributeArrayObject()) {

			if (attribute->IsDeleted())
				continue;

			if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
				continue;

			//const wxString strName = attribute->GetName();

			//if (!m_strSearch.IsEmpty()
			//	&& strName.Find(m_strSearch) < 0)
			//	continue;

			AppendItem(hItem, attribute);
		}
	}

	//Формы
	const wxTreeItemId& hForm = AppendGroupItem(hParentID, g_metaFormCLSID, objectFormsName);
	for (auto metaForm : metaObjectValue->GetFormArrayObject()) {

		if (metaForm->IsDeleted())
			continue;

		//const wxString strName = metaForm->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hForm, metaForm);
	}

	//Таблицы
	const wxTreeItemId& hTemplates = AppendGroupItem(hParentID, g_metaTemplateCLSID, objectTemplatesName);
	for (auto metaTemplate : metaObjectValue->GetTemplateArrayObject()) {

		if (metaTemplate->IsDeleted())
			continue;

		//const wxString strName = metaTemplate->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hTemplates, metaTemplate);
	}
}

void CMetadataTree::AddDocumentItem(IValueMetaObject* metaObject, const wxTreeItemId& hParentID)
{
	IValueMetaObjectRecordDataRef* metaObjectValue = metaObject->ConvertToType<IValueMetaObjectRecordDataRef>();

	wxASSERT(metaObject);

	//Список аттрибутов 	
	const wxTreeItemId& hAttributes = AppendGroupItem(hParentID, g_metaAttributeCLSID, objectAttributesName);
	for (auto attribute : metaObjectValue->GetAttributeArrayObject()) {

		if (attribute->IsDeleted())
			continue;

		if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
			continue;

		//const wxString strName = attribute->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hAttributes, attribute);
	}

	//список табличных частей 
	const wxTreeItemId& hTables = AppendGroupItem(hParentID, g_metaTableCLSID, objectTablesName);
	for (auto metaTable : metaObjectValue->GetTableArrayObject()) {

		if (metaTable->IsDeleted())
			continue;

		//const wxString strName = metaTable->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		const wxTreeItemId& hItem = AppendGroupItem(hTables, g_metaAttributeCLSID, metaTable);
		for (auto attribute : metaTable->GetAttributeArrayObject()) {

			if (attribute->IsDeleted())
				continue;

			if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
				continue;

			//const wxString strName = attribute->GetName();

			//if (!m_strSearch.IsEmpty()
			//	&& strName.Find(m_strSearch) < 0)
			//	continue;

			AppendItem(hItem, attribute);
		}
	}

	//Формы
	const wxTreeItemId& hForm = AppendGroupItem(hParentID, g_metaFormCLSID, objectFormsName);
	for (auto metaForm : metaObjectValue->GetFormArrayObject()) {

		if (metaForm->IsDeleted())
			continue;

		//const wxString strName = metaForm->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hForm, metaForm);
	}

	//Таблицы
	const wxTreeItemId& hTemplates = AppendGroupItem(hParentID, g_metaTemplateCLSID, objectTemplatesName);
	for (auto metaTemplate : metaObjectValue->GetTemplateArrayObject()) {

		if (metaTemplate->IsDeleted())
			continue;

		//const wxString strName = metaTemplate->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hTemplates, metaTemplate);
	}
}

void CMetadataTree::AddEnumerationItem(IValueMetaObject* metaObject, const wxTreeItemId& hParentID)
{
	IValueMetaObjectRecordDataEnumRef* metaObjectValue = metaObject->ConvertToType <IValueMetaObjectRecordDataEnumRef>();
	wxASSERT(metaObjectValue);

	//Enumerations
	wxTreeItemId hEnums = AppendGroupItem(hParentID, g_metaEnumCLSID, objectEnumerationsName);

	for (auto metaEnumerations : metaObjectValue->GetEnumObjectArray()) {

		if (metaEnumerations->IsDeleted())
			continue;

		//const wxString strName = metaEnumerations->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hEnums, metaEnumerations);
	}

	//Формы
	const wxTreeItemId& hForm = AppendGroupItem(hParentID, g_metaFormCLSID, objectFormsName);
	for (auto metaForm : metaObjectValue->GetFormArrayObject()) {

		if (metaForm->IsDeleted())
			continue;

		//const wxString strName = metaForm->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hForm, metaForm);
	}

	//Таблицы
	const wxTreeItemId& hTemplates = AppendGroupItem(hParentID, g_metaTemplateCLSID, objectTemplatesName);
	for (auto metaTemplate : metaObjectValue->GetTemplateArrayObject()) {

		if (metaTemplate->IsDeleted())
			continue;

		//const wxString strName = metaTemplate->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hTemplates, metaTemplate);
	}
}

void CMetadataTree::AddDataProcessorItem(IValueMetaObject* metaObject, const wxTreeItemId& hParentID)
{
	IValueMetaObjectRecordData* metaObjectValue = metaObject->ConvertToType <IValueMetaObjectRecordData>();
	wxASSERT(metaObjectValue);

	//Список аттрибутов 	
	const wxTreeItemId& hAttributes = AppendGroupItem(hParentID, g_metaAttributeCLSID, objectAttributesName);
	for (auto attribute : metaObjectValue->GetAttributeArrayObject()) {

		if (attribute->IsDeleted())
			continue;

		if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
			continue;

		//const wxString strName = attribute->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hAttributes, attribute);
	}

	//список табличных частей 
	const wxTreeItemId& hTables = AppendGroupItem(hParentID, g_metaTableCLSID, objectTablesName);
	for (auto metaTable : metaObjectValue->GetTableArrayObject()) {

		if (metaTable->IsDeleted())
			continue;

		//const wxString strName = metaTable->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		const wxTreeItemId& hItem = AppendGroupItem(hTables, g_metaAttributeCLSID, metaTable);
		for (auto attribute : metaTable->GetAttributeArrayObject()) {

			if (attribute->IsDeleted())
				continue;

			if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
				continue;

			//const wxString strName = attribute->GetName();

			//if (!m_strSearch.IsEmpty()
			//	&& strName.Find(m_strSearch) < 0)
			//	continue;

			AppendItem(hItem, attribute);
		}
	}

	//Формы
	const wxTreeItemId& hForm = AppendGroupItem(hParentID, g_metaFormCLSID, objectFormsName);
	for (auto metaForm : metaObjectValue->GetFormArrayObject()) {

		if (metaForm->IsDeleted())
			continue;

		//const wxString strName = metaForm->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hForm, metaForm);
	}

	//Таблицы
	const wxTreeItemId& hTemplates = AppendGroupItem(hParentID, g_metaTemplateCLSID, objectTemplatesName);
	for (auto metaTemplate : metaObjectValue->GetTemplateArrayObject()) {

		if (metaTemplate->IsDeleted())
			continue;

		//const wxString strName = metaTemplate->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hTemplates, metaTemplate);
	}
}

void CMetadataTree::AddReportItem(IValueMetaObject* metaObject, const wxTreeItemId& hParentID)
{
	IValueMetaObjectRecordData* metaObjectValue = metaObject->ConvertToType<IValueMetaObjectRecordData>();
	wxASSERT(metaObjectValue);

	//Список аттрибутов 	
	const wxTreeItemId& hAttributes = AppendGroupItem(hParentID, g_metaAttributeCLSID, objectAttributesName);
	for (auto attribute : metaObjectValue->GetAttributeArrayObject()) {

		if (attribute->IsDeleted())
			continue;

		if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
			continue;

		//const wxString strName = attribute->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hAttributes, attribute);
	}

	//список табличных частей 
	const wxTreeItemId& hTables = AppendGroupItem(hParentID, g_metaTableCLSID, objectTablesName);
	for (auto metaTable : metaObjectValue->GetTableArrayObject()) {
		if (metaTable->IsDeleted())
			continue;

		//const wxString strName = metaTable->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		const wxTreeItemId& hItem = AppendGroupItem(hTables, g_metaAttributeCLSID, metaTable);
		for (auto attribute : metaTable->GetAttributeArrayObject()) {

			if (attribute->IsDeleted())
				continue;

			if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
				continue;

			//const wxString strName = attribute->GetName();

			//if (!m_strSearch.IsEmpty()
			//	&& strName.Find(m_strSearch) < 0)
			//	continue;

			AppendItem(hItem, attribute);
		}
	}

	//Формы
	const wxTreeItemId& hForm = AppendGroupItem(hParentID, g_metaFormCLSID, objectFormsName);
	for (auto metaForm : metaObjectValue->GetFormArrayObject()) {

		if (metaForm->IsDeleted())
			continue;

		//const wxString strName = metaForm->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hForm, metaForm);
	}

	//Таблицы
	const wxTreeItemId& hTemplates = AppendGroupItem(hParentID, g_metaTemplateCLSID, objectTemplatesName);
	for (auto metaTemplate : metaObjectValue->GetTemplateArrayObject()) {

		if (metaTemplate->IsDeleted())
			continue;

		//const wxString strName = metaTemplate->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hTemplates, metaTemplate);
	}
}

void CMetadataTree::AddInformationRegisterItem(IValueMetaObject* metaObject, const wxTreeItemId& hParentID)
{
	IValueMetaObjectRegisterData* metaObjectValue = metaObject->ConvertToType<IValueMetaObjectRegisterData>();
	wxASSERT(metaObjectValue);

	//Список измерений 
	const wxTreeItemId& hDimentions = AppendGroupItem(hParentID, g_metaDimensionCLSID, objectDimensionsName);
	for (auto metaDimension : metaObjectValue->GetDimentionArrayObject()) {

		if (metaDimension->IsDeleted())
			continue;

		if (metaDimension->GetClassType() == g_metaPredefinedAttributeCLSID)
			continue;

		//const wxString strName = metaDimension->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hDimentions, metaDimension);
	}

	//Список ресурсов 
	const wxTreeItemId& hResources = AppendGroupItem(hParentID, g_metaResourceCLSID, objectResourcesName);
	for (auto metaResource : metaObjectValue->GetResourceArrayObject()) {

		if (metaResource->IsDeleted())
			continue;

		if (metaResource->GetClassType() == g_metaPredefinedAttributeCLSID)
			continue;

		//const wxString strName = metaResource->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hResources, metaResource);
	}

	//Список аттрибутов 	
	const wxTreeItemId& hAttributes = AppendGroupItem(hParentID, g_metaAttributeCLSID, objectAttributesName);
	for (auto attribute : metaObjectValue->GetAttributeArrayObject()) {

		if (attribute->IsDeleted())
			continue;

		if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
			continue;

		//const wxString strName = attribute->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hAttributes, attribute);
	}

	//Формы
	const wxTreeItemId& hForm = AppendGroupItem(hParentID, g_metaFormCLSID, objectFormsName);
	for (auto metaForm : metaObjectValue->GetFormArrayObject()) {

		if (metaForm->IsDeleted())
			continue;

		//const wxString strName = metaForm->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hForm, metaForm);
	}

	//Таблицы
	const wxTreeItemId& hTemplates = AppendGroupItem(hParentID, g_metaTemplateCLSID, objectTemplatesName);
	for (auto metaTemplate : metaObjectValue->GetTemplateArrayObject()) {

		if (metaTemplate->IsDeleted())
			continue;

		//const wxString strName = metaTemplate->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hTemplates, metaTemplate);
	}
}

void CMetadataTree::AddAccumulationRegisterItem(IValueMetaObject* metaObject, const wxTreeItemId& hParentID)
{
	IValueMetaObjectRegisterData* metaObjectValue = metaObject->ConvertToType<IValueMetaObjectRegisterData>();
	wxASSERT(metaObjectValue);

	//Список измерений 
	const wxTreeItemId& hDimentions = AppendGroupItem(hParentID, g_metaDimensionCLSID, objectDimensionsName);
	for (auto metaDimension : metaObjectValue->GetDimentionArrayObject()) {

		if (metaDimension->IsDeleted())
			continue;

		if (metaDimension->GetClassType() == g_metaPredefinedAttributeCLSID)
			continue;

		//const wxString strName = metaDimension->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hDimentions, metaDimension);
	}

	//Список ресурсов 
	const wxTreeItemId& hResources = AppendGroupItem(hParentID, g_metaResourceCLSID, objectResourcesName);
	for (auto metaResource : metaObjectValue->GetResourceArrayObject()) {

		if (metaResource->IsDeleted())
			continue;

		if (metaResource->GetClassType() == g_metaPredefinedAttributeCLSID)
			continue;

		//const wxString strName = metaResource->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hResources, metaResource);
	}

	//Список аттрибутов 	
	const wxTreeItemId& hAttributes = AppendGroupItem(hParentID, g_metaAttributeCLSID, objectAttributesName);
	for (auto attribute : metaObjectValue->GetAttributeArrayObject()) {

		if (attribute->IsDeleted())
			continue;

		if (attribute->GetClassType() == g_metaPredefinedAttributeCLSID)
			continue;

		//const wxString strName = attribute->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hAttributes, attribute);
	}

	//Формы
	const wxTreeItemId& hForm = AppendGroupItem(hParentID, g_metaFormCLSID, objectFormsName);
	for (auto metaForm : metaObjectValue->GetFormArrayObject()) {

		if (metaForm->IsDeleted())
			continue;

		//const wxString strName = metaForm->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hForm, metaForm);
	}

	//Таблицы
	const wxTreeItemId& hTemplates = AppendGroupItem(hParentID, g_metaTemplateCLSID, objectTemplatesName);
	for (auto metaTemplate : metaObjectValue->GetTemplateArrayObject()) {

		if (metaTemplate->IsDeleted())
			continue;

		//const wxString strName = metaTemplate->GetName();

		//if (!m_strSearch.IsEmpty()
		//	&& strName.Find(m_strSearch) < 0)
		//	continue;

		AppendItem(hTemplates, metaTemplate);
	}
}

#include "frontend/artProvider/artProvider.h"

void CMetadataTree::InitTree()
{
	wxImageList* imageList = m_metaTreeCtrl->GetImageList();
	wxASSERT(imageList);

	m_treeMETADATA = AppendRootItem(g_metaCommonMetadataCLSID, _("configuration"));

	//*****************************************************************************************************
	//*                                      Common objects                                               *
	//*****************************************************************************************************

	const int imageCommonIndex = imageList->Add(wxArtProvider::GetBitmap(wxART_COMMON_FOLDER, wxART_METATREE));
	m_treeCOMMON = m_metaTreeCtrl->AppendItem(m_treeMETADATA, commonName, imageCommonIndex, imageCommonIndex);

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	m_treeMODULES = AppendGroupItem(m_treeCOMMON, g_metaCommonModuleCLSID, commonModulesName);
	m_treeFORMS = AppendGroupItem(m_treeCOMMON, g_metaCommonFormCLSID, commonFormsName);
	m_treeTEMPLATES = AppendGroupItem(m_treeCOMMON, g_metaCommonTemplateCLSID, commonTemplatesName);

	m_treePICTURES = AppendGroupItem(m_treeCOMMON, g_metaPictureCLSID, picturesName);

	m_treeINTERFACES = AppendGroupItem(m_treeCOMMON, g_metaInterfaceCLSID, interfacesName);
	m_treeROLES = AppendGroupItem(m_treeCOMMON, g_metaRoleCLSID, rolesName);

	m_treeLANGUAGES = AppendGroupItem(m_treeCOMMON, g_metaLanguageCLSID, languagesName);

	//*****************************************************************************************************
	//*                                      Custom objects                                               *
	//*****************************************************************************************************

	m_treeCONSTANTS = AppendGroupItem(m_treeMETADATA, g_metaConstantCLSID, constantsName);
	m_treeCATALOGS = AppendGroupItem(m_treeMETADATA, g_metaCatalogCLSID, catalogsName);
	m_treeDOCUMENTS = AppendGroupItem(m_treeMETADATA, g_metaDocumentCLSID, documentsName);
	m_treeENUMERATIONS = AppendGroupItem(m_treeMETADATA, g_metaEnumerationCLSID, enumerationsName);
	m_treeDATAPROCESSORS = AppendGroupItem(m_treeMETADATA, g_metaDataProcessorCLSID, dataProcessorName);
	m_treeREPORTS = AppendGroupItem(m_treeMETADATA, g_metaReportCLSID, reportsName);

	m_treeINFORMATION_REGISTERS = AppendGroupItem(m_treeMETADATA, g_metaInformationRegisterCLSID, informationRegisterName);
	m_treeACCUMULATION_REGISTERS = AppendGroupItem(m_treeMETADATA, g_metaAccumulationRegisterCLSID, accumulationRegisterName);

	//Set item bold and name
	m_metaTreeCtrl->SetItemText(m_treeMETADATA, _("configuration"));
	m_metaTreeCtrl->SetItemBold(m_treeMETADATA);
}

void CMetadataTree::ActivateTree()
{
	if (m_metaData != nullptr)
		objectInspector->SelectObject(GetMetaObject(m_metaTreeCtrl->GetSelection()));
}

void CMetadataTree::ClearTree()
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

	//*****************************************************************************************************
	//*                                      Common objects                                               *
	//*****************************************************************************************************

	if (m_treeMODULES.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeMODULES);
	if (m_treeFORMS.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeFORMS);
	if (m_treeTEMPLATES.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeTEMPLATES);

	if (m_treeINTERFACES.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeINTERFACES);
	if (m_treeROLES.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeROLES);
	if (m_treePICTURES.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treePICTURES);

	if (m_treeCONSTANTS.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeCONSTANTS);

	//*****************************************************************************************************
	//*                                      Custom objects                                               *
	//*****************************************************************************************************

	if (m_treeCATALOGS.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeCATALOGS);
	if (m_treeDOCUMENTS.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeDOCUMENTS);
	if (m_treeENUMERATIONS.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeENUMERATIONS);
	if (m_treeDATAPROCESSORS.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeDATAPROCESSORS);
	if (m_treeREPORTS.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeREPORTS);
	if (m_treeINFORMATION_REGISTERS.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeINFORMATION_REGISTERS);
	if (m_treeACCUMULATION_REGISTERS.IsOk())
		m_metaTreeCtrl->DeleteChildren(m_treeACCUMULATION_REGISTERS);

	//delete all items
	m_metaTreeCtrl->DeleteAllItems();

	//initialize tree
	InitTree();

	//enable event 
	m_metaTreeCtrl->SetEvtHandlerEnabled(true);
}

void CMetadataTree::FillData()
{
	IValueMetaObject* commonMetadata = m_metaData->GetCommonMetaObject();
	wxASSERT(commonMetadata);

	m_metaTreeCtrl->SetItemText(m_treeMETADATA, m_metaData->GetConfigName());
	m_metaTreeCtrl->SetItemData(m_treeMETADATA, new wxTreeItemMetaData(commonMetadata));

	//****************************************************************
	//*                          CommonModules                       *
	//****************************************************************
	for (auto commonModule : m_metaData->GetAnyArrayObject(g_metaCommonModuleCLSID)) {

		if (commonModule->IsDeleted())
			continue;

		const wxString& strName = commonModule->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AppendItem(m_treeMODULES, commonModule);
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeMODULES))
		m_metaTreeCtrl->Delete(m_treeMODULES);

	//****************************************************************
	//*                          CommonForms                         *
	//****************************************************************
	for (auto commonForm : m_metaData->GetAnyArrayObject(g_metaCommonFormCLSID)) {

		if (commonForm->IsDeleted())
			continue;

		const wxString& strName = commonForm->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AppendItem(m_treeFORMS, commonForm);
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeFORMS))
		m_metaTreeCtrl->Delete(m_treeFORMS);

	//****************************************************************
	//*                          CommonMakets                        *
	//****************************************************************
	for (auto commonTemlate : m_metaData->GetAnyArrayObject(g_metaCommonTemplateCLSID)) {

		if (commonTemlate->IsDeleted())
			continue;

		const wxString& strName = commonTemlate->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AppendItem(m_treeTEMPLATES, commonTemlate);
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeTEMPLATES))
		m_metaTreeCtrl->Delete(m_treeTEMPLATES);

	//****************************************************************
	//*                          Pictures							 *
	//****************************************************************
	for (auto picture : m_metaData->GetAnyArrayObject(g_metaPictureCLSID)) {

		if (picture->IsDeleted())
			continue;

		const wxString& strName = picture->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AppendItem(m_treePICTURES, picture);
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treePICTURES))
		m_metaTreeCtrl->Delete(m_treePICTURES);

	//****************************************************************
	//*                          Interfaces							 *
	//****************************************************************

	for (auto commonInterface : m_metaData->GetAnyArrayObject(g_metaInterfaceCLSID)) {

		if (commonInterface->IsDeleted())
			continue;

		const wxString& strName = commonInterface->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AddInterfaceItem(commonInterface,
			AppendGroupItem(m_treeINTERFACES, g_metaInterfaceCLSID, commonInterface));
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeINTERFACES))
		m_metaTreeCtrl->Delete(m_treeINTERFACES);

	//****************************************************************
	//*                          Roles								 *
	//****************************************************************
	for (auto role : m_metaData->GetAnyArrayObject(g_metaRoleCLSID)) {

		if (role->IsDeleted())
			continue;

		const wxString& strName = role->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AppendItem(m_treeROLES, role);
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeROLES))
		m_metaTreeCtrl->Delete(m_treeROLES);

	//****************************************************************
	//*                          Languages							 *
	//****************************************************************
	for (auto language : m_metaData->GetAnyArrayObject(g_metaLanguageCLSID)) {

		if (language->IsDeleted())
			continue;

		const wxString& strName = language->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AppendItem(m_treeLANGUAGES, language);
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeLANGUAGES))
		m_metaTreeCtrl->Delete(m_treeLANGUAGES);

	//****************************************************************
	//*                          Constants                           *
	//****************************************************************
	for (auto constant : m_metaData->GetAnyArrayObject(g_metaConstantCLSID)) {

		if (constant->IsDeleted())
			continue;

		const wxString& strName = constant->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AppendItem(m_treeCONSTANTS, constant);
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeCONSTANTS))
		m_metaTreeCtrl->Delete(m_treeCONSTANTS);

	//****************************************************************
	//*                        Catalogs                              *
	//****************************************************************
	for (auto catalog : m_metaData->GetAnyArrayObject(g_metaCatalogCLSID)) {

		if (catalog->IsDeleted())
			continue;

		const wxString& strName = catalog->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AddCatalogItem(catalog,
			AppendItem(m_treeCATALOGS, catalog));
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeCATALOGS))
		m_metaTreeCtrl->Delete(m_treeCATALOGS);

	//****************************************************************
	//*                        Documents                             *
	//****************************************************************
	for (auto document : m_metaData->GetAnyArrayObject(g_metaDocumentCLSID)) {

		if (document->IsDeleted())
			continue;

		const wxString& strName = document->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AddDocumentItem(document,
			AppendItem(m_treeDOCUMENTS, document));
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeDOCUMENTS))
		m_metaTreeCtrl->Delete(m_treeDOCUMENTS);

	//****************************************************************
	//*                          Enumerations                        *
	//****************************************************************
	for (auto enumeration : m_metaData->GetAnyArrayObject(g_metaEnumerationCLSID)) {

		if (enumeration->IsDeleted())
			continue;

		const wxString& strName = enumeration->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AddEnumerationItem(enumeration,
			AppendItem(m_treeENUMERATIONS, enumeration));
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeENUMERATIONS))
		m_metaTreeCtrl->Delete(m_treeENUMERATIONS);

	//****************************************************************
	//*                          Data processor                      *
	//****************************************************************
	for (auto dataProcessor : m_metaData->GetAnyArrayObject(g_metaDataProcessorCLSID)) {

		if (dataProcessor->IsDeleted())
			continue;

		const wxString& strName = dataProcessor->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AddDataProcessorItem(dataProcessor,
			AppendItem(m_treeDATAPROCESSORS, dataProcessor));
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeDATAPROCESSORS))
		m_metaTreeCtrl->Delete(m_treeDATAPROCESSORS);

	//****************************************************************
	//*                          Report			                     *
	//****************************************************************
	for (auto report : m_metaData->GetAnyArrayObject(g_metaReportCLSID)) {

		if (report->IsDeleted())
			continue;

		const wxString& strName = report->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AddReportItem(report,
			AppendItem(m_treeREPORTS, report));
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeREPORTS))
		m_metaTreeCtrl->Delete(m_treeREPORTS);

	//****************************************************************
	//*                          Information register			     *
	//****************************************************************
	for (auto informationRegister : m_metaData->GetAnyArrayObject(g_metaInformationRegisterCLSID)) {

		if (informationRegister->IsDeleted())
			continue;

		const wxString& strName = informationRegister->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AddInformationRegisterItem(informationRegister,
			AppendItem(m_treeINFORMATION_REGISTERS, informationRegister));
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeINFORMATION_REGISTERS))
		m_metaTreeCtrl->Delete(m_treeINFORMATION_REGISTERS);

	//****************************************************************
	//*                          Accumulation register			     *
	//****************************************************************
	for (auto accumulationRegister : m_metaData->GetAnyArrayObject(g_metaAccumulationRegisterCLSID)) {

		if (accumulationRegister->IsDeleted())
			continue;

		const wxString& strName = accumulationRegister->GetName();

		if (!m_strSearch.IsEmpty()
			&& strName.Find(m_strSearch) < 0)
			continue;

		AddAccumulationRegisterItem(accumulationRegister,
			AppendItem(m_treeACCUMULATION_REGISTERS, accumulationRegister));
	}

	if (!m_strSearch.IsEmpty() && !m_metaTreeCtrl->HasChildren(m_treeACCUMULATION_REGISTERS))
		m_metaTreeCtrl->Delete(m_treeACCUMULATION_REGISTERS);

	//set modify 
	Modify(m_metaData->IsModified());

	//update toolbar 
	UpdateToolbar(commonMetadata, m_treeMETADATA);
}

bool CMetadataTree::Load(IMetaDataConfiguration* metaData)
{
	m_metaTreeCtrl->Freeze();
	ClearTree();
	m_metaData = metaData ? metaData : IMetaDataConfiguration::Get();
	FillData(); //Fill all data from metaData
	m_metaData->SetMetaTree(this);
	m_metaTreeCtrl->SelectItem(m_treeMETADATA);
	m_metaTreeCtrl->Expand(m_treeMETADATA);
	m_metaTreeCtrl->Expand(m_treeCOMMON);
	m_metaTreeCtrl->Thaw();
	return true;
}

bool CMetadataTree::Save()
{
	wxASSERT(m_metaData);

	if (m_metaData->IsModified() && wxMessageBox("Configuration '" + m_metaData->GetConfigName() + "' has been changed. Save?", wxT("Save project"), wxYES_NO | wxCENTRE | wxICON_QUESTION, this) == wxYES)
		return m_metaData->SaveDatabase();

	return false;
}

/////////////////////////////////////////////////////////////

void CMetadataTree::Search(const wxString& strSearch)
{
	m_metaTreeCtrl->Freeze();

	//InitTree();
	ClearTree();

	m_strSearch = strSearch;

	FillData(); //Fill all data from metaData

	m_metaTreeCtrl->SelectItem(m_treeMETADATA);
	m_metaTreeCtrl->Expand(m_treeMETADATA);
	m_metaTreeCtrl->Expand(m_treeCOMMON);

	if (!m_strSearch.IsEmpty())
		m_metaTreeCtrl->ExpandAll();

	m_strSearch = wxEmptyString;

	m_metaTreeCtrl->Thaw();
}

/////////////////////////////////////////////////////////////