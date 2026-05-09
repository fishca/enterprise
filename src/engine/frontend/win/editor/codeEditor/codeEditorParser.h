#ifndef __IB_CODE_EDITOR_PARSER_H__
#define __IB_CODE_EDITOR_PARSER_H__

#include "codeEditor.h"

enum ibContentType
{
	eVariable = 0,
	eExportVariable,
	eProcedure,
	eExportProcedure,
	eFunction,
	eExportFunction,
	eLambda,           // anonymous Function/Procedure used as expression —
	                   // no name, navigable by line range. m_name carries
	                   // a synthetic label like "<lambda @ line 23>" for
	                   // listing in symbol pickers; declaration kind
	                   // (function vs procedure) is encoded in m_imageIndex.

	eEmpty
};

struct ibModuleElement
{
	wxString      m_name;            // element identifier (function / procedure / variable name)
	wxString      m_shortDescription;  // object kind label (one-line tooltip)

	int           m_imageIndex = 0;    // icon index in the autocomplete image list
	int           m_lineStart  = -1;   // first source line where the element appears
	int           m_lineEnd    = -1;   // last source line where the element appears

	wxString      m_moduleName;        // owning module name
	ibContentType m_eType      = eEmpty;
};

class FRONTEND_API ibParserModule : public ibTranslateCode
{
	int                          m_cursor = wxNOT_FOUND;  // current position in the lexem array
	std::vector<ibModuleElement> m_content;

protected:

	const ibLexem& PreviewGetLexem();
	const ibLexem& GetLexem();
	const ibLexem& ExpectLexem();
	void           ExpectDelimeter(const wxUniChar& c);

	bool           IsNextDelimeter(const wxUniChar& c);
	bool           IsNextKeyWord(int keyword);
	void           ExpectKeyword(int keyword);
	wxString       ExpectIdentifier(bool strRealName = false);
	ibValue        ExpectConstant();

public:

	ibParserModule();
	bool ParseModule(const wxString& sModule);

	// Module elements collected by ParseModule — list of every
	// procedure / function / variable declaration found in the source.
	// Callers iterate and filter themselves; the dedicated GetVariables
	// / GetFunctions / GetProcedures helpers were unreachable and got
	// removed.
	std::vector<ibModuleElement>& GetAllContent() { return m_content; }
};

#endif 