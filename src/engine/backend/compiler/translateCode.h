#ifndef __TRANSLATE_CODE_H__
#define __TRANSLATE_CODE_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <map>
#include <vector>

#include "backend/backend_exception.h"

#include "codeDef.h"
#include "value.h"

//List of keywords
struct ÑKeyWords {
	wxString m_strKeyWord;
	wxString m_strShortDescription;
};

extern BACKEND_API struct ÑKeyWords s_listKeyWord[];

enum {
	LEXEM_ADD = 0,
	LEXEM_ADDDEF,
	LEXEM_IGNORE,
};

//Function properties:
enum {
	RETURN_NONE = 0,//no return (module code)
	RETURN_PROCEDURE,//return from procedure
	RETURN_FUNCTION,//return from function
};

//variable flags (specified with a negative value in the nArray attribute of the bytecode)
enum {
	DEF_VAR_SKIP = -1,// missing parameter
	DEF_VAR_DEFAULT = -2,//default parameter
	DEF_VAR_TEMP = -3,//flag of a temporary local variable
	DEF_VAR_NORET = -7,//function (procedure) does not return values
	DEF_VAR_CONST = 1000,//loading constants

};

//definitions

//storing one primitive from the source code
struct CLexem {

	//lexem type:
	short m_lexType;

	//lexem content:
	short m_numData;			// keyword number (KEYWORD) or delimiter symbol (DELIMITER)
	wxString m_strData;			// or identifier name (variable, function, etc.)
	CValue m_valData;			// value, if it is a constant or real identifier name

	//additional information:
	wxString m_strModuleName;	// module name (since it is possible to include connections from different modules)
	wxString m_strDocPath;		// unique path to the document
	wxString m_strFileName;		// file path (if external processing)

	unsigned int m_numLine;		//source line number (for breakpoints)
	unsigned int m_numString;	//source text number (for error output)

public:

	unsigned int GetLine() const { return m_numLine + 1; }

	unsigned int GetLength() const {
		if (m_lexType == DELIMITER)
			return 1;
		else if (m_lexType == IDENTIFIER)
			return m_strData.length();
		else if (m_lexType == CONSTANT && m_valData.GetType() == eValueTypes::TYPE_DATE)
			return m_strData.length() + 2;
		else if (m_lexType == CONSTANT && m_valData.GetType() == eValueTypes::TYPE_STRING)
			return m_strData.length() + 2;
		else if (m_lexType == CONSTANT)
			return m_strData.length();
		else if (m_lexType == KEYWORD)
			return m_strData.length();
		return 0;
	}

	unsigned int StartPos() const { return m_numString; }
	unsigned int EndPos() const { return m_numString + GetLength(); }

	//Constructor:
	CLexem() :
		m_lexType(0),
		m_numData(0),
		m_numString(0),
		m_numLine(0)
	{
	}

	CLexem(const CLexem& src) :
		m_lexType(src.m_lexType),
		m_numData(src.m_numData),
		m_numString(src.m_numString),
		m_numLine(src.m_numLine),

		m_valData(src.m_valData),
		m_strData(src.m_strData),

		m_strModuleName(src.m_strModuleName),
		m_strDocPath(src.m_strDocPath),
		m_strFileName(src.m_strFileName)
	{
	}

	CLexem(CLexem&& src) :
		m_lexType(src.m_lexType),
		m_numData(src.m_numData),
		m_numString(src.m_numString),
		m_numLine(src.m_numLine),

		m_valData(std::move(src.m_valData)),
		m_strData(std::move(src.m_strData)),

		m_strModuleName(std::move(src.m_strModuleName)),
		m_strDocPath(std::move(src.m_strDocPath)),
		m_strFileName(std::move(src.m_strFileName))
	{

		src.m_lexType = 0;
		src.m_numData = 0;
		src.m_numString = 0;
		src.m_numLine = 0;
	}

	CLexem& operator =(const CLexem& src) {
		m_lexType = src.m_lexType;
		m_numData = src.m_numData;
		m_numString = src.m_numString;
		m_numLine = src.m_numLine;

		m_valData = src.m_valData;
		m_strData = src.m_strData;

		m_strModuleName = src.m_strModuleName;
		m_strDocPath = src.m_strDocPath;
		m_strFileName = src.m_strFileName;

		return *this;
	}

	CLexem& operator =(CLexem&& src)
	{
		m_lexType = src.m_lexType;
		m_numData = src.m_numData;
		m_numString = src.m_numString;
		m_numLine = src.m_numLine;

		m_valData = std::move(src.m_valData);
		m_strData = std::move(src.m_strData);

		m_strModuleName = std::move(src.m_strModuleName);
		m_strDocPath = std::move(src.m_strDocPath);
		m_strFileName = std::move(src.m_strFileName);

		src.m_lexType = 0;
		src.m_numData = 0;
		src.m_numString = 0;
		src.m_numLine = 0;

		return *this;
	}
};

typedef std::vector<CLexem> CLexemList;

/***************************************************
CTranslateCode-stage of source code parsing
The entry point is the Load() and TranslateModule() procedures.
The first procedure initializes variables and loads
the text of the executable code, the second procedure performs translation
(parsing the code). As a result, an array of "raw" bytecode in the cByteCode variable is filled in the class structure.
****************************************************/

