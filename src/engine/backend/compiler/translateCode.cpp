////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, 2С-team
//	Description : translate module 
////////////////////////////////////////////////////////////////////////////

#include "translateCode.h"

//////////////////////////////////////////////////////////////////////
//                           Constants
//////////////////////////////////////////////////////////////////////

static std::map<wxString, void*> s_listHelpDescription; //description of keywords and system functions
static std::map<wxString, void*> s_listHashKeyword;

CTranslateCode::CDefineList CTranslateCode::ms_listDefine; //глобальный массив определений
std::map<wxString, void*>	CTranslateCode::ms_listHashKeyWord;//список ключевых слов

//////////////////////////////////////////////////////////////////////
// Global array
//////////////////////////////////////////////////////////////////////
struct СKeyWords s_listKeyWord[] =
{
	{"if"},
	{"then"},
	{"else"},
	{"elseif"},
	{"endif"},
	{"for"},
	{"foreach"},
	{"to"},
	{"in"},
	{"do"},
	{"enddo"},
	{"while"},
	{"goto"},
	{"not"},
	{"and"},
	{"or"},
	{"procedure"},
	{"endprocedure"},
	{"function"},
	{"endfunction"},
	{"export"},
	{"val"},
	{"return"},
	{"try"},
	{"except"},
	{"endtry"},
	{"continue"},
	{"break"},
	{"raise"},
	{"var"},

	//create object
	{"new"},

	//undefined type
	{"undefined"},

	//null type
	{"null"},

	//boolean type
	{"true"},
	{"false"},

	//preprocessor:
	{"#define"},
	{"#undef"},
	{"#ifdef"},
	{"#ifndef"},
	{"#else"},
	{"#endif"},
	{"#region"},
	{"#endregion"},
};

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

void CTranslateCode::CDefineList::RemoveDef(const wxString& strName)
{
	m_defineList.erase(stringUtils::MakeUpper(strName));
}

bool CTranslateCode::CDefineList::HasDefine(const wxString& strName) const
{
	auto it = m_defineList.find(stringUtils::MakeUpper(strName));
	if (it != m_defineList.end())
		return true;

	static int nLevel = 0;
	nLevel++;
	if (nLevel > MAX_OBJECTS_LEVEL) {
		CBackendException::Error(_("Recursive module call (#3)"));
	}

	//ищем в родителях
	bool bRes = false;
	if (m_parentDefine != nullptr)
		bRes = m_parentDefine->HasDefine(strName);
	nLevel--;
	return bRes;
}

CLexemList* CTranslateCode::CDefineList::GetDefine(const wxString& strName)
{
	auto it = m_defineList.find(stringUtils::MakeUpper(strName));
	if (it != m_defineList.end())
		return it->second;

	//ищем в родителях
	if (m_parentDefine != nullptr && m_parentDefine->HasDefine(strName))
		return m_parentDefine->GetDefine(strName);

	CLexemList* lexList = new CLexemList();
	m_defineList[stringUtils::MakeUpper(strName)] = lexList;
	return lexList;
}

void CTranslateCode::CDefineList::SetDefine(const wxString& strName, CLexemList* src)
{
	CLexemList* dst = GetDefine(strName);
	dst->clear();
	if (src != nullptr) {
		for (unsigned int i = 0; i < src->size(); i++) dst->push_back(*src[i].data());
	}
}

