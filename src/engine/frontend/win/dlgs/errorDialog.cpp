////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuilder community
//	Description : error dialog window
////////////////////////////////////////////////////////////////////////////

#include "errorDialog.h"
#include "frontend/mainFrame/mainFrame.h"

#define DEF_LINENUMBER_ID 0
#define DEF_IMAGE_ID 1

CDialogError::CDialogError(CDocMDIFrame* parent, wxWindowID id,
	const wxString& title, const wxPoint& pos, const wxSize& size, long style) :
	wxDialog(parent, id, title, pos, size, style)
{
	wxDialog::SetSizeHints(wxDefaultSize, wxDefaultSize);
	wxBoxSizer* bSizerMain = new wxBoxSizer(wxHORIZONTAL);

	m_errorOutput = new wxStyledTextCtrl(this, wxID_ANY, wxDefaultPosition, wxSize(550, 250), 0, wxEmptyString);

	// initialize styles
	m_errorOutput->StyleClearAll();

	//set Lexer to LEX_CONTAINER: This will trigger the styleneeded event so you can do your own highlighting
	m_errorOutput->SetLexer(wxSTC_LEX_CONTAINER);

	//Set margin cursor
	for (int margin = 0; margin < m_errorOutput->GetMarginCount(); margin++)
		m_errorOutput->SetMarginCursor(margin, wxSTC_CURSORARROW);

	m_errorOutput->SetMarginType(DEF_LINENUMBER_ID, wxSTC_MARGIN_NUMBER);
	m_errorOutput->SetMarginWidth(DEF_LINENUMBER_ID, 0);

	// set margin as unused
	m_errorOutput->SetMarginType(DEF_IMAGE_ID, wxSTC_MARGIN_SYMBOL);
	m_errorOutput->SetMarginMask(DEF_IMAGE_ID, ~(1024 | 256 | 512 | 128 | 64 | wxSTC_MASK_FOLDERS));
	m_errorOutput->StyleSetBackground(DEF_IMAGE_ID, *wxWHITE);

	m_errorOutput->SetMarginWidth(DEF_IMAGE_ID, FromDIP(16));
	m_errorOutput->SetMarginSensitive(DEF_IMAGE_ID, true);

	bSizerMain->Add(m_errorOutput, 1, wxEXPAND | wxALL, 5);

	wxBoxSizer* m_bSizerButtons = new wxBoxSizer(wxVERTICAL);

	m_buttonCloseWindow = new wxButton(this, wxID_ANY, _("Close window"), wxDefaultPosition, wxDefaultSize, 0);
	m_bSizerButtons->Add(m_buttonCloseWindow, 0, wxALL | wxEXPAND, 5);

	m_buttonGotoDesigner = new wxButton(this, wxID_ANY, _("Go to designer"), wxDefaultPosition, wxDefaultSize, 0);
	m_bSizerButtons->Add(m_buttonGotoDesigner, 0, wxALL | wxEXPAND, 5);

	m_buttonCloseProgram = new wxButton(this, wxID_ANY, _("Close program"), wxDefaultPosition, wxDefaultSize, 0);
	m_bSizerButtons->Add(m_buttonCloseProgram, 0, wxALL | wxEXPAND, 5);

	bSizerMain->Add(m_bSizerButtons, 0, wxEXPAND, 5);

	wxDialog::SetSizer(bSizerMain);
	wxDialog::Layout();
	bSizerMain->Fit(this);

	wxDialog::Centre(wxBOTH);

	// Connect Events
	m_buttonCloseProgram->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CDialogError::OnButtonCloseProgramClick), nullptr, this);
	m_buttonGotoDesigner->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CDialogError::OnButtonGotoDesignerClick), nullptr, this);
	m_buttonCloseWindow->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CDialogError::OnButtonCloseWindowClick), nullptr, this);

	CDialogError::SetEditorSettings(parent->GetEditorSettings());
	CDialogError::SetFontColorSettings(parent->GetFontColorSettings());

	/** Enumeration of commands and child windows. */
	enum
	{
		idcmdUndo = 10,
		idcmdRedo = 11,
		idcmdCut = 12,
		idcmdCopy = 13,
		idcmdPaste = 14,
		idcmdDelete = 15,
		idcmdSelectAll = 16
	};

	wxAcceleratorEntry entries[2];
	entries[0].Set(wxACCEL_CTRL, (int)'A', idcmdSelectAll);
	entries[1].Set(wxACCEL_CTRL, (int)'C', idcmdCopy);

	wxAcceleratorTable accel(2, entries);
	m_errorOutput->SetAcceleratorTable(accel);

	wxIcon dlg_icon;
	dlg_icon.CopyFromBitmap(CBackendPicture::GetPicture(g_picErrorCLSID));

	wxDialog::SetIcon(dlg_icon);
	wxDialog::SetFocus();
}

