#ifndef __IB_CODE_EDITOR_PRINTOUT_H__
#define __IB_CODE_EDITOR_PRINTOUT_H__

#include "codeEditor.h"

#if wxUSE_PRINTING_ARCHITECTURE

//----------------------------------------------------------------------------
//! EditPrint
class FRONTEND_API ibCodeEditorPrintout : public wxPrintout {

public:

	//! constructor
	ibCodeEditorPrintout(wxStyledTextCtrl *edit, const wxString& title = "");

	//! event handlers
	bool OnPrintPage(int page) wxOVERRIDE;
	bool OnBeginDocument(int startPage, int endPage) wxOVERRIDE;

	//! print functions
	bool HasPage(int page) wxOVERRIDE;
	void GetPageInfo(int *minPage, int *maxPage, int *selPageFrom, int *selPageTo) wxOVERRIDE;

private:
	wxStyledTextCtrl* m_edit = nullptr;
	wxArrayInt        m_pageEnds;
	wxRect            m_pageRect;
	wxRect            m_printRect;

	bool PrintScaling(wxDC* dc);
};

#endif // wxUSE_PRINTING_ARCHITECTURE

#endif // __IB_CODE_EDITOR_PRINTOUT_H__
