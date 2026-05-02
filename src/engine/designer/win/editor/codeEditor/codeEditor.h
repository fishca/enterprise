#ifndef __IB_CODE_EDITOR_H__
#define __IB_CODE_EDITOR_H__

#include <wx/docview.h>
#include <wx/cmdproc.h>
#include <wx/listctrl.h>
#include <wx/stc/stc.h>

#include <vector>
#include <array>
#include <map>

#include "codeEditorInterpreter.h"

#include "frontend/mainFrame/settings/editorsettings.h"
#include "frontend/mainFrame/settings/fontcolorsettings.h"

#include "backend/debugger/debugDefs.h"

#include "win/editor/codeEditor/components/autoComplete.h"
#include "win/editor/codeEditor/components/callTip.h"

class ibMetaDocument;
class ibReaderMemory;

// Forwards wxCommandProcessor's undo/redo stack to the underlying
// wxStyledTextCtrl — STC owns its own undo buffer, but wxDocument
// expects a wxCommandProcessor on the document side.
class ibModuleCommandProcessor : public wxCommandProcessor {
	wxStyledTextCtrl* m_codeEditor = nullptr;
public:
	explicit ibModuleCommandProcessor(wxStyledTextCtrl* codeEditor)
		: m_codeEditor(codeEditor) {}

	bool Undo() override        { m_codeEditor->Undo(); return true; }
	bool Redo() override        { m_codeEditor->Redo(); return true; }
	bool CanUndo() const override { return m_codeEditor->CanUndo(); }
	bool CanRedo() const override { return m_codeEditor->CanRedo(); }
};

///////////////////////////////////////////////////////////////////////////

#define wxSTC_FOLDLEVELBASE_FLAG 0x400

#define wxSTC_FOLDLEVELWHITE_FLAG 0x1000
#define wxSTC_FOLDLEVELHEADER_FLAG 0x2000
#define wxSTC_FOLDLEVELELSE_FLAG 0x4000

///////////////////////////////////////////////////////////////////////////

class ibCodeEditor : public wxStyledTextCtrl {

	enum {
		Breakpoint = 1,
		CurrentLine,
		BreakLine,
	};

	class ibFoldLevelParser {

		enum FoldKind : short {
			FoldProcedure = 0,
			FoldFunction,
			FoldIf,
			FoldDo,
			FoldTry,
			FoldKindCount
		};

		struct FoldPoint {
			int   line;   // source line m_number
			short delta;  // +1 open, 0 mark (Else/ElseIf/Except), -1 close
		};

	public:

		ibFoldLevelParser(ibCodeEditor* codeEditor)
			: m_codeEditor(codeEditor), m_update_fold(false), m_counts{} {}

		void RecalcFoldLevel() const { m_update_fold = true; }

		void UpdateFoldLevel() {

			if (!m_update_fold)
				return;

			CalcFoldLevel();

			// Detect a tail of opens that never close (incomplete code while
			// the user is still typing) and skip them so half-finished folds
			// don't drag the whole tail of the file into a phantom region.
			auto hint_ignore_fold = m_folding_vector.end();
			if (!m_folding_vector.empty()) {
				int unclosed = 0;
				for (const auto& v : m_folding_vector) unclosed += v.delta;
				while (hint_ignore_fold != m_folding_vector.begin()) {
					if (unclosed == 0) break;
					hint_ignore_fold = std::prev(hint_ignore_fold);
					if (hint_ignore_fold->delta > 0) unclosed--;
				}
			}

			int level = 0, fold_start = 0;

			for (auto it = m_folding_vector.begin(); it != m_folding_vector.end(); ++it) {

				if (hint_ignore_fold != m_folding_vector.end() && it >= hint_ignore_fold && it->delta > 0)
					continue;

				for (int l = fold_start + 1; l <= it->line - 1; l++)
					m_codeEditor->SetFoldLevel(l, wxSTC_FOLDLEVELBASE_FLAG + std::max(level, 0));

				if (it->delta > 0)
					m_codeEditor->SetFoldLevel(it->line, wxSTC_FOLDLEVELBASE_FLAG + std::max(level, 0) | wxSTC_FOLDLEVELHEADER_FLAG);
				else if (it->delta < 0)
					m_codeEditor->SetFoldLevel(it->line, wxSTC_FOLDLEVELBASE_FLAG + std::max(level, 0) | wxSTC_FOLDLEVELWHITE_FLAG);
				else
					m_codeEditor->SetFoldLevel(it->line, wxSTC_FOLDLEVELBASE_FLAG + std::max(level, 0));

				level += it->delta;
				fold_start = it->line;
			}

			for (int l = fold_start + 1; l <= m_codeEditor->GetLineCount(); l++)
				m_codeEditor->SetFoldLevel(l, wxSTC_FOLDLEVELBASE_FLAG + std::max(level, 0));

			m_update_fold = false;
		}

