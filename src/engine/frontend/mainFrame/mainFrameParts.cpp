////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxwidgets community
//	Description : main frame window
////////////////////////////////////////////////////////////////////////////

#include "mainFrame.h"
#include "frontend/mainFrame/objinspect/objinspect.h"

void CFrontendDocMDIFrame::CreatePropertyPane()
{
	if (m_mgr.GetPane(wxAUI_PANE_PROPERTY).IsOk())
		return;

	m_objectInspector = new CObjectInspector(this, wxID_ANY);

	wxAuiPaneInfo paneInfo;
	paneInfo.Name(wxAUI_PANE_PROPERTY);
	paneInfo.CloseButton(true);
	paneInfo.MinimizeButton(false);
	paneInfo.MaximizeButton(false);
	paneInfo.DestroyOnClose(false);
	paneInfo.Caption(_("Properties"));
	paneInfo.MinSize(300, 0);
	paneInfo.Right();
	paneInfo.Show(false);

	m_mgr.AddPane(m_objectInspector, paneInfo);
}

////////////////////////////////////////////////////////////////////////////////////////////////////

bool CFrontendDocMDIFrame::IsShownInspector()
{
	const wxAuiPaneInfo propertyPane = m_mgr.GetPane(wxAUI_PANE_PROPERTY);
	if (!propertyPane.IsOk()) return false;
	return propertyPane.IsShown();
}

