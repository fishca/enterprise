#include "dvc.h"

#include "frontend/win/ctrls/checkBoxEditor.h"
#include "frontend/win/ctrls/textEditor.h"
#include "frontend/visualView/ctrl/tableBox.h"
#include "backend/appData.h"

wxWindow* CValueViewRenderer::CreateEditorCtrl(wxWindow* dv,
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

	textEditor->ShowSelectButton(m_colControl->GetSelectButton());
	textEditor->ShowClearButton(m_colControl->GetClearButton());
	textEditor->ShowOpenButton(m_colControl->GetOpenButton());

	wxDataViewCtrl* parentWnd = dynamic_cast<wxDataViewCtrl*>(dv->GetParent());
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

	textEditor->SetPasswordMode(m_colControl->GetPasswordMode());
	textEditor->SetMultilineMode(m_colControl->GetMultilineMode());
	textEditor->SetTextEditMode(m_colControl->GetTextEditMode());

	if (!appData->DesignerMode()) {
		
		textEditor->Bind(wxEVT_CONTROL_BUTTON_SELECT, &CValueTableBoxColumn::OnSelectButtonPressed, m_colControl);
		textEditor->Bind(wxEVT_CONTROL_BUTTON_OPEN, &CValueTableBoxColumn::OnOpenButtonPressed, m_colControl);
		textEditor->Bind(wxEVT_CONTROL_BUTTON_CLEAR, &CValueTableBoxColumn::OnClearButtonPressed, m_colControl);
		
		textEditor->Bind(wxEVT_CONTROL_TEXT_ENTER, &CValueTableBoxColumn::OnTextEnter, m_colControl);
	}

	textEditor->LayoutControls();
	textEditor->Show(true);

	textEditor->SetInsertionPointEnd();
	return textEditor;
}

bool CValueViewRenderer::GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value)
{
	wxControlTextEditor* textEditor = wxDynamicCast(ctrl, wxControlTextEditor);

	if (textEditor == nullptr)
		return false;

	if (!appData->DesignerMode()) {
		
		textEditor->Unbind(wxEVT_CONTROL_BUTTON_SELECT, &CValueTableBoxColumn::OnSelectButtonPressed, m_colControl);
		textEditor->Unbind(wxEVT_CONTROL_BUTTON_OPEN, &CValueTableBoxColumn::OnOpenButtonPressed, m_colControl);
		textEditor->Unbind(wxEVT_CONTROL_BUTTON_CLEAR, &CValueTableBoxColumn::OnClearButtonPressed, m_colControl);

		textEditor->Unbind(wxEVT_CONTROL_TEXT_ENTER, &CValueTableBoxColumn::OnTextEnter, m_colControl);
	}

	value = textEditor->GetValue();
	return true;
}