		int GetFoldMask(int line) const {
			int level = 0; short flag = 0; int prev_line = -1;
			for (const auto& v : m_folding_vector) {

				if (v.line > line)
					break;

				if (prev_line != v.line) {

					if (v.line == line && v.delta > 0)
						flag = wxSTC_FOLDLEVELHEADER_FLAG;
					else if (v.line == line && v.delta < 0)
						flag = wxSTC_FOLDLEVELWHITE_FLAG;
					else if (v.line == line && v.delta == 0)
						flag = wxSTC_FOLDLEVELELSE_FLAG;

					level += v.delta;
				}

				prev_line = v.line;
			}

			if (level < 0) level = 0;

			if ((flag & wxSTC_FOLDLEVELHEADER_FLAG) != 0)
				return wxSTC_FOLDLEVELBASE_FLAG + (level - 1) | flag;
			else if ((flag & wxSTC_FOLDLEVELWHITE_FLAG) != 0)
				return wxSTC_FOLDLEVELBASE_FLAG + (level + 1) | flag;

			return wxSTC_FOLDLEVELBASE_FLAG + level | flag;
		}

	private:

		void OpenFold(const int line, FoldKind kind) {
			m_counts[kind]++;
			m_folding_vector.push_back({ line, +1 });
		}

		void MarkFold(const int line) {
			m_folding_vector.push_back({ line, 0 });
		}

		void CloseFold(const int line, FoldKind kind) {
			if (m_counts[kind] > 0) {
				m_folding_vector.push_back({ line, -1 });
				m_counts[kind]--;
			}
		}

		void ClearFoldLevel() {
			m_counts.fill(0);
			m_folding_vector.clear();
		}

		// Map a script keyword to a fold-kind. Returns FoldKindCount if the
		// keyword does not open/close a fold region.
		static FoldKind OpenKindFor(short numData) {
			switch (numData) {
				case KEY_PROCEDURE: return FoldProcedure;
				case KEY_FUNCTION:  return FoldFunction;
				case KEY_IF:        return FoldIf;
				case KEY_FOR:
				case KEY_FOREACH:
				case KEY_WHILE:     return FoldDo;
				case KEY_TRY:       return FoldTry;
				default:            return FoldKindCount;
			}
		}

		static FoldKind CloseKindFor(short numData) {
			switch (numData) {
				case KEY_ENDPROCEDURE: return FoldProcedure;
				case KEY_ENDFUNCTION:  return FoldFunction;
				case KEY_ENDIF:        return FoldIf;
				case KEY_ENDDO:        return FoldDo;
				case KEY_ENDTRY:       return FoldTry;
				default:               return FoldKindCount;
			}
		}

		static bool IsMarkKeyword(short numData) {
			return numData == KEY_ELSE
				|| numData == KEY_ELSEIF
				|| numData == KEY_EXCEPT;
		}