class BACKEND_API CTranslateCode {

	//class for storing user definitions
	class CDefineList {
		CDefineList* m_parentDefine;
		std::map<wxString, CLexemList*> m_defineList;//contains arrays of lexemes
	public:

		CDefineList() : m_parentDefine(nullptr) {};
		~CDefineList() { Clear(); }

		void Clear() { m_defineList.clear(); }

		void SetParent(CDefineList* parent) {
			m_parentDefine = parent;
		}

		void RemoveDef(const wxString& strName);
		bool HasDefine(const wxString& strName) const;
		CLexemList* GetDefine(const wxString& strName);
		void SetDefine(const wxString& strName, CLexemList*);
		void SetDefine(const wxString& strName, const wxString& strValue);
	};

	static CDefineList ms_listDefine;

public:

	CTranslateCode();
	CTranslateCode(const wxString& strModuleName, const wxString& strDocPath);
	CTranslateCode(const wxString& strFileName);

	virtual ~CTranslateCode();

	bool HasDefine(const wxString& strName) const {
		if (m_defineList != nullptr)
			return m_defineList->HasDefine(strName);
		return false;
	};

	//methods:
	void Load(const wxString& strCode);

	void AppendModule(CTranslateCode* module);
	void RemoveModule(CTranslateCode* module);

	virtual void OnSetParent(CTranslateCode* setParent);

	virtual void Clear();

	void ClearLexem() {
		m_listLexem.clear();
		m_listTranslateCode.clear();
	}

	bool PrepareLexem();

protected:
	void SetError(int codeError, unsigned int currPos, const wxString& errorDesc = wxEmptyString) const;
	void SetError(int codeError,
		const wxString& strFileName, const wxString& strModuleName, const wxString& strDocPath,
		int currPos, int currLine,
		const wxString& errorDesc = wxEmptyString) const
	{
		DoSetError(codeError,
			strFileName, strModuleName, strDocPath,
			currPos, currLine, errorDesc
		);
	}
public:

	inline void SkipSpaces() const;

	bool IsByte(const wxUniChar& c) const;
	wxUniChar GetByte() const;

	bool IsWord() const;
#pragma region get_word
	bool GetWord(bool realName = false, bool get_point = false) const { return GetWord(nullptr, nullptr, realName, get_point); }
	bool GetWord(wxString& strWord, bool realName = false, bool get_point = false) const { return GetWord(&strWord, nullptr, realName, get_point); }
	bool GetWord(wxString& strWord, wxString& strRealName, bool realName = false, bool get_point = false) const { return GetWord(&strWord, &strRealName, realName, get_point); }
	bool GetWord(wxString* strWord, wxString* strRealName, bool realName, bool get_point) const;
#pragma endregion  

	bool IsNumber() const;
#pragma region get_number
	bool GetNumber() const { return GetNumber(nullptr); }
	bool GetNumber(wxString& strNumber) const { return GetNumber(&strNumber); }
	bool GetNumber(wxString* strNumber) const;
#pragma endregion  

	bool IsString() const;
#pragma region get_string
	bool GetString() const { return GetString(nullptr); }
	bool GetString(wxString& strString) const { return GetString(&strString); }
	bool GetString(wxString* strString) const;
#pragma endregion  

	bool IsDate() const;
#pragma region get_date
	bool GetDate() const { return GetDate(nullptr); }
	bool GetDate(wxString& strDate) const { return GetDate(&strDate); }
	bool GetDate(wxString* strDate) const;
#pragma endregion 

	bool IsEnd() const;

	static int IsKeyWord(const wxString& sKeyWord);
	static wxString GetKeyWord(int keyword);

	wxString GetStrToEndLine() const;
	void PrepareFromCurrent(int nMode, const wxString& strName = wxEmptyString);

	wxString GetModuleName() const { return m_strModuleName; }

	unsigned int GetBufferSize() const { return m_strBuffer.size(); }
	unsigned int GetCurrentPos() const { return m_currentPos; }
	unsigned int GetCurrentLine() const { return m_currentLine; }

public:

	static std::map<wxString, void*> ms_listHashKeyWord;
	static void LoadKeyWords();

protected:

	virtual void DoSetError(int codeError,
		const wxString& strFileName, const wxString& strModuleName, const wxString& strDocPath,
		unsigned int currPos, unsigned int currLine,
		const wxString& errorDesc = wxEmptyString) const
	{
	}

	size_t CalcAllocSize() const;

	//current lexem
	CLexem m_current_lex;

	//methods and variables for text parsing
	std::vector<CTranslateCode*> m_listTranslateCode;

	//Support for "defines":
	CDefineList* m_defineList;

	bool m_bAutoDeleteDefList;
	int m_nModePreparing;

	//attributes:
	wxString m_strModuleName;//name of the compiled module (to display information in case of errors)
	wxString m_strDocPath; // unique path to the document
	wxString m_strFileName; // path to the file (if external processing)

	unsigned int m_bufferSize;//size of the original text

	//original and upper text :
	wxStringImpl m_strBuffer, m_strBUFFER;

	mutable unsigned int m_currentPos; //current position of the processed text
	mutable unsigned int m_currentLine; //current line of the processed text

	//intermediate array with lexemes:
	std::vector<CLexem> m_listLexem;
};
#endif