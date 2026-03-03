
#include "keymonitortextctrl.h"
#include "keybinder.h"

IMPLEMENT_CLASS(CKeyMonitorTextCtrl, wxTextCtrl)

BEGIN_EVENT_TABLE(CKeyMonitorTextCtrl, wxTextCtrl)
    EVT_KEY_DOWN(CKeyMonitorTextCtrl::OnKey)
    EVT_KEY_UP(CKeyMonitorTextCtrl::OnKey)
END_EVENT_TABLE()

void CKeyMonitorTextCtrl::OnKey(wxKeyEvent &event)
{

    // backspace cannot be used as shortcut key...
    if (event.GetKeyCode() == WXK_BACK)
    {

        // this text ctrl contains something and the user pressed backspace...
        // we must delete the keypress...
        Clear();
        return;
    }

    if (event.GetEventType() == wxEVT_KEY_DOWN ||
        (event.GetEventType() == wxEVT_KEY_UP && !IsValidKeyComb()))
    {

        // the user pressed some key combination which must be displayed
        // in this text control.... or he has just stopped pressing a
        // modifier key like shift, ctrl or alt without adding any
        // other alphanumeric char, thus generating an invalid keystroke
        // which must be cleared out...

        CKeyBinder::Key key;
        key.code    = event.GetKeyCode();
        key.flags   = event.GetModifiers();

        SetValue(CKeyBinder::GetKeyBindingAsText(key));
        SetInsertionPointEnd();

    }

}