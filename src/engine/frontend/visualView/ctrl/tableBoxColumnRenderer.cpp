#include "tableBoxColumnRenderer.h"

#include "frontend/win/ctrls/controlTextEditor.h"
#include "backend/appData.h"

wxWindow* CDataViewValueRenderer::CreateEditorCtrl(wxWindow* dv,
	wxRect labelRect,
	const wxVariant& value)
{
	wxControlTextEditor* textEditor = new wxControlTextEditor;

	textEditor->SetDVCMode(true);
	
	// create the window hidden to prevent flicker
	textEditor->Show(false);
	
	bool result = textEditor->Create(dv, wxID_ANY, value,
		labelRect.GetPosition(),
		labelRect.GetSize());

	if (!result)
		return nullptr;

	textEditor->ShowSelectButton(m_tableBoxColumn->GetSelectButton());
	textEditor->ShowClearButton(m_tableBoxColumn->GetClearButton());
	textEditor->ShowOpenButton(m_tableBoxColumn->GetOpenButton());

	wxDataViewExtCtrl* parentWnd = dynamic_cast<wxDataViewExtCtrl*>(dv->GetParent());
	if (parentWnd != nullptr) {
		textEditor->SetBackgroundColour(parentWnd->GetBackgroundColour());
		textEditor->SetForegroundColour(parentWnd->GetForegroundColour());
		textEditor->SetFont(parentWnd->GetFont());
	}
	else {
		textEditor->SetBackgroundColour(dv->GetBackgroundColour());
		textEditor->SetForegroundColour(dv->GetForegroundColour());
		textEditor->SetFont(dv->GetFont());
	}

	textEditor->SetPasswordMode(m_tableBoxColumn->GetPasswordMode());
	textEditor->SetMultilineMode(m_tableBoxColumn->GetMultilineMode());
	textEditor->SetTextEditMode(m_tableBoxColumn->GetTextEditMode());

	if (!appData->DesignerMode()) {
		
		textEditor->Bind(wxEVT_CONTROL_BUTTON_SELECT, &CValueTableBoxColumn::OnSelectButtonPressed, m_tableBoxColumn);
		textEditor->Bind(wxEVT_CONTROL_BUTTON_OPEN, &CValueTableBoxColumn::OnOpenButtonPressed, m_tableBoxColumn);
		textEditor->Bind(wxEVT_CONTROL_BUTTON_CLEAR, &CValueTableBoxColumn::OnClearButtonPressed, m_tableBoxColumn);
		
		textEditor->Bind(wxEVT_CONTROL_TEXT_ENTER, &CValueTableBoxColumn::OnTextEnter, m_tableBoxColumn);
	}

	textEditor->LayoutControls();
	textEditor->Show(true);

	textEditor->SetInsertionPointEnd();
	return textEditor;
}

bool CDataViewValueRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
	wxControlTextEditor* textEditor = wxDynamicCast(ctrl, wxControlTextEditor);

	if (textEditor == nullptr)
		return false;

	if (!appData->DesignerMode()) {
		
		textEditor->Unbind(wxEVT_CONTROL_BUTTON_SELECT, &CValueTableBoxColumn::OnSelectButtonPressed, m_tableBoxColumn);
		textEditor->Unbind(wxEVT_CONTROL_BUTTON_OPEN, &CValueTableBoxColumn::OnOpenButtonPressed, m_tableBoxColumn);
		textEditor->Unbind(wxEVT_CONTROL_BUTTON_CLEAR, &CValueTableBoxColumn::OnClearButtonPressed, m_tableBoxColumn);

		textEditor->Unbind(wxEVT_CONTROL_TEXT_ENTER, &CValueTableBoxColumn::OnTextEnter, m_tableBoxColumn);
	}

	value = textEditor->GetValue();
	return true;
}
