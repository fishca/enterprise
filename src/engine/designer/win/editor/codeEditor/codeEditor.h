#ifndef _AUTOCOMPLETIONCTRL_H__
#define _AUTOCOMPLETIONCTRL_H__

#include <wx/docview.h>
#include <wx/cmdproc.h>
#include <wx/listctrl.h>
#include <wx/stc/stc.h>

#include <vector>
#include <map>

#include "codeEditorInterpreter.h"

#include "frontend/mainFrame/settings/editorsettings.h"
#include "frontend/mainFrame/settings/fontcolorsettings.h"

#include "backend/debugger/debugDefs.h"

#include "win/editor/codeEditor/components/autoComplete.h"
#include "win/editor/codeEditor/components/callTip.h"

class CMetaDocument;
class CMemoryReader;

class CModuleCommandProcessor : public wxCommandProcessor {
	wxStyledTextCtrl* m_codeEditor;
public:

	CModuleCommandProcessor(wxStyledTextCtrl* codeEditor) :
		wxCommandProcessor(), m_codeEditor(codeEditor) {
	}

	virtual bool Undo() override {
		m_codeEditor->Undo();
		return true;
	}

	virtual bool Redo() override {
		m_codeEditor->Redo();
		return true;
	}

	virtual bool CanUndo() const override {
		return m_codeEditor->CanUndo();
	}

	virtual bool CanRedo() const override {
		return m_codeEditor->CanRedo();
	}
};

///////////////////////////////////////////////////////////////////////////

#define wxSTC_FOLDLEVELBASE_FLAG 0x400

#define wxSTC_FOLDLEVELWHITE_FLAG 0x1000
#define wxSTC_FOLDLEVELHEADER_FLAG 0x2000
#define wxSTC_FOLDLEVELELSE_FLAG 0x4000

///////////////////////////////////////////////////////////////////////////

class CCodeEditor : public wxStyledTextCtrl {

	enum {
		Breakpoint = 1,
		CurrentLine,
		BreakLine,
	};

	class CFoldLevelParser {
	public:

		CFoldLevelParser(CCodeEditor* codeEditor) : m_codeEditor(codeEditor), m_update_fold(false) {}

		void RecreateFoldLevel() const { m_update_fold = true; }
		void UpdateFoldLevel() {

			if (m_update_fold) {
				CalcFoldLevel();
				int level = 0;
				const unsigned int line_count = m_codeEditor->GetLineCount();
				for (unsigned int l = 0; l <= line_count; l++) {

					const int curr_level = GetFoldLevel(l);

					int unclosed = 0, open = 0;
					for (unsigned int idx = 0; idx < m_folding_vector.size(); idx++) {
						const std::pair<size_t, int>& v = m_folding_vector.at(idx);
						unclosed += v.second;
						if (v.first < l && unclosed == 0)
							open = 0;
						if (v.second > 0) open++;
					}

					bool allowed = true;

					if (curr_level > 0) { allowed = (open - unclosed) > level; }
					else if (curr_level < 0) { allowed = level > 0; }

					if (allowed && curr_level > 0)
						m_codeEditor->SetFoldLevel(l, wxSTC_FOLDLEVELBASE_FLAG + std::max(level, 0) | wxSTC_FOLDLEVELHEADER_FLAG);
					else if (allowed && curr_level < 0)
						m_codeEditor->SetFoldLevel(l, wxSTC_FOLDLEVELBASE_FLAG + std::max(level, 0) | wxSTC_FOLDLEVELWHITE_FLAG);
					else
						m_codeEditor->SetFoldLevel(l, wxSTC_FOLDLEVELBASE_FLAG + std::max(level, 0));

					if (allowed) level += curr_level;
				}

				m_update_fold = false;
			}
		}

		int GetFoldMask(unsigned int line) const {
			int level = 0; short flag = 0; const size_t fold_size = m_folding_vector.size();
			for (unsigned int idx = 0; idx < fold_size; idx++) {
				const auto& v = m_folding_vector[idx];
				if (v.first > line)
					break;
				if (v.first == line && v.second > 0)
					flag = wxSTC_FOLDLEVELHEADER_FLAG;
				else if (v.first == line && v.second < 0)
					flag = wxSTC_FOLDLEVELWHITE_FLAG;
				else if (v.first == line && v.second == 0)
					flag = wxSTC_FOLDLEVELELSE_FLAG;

				level += v.second;
			}

			if (level < 0) level = 0;

			if ((flag & wxSTC_FOLDLEVELHEADER_FLAG) != 0)
				return wxSTC_FOLDLEVELBASE_FLAG + (level - 1) | flag;
			else if ((flag & wxSTC_FOLDLEVELWHITE_FLAG) != 0)
				return wxSTC_FOLDLEVELBASE_FLAG + (level + 1) | flag;

			return wxSTC_FOLDLEVELBASE_FLAG + level | flag;
		}

	private:

		void OpenFold(const int currentLine) { m_folding_vector.emplace_back(currentLine, +1); }
		void MarkFold(const int currentLine) { m_folding_vector.emplace_back(currentLine, 0); }
		void CloseFold(const int currentLine) { m_folding_vector.emplace_back(currentLine, -1); }

		void ClearFoldLevel() { m_folding_vector.clear(); }