void CFrontendDocMDIFrame::ShowInspector()
{
	wxAuiPaneInfo& propertyPane = m_mgr.GetPane(wxAUI_PANE_PROPERTY);
	if (!propertyPane.IsOk())
		return;
	if (!propertyPane.IsShown()) {
		propertyPane.Show();
		m_objectInspector->SetFocus();
		m_objectInspector->Raise();
		m_mgr.Update();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#include "frontend/docView/docManager.h"

void CFrontendDocMDIFrame::ActivateView(CMetaView* view, bool activate) {

	if (m_docToolbar != nullptr) {

		m_docToolbar->Freeze();

		if (activate) {

			wxFrame* viewFrame = dynamic_cast<wxFrame*>(view->GetFrame());
#if wxUSE_MENUS	
			if (viewFrame != nullptr) viewFrame->SetMenuBar(view->CreateMenuBar());
#endif
			if (viewFrame != nullptr) {
				m_docToolbar->Clear();
				view->OnCreateToolbar(m_docToolbar);
				m_docToolbar->Realize();
			}
		}
		else {

#if wxUSE_MENUS		

			wxFrame* viewFrame = dynamic_cast<wxFrame*>(view->GetFrame());

			if (viewFrame != nullptr) {

				class CProcSubMenu {

					static void SetChildEnable(wxMenu* dst, bool enable = false) {

						for (const auto it : dst->GetMenuItems()) {
							if (!it->IsSubMenu()) it->Enable(enable);
							if (it->IsSubMenu()) SetChildEnable(it->GetSubMenu(), enable);
						}
					}

				public:

					static void SetMenuEnabled(wxMenuBar* menuBar, bool enable = false) {
						for (size_t idx = 0; idx < menuBar->GetMenuCount(); idx++) {
							CProcSubMenu::SetChildEnable(menuBar->GetMenu(idx), enable);
						}
					}
				};

				wxMenuBar* menuBar = view->CreateMenuBar();
				if (menuBar != nullptr)
					CProcSubMenu::SetMenuEnabled(menuBar);
				
				viewFrame->SetMenuBar(menuBar);
			}
#endif
			for (size_t idx = 0; idx < m_docToolbar->GetToolCount(); idx++) {
				m_docToolbar->EnableTool(m_docToolbar->FindToolByIndex(idx)->GetId(), false);
			}
		}

		m_docToolbar->Thaw();

		// update frame manager 
		UpdateManager();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#include "frontend/win/dlgs/authorization.h"
#include "backend/appData.h"

bool CFrontendDocMDIFrame::AuthenticationUser(const wxString& userName, const wxString& userPassword) const
{
	if (appData == nullptr)
		return false;

	CDialogAuthentication dlg;

	dlg.SetLogin(userName);
	dlg.SetPassword(userPassword);

	return dlg.ShowModal() != wxID_CANCEL;
}

IMetaData* CFrontendDocMDIFrame::FindMetadataByPath(const wxString& strFileName) const
{
	IMetaDataDocument* const foundedDoc = dynamic_cast<IMetaDataDocument*>(docManager->FindDocumentByPath(strFileName));
	if (foundedDoc != nullptr)
		return foundedDoc->GetMetaData();
	return nullptr;
}

#pragma region _frontend_call_h__

#include "frontend/visualView/visualHostClient.h"

// Form support
IBackendValueForm* CFrontendDocMDIFrame::ActiveWindow() const {

	if (CFrontendDocMDIFrame::GetFrame() != nullptr) {
		wxDocChildFrameAnyBase* activeChild =
			dynamic_cast<wxDocChildFrameAnyBase*>(CFrontendDocMDIFrame::GetActiveChild());
		if (activeChild != nullptr) {
			CFormVisualDocument* const ownerFormDoc = dynamic_cast<CFormVisualDocument*>(activeChild->GetDocument());
			if (ownerFormDoc != nullptr) {
				return ownerFormDoc->GetValueForm();
			}
		}
	}

	return nullptr;
}

IBackendValueForm* CFrontendDocMDIFrame::CreateNewForm(const IValueMetaObjectForm* creator, IBackendControlFrame* backendControl, ISourceDataObject* srcObject, const CUniqueKey& formGuid)
{
	IControlFrame* ownerControl = dynamic_cast<IControlFrame*>(backendControl);
	wxASSERT(!(backendControl == nullptr && ownerControl != nullptr));
	return CValue::CreateAndPrepareValueRef<CValueForm>(creator, ownerControl, srcObject, formGuid);
}

CUniqueKey CFrontendDocMDIFrame::CreateFormUniqueKey(const IBackendControlFrame* ownerControl, const ISourceDataObject* sourceObject, const CUniqueKey& formGuid)
{
	return CFormVisualDocument::CreateFormUniqueKey(ownerControl, sourceObject, formGuid);
}

IBackendValueForm* CFrontendDocMDIFrame::FindFormByUniqueKey(const IBackendControlFrame* ownerControl, const ISourceDataObject* sourceObject, const CUniqueKey& formGuid)
{
	return CFormVisualDocument::FindFormByUniqueKey(ownerControl, sourceObject, formGuid);
}

IBackendValueForm* CFrontendDocMDIFrame::FindFormByUniqueKey(const CUniqueKey& guid)
{
	return CFormVisualDocument::FindFormByUniqueKey(guid);
}

IBackendValueForm* CFrontendDocMDIFrame::FindFormByControlUniqueKey(const CUniqueKey& guid)
{
	return CFormVisualDocument::FindFormByControlUniqueKey(guid);
}

IBackendValueForm* CFrontendDocMDIFrame::FindFormBySourceUniqueKey(const CUniqueKey& guid)
{
	return CFormVisualDocument::FindFormBySourceUniqueKey(guid);
}

bool CFrontendDocMDIFrame::UpdateFormUniqueKey(const CUniquePairKey& guid)
{
	return CFormVisualDocument::UpdateFormUniqueKey(guid);
}

#include "frontend/docView/templates/docViewSpreadsheet.h"

// Grid support
bool CFrontendDocMDIFrame::ShowSpreadSheetDocument(const wxString& strTitle, wxObjectDataPtr<CBackendSpreadsheetObject>& spreadSheetDocument)
{
	class CSpreadsheetMemoryDocument :
		public CSpreadsheetFileDocument {
	public:

		CSpreadsheetMemoryDocument(const wxString& strTitle, const wxObjectDataPtr<CBackendSpreadsheetObject>& spreadSheetDocument) :
			CSpreadsheetFileDocument(spreadSheetDocument)
		{
			CSpreadsheetFileDocument::SetTitle(strTitle);
			CSpreadsheetFileDocument::SetFilename(strTitle);
		}

		virtual bool OnCreate(const wxString& path, long flags) override {

			if (!CMetaDocument::OnCreate(path, flags))
				return false;

			return GetGridCtrl()->LoadDocument(m_spreadSheetDocument);
		}
	};

	CSpreadsheetMemoryDocument* createdDoc =
		docManager->CreateDocument<CSpreadsheetMemoryDocument>(strTitle, spreadSheetDocument);

	wxASSERT(createdDoc != nullptr);

	if (createdDoc->OnCreate(wxEmptyString, 0)) {
		createdDoc->SetCommandProcessor(createdDoc->OnCreateCommandProcessor());
		docManager->AddDocument(createdDoc);
		return true;
	}

	// The document may be already destroyed, this happens if its view
	// creation fails as then the view being created is destroyed
	// triggering the destruction of the document as this first view is
	// also the last one. However if OnCreate() fails for any reason other
	// than view creation failure, the document is still alive and we need
	// to clean it up ourselves to avoid having a zombie document.

	createdDoc->DeleteAllViews();
	return false;
}

#include "frontend/win/editor/gridEditor/gridPrintout.h"

bool CFrontendDocMDIFrame::PrintSpreadSheetDocument(const wxObjectDataPtr<CBackendSpreadsheetObject>& doc, bool showPrintDlg)
{
	wxScopedPtr<CGridEditorPrintout> printout(new CGridEditorPrintout(doc));

	const wxPageSetupDialogData& pageSetupDialogData =
		docManager->GetPageSetupDialogData();

	wxPrintDialogData printDialogData(pageSetupDialogData.GetPrintData());
	wxPrinter printer(&printDialogData);

	return printer.Print(this, printout.get(), true);
}

#pragma endregion 

IPropertyObject* CFrontendDocMDIFrame::GetProperty() const
{
	if (m_objectInspector != nullptr)
		return m_objectInspector->GetSelectedObject();

	return nullptr;
}

bool CFrontendDocMDIFrame::SetProperty(IPropertyObject* prop)
{
	if (m_objectInspector != nullptr) {
		m_objectInspector->SelectObject(prop);
		return true;
	}

	return false;
}
