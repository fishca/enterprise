#include "mainFrameEnterprise.h"

#include <wx/artprov.h>
#include <wx/renderer.h>
#include <wx/popupwin.h>
#include <wx/statline.h>
#include <wx/tglbtn.h>
#include <wx/hyperlink.h>

#define THEME_COLOUR_MAIN wxColour(255, 232, 166) 
#define THEME_COLOUR_BORDER wxColour(41, 57, 85) 

class wxSubSystemWindow : public wxWindow {

	// ----------------------------------------------------------------------------
	// wxSubSystemButton: search button used by search control
	// ----------------------------------------------------------------------------

	class wxSubSystemButton : public wxControl {

		wxString ChopText(wxDC& dc, const wxString& text, int max_size) {

			wxCoord x, y;

			// first check if the text fits with no problems
			dc.GetMultiLineTextExtent(text, &x, &y);
			if (x <= max_size)
				return text;
			unsigned int i, len = text.Length();
			unsigned int last_good_length = 0;
			for (i = 0; i < len; ++i) {

				wxString s = text.Left(i);
				s += wxT("...");
				dc.GetMultiLineTextExtent(s, &x, &y);
				if (x > max_size)
					break;
				last_good_length = i;
			}

			wxString ret = text.Left(last_good_length);
			ret += wxT("...");
			return ret;
		}

	public:

		wxSubSystemButton(wxSubSystemWindow* search, wxWindowID id, const wxString& label)
			: wxControl(search, id, wxDefaultPosition, wxDefaultSize, wxNO_BORDER),
			m_search(search),
			m_eventType(wxEVT_BUTTON) {

			m_baseColour = wxSystemSettings::GetColour(wxSYS_COLOUR_3DFACE);
			m_highlightColour = wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT);

			SetLabel(label);

			m_normalFont = *wxNORMAL_FONT;
			m_selectedFont = *wxNORMAL_FONT;
			m_selectedFont.SetWeight(wxFONTWEIGHT_BOLD);
			m_measuringFont = m_selectedFont;

			wxColor baseColour = THEME_COLOUR_BORDER;

			m_activeColour = baseColour;
			m_baseColour = baseColour;

			m_borderPen = wxPen(THEME_COLOUR_MAIN);
			m_baseColourPen = wxPen(THEME_COLOUR_MAIN);
			m_baseColourBrush = wxBrush(THEME_COLOUR_MAIN);

			SetCursor(wxCURSOR_HAND);
			SetBackgroundStyle(wxBG_STYLE_PAINT);
		}

		void SetActiveButton(bool a = true) {
			m_active = a; Refresh(); Update();
		}

		bool IsActiveButton() const { return m_active; }

		// The buttons in wxSearchCtrl shouldn't accept focus from keyboard because
		// this would interfere with the usual TAB processing: the user expects
		// that pressing TAB in the search control should switch focus to the next
		// control and not give it to the button inside the same control. Besides,
		// the search button can be already activated by pressing "Enter" so there
		// is really no reason for it to be able to get focus from keyboard.
		virtual bool AcceptsFocusFromKeyboard() const override { return false; }
		virtual wxWindow* GetMainWindowOfCompositeControl() override { return m_search; }

	protected:

		void OnLeftUp(wxMouseEvent&) {

			m_active = !m_active;

			wxCommandEvent event(m_eventType, m_search->GetId());

			event.SetEventObject(this);
			event.SetString(GetLabel());
			event.SetExtraLong(m_active);

			GetEventHandler()->ProcessEvent(event);

			Refresh();
		}