void CDialogError::SetEditorSettings(const EditorSettings& settings)
{
	unsigned int m_bIndentationSize = settings.GetIndentSize();

	m_errorOutput->SetIndent(m_bIndentationSize);
	m_errorOutput->SetTabWidth(m_bIndentationSize);

	bool useTabs = settings.GetUseTabs();
	bool showWhiteSpace = settings.GetShowWhiteSpace();

	m_errorOutput->SetUseTabs(useTabs);
	m_errorOutput->SetTabIndents(useTabs);
	m_errorOutput->SetBackSpaceUnIndents(useTabs);
	m_errorOutput->SetViewWhiteSpace(showWhiteSpace);

	m_errorOutput->SetMarginType(DEF_LINENUMBER_ID, wxSTC_MARGIN_NUMBER);
	m_errorOutput->SetMarginWidth(DEF_LINENUMBER_ID, 0);
}

inline wxColour GetInverse(const wxColour& color)
{
	unsigned char r = color.Red();
	unsigned char g = color.Green();
	unsigned char b = color.Blue();

	return wxColour(r ^ 0xFF, g ^ 0xFF, b ^ 0xFF);
}

void CDialogError::SetFontColorSettings(const FontColorSettings& settings)
{
	// For some reason StyleSetFont takes a (non-const) reference, so we need to make
	// a copy before passing it in.
	wxFont font = settings.GetFont();

	m_errorOutput->StyleClearAll();
	m_errorOutput->StyleSetFont(wxSTC_STYLE_DEFAULT, font);

	m_errorOutput->SetSelForeground(true, settings.GetColors(FontColorSettings::DisplayItem_Selection).foreColor);
	m_errorOutput->SetSelBackground(true, settings.GetColors(FontColorSettings::DisplayItem_Selection).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_Default);

	m_errorOutput->StyleSetFont(wxSTC_C_DEFAULT, font);
	m_errorOutput->StyleSetFont(wxSTC_C_IDENTIFIER, font);

	m_errorOutput->StyleSetForeground(wxSTC_C_DEFAULT, settings.GetColors(FontColorSettings::DisplayItem_Default).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_C_DEFAULT, settings.GetColors(FontColorSettings::DisplayItem_Default).backColor);

	m_errorOutput->StyleSetForeground(wxSTC_STYLE_DEFAULT, settings.GetColors(FontColorSettings::DisplayItem_Default).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_STYLE_DEFAULT, settings.GetColors(FontColorSettings::DisplayItem_Default).backColor);

	m_errorOutput->StyleSetForeground(wxSTC_C_IDENTIFIER, settings.GetColors(FontColorSettings::DisplayItem_Default).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_C_IDENTIFIER, settings.GetColors(FontColorSettings::DisplayItem_Default).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_Comment);

	m_errorOutput->StyleSetFont(wxSTC_C_COMMENT, font);
	m_errorOutput->StyleSetFont(wxSTC_C_COMMENTLINE, font);
	m_errorOutput->StyleSetFont(wxSTC_C_COMMENTDOC, font);

	m_errorOutput->StyleSetForeground(wxSTC_C_COMMENT, settings.GetColors(FontColorSettings::DisplayItem_Comment).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_C_COMMENT, settings.GetColors(FontColorSettings::DisplayItem_Comment).backColor);

	m_errorOutput->StyleSetForeground(wxSTC_C_COMMENTLINE, settings.GetColors(FontColorSettings::DisplayItem_Comment).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_C_COMMENTLINE, settings.GetColors(FontColorSettings::DisplayItem_Comment).backColor);

	m_errorOutput->StyleSetForeground(wxSTC_C_COMMENTDOC, settings.GetColors(FontColorSettings::DisplayItem_Comment).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_C_COMMENTDOC, settings.GetColors(FontColorSettings::DisplayItem_Comment).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_Keyword);

	m_errorOutput->StyleSetFont(wxSTC_C_WORD, font);
	m_errorOutput->StyleSetForeground(wxSTC_C_WORD, settings.GetColors(FontColorSettings::DisplayItem_Keyword).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_C_WORD, settings.GetColors(FontColorSettings::DisplayItem_Keyword).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_Operator);
	m_errorOutput->StyleSetFont(wxSTC_C_OPERATOR, font);
	m_errorOutput->StyleSetForeground(wxSTC_C_OPERATOR, settings.GetColors(FontColorSettings::DisplayItem_Operator).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_C_OPERATOR, settings.GetColors(FontColorSettings::DisplayItem_Operator).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_String);

	m_errorOutput->StyleSetFont(wxSTC_C_STRING, font);
	m_errorOutput->StyleSetForeground(wxSTC_C_STRING, settings.GetColors(FontColorSettings::DisplayItem_String).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_C_STRING, settings.GetColors(FontColorSettings::DisplayItem_String).backColor);

	m_errorOutput->StyleSetFont(wxSTC_C_STRINGEOL, font);
	m_errorOutput->StyleSetForeground(wxSTC_C_STRINGEOL, settings.GetColors(FontColorSettings::DisplayItem_String).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_C_STRINGEOL, settings.GetColors(FontColorSettings::DisplayItem_String).backColor);

	m_errorOutput->StyleSetFont(wxSTC_C_CHARACTER, font);
	m_errorOutput->StyleSetForeground(wxSTC_C_CHARACTER, settings.GetColors(FontColorSettings::DisplayItem_String).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_C_CHARACTER, settings.GetColors(FontColorSettings::DisplayItem_String).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_Number);

	m_errorOutput->StyleSetFont(wxSTC_C_NUMBER, font);
	m_errorOutput->StyleSetForeground(wxSTC_C_NUMBER, settings.GetColors(FontColorSettings::DisplayItem_Number).foreColor);
	m_errorOutput->StyleSetBackground(wxSTC_C_NUMBER, settings.GetColors(FontColorSettings::DisplayItem_Number).backColor);

	m_errorOutput->StyleSetSize(wxSTC_STYLE_LINENUMBER, font.GetPointSize());

	// Set the caret color as the inverse of the background color so it's always visible.
	m_errorOutput->SetCaretForeground(GetInverse(settings.GetColors(FontColorSettings::DisplayItem_Default).backColor));
}

void CDialogError::OnButtonCloseProgramClick(wxCommandEvent& event)
{
	EndModal(3); Destroy(); event.Skip();
}

void CDialogError::OnButtonGotoDesignerClick(wxCommandEvent& event)
{
	EndModal(2); Destroy(); event.Skip();
}

void CDialogError::OnButtonCloseWindowClick(wxCommandEvent& event)
{
	EndModal(1); Destroy(); event.Skip();
}

CDialogError::~CDialogError()
{
	// Disconnect Events
	m_buttonCloseProgram->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CDialogError::OnButtonCloseProgramClick), nullptr, this);
	m_buttonGotoDesigner->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CDialogError::OnButtonGotoDesignerClick), nullptr, this);
	m_buttonCloseWindow->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(CDialogError::OnButtonCloseWindowClick), nullptr, this);
}
