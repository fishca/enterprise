
#include "settingsdialog.h"
#include "keybinderdialog.h"
#include "fontcolorsettingspanel.h"
#include "editorsettingspanel.h"
#include "showhelpevent.h"

#include <wx/bookctrl.h>
    
BEGIN_EVENT_TABLE(CSettingsDialog, wxPropertySheetDialog)
    EVT_INIT_DIALOG(        CSettingsDialog::OnInitDialog)
    EVT_HELP(wxID_ANY,                      OnHelp) 
END_EVENT_TABLE()

CSettingsDialog::CSettingsDialog(wxWindow* parent)
    : wxPropertySheetDialog(parent, -1, _("Settings"), wxDefaultPosition, wxSize(450, 450), wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER) 
{
    SetMinSize(wxSize(450, 450));

    CreateButtons(wxOK | wxCANCEL);
    
    m_keyBinderDialog = new CKeyBinderDialog(GetBookCtrl());
    GetBookCtrl()->AddPage(m_keyBinderDialog, _("Key Bindings"));

    m_editorSettingsPanel = new CEditorSettingsPanel(GetBookCtrl());
    GetBookCtrl()->AddPage(m_editorSettingsPanel, _("Editor"));

    m_fontColorSettingsPanel = new CFontColorSettingsPanel(GetBookCtrl());
    GetBookCtrl()->AddPage(m_fontColorSettingsPanel, _("Font and Colors"));

    LayoutDialog();

}

void CSettingsDialog::OnInitDialog(wxInitDialogEvent& event)
{
    m_keyBinderDialog->Initialize();
    m_fontColorSettingsPanel->Initialize();
    m_editorSettingsPanel->Initialize();
}

CKeyBinderDialog* CSettingsDialog::GetKeyBinderDialog() const
{
    return m_keyBinderDialog;
}

CFontColorSettingsPanel* CSettingsDialog::GetFontColorSettingsPanel() const
{
    return m_fontColorSettingsPanel;
}

CEditorSettingsPanel* CSettingsDialog::GetEditorSettingsPanel() const
{
    return m_editorSettingsPanel;
}

void CSettingsDialog::OnHelp(wxHelpEvent&)
{
    wxCommandEvent event( wxEVT_SHOW_HELP_EVENT, GetId() );
    event.SetEventObject( this );
    event.SetString( wxT("Key Bindings") );
    GetParent()->GetEventHandler()->ProcessEvent( event );
}