		void OnPaint(wxPaintEvent& e) {

			wxPaintDC dc(this);

			// Clear the background in case of a user bitmap with alpha channel
			dc.SetBrush(m_search->GetBackgroundColour());
			dc.Clear();

			wxCoord normal_textx, normal_texty;
			wxCoord selected_textx, selected_texty;
			wxCoord texty;

			// if the caption is empty, measure some temporary text
			wxString caption = m_labelOrig;
			if (caption.empty())
				caption = wxT("Xj");

			dc.SetFont(m_selectedFont);
			dc.GetMultiLineTextExtent(caption, &selected_textx, &selected_texty);

			dc.SetFont(m_normalFont);
			dc.GetMultiLineTextExtent(caption, &normal_textx, &normal_texty);

			// figure out the size of the tab
			wxSize tab_size = GetSize();

			wxCoord tab_height = tab_size.y;//tab_size.y -3;
			wxCoord tab_width = tab_size.x;

			wxRect in_rect = GetSize();//GetRect();

			wxCoord tab_x = in_rect.x;
			wxCoord tab_y = in_rect.y;// +in_rect.height - tab_height;

			bool active = m_active;

			// select pen, brush and font for the tab to be drawn
			if (active) {
				dc.SetFont(m_selectedFont);
				texty = selected_texty;
			}
			else {
				dc.SetFont(m_normalFont);
				texty = normal_texty;
			}

			// create points that will make the tab outline
			int clip_width = tab_width;
			if (tab_x + clip_width > in_rect.x + in_rect.width)
				clip_width = (in_rect.x + in_rect.width) - tab_x;

			// since the above code above doesn't play well with WXDFB or WXCOCOA,
			// we'll just use a rectangle for the clipping region for now --		
			dc.SetClippingRegion(tab_x, tab_y, clip_width, tab_height);

			wxPoint border_points[6];
			if (m_flags & wxAUI_NB_BOTTOM)
			{
				border_points[0] = wxPoint(tab_x, tab_y);
				border_points[1] = wxPoint(tab_x, tab_y + tab_height - 6);
				border_points[2] = wxPoint(tab_x + 2, tab_y + tab_height - 4);
				border_points[3] = wxPoint(tab_x + tab_width - 2, tab_y + tab_height - 4);
				border_points[4] = wxPoint(tab_x + tab_width, tab_y + tab_height - 6);
				border_points[5] = wxPoint(tab_x + tab_width, tab_y);
			}
			else //if (m_flags & wxAUI_NB_TOP) {}
			{
				//border_points[0] = wxPoint(tab_x, tab_y + tab_height - 4);
				//border_points[1] = wxPoint(tab_x, tab_y + 2);
				//border_points[2] = wxPoint(tab_x + 2, tab_y);
				//border_points[3] = wxPoint(tab_x + tab_width - 2, tab_y);
				//border_points[4] = wxPoint(tab_x + tab_width, tab_y + 2);
				//border_points[5] = wxPoint(tab_x + tab_width, tab_y + tab_height - 4);


				border_points[0] = wxPoint(tab_x, tab_y + tab_height);
				border_points[1] = wxPoint(tab_x, tab_y);
				border_points[2] = wxPoint(tab_x, tab_y);
				border_points[3] = wxPoint(tab_x + tab_width, tab_y);
				border_points[4] = wxPoint(tab_x + tab_width, tab_y);
				border_points[5] = wxPoint(tab_x + tab_width, tab_y + tab_height);

			}
			// TODO: else if (m_flags &wxAUI_NB_LEFT) {}
			// TODO: else if (m_flags &wxAUI_NB_RIGHT) {}

			int drawn_tab_yoff = border_points[1].y;
			int drawn_tab_height = border_points[0].y - border_points[1].y;

			wxColor back_color = m_baseColour;
			if (active) {

				// draw active tab
				// draw base background color
				wxRect r(tab_x, tab_y, tab_width, tab_height);
				dc.SetPen(wxPen(m_activeColour));
				dc.SetBrush(wxBrush(m_activeColour));
				dc.DrawRectangle(r.x, r.y, r.width, r.height);

				// this white helps fill out the gradient at the top of the tab
				wxColor gradient = THEME_COLOUR_MAIN;

				if (m_flags & wxAUI_NB_BOTTOM) {

					dc.SetPen(wxPen(gradient));
					dc.SetBrush(wxBrush(gradient));
					dc.DrawRectangle(r.x - 2, r.y - 1, r.width + 3, r.height + 4);

					// these two points help the rounded corners appear more antisynonymed
					dc.SetPen(wxPen(m_activeColour));

					dc.DrawPoint(r.x - 2, r.y - 1);
					dc.DrawPoint(r.x - r.width + 2, r.y - 1);
				}
				else {

					dc.SetPen(wxPen(gradient));
					dc.SetBrush(wxBrush(gradient));
					dc.DrawRectangle(r.x + 2, r.y + 1, r.width - 3, r.height - 5);

					// these two points help the rounded corners appear more antisynonymed
					dc.SetPen(wxPen(m_activeColour));


					dc.DrawPoint(r.x + 2, r.y + 1);
					dc.DrawPoint(r.x + r.width - 2, r.y + 1);

					dc.DrawPoint(r.x + 2, r.y + r.height - 5);
					dc.DrawPoint(r.x + r.width - 2, r.y + r.height - 5);

				}

				// set rectangle down a bit for gradient drawing
				r.SetHeight(r.GetHeight() / 2);
				r.x += 2;
				r.width -= 3;

				r.y += r.height;
				r.y -= 6;

				// draw gradient background
				wxColor top_color = gradient;
				wxColor bottom_color = gradient;

				dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);
			}
			else
			{
				// draw inactive tab
				wxRect r(tab_x, tab_y, tab_width, tab_height);

				// -- draw top gradient fill for glossy look
				wxColor top_color = m_baseColour;
				wxColor bottom_color = top_color;// .ChangeLightness(160);

				dc.GradientFillLinear(r, bottom_color, top_color, wxNORTH);

				// -- draw bottom fill for glossy look
				top_color = m_baseColour;
				bottom_color = m_baseColour;
				dc.GradientFillLinear(r, top_color, bottom_color, wxSOUTH);
			}

