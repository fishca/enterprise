////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : autoComplete window 
////////////////////////////////////////////////////////////////////////////

#include "codeEditor.h"
#include "frontend/mainFrame/mainFrame.h"
#include "backend/debugger/debugClient.h"
#include "backend/moduleManager/moduleManager.h"
#include "backend/metaData.h"
#include "frontend/docView/docView.h" 
#include "res/bitmaps_res.h"

#define DEF_LINENUMBER_ID 0
#define DEF_BREAKPOINT_ID 1
#define DEF_FOLDING_ID 2

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

CCodeEditor::CCodeEditor()
	: wxStyledTextCtrl(), m_document(nullptr), m_ac(this), m_ct(this), m_fp(this), m_precompileModule(nullptr), m_bInitialized(false) {
}

CCodeEditor::CCodeEditor(CMetaDocument* document, wxWindow* parent, wxWindowID id, const wxPoint& pos, const wxSize& size, long style, const wxString& name)
	: wxStyledTextCtrl(parent, id, pos, size, style, name), m_document(document), m_ac(this), m_ct(this), m_fp(this), m_precompileModule(nullptr), m_bInitialized(false)
{
	// initialize styles
	StyleClearAll();

	//set Lexer to LEX_CONTAINER: This will trigger the styleneeded event so you can do your own highlighting
	SetLexer(wxSTC_LEX_CONTAINER);

	//Set margin cursor
	for (int margin = 0; margin < GetMarginCount(); margin++)
		SetMarginCursor(margin, wxSTC_CURSORARROW);

	//register event
	Connect(wxEVT_STC_MARGINCLICK, wxStyledTextEventHandler(CCodeEditor::OnMarginClick), nullptr, this);
	Connect(wxEVT_STC_STYLENEEDED, wxStyledTextEventHandler(CCodeEditor::OnStyleNeeded), nullptr, this);
	Connect(wxEVT_STC_MODIFIED, wxStyledTextEventHandler(CCodeEditor::OnTextChange), nullptr, this);

	Connect(wxEVT_KEY_DOWN, wxKeyEventHandler(CCodeEditor::OnKeyDown), nullptr, this);
	Connect(wxEVT_MOTION, wxMouseEventHandler(CCodeEditor::OnMouseMove), nullptr, this);

	//set edge mode
	SetEdgeMode(wxSTC_EDGE_MULTILINE);

	// set visibility
	SetVisiblePolicy(wxSTC_VISIBLE_STRICT | wxSTC_VISIBLE_SLOP, 1);
	SetXCaretPolicy(wxSTC_CARET_EVEN | wxSTC_VISIBLE_STRICT | wxSTC_CARET_SLOP, 1);
	SetYCaretPolicy(wxSTC_CARET_EVEN | wxSTC_VISIBLE_STRICT | wxSTC_CARET_SLOP, 1);

	// Set the marker bitmaps.
	MarkerDefineBitmap(Breakpoint, wxMEMORY_BITMAP(Breakpoint_png));
	MarkerDefineBitmap(CurrentLine, wxMEMORY_BITMAP(Currentline_png));
	MarkerDefineBitmap(BreakLine, wxMEMORY_BITMAP(Breakline_png));

	//markers
	MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS, *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS, *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_VLINE, *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUSCONNECTED, *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUSCONNECTED, *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_TCORNER, *wxWHITE, *wxBLACK);
	MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_LCORNER, *wxWHITE, *wxBLACK);

	// annotations
	AnnotationSetVisible(wxSTC_ANNOTATION_BOXED);

	// Set fold flags
	SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);

	// Setup the dwell time before a tooltip is displayed.
	SetMouseDwellTime(200);

	// Setup caret line
	//SetCaretLineVisible(true);

	// miscellaneous
	SetLayoutCache(wxSTC_CACHE_PAGE);

	//Turn the fold markers red when the caret is a line in the group (optional)
	MarkerEnableHighlight(true);
}

