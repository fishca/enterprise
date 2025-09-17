////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxwidgets community
//	Description : main child window
////////////////////////////////////////////////////////////////////////////

#include "mainFrameChild.h"

wxIMPLEMENT_CLASS(CAuiDocChildFrame, wxAuiMDIChildFrame);
wxIMPLEMENT_CLASS(CDialogDocChildFrame, wxDialog);

#include "mainFrame.h"

CAuiDocChildFrame::~CAuiDocChildFrame()
{
	wxAuiMDIParentFrame* pParentFrame = GetMDIParentFrame();

	if (pParentFrame && CDocMDIFrame::GetFrame()) {
		if (pParentFrame->GetActiveChild() == this) {
			pParentFrame->SetActiveChild(nullptr);
			pParentFrame->SetChildMenuBar(nullptr);
		}
		wxAuiMDIClientWindow* pClientWindow = pParentFrame->GetClientWindow();
		wxASSERT(pClientWindow);
		int idx = pClientWindow->GetPageIndex(this);
		if (idx != wxNOT_FOUND) {
			pClientWindow->RemovePage(idx);
		}
	}
}

#if wxUSE_MENUS
void CAuiDocChildFrame::SetMenuBar(wxMenuBar* menuBar)
{
	wxMenuBar* pOldMenuBar = m_pMenuBar;
	m_pMenuBar = menuBar;

	struct CProcSubMenu {

		static void ConstructMenu(wxMenu* dst, wxMenu* src) {

			for (auto& it : dst->GetMenuItems()) {

				wxMenuItem* menuItem = src->Append(it->GetId(),
					it->GetItemLabel(),
					it->GetHelp(),
					it->GetKind()
				);

				menuItem->SetMarginWidth(it->GetMarginWidth());
				menuItem->SetBitmap(it->GetBitmap());

				if (it->IsSubMenu()) {
					wxMenu* subMenu = new wxMenu;
					menuItem->SetSubMenu(subMenu);
					ConstructMenu(subMenu, it->GetSubMenu());
				}
			}
		}
	};

	if (m_pMenuBar != nullptr) {

		wxAuiMDIParentFrame* pParentFrame = GetMDIParentFrame();
		wxASSERT_MSG(pParentFrame, wxT("Missing MDI Parent Frame"));

		wxMenuBar* pMenuBar = pParentFrame->GetMenuBar();
		if (pMenuBar != nullptr) {

			const int pos_edit = pMenuBar->FindMenu(wxGetStockLabel(wxID_EDIT, wxSTOCK_NOFLAGS));

			for (unsigned int idx = 0; idx < pMenuBar->GetMenuCount(); idx++) {

				wxMenu* dst = pMenuBar->GetMenu(idx);
				wxMenu* src = new wxMenu;

				if (pos_edit != wxNOT_FOUND) {
					if ((int)idx > pos_edit) {
						m_pMenuBar->Append(src,
							pMenuBar->GetMenuLabel(idx)
						);
					}
					else {
						m_pMenuBar->Insert(idx, src,
							pMenuBar->GetMenuLabel(idx)
						);
					}
				}
				else {
					m_pMenuBar->Append(src,
						pMenuBar->GetMenuLabel(idx)
					);
				}

				CProcSubMenu::ConstructMenu(dst, src);
			}
		}

		m_pMenuBar->SetParent(pParentFrame);

		if (pParentFrame->GetActiveChild() == this) {

			// replace current menu bars
			if (pOldMenuBar != nullptr)
				pParentFrame->SetChildMenuBar(nullptr);
			pParentFrame->SetChildMenuBar(this);
		}
	}
	else {

		wxAuiMDIParentFrame* pParentFrame = GetMDIParentFrame();
		wxASSERT_MSG(pParentFrame, wxT("Missing MDI Parent Frame"));

		pParentFrame->SetChildMenuBar(nullptr);
	}
}

wxMenuBar* CAuiDocChildFrame::GetMenuBar() const
{
	return m_pMenuBar;
}
#endif

void CAuiDocChildFrame::SetLabel(const wxString& label)
{
	if (label != GetLabel()) {
		wxAuiMDIChildFrame::SetLabel(label);
		wxAuiMDIChildFrame::SetTitle(label);
	}
}