			// draw tab outline
			dc.SetPen(m_borderPen);
			dc.SetBrush(*wxTRANSPARENT_BRUSH);

			// there are two horizontal grey lines at the bottom of the tab control,
			// this gets rid of the top one of those lines in the tab control
			if (active)
			{
				if (m_flags & wxAUI_NB_BOTTOM)
					dc.SetPen(wxPen(m_baseColour.ChangeLightness(170)));
				// TODO: else if (m_flags &wxAUI_NB_LEFT) {}
				// TODO: else if (m_flags &wxAUI_NB_RIGHT) {}
				else //for wxAUI_NB_TOP
					dc.SetPen(m_baseColourPen);

				dc.DrawLine(border_points[0].x + 1,
					border_points[0].y,
					border_points[5].x,
					border_points[5].y);
			}

			int text_offset;
			int bitmap_offset = 0;

			//if (page.bitmap.IsOk())
			//{
			//	bitmap_offset = tab_x + wxControl::FromDIP(8);

			//	const wxBitmap bitmap = page.bitmap.GetBitmapFor(this);

			//	// draw bitmap
			//	dc.DrawBitmap(bitmap,
			//		bitmap_offset,
			//		drawn_tab_yoff + (drawn_tab_height / 2) - (bitmap.GetLogicalHeight() / 2),
			//		true);

			//	text_offset = bitmap_offset + bitmap.GetLogicalWidth();
			//	text_offset += wxControl::FromDIP(3); // bitmap padding
			//}
			//else
			//{
			//	text_offset = tab_x + wxControl::FromDIP(8);
			//}

			text_offset = tab_x + wxControl::FromDIP(8);

			// draw close button if necessary
			int close_button_width = 0;
			//if (close_button_state != wxAUI_BUTTON_STATE_HIDDEN) {

			//	wxBitmapBundle bb = m_disabledCloseBmp;

			//	if (close_button_state == wxAUI_BUTTON_STATE_HOVER ||
			//		close_button_state == wxAUI_BUTTON_STATE_PRESSED)
			//	{
			//		bb = m_activeCloseBmp;
			//	}

			//	const wxBitmap bmp = bb.GetBitmapFor(wnd);

			//	int offsetY = tab_y - 1;
			//	if (m_flags & wxAUI_NB_BOTTOM)
			//		offsetY = 1;

			//	wxRect rect(tab_x + tab_width - bmp.GetLogicalWidth() - wnd->FromDIP(1),
			//		offsetY + (tab_height / 2) - (bmp.GetLogicalHeight() / 2),
			//		bmp.GetLogicalWidth(),
			//		tab_height);

