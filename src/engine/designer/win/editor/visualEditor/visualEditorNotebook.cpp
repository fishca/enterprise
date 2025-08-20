#include "visualEditor.h"
#include "frontend/artProvider/artProvider.h"
#include "frontend/win/theme/luna_tabart.h"

#include "win/editor/codeEditor/codeEditorParser.h"

void CVisualEditorNotebook::CreateVisualEditor(CMetaDocument* document, wxWindow* parent, wxWindowID id, long flags)
{
	wxAuiNotebook::AddPage(m_visualEditor, _("Designer"), false, wxArtProvider::GetBitmap(wxART_DESIGNER_PAGE, wxART_DOC_FORM));
	wxAuiNotebook::AddPage(m_codeEditor, _("Code"), false, wxArtProvider::GetBitmap(wxART_CODE_PAGE, wxART_DOC_FORM));
	m_visualEditor->SetReadOnly(flags == wxDOC_READONLY);
	m_codeEditor->SetReadOnly(flags == wxDOC_READONLY);
	wxAuiNotebook::SetSelection(wxNOTEBOOK_PAGE_DESIGNER);

	wxAuiNotebook::Bind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &CVisualEditorNotebook::OnPageChanged, this);
	wxAuiNotebook::SetArtProvider(new wxAuiLunaTabArt());
}

void CVisualEditorNotebook::DestroyVisualEditor()
{
	wxAuiNotebook::Unbind(wxEVT_AUINOTEBOOK_PAGE_CHANGED, &CVisualEditorNotebook::OnPageChanged, this);
}

bool CVisualEditorNotebook::Undo()
{
	if (wxAuiNotebook::GetSelection() == wxNOTEBOOK_PAGE_DESIGNER)
		m_visualEditor->Undo();
	else if (wxAuiNotebook::GetSelection() == wxNOTEBOOK_PAGE_CODE_EDITOR)
		m_codeEditor->Undo();
	return false;
}

bool CVisualEditorNotebook::Redo()
{
	if (wxAuiNotebook::GetSelection() == wxNOTEBOOK_PAGE_DESIGNER)
		m_visualEditor->Redo();
	else if (wxAuiNotebook::GetSelection() == wxNOTEBOOK_PAGE_CODE_EDITOR)
		m_codeEditor->Redo();
	return false;
}

bool CVisualEditorNotebook::CanUndo() const
{
	if (wxAuiNotebook::GetSelection() == wxNOTEBOOK_PAGE_DESIGNER)
		return m_visualEditor->CanUndo();
	else if (wxAuiNotebook::GetSelection() == wxNOTEBOOK_PAGE_CODE_EDITOR)
		return m_codeEditor->CanUndo();
	return false;
}

bool CVisualEditorNotebook::CanRedo() const
{
	if (wxAuiNotebook::GetSelection() == wxNOTEBOOK_PAGE_DESIGNER)
		return m_visualEditor->CanRedo();
	else if (wxAuiNotebook::GetSelection() == wxNOTEBOOK_PAGE_CODE_EDITOR)
		return m_codeEditor->CanRedo();
	return false;
}

void CVisualEditorNotebook::ModifyEvent(IEvent* event, const wxVariant& oldValue, const wxVariant& newValue)
{
	CParserModule parser; bool procFounded = false;

	const wxString& strEvent = newValue.GetString();

	unsigned int lineStart = m_codeEditor->GetLineCount();
	unsigned int lineEnd = lineStart;

	bool changeSel = true;

	long action = wxNOT_FOUND;
	if (strEvent.ToLong(&action)) {
		changeSel = false;
	}
	else if (strEvent.IsEmpty()) {
		changeSel = false;
	}

	if (parser.ParseModule(m_codeEditor->GetText())) {
		for (auto content : parser.GetAllContent()) {
			if (content.eType == eContentType::eProcedure ||
				content.eType == eContentType::eFunction ||
				content.eType == eContentType::eExportProcedure ||
				content.eType == eContentType::eExportFunction) {
				lineStart = content.nLineStart; lineEnd = content.nLineEnd;
				if (stringUtils::CompareString(content.strName, oldValue) && !stringUtils::CompareString(content.strName, newValue) && changeSel) {
					int answer = wxMessageBox(_("Do you rename an existing procedure?"), _("Rename"), wxYES_NO | wxCENTRE);
					if (answer == wxYES) {
						wxString currentLine = m_codeEditor->GetLine(content.nLineStart);
						currentLine.Replace(content.strName, newValue);
						m_codeEditor->Replace(m_codeEditor->GetLineEndPosition(content.nLineStart - 1) + 1, m_codeEditor->GetLineEndPosition(content.nLineStart) + 1, currentLine);
						procFounded = true; lineStart = content.nLineStart; lineEnd = content.nLineEnd;
					}
					break;
				}
				else if (stringUtils::CompareString(content.strName, newValue)) {
					procFounded = true; lineStart = content.nLineStart; lineEnd = content.nLineEnd;
					break;
				}
				else if (stringUtils::CompareString(content.strName, oldValue) && !changeSel) {
					int answer = wxMessageBox(_("Do you delete an existing procedure?"), _("Delete"), wxYES_NO | wxCENTRE);
					if (answer == wxYES) {
						m_codeEditor->Replace(
							m_codeEditor->GetLineEndPosition(content.nLineStart - 1) + 1,
							m_codeEditor->GetLineEndPosition(content.nLineEnd + 1),
							wxEmptyString
						);
					}
					procFounded = true;
				}
			}
		}
	}

	if (changeSel) {

		wxString prcArgs;

		for (auto& args : event->GetArgs()) {
			if (!prcArgs.IsEmpty()) prcArgs += ", ";
			prcArgs += args;
		}

		if (wxAuiNotebook::GetSelection() != wxNOTEBOOK_PAGE_CODE_EDITOR)
			wxAuiNotebook::SetSelection(wxNOTEBOOK_PAGE_CODE_EDITOR);

		if (!procFounded) {
			int endPos = m_codeEditor->GetLineEndPosition(lineEnd);
			wxString offset = endPos > 0 ?
				"\r\n\r\n" : "";
			m_codeEditor->Replace(endPos, endPos,
				offset +
				"procedure " + strEvent + "(" + prcArgs + ")\r\n"
				"\t\r\n"
				"endProcedure"
			);
			int patchLine = endPos > 0 ?
				2 : -1;
			lineStart = lineEnd + patchLine;
		}
		m_codeEditor->GotoLine(lineStart);
		m_codeEditor->SetSTCFocus(true);
	}

	m_visualEditor->ModifyEvent(event, oldValue, newValue);
}

#include "frontend/mainFrame/mainFrame.h"
#include "backend/metaCollection/metaObject.h"

void CVisualEditorNotebook::OnPageChanged(wxAuiNotebookEvent& event) {
	if (wxAuiNotebook::GetSelection() == wxNOTEBOOK_PAGE_DESIGNER) {
		m_visualEditor->SetFocus();
		objectInspector->SelectObject(m_visualEditor->GetSelectedObject());
	}
	else if (wxAuiNotebook::GetSelection() == wxNOTEBOOK_PAGE_CODE_EDITOR) {
		if (m_visualEditor->m_document != nullptr) {
			IMetaObject* moduleObject = m_visualEditor->m_document->GetMetaObject();
			if (moduleObject != nullptr) objectInspector->SelectObject(moduleObject);
		}
		m_codeEditor->SetSTCFocus(true);
		m_codeEditor->SetFocus();
	}
	if (m_visualEditor->m_document != nullptr)
		m_visualEditor->m_document->Activate();
	event.Skip();
}