#include "debugClientImpl.h"
#include "docManager/docManager.h"

void CDebuggerClientBridge::OnSessionStart(wxSocketClient* sock)
{
	if (docManager != nullptr) {
		for (auto& doc : docManager->GetDocumentsVector()) {
			CMetaDocument* metaDoc = dynamic_cast<CMetaDocument*>(doc);
			if (metaDoc != nullptr) {
				IModuleDocument* foundedDoc = dynamic_cast<IModuleDocument*>(metaDoc);
				if (foundedDoc != nullptr) {
					foundedDoc->SetCurrentLine(wxNOT_FOUND, false);
					foundedDoc->SetToolTip(wxEmptyString);
				}
				for (auto& child_doc : metaDoc->GetChild()) {
					IModuleDocument* foundedDoc = dynamic_cast<IModuleDocument*>(child_doc);
					if (foundedDoc != nullptr) {
						foundedDoc->SetCurrentLine(wxNOT_FOUND, false);
						foundedDoc->SetToolTip(wxEmptyString);
					}
				}
			}
		}
	}

	if (mainFrame != nullptr) {
		mainFrame->Debugger_OnSessionStart();
		//mainFrame->RefreshFrame(); // Lots of unnecessary updates
	}
}

void CDebuggerClientBridge::OnSessionEnd(wxSocketClient* sock)
{
	if (docManager != nullptr) {
		for (auto& doc : docManager->GetDocumentsVector()) {
			CMetaDocument* metaDoc = dynamic_cast<CMetaDocument*>(doc);
			if (metaDoc != nullptr) {
				IModuleDocument* foundedDoc = dynamic_cast<IModuleDocument*>(metaDoc);
				if (foundedDoc != nullptr) {
					foundedDoc->SetCurrentLine(wxNOT_FOUND, false);
					foundedDoc->SetToolTip(wxEmptyString);
				}
				for (auto& child_doc : metaDoc->GetChild()) {
					IModuleDocument* foundedDoc = dynamic_cast<IModuleDocument*>(child_doc);
					if (foundedDoc != nullptr) {
						foundedDoc->SetCurrentLine(wxNOT_FOUND, false);
						foundedDoc->SetToolTip(wxEmptyString);
					}
				}
			}
		}
	}

	if (localWindow != nullptr) localWindow->ClearAndCreate();
	if (stackWindow != nullptr) stackWindow->ClearAndCreate();

	if (mainFrame != nullptr) {
		mainFrame->Debugger_OnSessionEnd();
		//mainFrame->RefreshFrame(); // Lots of unnecessary updates
	}
}

void CDebuggerClientBridge::OnEnterLoop(wxSocketClient* sock, const CDebugLineData& data)
{
	if (mainFrame != nullptr) mainFrame->RaiseFrame();

	if (docManager != nullptr) {
		if (data.m_fileName.IsEmpty()) {
			IBackendMetadataTree* metaTree = commonMetaData->GetMetaTree();
			wxASSERT(metaTree);
			metaTree->EditModule(data.m_moduleName, data.m_line, true);
		}
		else if (!data.m_fileName.IsEmpty()) {
			IMetaDataDocument* foundedDoc = dynamic_cast<IMetaDataDocument*>(
				docManager->FindDocumentByPath(data.m_fileName)
				);
			if (foundedDoc == nullptr) {
				foundedDoc = dynamic_cast<IMetaDataDocument*>(
					docManager->CreateDocument(data.m_fileName, wxDOC_SILENT)
					);
			}
			if (foundedDoc != nullptr) {
				IMetaData* metaData = foundedDoc->GetMetaData();
				wxASSERT(metaData);
				IBackendMetadataTree* metaTree = metaData->GetMetaTree();
				wxASSERT(metaTree);
				metaTree->EditModule(data.m_moduleName, data.m_line, true);
			}
		}
	}

	if (mainFrame != nullptr) {
		mainFrame->Debugger_OnEnterLoop();
		//mainFrame->RefreshFrame(); // Lots of unnecessary updates
	}
}

void CDebuggerClientBridge::OnLeaveLoop(wxSocketClient* sock, const CDebugLineData& data)
{
	if (docManager != nullptr) {
		if (data.m_fileName.IsEmpty()) {
			IBackendMetadataTree* metaTree = commonMetaData->GetMetaTree();
			if (metaTree != nullptr) {
				IMetaObject* foundedMeta = commonMetaData->FindByName(data.m_moduleName);
				if (foundedMeta != nullptr) {
					IModuleDocument* foundedDoc = dynamic_cast<IModuleDocument*>(metaTree->GetDocument(foundedMeta));
					if (foundedDoc != nullptr) {
						foundedDoc->SetCurrentLine(data.m_line, false);
						foundedDoc->SetToolTip(wxEmptyString);
					}
				}
			}
		}
		else if (!data.m_fileName.IsEmpty()) {
			IMetaDataDocument* foundedDoc = dynamic_cast<IMetaDataDocument*>(docManager->FindDocumentByPath(data.m_fileName));
			if (foundedDoc != nullptr) {
				IMetaData* foundedMetadata = foundedDoc->GetMetaData();
				wxASSERT(foundedMetadata);
				IBackendMetadataTree* metaTree = foundedMetadata->GetMetaTree();
				if (metaTree != nullptr) {
					IMetaObject* foundedMeta = foundedMetadata->FindByName(data.m_moduleName);
					if (foundedMeta != nullptr) {
						IModuleDocument* foundedDoc = dynamic_cast<IModuleDocument*>(metaTree->GetDocument(foundedMeta));
						if (foundedDoc != nullptr) {
							foundedDoc->SetCurrentLine(data.m_line, false);
							foundedDoc->SetToolTip(wxEmptyString);
						}
					}
				}
			}
		}
	}

	if (localWindow != nullptr) localWindow->ClearAndCreate();
	if (stackWindow != nullptr) stackWindow->ClearAndCreate();

	if (mainFrame != nullptr) {
		mainFrame->Debugger_OnLeaveLoop();
		//mainFrame->RefreshFrame(); // Lots of unnecessary updates
	}
}