			//	IndentPressedBitmap(wnd->FromDIP(wxSize(1, 1)), &rect, close_button_state);
			//	dc.DrawBitmap(bmp, rect.x, rect.y, true);

			//	//*out_button_rect = rect;
			//	close_button_width = bmp.GetLogicalWidth();
			//}

			wxString draw_text = ChopText(dc,
				caption,
				tab_width - (text_offset - tab_x) - close_button_width);

			// draw tab text
			if (!active) {
				dc.SetTextForeground(*wxWHITE);
			}
			else {
				dc.SetTextForeground(*wxBLACK);
			}

			dc.DrawText(draw_text,
				text_offset,
				drawn_tab_yoff + (drawn_tab_height) / 2 - (texty / 2) - 1);

			// draw focus rectangle
			if (false) {

				wxRect focusRectText(text_offset, (drawn_tab_yoff + (drawn_tab_height) / 2 - (texty / 2) - 1),
					selected_textx, selected_texty);

				wxRect focusRect;
				wxRect focusRectBitmap;

				//if (page.bitmap.IsOk())
				//{
				//	const wxBitmap bitmap = page.bitmap.GetBitmapFor(this);

				//	focusRectBitmap = wxRect(bitmap_offset, drawn_tab_yoff + (drawn_tab_height / 2) - (bitmap.GetLogicalHeight() / 2),
				//		bitmap.GetLogicalWidth(), bitmap.GetLogicalHeight());
				//}

				//if (page.bitmap.IsOk() && draw_text.IsEmpty())
				//	focusRect = focusRectBitmap;
				//else if (!page.bitmap.IsOk() && !draw_text.IsEmpty())
				//	focusRect = focusRectText;
				//else if (page.bitmap.IsOk() && !draw_text.IsEmpty())
				//	focusRect = focusRectText.Union(focusRectBitmap);

				focusRect.Inflate(2, 2);
				wxRendererNative::Get().DrawFocusRect(this, dc, focusRect, 0);
			}

