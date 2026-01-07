////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxwidgets community
//	Description : main frame window
////////////////////////////////////////////////////////////////////////////

#include "mainFrameDesigner.h"

#include "backend/appData.h"
#include "backend/debugger/debugClient.h"

#include "frontend/window_ptr.h"

#include "frontend/mainFrame/settings/settingsdialog.h"
#include "frontend/mainFrame/settings/keybinderdialog.h" 
#include "frontend/mainFrame/settings/fontcolorsettingspanel.h"
#include "frontend/mainFrame/settings/editorsettingspanel.h"

#include "frontend/win/dlgs/applyChange.h"

//********************************************************************************
//*                                 Debug commands                               *
//********************************************************************************

#include "backend/metadataConfiguration.h"

void CDocDesignerMDIFrame::OnStartDebug(wxCommandEvent& WXUNUSED(event))
{
	if (debugClient->HasConnections()) {
		wxMessageBox(_("Debugger is already running!"));
		return;
	}

	if (activeMetaData->IsModified()) {
		if (wxMessageBox(_("Configuration '" + activeMetaData->GetConfigName() + "' has been changed.\nDo you want to save?"), _("Save project"), wxYES_NO | wxCENTRE | wxICON_QUESTION, this) == wxYES) {
			if (!activeMetaData->SaveDatabase(saveConfigFlag)) {
				return;
			}
		}
	}

	appData->RunApplication(wxT("enterprise"));
}

void CDocDesignerMDIFrame::OnStartDebugWithoutDebug(wxCommandEvent& WXUNUSED(event))
{
	if (activeMetaData->IsModified()) {
		if (wxMessageBox(_("Configuration '" + activeMetaData->GetConfigName() + "' has been changed.\nDo you want to save?"), _("Save project"), wxYES_NO | wxCENTRE | wxICON_QUESTION, this) == wxYES) {
			if (!activeMetaData->SaveDatabase(saveConfigFlag)) {
				return;
			}
		}
	}

	appData->RunApplication(wxT("enterprise"), false);
}

#include "win/dlg/debugItem/debugItem.h"

void CDocDesignerMDIFrame::OnAttachForDebugging(wxCommandEvent& WXUNUSED)
{
	CWindowPtr<CDialogDebugItem> dlg(new CDialogDebugItem(this, wxID_ANY));
	dlg->Show();
}

#include "backend/metaData.h"
#include "frontend/mainFrame/objinspect/objinspect.h"

#include "docManager/docManager.h"
#include "docManager/templates/metaFile.h"

void CDocDesignerMDIFrame::OnOpenConfiguration(wxCommandEvent& event)
{
	IMetaDataConfiguration* configDatabase = activeMetaData->GetConfiguration();
	wxASSERT(configDatabase);

	for (auto& doc : docManager->GetDocumentsVector()) {
		CMetadataBrowserDocument* metaDoc = wxDynamicCast(doc, CMetadataBrowserDocument);
		if (metaDoc != nullptr &&
			metaDoc->GetMetaObject() == configDatabase->GetCommonMetaObject()) {
			metaDoc->Activate();
			return;
		}
	}

	CMetadataBrowserDocument* newDocument =
		new CMetadataBrowserDocument(configDatabase);

	wxASSERT(newDocument);
	try {

		m_docManager->AddDocument(newDocument);

		IMetaObject* metaObject = configDatabase->GetCommonMetaObject();

		newDocument->SetTitle(metaObject->GetModuleName());
		newDocument->SetFilename(metaObject->GetDocPath());
		newDocument->SetMetaObject(metaObject);

		if (newDocument->OnCreate(metaObject->GetModuleName(), wxDOC_NEW)) {

			wxCommandProcessor* cmdProc = newDocument->CreateCommandProcessor();

			if (cmdProc != nullptr)
				newDocument->SetCommandProcessor(cmdProc);

			//newDocument->UpdateAllViews();
		}
		else {

			// The document may be already destroyed, this happens if its view
			// creation fails as then the view being created is destroyed
			// triggering the destruction of the document as this first view is
			// also the last one. However if OnCreate() fails for any reason other
			// than view creation failure, the document is still alive and we need
			// to clean it up ourselves to avoid having a zombie document.
			newDocument->DeleteAllViews();
		}
	}
	catch (...) {
		if (m_docManager->GetDocuments().Member(newDocument)) {
			newDocument->DeleteAllViews();
		}
	}
}