		short GetFoldLevel(const int l) const {
			short foldLevel = 0; const size_t fold_size = m_folding_vector.size();
			for (unsigned int idx = 0; idx < fold_size; idx++) {
				const auto& v = m_folding_vector[idx];
				if (v.first == l) foldLevel += v.second;
			}

			return foldLevel;
		}

		void CalcFoldLevel() {

			ClearFoldLevel();

			for (unsigned int idx = 0; idx < m_codeEditor->m_precompileModule->m_listLexem.size(); idx++) {
				const CLexem& lex = m_codeEditor->m_precompileModule->m_listLexem[idx];
				if (lex.m_lexType == KEYWORD) {

					if (lex.m_numData == KEY_PROCEDURE
						|| lex.m_numData == KEY_FUNCTION
						|| lex.m_numData == KEY_IF
						|| lex.m_numData == KEY_FOR
						|| lex.m_numData == KEY_FOREACH
						|| lex.m_numData == KEY_WHILE
						|| lex.m_numData == KEY_TRY
						)
					{
						OpenFold(lex.m_numLine);
					}
					else if (
						lex.m_numData == KEY_ELSE
						|| lex.m_numData == KEY_ELSEIF
						|| lex.m_numData == KEY_EXCEPT
						)
					{
						MarkFold(lex.m_numLine);
					}
					else if (
						lex.m_numData == KEY_ENDPROCEDURE
						|| lex.m_numData == KEY_ENDFUNCTION
						|| lex.m_numData == KEY_ENDIF
						|| lex.m_numData == KEY_ENDDO
						|| lex.m_numData == KEY_ENDTRY
						)
					{
						CloseFold(lex.m_numLine);
					}
				}
			}
		}

	private:

		// calculate flag
		mutable bool m_update_fold;

		// current code editor
		CCodeEditor* m_codeEditor;

		// build a vector to include line and folding level
		std::vector<std::pair<size_t, int>> m_folding_vector;
	};

public:

	CCodeEditor();
	CCodeEditor(CMetaDocument* document, wxWindow* parent, wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = 0,
		const wxString& strName = wxSTCNameStr);
	virtual ~CCodeEditor();

	bool LoadModule();
	bool SaveModule();

	int GetRealPosition();
	int GetRealPositionFromPoint(const wxPoint& pt);

	void RefreshEditor();

	void FindText(const wxString& findString, int wxflags);

	void ShowGotoLine();
	void ShowMethods();

	void SetCurrentLine(int line, bool setLine = true);
	bool SyntaxControl(bool throwMessage = true) const;

	//Update breakpoints 
	void RefreshBreakpoint(bool bDeleteCurrentBreakline = false);

	//Editor setting 
	void SetEditorSettings(const EditorSettings& settings);

	//Font setting 
	void SetFontColorSettings(const FontColorSettings& settings);

	void ShowAutoComplete(
		const CDebugAutoCompleteData& autoCompleteData);

	void ShowCallTip(const wxString& title) { m_ct.Show(GetRealPosition(), title); }

	// hook the document manager into event handling chain here
	virtual bool TryBefore(wxEvent& event) override {
		wxEventType type = event.GetEventType();
		if (type == wxEVT_PAINT ||
			type == wxEVT_NC_PAINT ||
			type == wxEVT_ERASE_BACKGROUND) {
			return wxStyledTextCtrl::TryBefore(event);
		}
		if (m_ct.Active()) {
			if (type == wxEVT_KEY_DOWN) {
				m_ct.CallEvent(event); return wxStyledTextCtrl::TryBefore(event);
			}
		}

		if (m_ac.CallEvent(event))
			return true;
		if (m_ct.CallEvent(event))
			return true;
		else return wxStyledTextCtrl::TryBefore(event);
	}

protected:

	// Events
	void OnStyleNeeded(wxStyledTextEvent& event);
	void OnMarginClick(wxStyledTextEvent& event);
	void OnTextChange(wxStyledTextEvent& event);

	void OnKeyDown(wxKeyEvent& event);
	void OnMouseMove(wxMouseEvent& event) {
		LoadToolTip(event.GetPosition());
		event.Skip();
	}

private:

	// Private func
	void AddKeywordFromObject(const CValue& vObject);

	bool PrepareExpression(unsigned int currPos, wxString& strexpression, wxString& strKeyWord, wxString& sCurrWord, bool& hasPoint);
	void PrepareTooTipExpression(unsigned int currPos, wxString& strexpression, wxString& sCurrWord, bool& hasPoint);

	void PrepareTABs();

	void LoadSysKeyword();
	void LoadIntelliList();
	void LoadFromKeyWord(const wxString& keyWord);

	void LoadAutoComplete();
	void LoadToolTip(const wxPoint& pos);
	void LoadCallTip();

	//Support styling 
	void HighlightSyntaxAndCalculateFoldLevel(
		const int fromLine, const int toLine,
		const int fromPos, const int toPos,
		const wxString& strCode
	);

	//Support debugger 
	void EditDebugPoint(int line);

	CMetaDocument* m_document;
	CPrecompileCode* m_precompileModule;

	CAutoComplete m_ac;
	CCallTip m_ct;
	CTranslateCode m_tc;
	CFoldLevelParser m_fp;

	bool m_bInitialized;
	int  m_bIndentationSize;
	bool m_bEnableAutoComplete;

	std::map<wxString, wxString> m_expressions;
};

#endif 