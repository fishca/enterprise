#include "interfaceEditor.h"

#define ICON_SIZE 16

CInterfaceEditor::CInterfaceEditor(wxWindow* parent,
	wxWindowID winid, IMetaObject* metaObject) :
	wxWindow(parent, winid, wxDefaultPosition, wxDefaultSize), m_metaInterface(metaObject)
{
	m_interfaceCtrl = new wxCheckTree(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_HAS_BUTTONS | wxTR_ROW_LINES | wxTR_SINGLE | wxCR_EMPTY_CHECK | wxTR_TWIST_BUTTONS);
	m_interfaceCtrl->SetDoubleBuffered(true);
	m_interfaceCtrl->Bind(wxEVT_CHECKTREE_CHOICE, &CInterfaceEditor::OnCheckItem, this);

	//set image list
	m_interfaceCtrl->SetImageList(
		new wxImageList(ICON_SIZE, ICON_SIZE)
	);

	InitInterface();

	wxBoxSizer* mainSizer = new wxBoxSizer(wxVERTICAL);
	mainSizer->Add(m_interfaceCtrl, 1, wxEXPAND);
		
	m_interfaceCtrl->SelectItem(m_treeMETADATA);

	wxWindow::SetSizer(mainSizer);
	wxWindow::Layout();
}

void CInterfaceEditor::OnCheckItem(wxTreeEvent& event)
{
	wxTreeItemMetaData* data = dynamic_cast<wxTreeItemMetaData*>(
		m_interfaceCtrl->GetItemData(event.GetItem())
		);
	if (data != nullptr) {
		IInterfaceObject* metaObject = data->GetMetaObject();
		wxASSERT(metaObject);
		metaObject->SetInterface(m_metaInterface->GetMetaID(), event.GetExtraLong());
	}

	event.Skip();
}

#include "frontend/artProvider/artProvider.h"

#define commonName _("common")
#define commonFormsName _("common forms")

#define constantsName _("constants")

#define catalogsName _("catalogs")
#define documentsName _("documents")
#define dataProcessorName _("data processors")
#define reportsName _("reports")
#define informationRegisterName _("information Registers")
#define accumulationRegisterName _("accumulation Registers")

void CInterfaceEditor::InitInterface()
{
	const IAbstractTypeCtor* typeCtor = CValue::GetAvailableCtor(g_metaCommonMetadataCLSID);
	wxASSERT(typeCtor);

	wxImageList* imageList = m_interfaceCtrl->GetImageList();
	int imageIndex = imageList->Add(typeCtor->GetClassIcon());
	m_treeMETADATA = m_interfaceCtrl->AddRoot(_("configuration"), imageIndex, imageIndex, new wxTreeItemMetaData(commonMetaData->GetCommonMetaObject()));

	//*****************************************************************************************************
	//*                                      Common objects                                               *
	//*****************************************************************************************************

	int imageCommonIndex = imageList->Add(wxArtProvider::GetIcon(wxART_COMMON_FOLDER, wxART_METATREE));
	m_treeCOMMON = m_interfaceCtrl->AppendItem(m_treeMETADATA, commonName, imageCommonIndex, imageCommonIndex);

	///////////////////////////////////////////////////////////////////////////////////////////////////////

	m_treeFORMS = AppendGroupItem(m_treeCOMMON, g_metaCommonFormCLSID, commonFormsName);

	//*****************************************************************************************************
	//*                                      Custom objects                                               *
	//*****************************************************************************************************

	m_treeCONSTANTS = AppendGroupItem(m_treeMETADATA, g_metaConstantCLSID, constantsName);
	m_treeCATALOGS = AppendGroupItem(m_treeMETADATA, g_metaCatalogCLSID, catalogsName);
	m_treeDOCUMENTS = AppendGroupItem(m_treeMETADATA, g_metaDocumentCLSID, documentsName);

	m_treeDATAPROCESSORS = AppendGroupItem(m_treeMETADATA, g_metaDataProcessorCLSID, dataProcessorName);
	m_treeREPORTS = AppendGroupItem(m_treeMETADATA, g_metaReportCLSID, reportsName);

	m_treeINFORMATION_REGISTERS = AppendGroupItem(m_treeMETADATA, g_metaInformationRegisterCLSID, informationRegisterName);
	m_treeACCUMULATION_REGISTERS = AppendGroupItem(m_treeMETADATA, g_metaAccumulationRegisterCLSID, accumulationRegisterName);

	//Set item bold and name
	m_interfaceCtrl->SetItemText(m_treeMETADATA, _("configuration"));
	m_interfaceCtrl->SetItemBold(m_treeMETADATA);

	m_interfaceCtrl->ExpandAll();
}