		void CalcFoldLevel() {

			ClearFoldLevel();

			const auto& lexems = m_codeEditor->m_precompileModule->GetLexems();
			for (const ibLexem& lex : lexems) {

				if (lex.m_lexType != KEYWORD)
					continue;

				if (FoldKind k = OpenKindFor(lex.m_numData); k != FoldKindCount) {
					OpenFold(lex.m_numLine, k);
					continue;
				}
				if (FoldKind k = CloseKindFor(lex.m_numData); k != FoldKindCount) {
					CloseFold(lex.m_numLine, k);
					continue;
				}
				if (IsMarkKeyword(lex.m_numData))
					MarkFold(lex.m_numLine);
			}
		}

	private:

		// Recalc-pending flag. Set by RecalcFoldLevel (cheap, lazy);
		// consumed by UpdateFoldLevel on the next paint.
		mutable bool m_update_fold;

		// Per-kind open-fold counters — protect against unbalanced
		// close-tokens emitting spurious -1 deltas.
		std::array<short, FoldKindCount> m_counts;

		// Owning editor (non-owning back-pointer).
		ibCodeEditor* m_codeEditor;

		// Sorted-by-line list of fold transitions populated by
		// CalcFoldLevel — consumed by UpdateFoldLevel and GetFoldMask.
		std::vector<FoldPoint> m_folding_vector;
	};

public:

	ibCodeEditor();
	ibCodeEditor(ibMetaDocument* document, wxWindow* parent, wxWindowID id = wxID_ANY,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = 0,
		const wxString& strName = wxSTCNameStr);
	virtual ~ibCodeEditor();

	bool LoadModule();
	bool SaveModule();

	int GetRealPosition();
	int GetRealPositionFromPoint(const wxPoint& pt);

	void RefreshEditor();
	void ActivateEditor();

	void FindText(const wxString& findString, int wxflags);

	void ShowGotoLine();
	void ShowMethods();

	// Re-indent the lines covered by the current selection (or the
	// caret line if nothing selected) using the same fold-mask logic
	// as PrepareTABs. Wrapped in BeginUndoAction / EndUndoAction so
	// the user can undo the whole reformat with one Ctrl+Z.
	void FormatSelection();

	// Selection-aware indent / unindent — mirror of Tab / Shift+Tab
	// keystroke behaviour, exposed as a menu / toolbar command for
	// users who don't recall the shortcut.
	void IncreaseIndent();
	void DecreaseIndent();

	void SetCurrentLine(int line, bool setLine = true);
	bool SyntaxControl(bool throwMessage = true) const;

	//Update breakpoints 
	void RefreshBreakpoint(bool deleteCurrentBreakline = false);

	//Editor setting 
	void SetEditorSettings(const ibEditorSettings& settings);

	//Font setting 
	void SetFontColorSettings(const ibFontColorSettings& settings);

	void ShowAutoComplete(
		const ibDebugAutoCompleteData& autoCompleteData);

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
	void AddKeywordFromObject(const ibValue& vObject);

	bool PrepareExpression(unsigned int currPos, wxString& expression, wxString& keyword, wxString& currentWord, bool& hasPoint);
	void PrepareTooTipExpression(unsigned int currPos, wxString& expression, wxString& currentWord, bool& hasPoint);

	void PrepareTABs();

	void LoadSysKeyword();
	void LoadIntelliList();
	void LoadFromKeyWord(const wxString& keyWord);

	void LoadAutoComplete();
	void LoadToolTip(const wxPoint& pos);
	void LoadCallTip();

	//Support styling 
	void HighlightSyntaxAndCalculateFoldLevel(const int fromPos, const int toPos);

	//Support debugger 
	void EditDebugPoint(int line);

	ibMetaDocument*   m_document         = nullptr;
	ibPrecompileCode* m_precompileModule = nullptr;

	ibAutoComplete    m_ac;
	ibCallTip         m_ct;
	ibTranslateCode   m_tc;
	ibFoldLevelParser m_fp;

	bool m_initialized        = false;
	int  m_indentationSize    = 0;
	bool m_enableAutoComplete = false;

	int  m_lineBreakpoint     = wxNOT_FOUND;
};

#endif 