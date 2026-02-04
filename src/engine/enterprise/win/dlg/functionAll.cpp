#include "functionAll.h"
#include "backend/metadataConfiguration.h"
#include "frontend/visualView/ctrl/form.h"

#define ICON_SIZE 16

class CMetaDataItem : public wxTreeItemData {
	IValueMetaObject* m_metaObject;
public:
	CMetaDataItem(IValueMetaObject* metaObject) : m_metaObject(metaObject) {}
	IValueMetaObject* GetMetaObject() const { return m_metaObject; }
};

//////////////////////////////////////////////////////////////////////////

CDialogFunctionAll::CDialogFunctionAll(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style)
	: wxDialog(parent, id, title, pos, size, style)
{
	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	wxBoxSizer* bSizer = new wxBoxSizer(wxVERTICAL);
	m_treeCtrlElements = new wxTreeCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxTR_SINGLE | wxTR_HIDE_ROOT | wxTR_TWIST_BUTTONS);
	m_treeCtrlElements->SetDoubleBuffered(true);
	bSizer->Add(m_treeCtrlElements, 1, wxALL | wxEXPAND, 5);

	// Connect Events
	m_treeCtrlElements->Connect(wxEVT_LEFT_DCLICK, wxMouseEventHandler(CDialogFunctionAll::OnTreeCtrlElementsOnLeftDClick), nullptr, this);

	//set image list
	m_treeCtrlElements->SetImageList(
		new wxImageList(ICON_SIZE, ICON_SIZE)
	);

	wxDialog::SetSizer(bSizer);
	wxDialog::Layout();

	wxDialog::Centre(wxBOTH);

	wxIcon dlg_icon;
	dlg_icon.CopyFromBitmap(CBackendPicture::GetPicture(g_picStructureCLSID));

	wxDialog::SetIcon(dlg_icon);
	wxDialog::SetFocus();
}

wxTreeItemId CDialogFunctionAll::AppendGroupItem(const wxTreeItemId& parent,
	const class_identifier_t& clsid, const wxString& name) const {
	const IAbstractTypeCtor* typeCtor = CValue::GetAvailableCtor(clsid);
	wxASSERT(typeCtor);
	wxImageList* imageList = m_treeCtrlElements->GetImageList();
	wxASSERT(imageList);
	int imageIndex = imageList->Add(typeCtor->GetClassIcon());
	return m_treeCtrlElements->AppendItem(parent, name.IsEmpty() ? typeCtor->GetClassName() : name, imageIndex, imageIndex);
}

void CDialogFunctionAll::BuildTree()
{
	wxImageList* imageList = m_treeCtrlElements->GetImageList();
	wxASSERT(imageList);
	wxTreeItemId root = m_treeCtrlElements->AddRoot(wxEmptyString);
	wxTreeItemId constants = AppendGroupItem(root, g_metaConstantCLSID, _("Constants"));
	for (auto constant : activeMetaData->GetAnyArrayObject(g_metaConstantCLSID)) {
		const int imageIndex = imageList->Add(constant->GetIcon());
		m_treeCtrlElements->AppendItem(constants, constant->GetSynonym(), imageIndex, imageIndex, new CMetaDataItem(constant));
	}
	wxTreeItemId catalogs = AppendGroupItem(root, g_metaCatalogCLSID, _("Catalogs"));
	for (auto catalog : activeMetaData->GetAnyArrayObject(g_metaCatalogCLSID)) {
		const int imageIndex = imageList->Add(catalog->GetIcon());
		m_treeCtrlElements->AppendItem(catalogs, catalog->GetSynonym(), imageIndex, imageIndex, new CMetaDataItem(catalog));
	}
	wxTreeItemId documents = AppendGroupItem(root, g_metaDocumentCLSID, _("Documents"));
	for (auto document : activeMetaData->GetAnyArrayObject(g_metaDocumentCLSID)) {
		const int imageIndex = imageList->Add(document->GetIcon());
		m_treeCtrlElements->AppendItem(documents, document->GetSynonym(), imageIndex, imageIndex, new CMetaDataItem(document));
	}
	wxTreeItemId dataProcessors = AppendGroupItem(root, g_metaDataProcessorCLSID, _("Data processors"));
	for (auto dataProcessor : activeMetaData->GetAnyArrayObject(g_metaDataProcessorCLSID)) {
		const int imageIndex = imageList->Add(dataProcessor->GetIcon());
		m_treeCtrlElements->AppendItem(dataProcessors, dataProcessor->GetSynonym(), imageIndex, imageIndex, new CMetaDataItem(dataProcessor));
	}
	wxTreeItemId reports = AppendGroupItem(root, g_metaReportCLSID, _("Reports"));
	for (auto report : activeMetaData->GetAnyArrayObject(g_metaReportCLSID)) {
		const int imageIndex = imageList->Add(report->GetIcon());
		m_treeCtrlElements->AppendItem(reports, report->GetSynonym(), imageIndex, imageIndex, new CMetaDataItem(report));
	}
	wxTreeItemId informationRegisters = AppendGroupItem(root, g_metaInformationRegisterCLSID, _("Information registers"));
	for (auto informationRegister : activeMetaData->GetAnyArrayObject(g_metaInformationRegisterCLSID)) {
		const int imageIndex = imageList->Add(informationRegister->GetIcon());
		m_treeCtrlElements->AppendItem(informationRegisters, informationRegister->GetSynonym(), imageIndex, imageIndex, new CMetaDataItem(informationRegister));
	}
	wxTreeItemId accumulationRegisters = AppendGroupItem(root, g_metaAccumulationRegisterCLSID, _("Accumulation registers"));
	for (auto accumulationRegister : activeMetaData->GetAnyArrayObject(g_metaAccumulationRegisterCLSID)) {
		const int imageIndex = imageList->Add(accumulationRegister->GetIcon());
		m_treeCtrlElements->AppendItem(accumulationRegisters, accumulationRegister->GetSynonym(), imageIndex, imageIndex, new CMetaDataItem(accumulationRegister));
	}

	m_treeCtrlElements->ExpandAll();
}

void CDialogFunctionAll::OnTreeCtrlElementsOnLeftDClick(wxMouseEvent& event)
{
	const wxTreeItemId& selItem = m_treeCtrlElements->GetSelection();
	if (!selItem.IsOk())
		return;

	CMetaDataItem* itemData = dynamic_cast<CMetaDataItem*>(m_treeCtrlElements->GetItemData(selItem));
	if (itemData != nullptr) {
		IBackendCommandItem* metaObject = dynamic_cast<IBackendCommandItem*>(itemData->GetMetaObject());
		if (metaObject != nullptr && metaObject->ShowFormByCommandType())
			Close(true);
	}

	event.Skip();
}