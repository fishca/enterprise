
#include "statictextex.h"

BEGIN_EVENT_TABLE(CStaticTextEx, wxPanel)
    EVT_PAINT(CStaticTextEx::OnPaint)
END_EVENT_TABLE()

CStaticTextEx::CStaticTextEx(wxWindow* parent, wxWindowID id, const wxString& label, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
    : wxPanel(parent, id, pos, size, style, name), m_label(label)
{
}

void CStaticTextEx::OnPaint(wxPaintEvent& event)
{
    wxPaintDC dc(this);
    
    // Erase the background.
    wxBrush backBrush(GetBackgroundColour());
    dc.SetBackground(backBrush);
    dc.Clear();

    // Draw the text.
    dc.SetFont(GetFont());
    dc.SetTextForeground(GetForegroundColour());
    dc.DrawLabel(m_label, GetClientSize(), wxALIGN_CENTER);

}