			dc.DestroyClippingRegion();
		}

	private:

		bool m_active = false;

		unsigned int m_flags = wxAUI_TB_TEXT | wxAUI_NB_LEFT;
		int m_textOrientation = wxAUI_TBTOOL_TEXT_LEFT;

		wxAuiToolBarItem item;

		wxFont m_normalFont;
		wxFont m_selectedFont;
		wxFont m_measuringFont;
		wxColour m_baseColour;
		wxColour m_highlightColour;

		wxPen m_baseColourPen;
		wxPen m_borderPen;

		wxBrush m_baseColourBrush;
		wxColour m_activeColour;

		wxSubSystemWindow* m_search;
		wxEventType   m_eventType;

		wxDECLARE_EVENT_TABLE();
	};

	class wxPopupSubWindow : public wxPopupTransientWindow {

		class wxScrolledSubWindow : public wxScrolledWindow {

		public:

			wxScrolledSubWindow() : wxScrolledWindow()
			{
			}

			wxScrolledSubWindow(wxWindow* parent,
				wxWindowID winid = wxID_ANY,
				const wxPoint& pos = wxDefaultPosition,
				const wxSize& size = wxDefaultSize,
				long style = wxScrolledWindowStyle,
				const wxString& name = wxASCII_STR(wxPanelNameStr))
				: wxScrolledWindow(parent, winid, pos, size, style, name)
			{
				wxBoxSizer* sizerMain = new wxBoxSizer(wxHORIZONTAL);
				wxBoxSizer* sizerLeft = new wxBoxSizer(wxVERTICAL);

				wxBoxSizer* sizerSubCommonSpacer = new wxBoxSizer(wxHORIZONTAL);
				sizerSubCommonSpacer->Add(20, 0, 0, wxEXPAND, 5);

				wxBoxSizer* sizerSubCommonItem = new wxBoxSizer(wxVERTICAL);
				m_hyperlink16 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerSubCommonItem->Add(m_hyperlink16, 0, wxALL, 5);

				m_hyperlink17 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerSubCommonItem->Add(m_hyperlink17, 0, wxALL, 5);
				sizerSubCommonSpacer->Add(sizerSubCommonItem, 1, wxEXPAND, 5);

				sizerLeft->Add(sizerSubCommonSpacer, 0, wxEXPAND, 5);
				wxBoxSizer* sizerSubsystem = new wxBoxSizer(wxVERTICAL);

				m_staticSubSystem = new wxStaticText(this, wxID_ANY, _("<subsytem name>"), wxDefaultPosition, wxDefaultSize, 0);
				m_staticSubSystem->Wrap(-1);
				m_staticSubSystem->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));

				sizerSubsystem->Add(m_staticSubSystem, 0, wxALL, 5);

				wxBoxSizer* sizerSubsystemSpacer = new wxBoxSizer(wxHORIZONTAL);
				sizerSubsystemSpacer->Add(20, 0, 0, wxEXPAND, 5);
				wxBoxSizer* sizerSubSystemItem = new wxBoxSizer(wxVERTICAL);

				m_hyperlink11 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerSubSystemItem->Add(m_hyperlink11, 0, wxALL, 5);

				m_hyperlink12 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerSubSystemItem->Add(m_hyperlink12, 0, wxALL, 5);

				m_hyperlink13 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerSubSystemItem->Add(m_hyperlink13, 0, wxALL, 5);

				m_hyperlink14 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerSubSystemItem->Add(m_hyperlink14, 0, wxALL, 5);

				m_hyperlink15 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerSubSystemItem->Add(m_hyperlink15, 0, wxALL, 5);

				sizerSubsystemSpacer->Add(sizerSubSystemItem, 1, wxEXPAND, 5);
				sizerSubsystem->Add(sizerSubsystemSpacer, 1, wxEXPAND, 5);

				sizerLeft->Add(sizerSubsystem, 1, wxEXPAND, 0);
				sizerMain->Add(sizerLeft, 0, 0, 0);
				sizerMain->Add(50, 0, 0, wxEXPAND, 5);

				wxBoxSizer* sizerRight = new wxBoxSizer(wxVERTICAL);
				wxBoxSizer* sizerCreate = new wxBoxSizer(wxVERTICAL);

				m_staticCreate = new wxStaticText(this, wxID_ANY, _("Create"), wxDefaultPosition, wxDefaultSize, 0);
				m_staticCreate->Wrap(-1);
				m_staticCreate->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));

				sizerCreate->Add(m_staticCreate, 0, wxALL | wxEXPAND, 5);

				wxBoxSizer* sizerCreateSpacer = new wxBoxSizer(wxHORIZONTAL);
				sizerCreateSpacer->Add(20, 0, 0, wxEXPAND, 5);

				wxBoxSizer* sizerCreateItem = new wxBoxSizer(wxVERTICAL);
				m_hyperlink1 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerCreateItem->Add(m_hyperlink1, 0, wxALL, 5);

				m_hyperlink2 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerCreateItem->Add(m_hyperlink2, 0, wxALL, 5);

				m_hyperlink3 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerCreateItem->Add(m_hyperlink3, 0, wxALL, 5);

				sizerCreateSpacer->Add(sizerCreateItem, 1, wxEXPAND, 5);
				sizerCreate->Add(sizerCreateSpacer, 1, wxEXPAND, 5);
				sizerRight->Add(sizerCreate, 0, wxEXPAND, 5);

				wxBoxSizer* sizerReport = new wxBoxSizer(wxVERTICAL);

				m_staticReport = new wxStaticText(this, wxID_ANY, _("Report"), wxDefaultPosition, wxDefaultSize, 0);
				m_staticReport->Wrap(-1);
				m_staticReport->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));

				sizerReport->Add(m_staticReport, 0, wxALL | wxEXPAND, 5);

				wxBoxSizer* sizerReportSpacer = new wxBoxSizer(wxHORIZONTAL);
				sizerReportSpacer->Add(20, 0, 0, wxEXPAND, 5);

				wxBoxSizer* sizerReportItem = new wxBoxSizer(wxVERTICAL);

				m_hyperlink4 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerReportItem->Add(m_hyperlink4, 0, wxALL, 5);
				sizerReportSpacer->Add(sizerReportItem, 1, wxEXPAND, 5);
				sizerReport->Add(sizerReportSpacer, 1, wxEXPAND, 5);
				sizerRight->Add(sizerReport, 0, wxEXPAND, 5);

				wxBoxSizer* sizerService = new wxBoxSizer(wxVERTICAL);

				m_staticService = new wxStaticText(this, wxID_ANY, _("Service"), wxDefaultPosition, wxDefaultSize, 0);
				m_staticService->Wrap(-1);
				m_staticService->SetFont(wxFont(12, wxFONTFAMILY_SWISS, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_BOLD, false, wxT("Arial")));

				sizerService->Add(m_staticService, 0, wxALL | wxEXPAND, 5);

				wxBoxSizer* sizerServiceSpacer = new wxBoxSizer(wxHORIZONTAL);
				sizerServiceSpacer->Add(20, 0, 0, wxEXPAND, 1);
				wxBoxSizer* sizerServiceItem = new wxBoxSizer(wxVERTICAL);

				m_hyperlink7 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Websitedfgijlk;"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerServiceItem->Add(m_hyperlink7, 0, wxALL, 5);

				m_hyperlink19 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Websiteqwe5tyiuytwertyuiytrewrtywqw5r6tyres"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);
				sizerServiceItem->Add(m_hyperlink19, 0, wxALL, 5);

				m_hyperlink20 = new wxHyperlinkCtrl(this, wxID_ANY, _("wxFB Website"), wxT("http://www.wxformbuilder.org"), wxDefaultPosition, wxDefaultSize, wxHL_DEFAULT_STYLE);

				sizerServiceItem->Add(m_hyperlink20, 0, wxALL, 5);
				sizerServiceSpacer->Add(sizerServiceItem, 1, wxEXPAND, 5);
				sizerService->Add(sizerServiceSpacer, 1, wxEXPAND, 5);
				sizerRight->Add(sizerService, 0, wxEXPAND, 5);
				sizerMain->Add(sizerRight, 0, 0, 5);

				SetSizer(sizerMain);
				Layout();
			}

		private:

			wxHyperlinkCtrl* m_hyperlink16;
			wxHyperlinkCtrl* m_hyperlink17;
			wxStaticText* m_staticSubSystem;
			wxHyperlinkCtrl* m_hyperlink11;
			wxHyperlinkCtrl* m_hyperlink12;
			wxHyperlinkCtrl* m_hyperlink13;
			wxHyperlinkCtrl* m_hyperlink14;
			wxHyperlinkCtrl* m_hyperlink15;
			wxStaticText* m_staticCreate;
			wxHyperlinkCtrl* m_hyperlink1;
			wxHyperlinkCtrl* m_hyperlink2;
			wxHyperlinkCtrl* m_hyperlink3;
			wxStaticText* m_staticReport;
			wxHyperlinkCtrl* m_hyperlink4;
			wxStaticText* m_staticService;
			wxHyperlinkCtrl* m_hyperlink7;
			wxHyperlinkCtrl* m_hyperlink19;
			wxHyperlinkCtrl* m_hyperlink20;
		};

	public:
		// ctors
		wxPopupSubWindow() : m_currentButton(nullptr) {}
		wxPopupSubWindow(wxWindow* parent, wxSubSystemButton* btn, int style = wxBORDER_NONE)
			: wxPopupTransientWindow(parent, style), m_currentButton(btn) {

			wxPopupTransientWindow::SetBackgroundColour(THEME_COLOUR_MAIN);

			wxBoxSizer* bSizer1 = new wxBoxSizer(wxHORIZONTAL);

			m_staticLine = new wxWindow(this, wxID_ANY, wxDefaultPosition, wxDefaultSize);
			m_staticLine->SetBackgroundColour(THEME_COLOUR_MAIN);
			m_staticLine->SetMinSize(wxSize(1, -1));
			bSizer1->Add(m_staticLine, 0, wxEXPAND | wxALL, 0);

			m_mainWindow = new wxScrolledSubWindow(this, wxID_ANY, wxPoint(-1, -1), wxDefaultSize, wxBORDER_SUNKEN | wxHSCROLL | wxVSCROLL);
			m_mainWindow->SetScrollRate(5, 5);

			m_mainWindow->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
			//m_mainWindow->SetBackgroundColour(THEME_COLOUR_BORDER);

			bSizer1->Add(m_mainWindow, 1, wxEXPAND | wxALL, 0);

			SetSizer(bSizer1);
			Layout();
		}

		virtual void Dismiss() override {
			m_currentButton->SetActiveButton(false);
			wxPopupTransientWindow::Dismiss();
		}

	private:
		wxWindow* m_staticLine;
		wxSubSystemButton* m_currentButton;
		wxScrolledWindow* m_mainWindow;

	};

	wxSubSystemButton* CreateSubMenu() {
		return m_arrayStaticText.emplace_back(
			new wxSubSystemButton(this, wxID_ANY, "submenu rocks!"));
	}