CCodeEditor::~CCodeEditor()
{
	wxDELETE(m_precompileModule);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CCodeEditor::EditDebugPoint(int line_to_edit)
{
	//Update the list of breakpoints
	const int dwFlags = MarkerGet(line_to_edit);
	if ((dwFlags & (1 << CCodeEditor::Breakpoint))) {
		debugClient->RemoveBreakpoint(m_document->GetFilename(), line_to_edit);
	}
	else {
		debugClient->ToggleBreakpoint(m_document->GetFilename(), line_to_edit);
	}
}

void CCodeEditor::RefreshBreakpoint(bool deleteCurrentBreakline)
{
	MarkerDeleteAll(CCodeEditor::Breakpoint);

	//Update the list of breakpoints
	for (auto& line_to_edit : debugClient->GetDebugList(m_document->GetFilename())) {
		const int dwFlags = MarkerGet(line_to_edit);
		if (!(dwFlags & (1 << CCodeEditor::Breakpoint))) {
			MarkerAdd(line_to_edit, CCodeEditor::Breakpoint);
		}
	}
}

void CCodeEditor::SetCurrentLine(int lineBreakpoint, bool setBreakLine)
{
	const int firstVisibleLine = GetFirstVisibleLine(),
		linesOnScreen = LinesOnScreen();

	//Incorrect position when editing button title
	//if (!CCodeEditor::GetSTCFocus()) 
	// CodeEditor::SetSTCFocus(true);

	MarkerDeleteAll(CCodeEditor::BreakLine);

	if (setBreakLine) MarkerAdd(lineBreakpoint - 1, CCodeEditor::BreakLine);

	if (lineBreakpoint > 0) {

		if (firstVisibleLine > (lineBreakpoint - 1))
			ScrollToLine(lineBreakpoint - 1);
		else if (firstVisibleLine + linesOnScreen < (lineBreakpoint - 1))
			ScrollToLine(lineBreakpoint - 1);

	}

	//Set standart focus
	if (lineBreakpoint > 0) CCodeEditor::SetFocus();

	//if (!setBreakLine) GotoLine(lineBreakpoint - 1);
}

void CCodeEditor::SetEditorSettings(const EditorSettings& settings)
{
	m_bIndentationSize = settings.GetIndentSize();

	SetIndent(m_bIndentationSize);
	SetTabWidth(m_bIndentationSize);

	bool useTabs = settings.GetUseTabs();
	bool showWhiteSpace = settings.GetShowWhiteSpace();

	SetUseTabs(useTabs);
	SetTabIndents(useTabs);
	SetBackSpaceUnIndents(useTabs);
	SetViewWhiteSpace(showWhiteSpace);

	SetMarginType(DEF_LINENUMBER_ID, wxSTC_MARGIN_NUMBER);
	SetMarginWidth(DEF_LINENUMBER_ID, 0);

	if (settings.GetShowLineNumbers()) {
		// Figure out how wide the margin needs to be do display
		// the most number of linqes we'd reasonbly have.
		SetMarginWidth(DEF_LINENUMBER_ID, TextWidth(wxSTC_STYLE_LINENUMBER, "_9999999"));
	}

	// set margin as unused
	SetMarginType(DEF_BREAKPOINT_ID, wxSTC_MARGIN_SYMBOL);
	SetMarginMask(DEF_BREAKPOINT_ID, ~(1024 | 256 | 512 | 128 | 64 | wxSTC_MASK_FOLDERS));
	StyleSetBackground(DEF_BREAKPOINT_ID, *wxWHITE);

	SetMarginWidth(DEF_BREAKPOINT_ID, 0);
	SetMarginSensitive(DEF_BREAKPOINT_ID, false);

	if (true) {
		int foldingMargin = FromDIP(16);
		SetMarginWidth(DEF_BREAKPOINT_ID, foldingMargin);
		SetMarginSensitive(DEF_BREAKPOINT_ID, true);
	}

	// folding
	SetMarginType(DEF_FOLDING_ID, wxSTC_MARGIN_SYMBOL);
	SetMarginMask(DEF_FOLDING_ID, wxSTC_MASK_FOLDERS);

	SetMarginWidth(DEF_FOLDING_ID, 0);
	SetMarginSensitive(DEF_FOLDING_ID, false);

	if (true) {
		int foldingMargin = FromDIP(16);
		SetMarginWidth(DEF_FOLDING_ID, foldingMargin);
		SetMarginSensitive(DEF_FOLDING_ID, true);
	}

	m_bEnableAutoComplete = settings.GetEnableAutoComplete();
}

inline wxColour GetInverse(const wxColour& color)
{
	unsigned char r = color.Red();
	unsigned char g = color.Green();
	unsigned char b = color.Blue();

	return wxColour(r ^ 0xFF, g ^ 0xFF, b ^ 0xFF);
}

void CCodeEditor::SetFontColorSettings(const FontColorSettings& settings)
{
	// For some reason StyleSetFont takes a (non-const) reference, so we need to make
	// a copy before passing it in.
	wxFont font = settings.GetFont();

	StyleClearAll();
	StyleSetFont(wxSTC_STYLE_DEFAULT, font);

	SetSelForeground(true, settings.GetColors(FontColorSettings::DisplayItem_Selection).foreColor);
	SetSelBackground(true, settings.GetColors(FontColorSettings::DisplayItem_Selection).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_Default);

	StyleSetFont(wxSTC_C_DEFAULT, font);
	StyleSetFont(wxSTC_C_IDENTIFIER, font);

	StyleSetForeground(wxSTC_C_DEFAULT, settings.GetColors(FontColorSettings::DisplayItem_Default).foreColor);
	StyleSetBackground(wxSTC_C_DEFAULT, settings.GetColors(FontColorSettings::DisplayItem_Default).backColor);

	StyleSetForeground(wxSTC_STYLE_DEFAULT, settings.GetColors(FontColorSettings::DisplayItem_Default).foreColor);
	StyleSetBackground(wxSTC_STYLE_DEFAULT, settings.GetColors(FontColorSettings::DisplayItem_Default).backColor);

	StyleSetForeground(wxSTC_C_IDENTIFIER, settings.GetColors(FontColorSettings::DisplayItem_Default).foreColor);
	StyleSetBackground(wxSTC_C_IDENTIFIER, settings.GetColors(FontColorSettings::DisplayItem_Default).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_Comment);

	StyleSetFont(wxSTC_C_COMMENT, font);
	StyleSetFont(wxSTC_C_COMMENTLINE, font);
	StyleSetFont(wxSTC_C_COMMENTDOC, font);

	StyleSetForeground(wxSTC_C_COMMENT, settings.GetColors(FontColorSettings::DisplayItem_Comment).foreColor);
	StyleSetBackground(wxSTC_C_COMMENT, settings.GetColors(FontColorSettings::DisplayItem_Comment).backColor);

	StyleSetForeground(wxSTC_C_COMMENTLINE, settings.GetColors(FontColorSettings::DisplayItem_Comment).foreColor);
	StyleSetBackground(wxSTC_C_COMMENTLINE, settings.GetColors(FontColorSettings::DisplayItem_Comment).backColor);

	StyleSetForeground(wxSTC_C_COMMENTDOC, settings.GetColors(FontColorSettings::DisplayItem_Comment).foreColor);
	StyleSetBackground(wxSTC_C_COMMENTDOC, settings.GetColors(FontColorSettings::DisplayItem_Comment).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_Preprocessor);

	StyleSetFont(wxSTC_C_PREPROCESSOR, font);
	StyleSetForeground(wxSTC_C_PREPROCESSOR, settings.GetColors(FontColorSettings::DisplayItem_Preprocessor).foreColor);
	StyleSetBackground(wxSTC_C_PREPROCESSOR, settings.GetColors(FontColorSettings::DisplayItem_Preprocessor).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_Keyword);

	StyleSetFont(wxSTC_C_WORD, font);
	StyleSetForeground(wxSTC_C_WORD, settings.GetColors(FontColorSettings::DisplayItem_Keyword).foreColor);
	StyleSetBackground(wxSTC_C_WORD, settings.GetColors(FontColorSettings::DisplayItem_Keyword).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_Operator);
	StyleSetFont(wxSTC_C_OPERATOR, font);
	StyleSetForeground(wxSTC_C_OPERATOR, settings.GetColors(FontColorSettings::DisplayItem_Operator).foreColor);
	StyleSetBackground(wxSTC_C_OPERATOR, settings.GetColors(FontColorSettings::DisplayItem_Operator).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_String);

	StyleSetFont(wxSTC_C_STRING, font);
	StyleSetForeground(wxSTC_C_STRING, settings.GetColors(FontColorSettings::DisplayItem_String).foreColor);
	StyleSetBackground(wxSTC_C_STRING, settings.GetColors(FontColorSettings::DisplayItem_String).backColor);

	StyleSetFont(wxSTC_C_STRINGEOL, font);
	StyleSetForeground(wxSTC_C_STRINGEOL, settings.GetColors(FontColorSettings::DisplayItem_String).foreColor);
	StyleSetBackground(wxSTC_C_STRINGEOL, settings.GetColors(FontColorSettings::DisplayItem_String).backColor);

	StyleSetFont(wxSTC_C_CHARACTER, font);
	StyleSetForeground(wxSTC_C_CHARACTER, settings.GetColors(FontColorSettings::DisplayItem_String).foreColor);
	StyleSetBackground(wxSTC_C_CHARACTER, settings.GetColors(FontColorSettings::DisplayItem_String).backColor);

	StyleSetFont(wxSTC_C_CHARACTER, font);
	StyleSetForeground(wxSTC_C_CHARACTER, settings.GetColors(FontColorSettings::DisplayItem_Selection).foreColor);
	StyleSetBackground(wxSTC_C_CHARACTER, settings.GetColors(FontColorSettings::DisplayItem_Selection).backColor);

	font = settings.GetFont(FontColorSettings::DisplayItem_Number);

	StyleSetFont(wxSTC_C_NUMBER, font);
	StyleSetForeground(wxSTC_C_NUMBER, settings.GetColors(FontColorSettings::DisplayItem_Number).foreColor);
	StyleSetBackground(wxSTC_C_NUMBER, settings.GetColors(FontColorSettings::DisplayItem_Number).backColor);

	StyleSetSize(wxSTC_STYLE_LINENUMBER, font.GetPointSize());

	// Set the caret color as the inverse of the background color so it's always visible.
	SetCaretForeground(GetInverse(settings.GetColors(FontColorSettings::DisplayItem_Default).backColor));
}

bool CCodeEditor::LoadModule()
{
	ClearAll();
	wxDELETE(m_precompileModule);

	if (m_document != nullptr) {
		IMetaObjectModule* moduleObject = m_document->ConvertMetaObjectToType<IMetaObjectModule>();
		if (moduleObject != nullptr) {
			m_precompileModule = new CPrecompileCode(moduleObject);

			if (IsEditable()) {
				SetText(moduleObject->GetModuleText()); m_bInitialized = true;
			}
			else {
				SetReadOnly(false);
				SetText(moduleObject->GetModuleText()); m_bInitialized = true;
				SetReadOnly(true);
			}

			m_precompileModule->Load(moduleObject->GetModuleText());

			try {
				m_precompileModule->PrepareLexem();
			}
			catch (...) {
			}

			EmptyUndoBuffer();
		}

		m_fp.RecreateFoldLevel();
		RefreshEditor();
		return moduleObject != nullptr;
	}

	return m_document != nullptr;
}

bool CCodeEditor::SaveModule()
{
	if (m_document != nullptr) {

		IMetaObjectModule* moduleObject = m_document->ConvertMetaObjectToType<IMetaObjectModule>();

		if (moduleObject != nullptr) {
			moduleObject->SetModuleText(GetText());
			return true;
		}
	}

	return m_document != nullptr;
}

int CCodeEditor::GetRealPosition()
{
	const wxString& codeText = GetTextRange(0, GetCurrentPos());
	return codeText.Length();
}

int CCodeEditor::GetRealPositionFromPoint(const wxPoint& pt)
{
	const wxString& codeText = GetTextRange(0, PositionFromPoint(pt));
	return codeText.Length();
}

#include "win/dlg/lineInput/lineInput.h"
#include "win/dlg/functionSearcher/functionSearcher.h"

void CCodeEditor::RefreshEditor()
{
	CCodeEditor::SetEditorSettings(mainFrame->GetEditorSettings());
	CCodeEditor::SetFontColorSettings(mainFrame->GetFontColorSettings());

	CCodeEditor::RefreshBreakpoint();
}

#include <wx/fdrepdlg.h>

void CCodeEditor::FindText(const wxString& findString, int wxflags)
{
	int sciflags = 0;
	if ((wxflags & wxFR_WHOLEWORD) != 0) {
		sciflags |= wxSTC_FIND_WHOLEWORD;
	}
	if ((wxflags & wxFR_MATCHCASE) != 0) {
		sciflags |= wxSTC_FIND_MATCHCASE;
	}
	int result = 0;
	if ((wxflags & wxFR_DOWN) != 0) {
		CCodeEditor::SetSelectionStart(GetSelectionEnd());
		CCodeEditor::SearchAnchor();
		result = CCodeEditor::SearchNext(sciflags, findString);
	}
	else {
		CCodeEditor::SetSelectionEnd(GetSelectionStart());
		CCodeEditor::SearchAnchor();
		result = CCodeEditor::SearchPrev(sciflags, findString);
	}
	if (wxSTC_INVALID_POSITION == result) {
		wxMessageBox(wxString::Format(_("\"%s\" not found!"), findString.c_str()),
			_("Not Found!"), wxICON_ERROR, (wxWindow*)this);
	}
	else {
		CCodeEditor::EnsureCaretVisible();
		CCodeEditor::SetSTCFocus(true);
	}
}

void CCodeEditor::ShowGotoLine()
{
	CDialogLineInput* lineInput = new CDialogLineInput(this);
	const int ret = lineInput->ShowModal();
	if (ret != wxNOT_FOUND) {
		CCodeEditor::SetFocus();
		CCodeEditor::GotoLine(ret - 1);
	}
	lineInput->Destroy();
}

void CCodeEditor::ShowMethods()
{
	CFunctionList* funcList = new CFunctionList(m_document, this);
	const int ret = funcList->ShowModal();
	funcList->Destroy();
}

#include "backend/system/systemManager.h"

bool CCodeEditor::SyntaxControl(bool throwMessage) const
{
	const IMetaObject* metaObject = m_document->GetMetaObject();
	wxASSERT(metaObject);
	const IMetaData* metaData = metaObject->GetMetaData();
	wxASSERT(metaData);
	const IModuleManager* moduleManager = metaData->GetModuleManager();
	wxASSERT(moduleManager);

	IModuleDataObject* dataRef = nullptr;
	if (moduleManager->FindCompileModule(metaObject, dataRef)) {
		CCompileModule* compileModule = dataRef->GetCompileModule();
		try {
			if (compileModule->Compile()) {
				if (throwMessage)
					CSystemFunction::Message(_("No syntax errors detected!"));
				return true;
			}
			wxASSERT("CCompileCode::Compile return false");
			return false;

		}
		catch (...) {

			if (!throwMessage) {
				int answer = wxMessageBox(
					_("Errors were found while checking module. Do you want to continue ?"), compileModule->GetModuleName(),
					wxYES_NO | wxCENTRE);

				if (answer == wxNO)
					return false;
			}
		}
	}

	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef UTF8_LEXEM_TRANSLATE
#define appendStyle(style) \
CCodeEditor::StartStyling(currPos); \
CCodeEditor::SetStyling(fromPos + m_tc.GetCurrentUtf8Pos() - currPos, style);
#else
#define appendStyle(style) \
CCodeEditor::StartStyling(currPos); \
CCodeEditor::SetStyling(fromPos + m_tc.GetCurrentPos() - currPos, style);
#endif

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                          Styling                                                                       //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CCodeEditor::HighlightSyntaxAndCalculateFoldLevel(const int fromPos, const int toPos)
{
	m_tc.Load(CCodeEditor::GetTextRange(fromPos, toPos));

	//remove old styling
	CCodeEditor::StartStyling(fromPos); //from here
	CCodeEditor::SetStyling(toPos - fromPos, wxSTC_C_COMMENT); //with that length and style -> cleared

	wxString strWord;
	unsigned int currPos = fromPos;

	while (!m_tc.IsEnd()) {
#ifdef UTF8_LEXEM_TRANSLATE
		currPos = fromPos + m_tc.GetCurrentUtf8Pos();
#else 
		currPos = fromPos + m_tc.GetCurrentPos();
#endif 
		if (m_tc.IsWord()) {
			(void)m_tc.GetWord(strWord, false, true);
			const short keyWord = CTranslateCode::IsKeyWord(strWord);
			if (keyWord != wxNOT_FOUND) {
				if (strWord.Left(1) == '#') {
					appendStyle(wxSTC_C_PREPROCESSOR);
				}
				else {
					appendStyle(wxSTC_C_STRING);
				}
			}
			else {
				appendStyle(wxSTC_C_WORD);
			}
		}
		else if (m_tc.IsNumber() || m_tc.IsString() || m_tc.IsDate()) {
			if (m_tc.IsNumber()) {
				(void)m_tc.GetNumber();
				appendStyle(wxSTC_C_NUMBER);
			}
			else if (m_tc.IsString()) {
				(void)m_tc.GetString();
				appendStyle(wxSTC_C_OPERATOR);
			}
			else if (m_tc.IsDate()) {
				(void)m_tc.GetDate();
				appendStyle(wxSTC_C_OPERATOR);
			}
		}
		else {
			(void)m_tc.GetByte();
			appendStyle(wxSTC_C_IDENTIFIER);
		}
	}

	m_fp.UpdateFoldLevel();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                                          EVENT                                                                         //
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CCodeEditor::OnStyleNeeded(wxStyledTextEvent& event)
{
	/*this is called every time the styler detects a line that needs style, so we style that range.
	This will save a lot of performance since we only style text when needed instead of parsing the whole file every time.*/
	int line_start = CCodeEditor::LineFromPosition(CCodeEditor::GetEndStyled());
	int line_end = CCodeEditor::GetFirstVisibleLine() + CCodeEditor::LinesOnScreen();

	if (line_end > CCodeEditor::GetLineCount()) {
		line_end = CCodeEditor::GetLineCount() - 1;
	}

	/*fold level: May need to include the two lines in front because of the fold level these lines have- the line above
	may be affected*/
	if (line_start > 1) {
		line_start -= 2;
	}
	else {
		line_start = 0;
	}

	//if it is so small that all lines are visible, style the whole document
	if (CCodeEditor::GetLineCount() == CCodeEditor::LinesOnScreen()) {
		line_start = 0;
		line_end = CCodeEditor::GetLineCount() - 1;
	}

	if (line_end < line_start) {
		//that happens when you select parts that are in front of the styled area
		wxSwap(line_end, line_start);
	}

	//style the line following the style area too (if present) in case fold level decreases in that one
	if (line_end < CCodeEditor::GetLineCount() - 1) {
		line_end++;
	}

	//get exact start positions
	HighlightSyntaxAndCalculateFoldLevel(
		CCodeEditor::PositionFromLine(line_start),
		CCodeEditor::GetLineEndPosition(line_end)
	);

	event.Skip();
}

void CCodeEditor::OnMarginClick(wxStyledTextEvent& event)
{
	const int line_from_pos = LineFromPosition(event.GetPosition());

	switch (event.GetMargin())
	{
	case DEF_BREAKPOINT_ID: {
		const int dwFlags = CCodeEditor::MarkerGet(line_from_pos);
		if (IsEditable()) {
			//Update the list of breakpoints
			const wxString& strModuleName = m_document->GetFilename();
			if ((dwFlags & (1 << CCodeEditor::Breakpoint))) {
				if (debugClient->RemoveBreakpoint(strModuleName, line_from_pos)) {
					MarkerDelete(line_from_pos, CCodeEditor::Breakpoint);
				}
			}
			else if (debugClient->ToggleBreakpoint(strModuleName, line_from_pos)) {
				MarkerAdd(line_from_pos, CCodeEditor::Breakpoint);
			}
		}
		break;
	}
	case DEF_FOLDING_ID:
		ToggleFold(line_from_pos);
		break;
	}

	event.Skip();
}

void CCodeEditor::OnTextChange(wxStyledTextEvent& event)
{
	const int modFlags = event.GetModificationType();

	if ((modFlags & (wxSTC_MOD_INSERTTEXT)) == 0 &&
		(modFlags & (wxSTC_MOD_DELETETEXT)) == 0)
		return;

	if (m_bInitialized) {

		IMetaObjectModule* moduleObject = m_document->ConvertMetaObjectToType<IMetaObjectModule>();

		if (moduleObject != nullptr) {

			const wxString& codeText = GetText();
			const int line = LineFromPosition(event.GetPosition());

			{
				m_precompileModule->Load(codeText);

				if (event.m_linesAdded != 0) {

					debugClient->PatchBreakpointCollection(moduleObject->GetDocPath(),
						line + event.m_linesAdded, event.m_linesAdded);

					RefreshBreakpoint();
				}

				try {
#if _USE_OLD_TEXT_PARSER_IN_CODE_EDITOR == 0	
					const wxString& strPatch = event.GetString();
					const int str_length = strPatch.Length();
					const int str_utf8_length = event.GetLength();
					if ((modFlags & (wxSTC_MOD_INSERTTEXT)) != 0) {
						m_precompileModule->PrepareLexem(line,
#ifdef UTF8_LEXEM_TRANSLATE
							event.m_linesAdded, str_length, str_utf8_length);
#else
							event.m_linesAdded, str_length);
#endif
					}
					else if ((modFlags & (wxSTC_MOD_DELETETEXT)) != 0) {
						m_precompileModule->PrepareLexem(line,
#ifdef UTF8_LEXEM_TRANSLATE
							event.m_linesAdded, -str_length, str_utf8_length);
#else
							event.m_linesAdded, -str_length);
#endif
					}
#else 
					m_precompileModule->PrepareLexem();
#endif
				}
				catch (...)
				{
				}

				IMetaData* metaData = moduleObject->GetMetaData();
				wxASSERT(metaData);

				IModuleManager* moduleManager = metaData->GetModuleManager();
				wxASSERT(moduleManager);
				IModuleDataObject* pRefData = nullptr;
				if (moduleManager->FindCompileModule(m_document->GetMetaObject(), pRefData)) {
					CCompileCode* compileModule = pRefData->GetCompileModule();
					wxASSERT(compileModule);
					if (!compileModule->m_changedCode) compileModule->m_changedCode = true;
				}

				m_document->Modify(true);
				moduleObject->SetModuleText(codeText);
			}
		}

		m_fp.RecreateFoldLevel();
	}
}

void CCodeEditor::OnKeyDown(wxKeyEvent& event)
{
	if (!IsEditable()) {
		event.Skip(); return;
	}

	switch (event.GetKeyCode())
	{
	case WXK_LEFT:
		SetEmptySelection(GetCurrentPos() - 1);
		break;
	case WXK_RIGHT:
		SetEmptySelection(GetCurrentPos() + 1);
		break;
	case WXK_UP: {
		if (!event.ShiftDown()) {
			int currentPos = GetCurrentPos();
			int line = LineFromPosition(currentPos);

			int startPos = PositionFromLine(line);
			int endPos = GetLineEndPosition(line);

			int length = currentPos - startPos;

			int startNewPos = PositionFromLine(line - 1);
			int endNewPos = GetLineEndPosition(line - 1);

			if (endNewPos - startNewPos < length) {
				std::string m_stringBuffer;
				for (int c = 0; c < (length - (endNewPos - startNewPos)); c++) {
					m_stringBuffer.push_back(' ');
				}
				InsertText(endNewPos, m_stringBuffer);
			}
			SetEmptySelection(startNewPos + length);
		}
		else {
			event.Skip();
		}
		break;
	}
	case WXK_DOWN:
	{
		if (!event.ShiftDown()) {
			int currentPos = GetCurrentPos();
			int line = LineFromPosition(currentPos);

			int startPos = PositionFromLine(line);
			int endPos = GetLineEndPosition(line);

			int length = currentPos - startPos;

			int startNewPos = PositionFromLine(line + 1);
			int endNewPos = GetLineEndPosition(line + 1);

			if (endNewPos - startNewPos < length) {
				std::string m_stringBuffer;
				for (int c = 0; c < (length - (endNewPos - startNewPos)); c++) {
					m_stringBuffer.push_back(' ');
				}

				InsertText(endNewPos, m_stringBuffer);
			}
			SetEmptySelection(startNewPos + length);
		}
		else {
			event.Skip();
		}
		break;
	}
	case WXK_NUMPAD_ENTER:
	case WXK_RETURN: PrepareTABs(); break;
	case ' ': if (m_bEnableAutoComplete && event.ControlDown()) LoadAutoComplete(); event.Skip(); break;
	case '9': if (m_bEnableAutoComplete && event.ShiftDown()) LoadCallTip(); event.Skip(); break;
	case '0': if (m_bEnableAutoComplete && event.ShiftDown()) m_ct.Cancel(); event.Skip(); break;

	case WXK_F8:
	{
		const int line_from_pos = LineFromPosition(GetCurrentPos());
		if (IsEditable()) {
			//Update the list of breakpoints
			const wxString& strModuleName = m_document->GetFilename();
			if ((CCodeEditor::MarkerGet(line_from_pos) & (1 << CCodeEditor::Breakpoint))) {
				if (debugClient->RemoveBreakpoint(strModuleName, line_from_pos)) {
					MarkerDelete(line_from_pos, CCodeEditor::Breakpoint);
				}
			}
			else if (debugClient->ToggleBreakpoint(strModuleName, line_from_pos)) {
				MarkerAdd(line_from_pos, CCodeEditor::Breakpoint);
			}
		}
	}
	break;
	default: event.Skip(); break;
	}
}
