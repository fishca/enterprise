#ifndef SETTINGS_DIALOG_H
#define SETTINGS_DIALOG_H

#include <wx/wx.h>
#include <wx/propdlg.h> 

//
// Forward declarations.
//

class CKeyBinderDialog;
class CFontColorSettingsPanel;
class CEditorSettingsPanel;

#include "frontend/frontend.h"

/**
 * Settings dialog class.
 */
class FRONTEND_API CSettingsDialog : public wxPropertySheetDialog
{

public:

    /**
     * Constructor.
     */
    explicit CSettingsDialog(wxWindow* parent);

    /**
     * Called when the dialog is initialized.
     */
    void OnInitDialog(wxInitDialogEvent& event);

    /**
     * Called when the user activates the content help either by hitting the ? button
     * or pressing F1.
     */
    void OnHelp(wxHelpEvent& event);

    /**
     * Returns the key binder part of the settings dialog.
     */
    CKeyBinderDialog* GetKeyBinderDialog() const;

    /**
     * Returns the font and color part of the settings dialog.
     */
    CFontColorSettingsPanel* GetFontColorSettingsPanel() const;

    /**
     * Returns the editor part of the settings dialog.
     */
    CEditorSettingsPanel* GetEditorSettingsPanel() const;

    wxDECLARE_EVENT_TABLE();

private:

    CKeyBinderDialog*            m_keyBinderDialog;
    CFontColorSettingsPanel*     m_fontColorSettingsPanel;
    CEditorSettingsPanel*        m_editorSettingsPanel;

};

#endif