public:

	wxSubSystemWindow() : wxWindow() {}
	wxSubSystemWindow(wxWindow* parent,
		wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize,
		long style = 0,
		const wxString& name = wxASCII_STR(wxPanelNameStr))
		:
		wxWindow(parent, id, pos, size, style, name)
	{
		wxWindow::SetBackgroundColour(wxAUI_DEFAULT_COLOUR);
		FillSubMenu();
	}

	void FillSubMenu() {

		CreateSubMenu();
		CreateSubMenu();
		CreateSubMenu();
		CreateSubMenu();
		CreateSubMenu();
		CreateSubMenu();
		CreateSubMenu();
	}

	void FocusSubMenu() {
		for (auto& staticText : m_arrayStaticText) {
			staticText->SetActiveButton(false);
		}
	}

protected:

	void OnEventSize(wxSizeEvent& event) {
		unsigned int text_height_total = 0;
		for (auto& staticText : m_arrayStaticText) {
			staticText->SetSize(0, text_height_total, event.m_size.x, ms_text_height);
			text_height_total += ms_text_height;
		}
		event.Skip();
	}

	void OnEventButton(wxCommandEvent& event) {

		auto it = std::find(m_arrayStaticText.begin(), m_arrayStaticText.end(),
			event.GetEventObject()
		);

		if (event.GetExtraLong()) {

			const wxPoint& pos = GetScreenPosition();
			const wxSize& size = GetSize();

			const wxSize& main_size = mainFrame->GetSize();

			wxPopupSubWindow* sub = new wxPopupSubWindow(this, *it);
			sub->SetPosition(wxPoint(pos.x + size.x - 5, pos.y + 1));
			sub->SetSize(wxSize(main_size.x - size.x - 20, size.y - 5));

			sub->Popup();
		}

		event.Skip();
	}