void CTranslateCode::CDefineList::SetDefine(const wxString& strName, const wxString& strValue)
{
	CLexemList listLexem;
	if (strValue.length() > 0) {
		CLexem Lex;
		Lex.m_lexType = CONSTANT;
		if (strValue[0] == '-' || strValue[0] == '+' || (strValue[0] >= '0' && strValue[0] <= '9')) //digit
			Lex.m_valData.SetNumber(strValue);
		else
			Lex.m_valData.SetString(strValue);
		listLexem.push_back(Lex);
		SetDefine(stringUtils::MakeUpper(strName), &listLexem);
	}
	else {
		SetDefine(stringUtils::MakeUpper(strName), nullptr);
	}
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CTranslateCode::CTranslateCode() : m_defineList(nullptr),
m_bAutoDeleteDefList(false),
m_nModePreparing(LEXEM_ADD)
{
	//prepare keyword buffer
	if (ms_listHashKeyWord.size() == 0) {
		LoadKeyWords(); //only once
	}

	Clear();
}

CTranslateCode::CTranslateCode(const wxString& strModuleName, const wxString& strDocPath) : m_defineList(nullptr),
m_strModuleName(strModuleName), m_strDocPath(strDocPath),
m_bAutoDeleteDefList(false),
m_nModePreparing(LEXEM_ADD)
{
	//prepare keyword buffer
	if (ms_listHashKeyWord.size() == 0) {
		LoadKeyWords(); //only once
	}

	Clear();
}

CTranslateCode::CTranslateCode(const wxString& strFileName) : m_defineList(nullptr),
m_strFileName(strFileName),
m_bAutoDeleteDefList(false),
m_nModePreparing(LEXEM_ADD)
{
	//prepare keyword buffer
	if (ms_listHashKeyWord.size() == 0) {
		LoadKeyWords(); //only once
	}

	Clear();
}

CTranslateCode::~CTranslateCode()
{
	if (m_bAutoDeleteDefList) wxDELETE(m_defineList);
}

/**
* prepare keyword buffer
*/

void CTranslateCode::LoadKeyWords()
{
	ms_listHashKeyWord.clear();

#if defined(_LP64) || defined(__LP64__) || defined(__arch64__) || defined(_WIN64)
	for (unsigned long long i = 0; i < sizeof(s_listKeyWord) / sizeof(s_listKeyWord[0]); i++)
#else
	for (unsigned int i = 0; i < sizeof(s_listKeyWord) / sizeof(s_listKeyWord[0]); i++)
#endif
	{
		const wxString& strEng = stringUtils::MakeUpper(s_listKeyWord[i].m_strKeyWord);
		ms_listHashKeyWord[strEng] = (void*)(i + 1);

		//add to array for parser
		s_listHashKeyword[strEng] = (void*)1;
		s_listHelpDescription[strEng] = &s_listKeyWord[i].m_strShortDescription;
	}
}

//////////////////////////////////////////////////////////////////////
// Translating
//////////////////////////////////////////////////////////////////////

/**
* Clear
* Purpose:
* Prepare variables to start compilation
* Return value:
* none
*/

void CTranslateCode::Clear()
{
	m_strBuffer.clear();

	//m_listLexem.clear();
	//m_listTranslateCode.clear();

	if (m_defineList != nullptr) m_defineList->Clear();
	m_bufferSize = m_currentPos = m_currentLine = 0;
}

/**
* Load
* Purpose:
* Load the buffer with source text + prepare variables for compilation
* Return value:
* none
*/

void CTranslateCode::Load(const wxString& strCode)
{
	Clear();

	m_bufferSize = strCode.length();
	m_strBuffer = strCode;
}

/**
* SetError
* Purpose:
* Remember the translation error and raise an exception
* Return value:
* The method does not return control!strCurWord
*/

void CTranslateCode::SetError(int codeError, unsigned int currPos, const wxString& errorDesc) const
{
	unsigned int start_pos = 0;

	//look for the beginning of the string where the translation error message is returned
	for (unsigned int i = (currPos == m_strBuffer.length() ? currPos - 1 : currPos); i > 0; i--) {
		if (m_strBuffer[i] == '\n') {
			start_pos = i + 1; break;
		};
	}

	const int currLine = 1 + std::count(
		m_strBuffer.begin(), m_strBuffer.begin() + start_pos, '\n');

	CTranslateCode::SetError(codeError,
		m_strFileName, m_strModuleName, m_strDocPath,
		currPos, currLine,
		errorDesc
	);
}

/**
* SkipSpaces
* Purpose:
* Skip all insignificant spaces from input buffer
* plus comments from input buffer redirect to output buffer
* Return value:
* NONE
*/

void CTranslateCode::SkipSpaces() const
{
	unsigned int i = m_currentPos;
	for (; i < m_bufferSize; i++) {
		const wxUniChar& c = m_strBuffer[i];
		if (c != ' ' && c != '\t' && c != '\n' && c != '\r') {
			if (c == '/') { //maybe it's a comment
				if (i + 1 < m_bufferSize) {
					if (m_strBuffer[i + 1] == '/') { //skip comments
						for (unsigned int j = i; j < m_bufferSize; j++) {
							m_currentPos = j;
							if (m_strBuffer[j] == '\n' || m_strBuffer[j] == 13) {
								//process next line
								SkipSpaces();
								return;
							}
						}
						i = m_currentPos + 1;
					}
				}
			}
			m_currentPos = i;
			break;
		}
		else if (c == '\n') {
			m_currentLine++;
		}
	}

	if (i == m_bufferSize) {
		m_currentPos = m_bufferSize;
	}
}

/**
* IsByte
* Purpose:
* Check if the next byte (excluding spaces) is equal to
* the GIVEN byte
* Return value:
* true,false
*/

bool CTranslateCode::IsByte(const wxUniChar& c) const
{
	SkipSpaces();
	if (m_currentPos >= m_bufferSize)
		return false;
	if (m_strBuffer[m_currentPos] == c)
		return true;
	return false;
}

/**
* GetByte
* Purpose:
* Get a byte from the sample (excluding spaces)
* if there is no such byte, an exception is thrown
* Return value:
* Byte from the buffer
*/

wxUniChar CTranslateCode::GetByte() const
{
	SkipSpaces();
	if (m_currentPos < m_bufferSize)
		return m_strBuffer[m_currentPos++];
	SetError(ERROR_TRANSLATE_BYTE, m_currentPos);
	return '\0';
}

/**
* IsWord
* Purpose:
* Check (without changing the current cursor position)
* whether the next set of letters is a word (skipping spaces, etc.)
* Return value:
* true,false
*/

bool CTranslateCode::IsWord() const
{
	SkipSpaces();
	if (m_currentPos < m_bufferSize) {
		const wxUniChar& c = m_strBuffer[m_currentPos];
		if (((c == '_') ||
			(c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			(c >= 'А' && c <= 'Я') || (c >= 'а' && c <= 'я') ||
			(c == '#')) && (c != '[' && c != ']'))
			return true;
	}
	return false;
}

/**
* GetWord
* Purpose:
* Select the next word from the buffer
* if there is no word (i.e. the next set of letters is not a word), then an exception is thrown
* Parameter: getPoint
* true - consider the period as a component of the word (to obtain the constant number)
* Return value:
* Word from the buffer
*/

wxString CTranslateCode::GetWord(bool originName, bool getPoint, wxString* psOrig)
{
	SkipSpaces();
	if (m_currentPos >= m_bufferSize) {
		SetError(ERROR_TRANSLATE_WORD, m_currentPos);
		return wxEmptyString;
	}
	int nNext = m_currentPos;
	wxString strWord;
	for (unsigned int i = m_currentPos; i < m_bufferSize; i++) {
		const wxUniChar& c = m_strBuffer[i];
		// if array then break
		if (c == '[' || c == ']') break;
		if ((c == '_') ||
			(c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			(c >= 'А' && c <= 'Я') || (c >= 'а' && c <= 'я') ||

			(c >= '0' && c <= '9') ||
			(c == '#' && i == m_currentPos) || //if the first # symbol is a special word
			(c == '.' && getPoint)) {
			if (c == L'.' && getPoint)
				getPoint = false; //the dot must appear only once
			nNext = i + 1;
		}
		else break;
		strWord += c;
	}
	int nFirst = m_currentPos;
	m_currentPos = nNext;
	if (nFirst == nNext) {
		SetError(ERROR_TRANSLATE_WORD, nFirst);
		return wxEmptyString;
	}
	if (originName) {
		return strWord;
	}
	else {
		if (psOrig) {
			*psOrig = strWord;
		}
		return strWord.Upper();
	}
}

/**
* GetStrToEndLine
* ​​Purpose:
* Get the entire string to the end (character 13 or the end of the program code)
* String
*/

wxString CTranslateCode::GetStrToEndLine() const
{
	unsigned int nStart = m_currentPos;
	unsigned int i = m_currentPos;
	for (; i < m_bufferSize; i++) {
		if (m_strBuffer[i] == '\r' || m_strBuffer[i] == '\n') {
			i++; break;
		}
	}
	m_currentPos = i;
	return m_strBuffer.Mid(nStart, m_currentPos - nStart);
}

/**
* IsNumber
* Purpose:
* Check (without changing the current cursor position)
* whether the next set of letters is a constant number (skipping spaces, etc.)
* Return value:
* true,false
*/

bool CTranslateCode::IsNumber() const
{
	SkipSpaces();
	if (m_currentPos < m_bufferSize) {
		return m_strBuffer[m_currentPos] >= '0' && m_strBuffer[m_currentPos] <= '9';
	}
	return false;
}

/**
* GetNumber
* Purpose:
* Get a number from the selection
* if there is no number, an exception is thrown
* Return value:
* Number from the buffer
*/

wxString CTranslateCode::GetNumber() const
{
	if (!IsNumber()) {
		SetError(ERROR_TRANSLATE_NUMBER, m_currentPos);
		return wxEmptyString;
	}
	SkipSpaces();
	if (m_currentPos >= m_bufferSize) {
		SetError(ERROR_TRANSLATE_NUMBER, m_currentPos);
		return wxEmptyString;
	}
	int nNext = m_currentPos; int nErrorPos = m_currentPos; int nPoint = 0;
	wxString strNumber;
	for (unsigned int i = m_currentPos; i < m_bufferSize; i++) {
		const wxUniChar& c = m_strBuffer[i];
		if ((c >= '0' && c <= '9') || (c == '.')) {
			if (c == '.')
				nPoint++; //dot must appear only once
			nNext = i + 1;
		}
		else break;
		nErrorPos = i + 1;
		strNumber += c;
	}
	int nFirst = m_currentPos;
	m_currentPos = nNext;
	if (nFirst == nNext) {
		SetError(ERROR_TRANSLATE_NUMBER, nFirst);
		return wxEmptyString;
	}
	else if (IsWord() && m_strBuffer[nNext] != ' ') {
		SetError(ERROR_TRANSLATE_NUMBER, nErrorPos);
		return wxEmptyString;
	}
	else if (nPoint > 1) {
		SetError(ERROR_TRANSLATE_NUMBER, nErrorPos);
		return wxEmptyString;
	}
	return strNumber;
}

/**
* IsString
* Purpose:
* Check if the following set of characters (excluding spaces) is a constant string enclosed in quotes
* Return value:
* true,false
*/

bool CTranslateCode::IsString() const
{
	return IsByte('\"') || IsByte('|');
}

/**
* GetString
* Purpose:
* Get a string enclosed in quotes from the selection
* if there is no such string, an exception is thrown
* Return value:
* String from the buffer
*/

wxString CTranslateCode::GetString() const
{
	if (!IsString()) {
		SetError(ERROR_TRANSLATE_STRING, m_currentPos);
		return wxEmptyString;
	}
	unsigned int nCount = 0; bool skip_space = false;
	SkipSpaces();
	if (m_currentPos >= m_bufferSize) {
		SetError(ERROR_TRANSLATE_WORD, m_currentPos);
		return wxEmptyString;
	}
	int nNext = m_currentPos; int nErrorPos = m_currentPos;
	wxString strString;
	for (unsigned int i = m_currentPos; i < m_bufferSize; i++) {
		const wxUniChar& c = m_strBuffer[i];
		if (c == '\n') {
			strString += '\n';
			nNext = m_currentPos + 1;
			m_currentLine++;
			skip_space = true;
		}
		if (skip_space && c == '|') {
			skip_space = false;
			continue;
		}
		else if (skip_space && (c == ' ' || c == '\t' || c == '\n' || c == '\r')) {
			continue;
		}
		else if (skip_space && (c != ' ' && c != '\t' && c != '\n' && c != '\r')) {
			break;
		}
		nErrorPos = i + 1;
		if (nCount < 2) {
			nNext = i + 1;
			if (c == '\"') {
				if (i != m_currentPos && i + 1 < m_bufferSize) {
					if (m_strBuffer[i + 1] == '\"') {
						strString += '\"';
						i++;
						nNext = i + 1;
						continue;
					}
				}
				nCount++;
				continue;
			}
		}
		else break;
		strString += c;
	}
	const int nFirst = m_currentPos;
	m_currentPos = nNext;
	if (nFirst == nNext) {
		SetError(ERROR_TRANSLATE_STRING, nFirst);
		return wxEmptyString;
	}
	else if (nCount < 2) {
		SetError(ERROR_TRANSLATE_STRING, nErrorPos);
		return wxEmptyString;
	}
	return strString;
}

/**
* IsDate
* Purpose:
* Check if the following set of characters (excluding spaces) is a constant date enclosed in apostrophes
* Return value:
* true,false
*/

bool CTranslateCode::IsDate() const
{
	return IsByte('\'');
}

/**
* GetDate
* Purpose:
* Get a date from the selection enclosed in apostrophes
* if there is no such date, an exception is thrown
* Return value:
* The date from the buffer, enclosed in quotes
*/

wxString CTranslateCode::GetDate() const
{
	if (!IsDate()) {
		SetError(ERROR_TRANSLATE_DATE, m_currentPos);
		return wxEmptyString;
	}
	unsigned int nCount = 0;
	SkipSpaces();
	if (m_currentPos >= m_bufferSize) {
		SetError(ERROR_TRANSLATE_WORD, m_currentPos);
		return wxEmptyString;
	}
	int nNext = m_currentPos; int nErrorPos = m_currentPos;
	wxString strDate;
	for (unsigned int i = m_currentPos; i < m_bufferSize; i++) {
		const wxUniChar& c = m_strBuffer[i];
		if (c == '\n') {
			nNext = m_currentPos + 1;
			break;
		}
		nErrorPos = i + 1;
		if (nCount < 2) {
			if (c == '\'') {
				nCount++;
			}
			nNext = i + 1;
		}
		else break;
		strDate += c;
	}
	const int nFirst = m_currentPos;
	m_currentPos = nNext;
	if (nFirst == nNext) {
		SetError(ERROR_TRANSLATE_DATE, nFirst);
		return wxEmptyString;
	}
	else if (nCount < 2) {
		SetError(ERROR_TRANSLATE_DATE, nErrorPos);
		return wxEmptyString;
	}
	else if (IsWord()) {
		SetError(ERROR_TRANSLATE_DATE, nErrorPos);
		return wxEmptyString;
	}
	return strDate;
}

/**
* IsEnd
* Purpose:
* Check the end of translation sign (i.e. reached the end of the buffer)
* Return value:
* true,false
*/

bool CTranslateCode::IsEnd() const
{
	SkipSpaces();
	if (m_currentPos < m_bufferSize)
		return false;
	return true;
}

/**
* IsKeyWord
* Purpose:
* Determines whether the specified word is a service operator
* Return value if:
* -1: no
* greater than or equal to 0: number in the list of service words
*/

#if defined(_LP64) || defined(__LP64__) || defined(__arch64__) || defined(_WIN64)
long long CTranslateCode::IsKeyWord(const wxString& strKeyWord)
#else
int CTranslateCode::IsKeyWord(const wxString& strKeyWord)
#endif
{
	auto itHashKeyWords = ms_listHashKeyWord.find(stringUtils::MakeUpper(strKeyWord));
#if defined(_LP64) || defined(__LP64__) || defined(__arch64__) || defined(_WIN64)
	if (itHashKeyWords != ms_listHashKeyWord.end())
		return ((long long)itHashKeyWords->second) - 1;
#else
	if (itHashKeyWords != ms_listHashKeyWord.end())
		return ((int)itHashKeyWords->second) - 1;
#endif
	return wxNOT_FOUND;
}

/**
* PrepareLexem
* PASS1 - loading lexemes for subsequent quick access during recognition
*/

bool CTranslateCode::PrepareLexem()
{
	m_listLexem.clear();

	if (m_defineList == nullptr) {
		m_defineList = new CDefineList();
		m_defineList->SetParent(&ms_listDefine);
		m_bAutoDeleteDefList = true;//indication that the array with definitions was created by us (and not passed as a definition translation)
	}

	wxString strCurWord;

	while (!IsEnd()) {

		CLexem lex;

		lex.m_strModuleName = m_strModuleName;
		lex.m_strDocPath = m_strDocPath;
		lex.m_strFileName = m_strFileName;

		lex.m_numLine = m_currentLine;
		lex.m_numString = m_currentPos;//if an error occurs later, this is the string that should be returned to the user

		if (IsWord()) {
			wxString strOrig;
			strCurWord = GetWord(false, false, &strOrig);

			//processing user definitions (#define)
			if (m_defineList->HasDefine(strCurWord)) {
				CLexemList* pDef = m_defineList->GetDefine(strCurWord);
				for (unsigned int i = 0; i < pDef->size(); i++) {
					CLexem* lex = pDef[i].data();
					lex->m_numString = m_currentPos;
					lex->m_numLine = m_currentLine;//for breakpoints
					lex->m_strModuleName = m_strModuleName;
					lex->m_strDocPath = m_strDocPath;
					lex->m_strFileName = m_strFileName;
					m_listLexem.push_back(*lex);
				}
				continue;
			}
			const int key_word = IsKeyWord(strCurWord);
			//undefined
			if (key_word == KEY_UNDEFINED) {
				lex.m_lexType = CONSTANT;
				lex.m_valData.SetType(eValueTypes::TYPE_EMPTY);
			}
			//boolean
			else if (key_word == KEY_TRUE) {
				lex.m_lexType = CONSTANT;
				lex.m_valData.SetBoolean(wxT("true"));
			}
			else if (key_word == KEY_FALSE) {
				lex.m_lexType = CONSTANT;
				lex.m_valData.SetBoolean(wxT("false"));
			}
			//null
			else if (key_word == KEY_NULL) {
				lex.m_lexType = CONSTANT;
				lex.m_valData.SetType(eValueTypes::TYPE_NULL);
			}

			if (lex.m_lexType != CONSTANT) {
				lex.m_valData = strOrig;
				if (key_word >= 0) {
					lex.m_lexType = KEYWORD;
					lex.m_numData = key_word;
				}
				else {
					lex.m_lexType = IDENTIFIER;
				}
			}

			lex.m_strData = strCurWord;
		}
		else if (IsNumber() || IsString() || IsDate()) {
			lex.m_lexType = CONSTANT;
			if (IsNumber()) {
				const int curPos = m_currentPos;
				const wxString strData = GetNumber();
				if (!lex.m_valData.SetNumber(strData)) {
					SetError(ERROR_TRANSLATE_NUMBER, curPos);
				}
				lex.m_strData = strData;
				int n = m_listLexem.size() - 1;
				if (n >= 0) {
					if (m_listLexem[n].m_lexType == DELIMITER && (m_listLexem[n].m_numData == '-' || m_listLexem[n].m_numData == '+')) {
						n--;
						if (n >= 0) {
							if (m_listLexem[n].m_lexType == DELIMITER &&
								(
									m_listLexem[n].m_numData == '[' ||
									m_listLexem[n].m_numData == '(' ||
									m_listLexem[n].m_numData == ',' ||
									m_listLexem[n].m_numData == '<' ||
									m_listLexem[n].m_numData == '>' ||
									m_listLexem[n].m_numData == '=')) {
								n++;
								if (m_listLexem[n].m_numData == '-')
									lex.m_valData.m_fData = -lex.m_valData.m_fData;
								m_listLexem[n] = lex;
								continue;
							}
						}
					}
				}
			}
			else {
				if (IsString()) {
					const int curPos = m_currentPos;
					const wxString strData = GetString();
					if (!lex.m_valData.SetString(strData)) {
						SetError(ERROR_TRANSLATE_STRING, curPos);
					}
					lex.m_strData = strData;
				}
				else if (IsDate()) {
					const int curPos = m_currentPos;
					const wxString strData = GetDate();
					if (!lex.m_valData.SetDate(strData)) {
						SetError(ERROR_TRANSLATE_DATE, curPos);
					}
					lex.m_strData = strData;
				}
			}

			m_listLexem.push_back(lex);
			continue;
		}
		else if (IsByte('~')) {
			strCurWord.clear();
			GetByte();//skip the separator and auxiliary symbol of the mark (as unnecessary)
			continue;
		}
		else {

			strCurWord.clear();

			lex.m_lexType = DELIMITER;
			lex.m_numData = GetByte();

			if (lex.m_numData <= 13) {
				continue;
			}
		}
		lex.m_strData = strCurWord;
		if (lex.m_lexType == KEYWORD) {
			if (lex.m_numData == KEY_DEFINE && m_nModePreparing != LEXEM_ADDDEF) { //setting an arbitrary identifier
				if (!IsWord()) {
					SetError(ERROR_IDENTIFIER_DEFINE, m_currentPos);
				}
				const wxString& strName = GetWord();
				//add the translation result to the list of definitions
				if (LEXEM_ADD == m_nModePreparing)
					PrepareFromCurrent(LEXEM_ADDDEF, strName);
				else
					PrepareFromCurrent(LEXEM_IGNORE, strName);

				continue;
			}
			else if (lex.m_numData == KEY_UNDEF) {//removing the identifier
				if (!IsWord()) {
					SetError(ERROR_IDENTIFIER_DEFINE, m_currentPos);
				}
				const wxString& strName = GetWord();
				m_defineList->RemoveDef(strName);
				continue;
			}
			else if (lex.m_numData == KEY_IFDEF || lex.m_numData == KEY_IFNDEF) { //conditional compilation
				if (!IsWord()) {
					SetError(ERROR_IDENTIFIER_DEFINE, m_currentPos);
				}
				const wxString& strName = GetWord();
				bool bHasDef = m_defineList->HasDefine(strName);
				if (lex.m_numData == KEY_IFNDEF)
					bHasDef = !bHasDef;
				//translate the entire block until #else or #endif is encountered
				int nMode = 0;
				if (bHasDef)
					nMode = LEXEM_ADD;//add the translation result to the list of lexemes
				else
					nMode = LEXEM_IGNORE;//otherwise ignore
				PrepareFromCurrent(nMode);
				if (!IsWord()) {
					SetError(ERROR_USE_ENDDEF, m_currentPos);
				}
				wxString strWord = GetWord();
				if (IsKeyWord(strWord) == KEY_ELSEDEF) {//suddenly #else
					//translate again
					if (!bHasDef)
						nMode = LEXEM_ADD;//add the translation result to the list of lexemes
					else
						nMode = LEXEM_IGNORE;//otherwise ignore
					PrepareFromCurrent(nMode);
					if (!IsWord()) {
						SetError(ERROR_USE_ENDDEF, m_currentPos);
					}
					strWord = GetWord();
				}
				//Require #endif
				if (IsKeyWord(strWord) != KEY_ENDIFDEF) {
					SetError(ERROR_USE_ENDDEF, m_currentPos);
				}
				continue;
			}
			else if (lex.m_numData == KEY_ENDIFDEF) {//end of conditional compilation
				m_currentPos = lex.m_numString;//here we saved the previous value
				break;
			}
			else if (lex.m_numData == KEY_ELSEDEF) {//"Otherwise" of conditional compilation
				//return to the beginning of the conditional operator
				m_currentPos = lex.m_numString;//here we saved the previous value
				break;
			}
			else if (lex.m_numData == KEY_REGION) {
				if (!IsWord()) {
					SetError(ERROR_IDENTIFIER_REGION, m_currentPos);
				}
				/*const wxString &strName = */GetWord();
				PrepareFromCurrent(LEXEM_ADD);
				if (!IsWord()) {
					SetError(ERROR_USE_ENDREGION, m_currentPos);
				}
				const wxString& strWord = GetWord();
				//Require #endregion
				if (IsKeyWord(strWord) != KEY_ENDREGION) {
					SetError(ERROR_USE_ENDREGION, m_currentPos);
				}
				continue;
			}
			else if (lex.m_numData == KEY_ENDREGION) {
				m_currentPos = lex.m_numString;//here we saved the previous value
				break;
			}
		}
		m_listLexem.push_back(lex);
	}

	for (auto& translateCode : m_listTranslateCode) {
		translateCode->m_currentLine = translateCode->m_currentPos = 0;
		if (!translateCode->PrepareLexem())
			return false;
		for (auto& lex : translateCode->m_listLexem) {
			if (lex.m_lexType != ENDPROGRAM) {
				m_listLexem.push_back(lex);
			}
		}
		m_currentLine += translateCode->m_currentLine;
		m_currentPos += translateCode->m_currentPos;
	}

	CLexem lex;

	lex.m_lexType = ENDPROGRAM;
	lex.m_numData = 0;

	lex.m_strModuleName = m_strModuleName;
	lex.m_strDocPath = m_strDocPath;
	lex.m_strFileName = m_strFileName;

	lex.m_numLine = m_currentLine;
	lex.m_numString = m_currentPos;

	m_listLexem.push_back(lex);
	return true;
}

/**
* create lexemes starting from the current position
*/

void CTranslateCode::PrepareFromCurrent(int nMode, const wxString& strName)
{
	CTranslateCode translate;

	translate.m_defineList = m_defineList;
	translate.m_nModePreparing = nMode;
	translate.m_strModuleName = m_strModuleName;

	translate.Load(m_strBuffer);

	//start line number
	translate.m_currentLine = m_currentLine;
	translate.m_currentPos = m_currentPos;

	//end line number
	if (nMode == LEXEM_ADDDEF) {
		GetStrToEndLine();
		translate.m_bufferSize = m_currentPos;
	}

	translate.PrepareLexem();

	if (nMode == LEXEM_ADDDEF) {
		m_defineList->SetDefine(strName, &translate.m_listLexem);
		m_currentLine = translate.m_currentLine;
	}
	else if (nMode == LEXEM_ADD) {
		for (unsigned int i = 0; i < translate.m_listLexem.size() - 1; i++) {//excluding ENDPROGRAM
			m_listLexem.push_back(translate.m_listLexem[i]);
		}

		m_currentPos = translate.m_currentPos;
		m_currentLine = translate.m_currentLine;
	}
	else {
		m_currentPos = translate.m_currentPos;
		m_currentLine = translate.m_currentLine;
	}
}

void CTranslateCode::AppendModule(CTranslateCode* module)
{
	auto& it = std::find(m_listTranslateCode.begin(), m_listTranslateCode.end(), module);
	if (it == m_listTranslateCode.end()) {
		m_listTranslateCode.push_back(module);
	}
}

void CTranslateCode::RemoveModule(CTranslateCode* module)
{
	auto& it = std::find(m_listTranslateCode.begin(), m_listTranslateCode.end(), module);
	if (it != m_listTranslateCode.end()) {
		m_listTranslateCode.erase(it);
	}
}

void CTranslateCode::OnSetParent(CTranslateCode* setParent)
{
	if (!m_defineList) {
		m_defineList = new CDefineList;
		m_defineList->SetParent(&ms_listDefine);
		m_bAutoDeleteDefList = true;//sign of auto deletion
	}

	if (setParent) {
		m_defineList->SetParent(setParent->m_defineList);
	}
	else {
		m_defineList->SetParent(&ms_listDefine);
	}
}

///////////////////////////////////////////////////////////////////////////////////////////////////////

#include <wx/module.h>

class wxOESKeywordModule : public wxModule
{
public:
	wxOESKeywordModule() : wxModule() {}
	virtual bool OnInit() {
		CTranslateCode::LoadKeyWords();
		return true;
	}
	virtual void OnExit() {}
private:
	wxDECLARE_DYNAMIC_CLASS(wxOESKeywordModule);
};

wxIMPLEMENT_DYNAMIC_CLASS(wxOESKeywordModule, wxModule)