void CDocDesignerMDIFrame::OnRollbackConfiguration(wxCommandEvent& event)
{
	objectInspector->SelectObject(nullptr);

	wxAuiMDIClientWindow* client_window = GetClientWindow();
	wxCHECK_RET(client_window, wxS("Missing MDI Client Window"));

	client_window->Freeze();

	bool success = true;
	wxAuiMDIChildFrame* pActiveChild = nullptr;
	while ((pActiveChild = GetActiveChild()) != nullptr) {
		if (!pActiveChild->Close()) {
			// it refused to close, don't close the remaining ones either
			success = false;
			break;
		}
	}

	success = success && activeMetaData->RoolbackDatabase()
		&& m_metaWindow->Load();

	client_window->Thaw();

	if (success) {
		objectInspector->SelectObject(activeMetaData->GetCommonMetaObject());
		wxMessageBox(_("Successfully rolled back to database configuration!"), _("Designer"), wxOK | wxCENTRE, this);
	}
}

void CDocDesignerMDIFrame::OnUpdateConfiguration(wxCommandEvent& event)
{
	bool canSave = true;
	if (debugClient->HasConnections()) {
		if (wxMessageBox(
			_("To update the database configuration you need stop debugging.\nDo you want to continue?"), wxMessageBoxCaptionStr,
			wxYES_NO | wxCENTRE | wxICON_INFORMATION, this) == wxNO) {
			return;
		}
		debugClient->Stop(true);
	}

	for (auto& doc : m_docManager->GetDocumentsVector()) {

		if (!canSave)
			break;

		canSave = doc->OnSaveModified();
	}

	// stage one - save database  
	if (canSave && !activeMetaData->SaveDatabase()) {

		for (unsigned int idx = 0; idx < s_restructureInfo.GetCount(); idx++) {
			if (s_restructureInfo.GetType(idx) == ERestructure::restructure_error)
				outputWindow->OutputError(s_restructureInfo.GetDescription(idx));
		}

		wxMessageBox(_("Failed to save database!"),
			wxMessageBoxCaptionStr, wxOK | wxCENTRE | wxICON_ERROR, this
		);

		return;
	}

	// stage two - update database  
	if (canSave && activeMetaData->OnBeforeSaveDatabase(saveConfigFlag)) {

		bool roolback = false, success = true;

		if (activeMetaData->OnSaveDatabase(saveConfigFlag)) {
			roolback = !CDialogApplyChange::ShowApplyChange(s_restructureInfo, this);
		}
		else {
			success = false;
		}

		success = activeMetaData->OnAfterSaveDatabase(roolback || !success, saveConfigFlag);

		if (!success) {
			wxMessageBox(_("Failed to update database!"),
				wxMessageBoxCaptionStr, wxOK | wxCENTRE | wxICON_ERROR, this
			);
		}
	}
}

void CDocDesignerMDIFrame::OnConfiguration(wxCommandEvent& event)
{
	if (wxID_DESIGNER_CONFIGURATION_LOAD_FROM_FILE == event.GetId())
	{
		wxFileDialog openFileDialog(this, _("Open configuration file"), "", "",
			"Configuration files (*.conf)|*.conf", wxFD_OPEN | wxFD_FILE_MUST_EXIST);

		if (openFileDialog.ShowModal() == wxID_CANCEL)
			return;     // the user changed idea...

		objectInspector->SelectObject(nullptr);
		if (!m_docManager->CloseDocuments())
			return;

		// proceed loading the file chosen by the user;
		if (activeMetaData->LoadFromFile(openFileDialog.GetPath())) {
			if (m_metaWindow->Load()) {
				if (activeMetaData->IsModified()) {
					if (wxMessageBox("Configuration '" + activeMetaData->GetConfigName() + "' has been changed.\nDo you want to save?", _("Save project"), wxYES_NO | wxCENTRE | wxICON_QUESTION, this) == wxYES) {
						OnUpdateConfiguration(event);
					}
				}
				objectInspector->SelectObject(activeMetaData->GetCommonMetaObject());
			}
		}
	}
	else if (wxID_DESIGNER_CONFIGURATION_SAVE_TO_FILE == event.GetId())
	{
		wxFileDialog saveFileDialog(this, _("Save configuration file"), "", "",
			"Configuration files (*.conf)|*.conf", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);
		if (saveFileDialog.ShowModal() == wxID_CANCEL)
			return;     // the user changed idea...

		if (activeMetaData->SaveToFile(saveFileDialog.GetPath())) {
			wxMessageBox(_("Successfully unloaded to: ") + saveFileDialog.GetPath());
		}
	}
}