private:

	static const int ms_text_height = 30;
	static const int ms_text_width = 200;

	std::vector<wxSubSystemButton*> m_arrayStaticText;

	wxDECLARE_EVENT_TABLE();
};

wxBEGIN_EVENT_TABLE(wxSubSystemWindow::wxSubSystemButton, wxControl)
EVT_LEFT_UP(wxSubSystemWindow::wxSubSystemButton::OnLeftUp)
EVT_PAINT(wxSubSystemWindow::wxSubSystemButton::OnPaint)
wxEND_EVENT_TABLE()

wxBEGIN_EVENT_TABLE(wxSubSystemWindow, wxWindow)
EVT_BUTTON(wxID_ANY, wxSubSystemWindow::OnEventButton)
EVT_SIZE(wxSubSystemWindow::OnEventSize)
wxEND_EVENT_TABLE()

//////////////////////////////////////////////////////////////////////////////

void CDocEnterpriseMDIFrame::CreateSubSystem()
{
	//wxAuiPaneInfo m_infoAdditional1;
	//m_infoAdditional1.Name(wxT("additional1"));
	//m_infoAdditional1.Left();
	//m_infoAdditional1.CaptionVisible(false);
	//m_infoAdditional1.MinSize(200, 25);
	//m_infoAdditional1.Movable(false);
	//m_infoAdditional1.Dockable(false);
	//m_infoAdditional1.Fixed();

	//m_mgr.AddPane(new wxSubSystemWindow(this, wxID_ANY), m_infoAdditional1);
}

//////////////////////////////////////////////////////////////////////////////