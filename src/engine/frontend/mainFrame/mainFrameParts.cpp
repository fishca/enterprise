////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxwidgets community
//	Description : main frame window
////////////////////////////////////////////////////////////////////////////

#include "mainFrame.h"
#include "frontend/mainFrame/objinspect/objinspect.h"

void CDocMDIFrame::CreatePropertyPane()
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

bool CDocMDIFrame::IsShownProperty()
{
	const wxAuiPaneInfo propertyPane = m_mgr.GetPane(wxAUI_PANE_PROPERTY);
	if (!propertyPane.IsOk()) return false;
	return propertyPane.IsShown();
}

void CDocMDIFrame::ShowProperty()
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

void CDocMDIFrame::ActivateView(CMetaView* view, bool activate) {

	if (m_docToolbar != nullptr) {

		wxAuiPaneInfo& infoToolBar = m_mgr.GetPane(m_docToolbar);

		if (activate) {

			m_docToolbar->Freeze();
			m_docToolbar->Clear();
			view->OnCreateToolbar(m_docToolbar);
			m_docToolbar->Realize();

#if wxUSE_MENUS	
			wxFrame* viewFrame = dynamic_cast<wxFrame*>(view->GetFrame());
			if (viewFrame != nullptr)
				viewFrame->SetMenuBar(view->CreateMenuBar());
			else
				SetChildMenuBar(nullptr);
#endif
			m_docToolbar->Thaw();
		}
		else {

			unsigned int view_count = 0;
			for (auto& doc : m_docManager->GetDocumentsVector()) {
				for (auto& view : doc->GetViewsVector()) view_count++;
			}

			if (view_count <= 1) {

				m_docToolbar->Freeze();

#if wxUSE_MENUS		
				wxFrame* viewFrame = dynamic_cast<wxFrame*>(view->GetFrame());
				if (viewFrame != nullptr)
					viewFrame->SetMenuBar(nullptr);
#endif
				m_docToolbar->Clear();
				m_docToolbar->Realize();

				m_docToolbar->Thaw();
			}
		}

		infoToolBar.Show(m_docToolbar->GetToolCount() > 0);

		infoToolBar.BestSize(m_docToolbar->GetSize());
		infoToolBar.FloatingSize(
			m_docToolbar->GetSize().x,
			m_docToolbar->GetSize().y + 25
		);

		m_mgr.Update();
	}
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#include "frontend/win/dlgs/authorization.h"
#include "frontend/visualView/ctrl/form.h"

#include "backend/appData.h"

bool CDocMDIFrame::AuthenticationUser(const wxString& userName, const wxString& userPassword) const
{
	if (appData == nullptr)
		return false;

	CDialogAuthentication dlg;

	dlg.SetLogin(userName);
	dlg.SetPassword(userPassword);

	return dlg.ShowModal() != wxID_CANCEL;
}

IMetaData* CDocMDIFrame::FindMetadataByPath(const wxString& strFileName) const
{
	IMetaDataDocument* const foundedDoc = dynamic_cast<IMetaDataDocument*>(docManager->FindDocumentByPath(strFileName));
	if (foundedDoc != nullptr)
		return foundedDoc->GetMetaData();
	return nullptr;
}

#pragma region _frontend_call_h__

// Form support
IBackendValueForm* CDocMDIFrame::ActiveWindow() const {

	if (CDocMDIFrame::GetFrame() != nullptr) {
		wxDocChildFrameAnyBase* activeChild =
			dynamic_cast<wxDocChildFrameAnyBase*>(CDocMDIFrame::GetActiveChild());
		if (activeChild != nullptr) {
			CVisualDocument* const ownerFormDoc = dynamic_cast<CVisualDocument*>(activeChild->GetDocument());
			if (ownerFormDoc != nullptr) {
				return ownerFormDoc->GetValueForm();
			}
		}
	}
	
	return nullptr;
}

IBackendValueForm* CDocMDIFrame::CreateNewForm(const IValueMetaObjectForm* creator, IBackendControlFrame* backendControl, ISourceDataObject* srcObject, const CUniqueKey& formGuid)
{
	IControlFrame* ownerControl = dynamic_cast<IControlFrame*>(backendControl);
	wxASSERT(!(backendControl == nullptr && ownerControl != nullptr));
	return CValue::CreateAndPrepareValueRef<CValueForm>(creator, ownerControl, srcObject, formGuid);
}

CUniqueKey CDocMDIFrame::CreateFormUniqueKey(const IBackendControlFrame* ownerControl, const ISourceDataObject* sourceObject, const CUniqueKey& formGuid)
{
	return CValueForm::CreateFormUniqueKey(ownerControl, sourceObject, formGuid);
}

IBackendValueForm* CDocMDIFrame::FindFormByUniqueKey(const IBackendControlFrame* ownerControl, const ISourceDataObject* sourceObject, const CUniqueKey& formGuid)
{
	return CValueForm::FindFormByUniqueKey(ownerControl, sourceObject, formGuid);
}

IBackendValueForm* CDocMDIFrame::FindFormByUniqueKey(const CUniqueKey& guid)
{
	return CValueForm::FindFormByUniqueKey(guid);
}

IBackendValueForm* CDocMDIFrame::FindFormByControlUniqueKey(const CUniqueKey& guid)
{
	return CValueForm::FindFormByControlUniqueKey(guid);
}

IBackendValueForm* CDocMDIFrame::FindFormBySourceUniqueKey(const CUniqueKey& guid)
{
	return CValueForm::FindFormBySourceUniqueKey(guid);
}

bool CDocMDIFrame::UpdateFormUniqueKey(const CUniquePairKey& guid)
{
	return CValueForm::UpdateFormUniqueKey(guid);
}

#include "frontend/docView/templates/docViewSpreadsheet.h"

// Grid support
bool CDocMDIFrame::ShowSpreadSheetDocument(const wxString& strTitle, wxObjectDataPtr<CBackendSpreadsheetObject>& spreadSheetDocument)
{
	class CSpreadsheetMemoryDocument :
		public CSpreadsheetFileDocument {
	public:
		
		CSpreadsheetMemoryDocument(const wxString& strTitle, const wxObjectDataPtr<CBackendSpreadsheetObject>& spreadSheetDocument) :
			CSpreadsheetFileDocument(spreadSheetDocument)
		{
			CSpreadsheetFileDocument::SetTitle(strTitle);
			CSpreadsheetFileDocument::SetDocumentTemplate(docManager->GetTemplateByGuid(doc::s_guidSpreadsheet));
		}

		virtual bool OnCreate(const wxString& path, long flags) override {
			
			if (!CMetaDocument::OnCreate(path, flags))
				return false;

			return GetGridCtrl()->LoadDocument(m_spreadSheetDocument);
		}

	private:
		virtual CMetaView* DoCreateView() { return new CSpreadsheetEditView; }
	};

	CSpreadsheetMemoryDocument* createdDoc = 
		new CSpreadsheetMemoryDocument(strTitle, spreadSheetDocument);
	
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

bool CDocMDIFrame::PrintSpreadSheetDocument(const wxObjectDataPtr<CBackendSpreadsheetObject>& doc, bool showPrintDlg)
{
	wxScopedPtr<CGridEditorPrintout> printout(new CGridEditorPrintout(doc));

	const wxPageSetupDialogData& pageSetupDialogData =
		docManager->GetPageSetupDialogData();

	wxPrintDialogData printDialogData(pageSetupDialogData.GetPrintData());
	wxPrinter printer(&printDialogData);

	return printer.Print(this, printout.get(), true);
}

#pragma endregion 

IPropertyObject* CDocMDIFrame::GetProperty() const
{
	if (m_objectInspector != nullptr)
		return m_objectInspector->GetSelectedObject();

	return nullptr;
}

bool CDocMDIFrame::SetProperty(IPropertyObject* prop)
{
	if (m_objectInspector != nullptr) {
		m_objectInspector->SelectObject(prop);
		return true;
	}

	return false;
}