void CDocDesignerMDIFrame::OnRunDebugCommand(wxCommandEvent& event)
{
	switch (event.GetId())
	{
	case wxID_DESIGNER_DEBUG_STEP_OVER:
		debugClient->StepOver();
		break;
	case wxID_DESIGNER_DEBUG_STEP_INTO:
		debugClient->StepInto();
		break;
	case wxID_DESIGNER_DEBUG_PAUSE:
		debugClient->Pause();
		break;
	case wxID_DESIGNER_DEBUG_STOP_DEBUGGING:
		debugClient->Stop(false);
		break;
	case wxID_DESIGNER_DEBUG_STOP_PROGRAM:
		debugClient->Stop(true);
		break;
	case wxID_DESIGNER_DEBUG_NEXT_POINT:
		debugClient->Continue();
		break;
	case wxID_DESIGNER_DEBUG_REMOVE_ALL_DEBUGPOINTS:
		debugClient->RemoveAllBreakpoint();
		break;
	}
}

//********************************************************************************
//*                                    Tool                                      *
//********************************************************************************

void CDocDesignerMDIFrame::OnToolsSettings(wxCommandEvent& event)
{
	SettingsDialog dialog(this);

	KeyBinderDialog* keyBinder = dialog.GetKeyBinderDialog();

	for (unsigned int i = 0; i < m_keyBinder.GetNumCommands(); ++i)
	{
		keyBinder->AddCommand(m_keyBinder.GetCommand(i));
	}

	FontColorSettingsPanel* fontColorSettings = dialog.GetFontColorSettingsPanel();
	fontColorSettings->SetSettings(m_fontColorSettings);

	EditorSettingsPanel* editorSettings = dialog.GetEditorSettingsPanel();
	editorSettings->SetSettings(m_editorSettings);

	if (dialog.ShowModal() == wxID_OK)
	{
		m_keyBinder.ClearCommands();

		for (unsigned int i = 0; i < keyBinder->GetNumCommands(); ++i) {
			m_keyBinder.AddCommand(keyBinder->GetCommand(i));
		}

		m_keyBinder.UpdateWindow(this);

#if wxUSE_MENUBAR
		if (m_pMyMenuBar != nullptr)
			m_keyBinder.UpdateMenuBar(m_pMyMenuBar);

		m_keyBinder.UpdateMenuBar(GetMenuBar());
#endif

		m_fontColorSettings = fontColorSettings->GetSettings();
		m_editorSettings = editorSettings->GetSettings();

		UpdateEditorOptions();
	}

	SaveOptions();
}

#include "frontend/win/dlgs/userList.h"

void CDocDesignerMDIFrame::OnUsers(wxCommandEvent& event)
{
	CWindowPtr<CDialogUserList> dlg(new CDialogUserList(this, wxID_ANY));
	dlg->Show();
}

#include "frontend/win/dlgs/activeUser.h"

void CDocDesignerMDIFrame::OnActiveUsers(wxCommandEvent& event)
{
	CWindowPtr<CDialogActiveUser> dlg(new CDialogActiveUser(this, wxID_ANY));
	dlg->Show();
}

#include "frontend/win/dlgs/connectionDB.h"

void CDocDesignerMDIFrame::OnConnection(wxCommandEvent& event)
{
	CDialogConnection dlg(this, wxID_ANY);
	dlg.ShowModal();
}

#include "frontend/win/dlgs/about.h"

void CDocDesignerMDIFrame::OnAbout(wxCommandEvent& event)
{
	CDialogAbout dlg(this, wxID_ANY);
	dlg.ShowModal();
}