void CInterfaceEditor::ClearInterface() {

	//*****************************************************************************************************
	//*                                      Common objects                                               *
	//*****************************************************************************************************

	if (m_treeFORMS.IsOk()) m_interfaceCtrl->DeleteChildren(m_treeFORMS);

	if (m_treeCONSTANTS.IsOk()) m_interfaceCtrl->DeleteChildren(m_treeCONSTANTS);

	//*****************************************************************************************************
	//*                                      Custom objects                                               *
	//*****************************************************************************************************

	if (m_treeCATALOGS.IsOk()) m_interfaceCtrl->DeleteChildren(m_treeCATALOGS);
	if (m_treeDOCUMENTS.IsOk()) m_interfaceCtrl->DeleteChildren(m_treeDOCUMENTS);

	if (m_treeDATAPROCESSORS.IsOk()) m_interfaceCtrl->DeleteChildren(m_treeDATAPROCESSORS);
	if (m_treeREPORTS.IsOk()) m_interfaceCtrl->DeleteChildren(m_treeREPORTS);
	if (m_treeINFORMATION_REGISTERS.IsOk()) m_interfaceCtrl->DeleteChildren(m_treeINFORMATION_REGISTERS);
	if (m_treeACCUMULATION_REGISTERS.IsOk()) m_interfaceCtrl->DeleteChildren(m_treeACCUMULATION_REGISTERS);

	//delete all items
	m_interfaceCtrl->DeleteAllItems();

	//Initialize tree
	InitInterface();
}

void CInterfaceEditor::FillData()
{
	IMetaData* metaData = m_metaInterface->GetMetaData();
	wxASSERT(metaData);
	IMetaObject* commonObject = commonMetaData->GetCommonMetaObject();
	wxASSERT(commonObject);

	m_interfaceCtrl->SetItemText(m_treeMETADATA, commonObject->GetName());

	//****************************************************************
	//*                          CommonForms                         *
	//****************************************************************
	for (auto commonForm : commonMetaData->GetMetaObject(g_metaCommonFormCLSID)) {
		if (commonForm->IsDeleted())
			continue;
		AppendItem(m_treeFORMS, commonForm);
	}

	//****************************************************************
	//*                          Constants                           *
	//****************************************************************
	for (auto constant : commonMetaData->GetMetaObject(g_metaConstantCLSID)) {
		if (constant->IsDeleted())
			continue;
		AppendItem(m_treeCONSTANTS, constant);
	}

	//****************************************************************
	//*                        Catalogs                              *
	//****************************************************************
	for (auto catalog : commonMetaData->GetMetaObject(g_metaCatalogCLSID)) {
		if (catalog->IsDeleted())
			continue;
		AppendItem(m_treeCATALOGS, catalog);
	}

	//****************************************************************
	//*                        Documents                             *
	//****************************************************************
	for (auto document : commonMetaData->GetMetaObject(g_metaDocumentCLSID)) {
		if (document->IsDeleted())
			continue;
		AppendItem(m_treeDOCUMENTS, document);
	}

	//****************************************************************
	//*                          Data processor                      *
	//****************************************************************
	for (auto dataProcessor : commonMetaData->GetMetaObject(g_metaDataProcessorCLSID)) {
		if (dataProcessor->IsDeleted())
			continue;
		AppendItem(m_treeDATAPROCESSORS, dataProcessor);
	}

	//****************************************************************
	//*                          Report			                     *
	//****************************************************************
	for (auto report : commonMetaData->GetMetaObject(g_metaReportCLSID)) {
		if (report->IsDeleted())
			continue;
		AppendItem(m_treeREPORTS, report);
	}

	//****************************************************************
	//*                          Information register			     *
	//****************************************************************
	for (auto informationRegister : commonMetaData->GetMetaObject(g_metaInformationRegisterCLSID)) {
		if (informationRegister->IsDeleted())
			continue;
		AppendItem(m_treeINFORMATION_REGISTERS, informationRegister);
	}

	//****************************************************************
	//*                          Accumulation register			     *
	//****************************************************************
	for (auto accumulationRegister : commonMetaData->GetMetaObject(g_metaAccumulationRegisterCLSID)) {
		if (accumulationRegister->IsDeleted())
			continue;
		AppendItem(m_treeACCUMULATION_REGISTERS, accumulationRegister);
	}

	m_interfaceCtrl->Enable(m_metaInterface->IsEnabled());
}