void CDebuggerClientBridge::OnAutoComplete(const CDebugAutoCompleteData& data)
{
	if (docManager != nullptr) {
		if (data.m_fileName.IsEmpty()) {
			IBackendMetadataTree* metaTree = commonMetaData->GetMetaTree();
			if (metaTree != nullptr) {
				IMetaObject* foundedMeta = commonMetaData->FindByName(data.m_moduleName);
				if (foundedMeta != nullptr) {
					IModuleDocument* foundedDoc = static_cast<IModuleDocument*>(metaTree->GetDocument(foundedMeta));
					if (foundedDoc != nullptr) {
						foundedDoc->ShowAutoComplete(data);
					}
				}
			}
		}
		else if (!data.m_fileName.IsEmpty()) {
			IMetaDataDocument* foundedDoc = dynamic_cast<IMetaDataDocument*>(docManager->FindDocumentByPath(data.m_fileName));
			if (foundedDoc != nullptr) {
				IMetaData* metaData = foundedDoc->GetMetaData();
				wxASSERT(metaData);
				IBackendMetadataTree* metaTree = metaData->GetMetaTree();
				if (metaTree != nullptr) {
					IMetaObject* foundedMeta = metaData->FindByName(data.m_moduleName);
					if (foundedMeta != nullptr) {
						IModuleDocument* foundedDoc = static_cast<IModuleDocument*>(metaTree->GetDocument(foundedMeta));
						if (foundedDoc != nullptr) {
							foundedDoc->ShowAutoComplete(data);
						}
					}
				}
			}
		}
	}
}

void CDebuggerClientBridge::OnMessageFromServer(const CDebugLineData& data, const wxString& message)
{
	if (mainFrame != nullptr) mainFrame->RaiseFrame();

	if (docManager != nullptr) {
		if (data.m_fileName.IsEmpty()) {
			IBackendMetadataTree* metaTree = commonMetaData->GetMetaTree();
			wxASSERT(metaTree);
			metaTree->EditModule(data.m_moduleName, data.m_line, false);
		}
		if (!data.m_fileName.IsEmpty()) {
			IMetaDataDocument* foundedDoc = dynamic_cast<IMetaDataDocument*>(
				docManager->FindDocumentByPath(data.m_fileName)
				);
			if (foundedDoc == nullptr) {
				foundedDoc = dynamic_cast<IMetaDataDocument*>(
					docManager->CreateDocument(data.m_fileName, wxDOC_SILENT)
					);
			}
			if (foundedDoc != nullptr) {
				IMetaData* metaData = foundedDoc->GetMetaData();
				wxASSERT(metaData);
				IBackendMetadataTree* metaTree = metaData->GetMetaTree();
				wxASSERT(metaTree);
				metaTree->EditModule(data.m_moduleName, data.m_line, false);
			}
		}
	}

	outputWindow->OutputError(message,
		data.m_fileName, data.m_moduleName, data.m_line);
}

void CDebuggerClientBridge::OnSetToolTip(const CDebugExpressionData& data, const wxString& resultStr)
{
	if (docManager != nullptr) {
		if (data.m_fileName.IsEmpty()) {
			IBackendMetadataTree* metaTree = commonMetaData->GetMetaTree();
			if (metaTree != nullptr) {
				IMetaObject* foundedMeta = commonMetaData->FindByName(data.m_moduleName);
				if (foundedMeta != nullptr) {
					IModuleDocument* foundedDoc =
						static_cast<IModuleDocument*>(metaTree->GetDocument(foundedMeta));
					if (foundedDoc != nullptr) {
						foundedDoc->SetToolTip(resultStr);
					}
				}
			}
		}
		if (!data.m_fileName.IsEmpty()) {
			IMetaDataDocument* foundedDoc = dynamic_cast<IMetaDataDocument*>(
				docManager->FindDocumentByPath(data.m_fileName)
				);
			if (foundedDoc != nullptr) {
				IMetaData* metaData = foundedDoc->GetMetaData();
				wxASSERT(metaData);
				IBackendMetadataTree* metaTree = metaData->GetMetaTree();
				if (metaTree != nullptr) {
					IMetaObject* foundedMeta = metaData->FindByName(data.m_moduleName);
					if (foundedMeta != nullptr) {
						IModuleDocument* foundedDoc =
							static_cast<IModuleDocument*>(metaTree->GetDocument(foundedMeta));
						if (foundedDoc != nullptr) {
							foundedDoc->SetToolTip(resultStr);
						}
					}
				}
			}
		}
	}
}

void CDebuggerClientBridge::OnSetStack(const CStackData& stackData)
{
	stackWindow->SetStack(stackData);
}

void CDebuggerClientBridge::OnSetLocalVariable(const CLocalWindowData& data)
{
	localWindow->SetLocalVariable(data);
}

void CDebuggerClientBridge::OnSetVariable(const CWatchWindowData& watchData)
{
	watchWindow->SetVariable(watchData);
}

void CDebuggerClientBridge::OnSetExpanded(const CWatchWindowData& watchData)
{
	watchWindow->SetExpanded(watchData);
}
