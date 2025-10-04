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

	for (unsigned int i = 0; i < sizeof(s_listKeyWord) / sizeof(s_listKeyWord[0]); i++)
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

	//m_listLexem.Clear();
	//m_listTranslateCode.Clear();

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

	m_strBuffer.assign(strCode);
	m_strBUFFER.assign(strCode);

	std::transform(m_strBUFFER.begin(), m_strBUFFER.end(),
		m_strBUFFER.begin(), ::toupper
	);
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
		const auto& c = m_strBuffer[i];
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
		const auto& c = m_strBuffer[m_currentPos];
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

bool CTranslateCode::GetWord(wxString* strWord, wxString* strRealName, bool realName, bool get_point) const
{
	SkipSpaces();

	if (m_currentPos >= m_bufferSize) {
		SetError(ERROR_TRANSLATE_WORD, m_currentPos);
		return false;
	}

	unsigned int next_pos = m_currentPos;
	
	if (strWord != nullptr) strWord->Clear();
	for (unsigned int i = m_currentPos; i < m_bufferSize; i++) {
		const auto& c = m_strBuffer[i];
		// if array then break
		if (c == '[' || c == ']') break;
		if ((c == '_') ||
			(c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
			(c >= 'А' && c <= 'Я') || (c >= 'а' && c <= 'я') ||

			(c >= '0' && c <= '9') ||
			(c == '#' && i == m_currentPos) || //if the first # symbol is a special word
			(c == '.' && get_point)) {
			if (c == L'.' && get_point)
				get_point = false; //the dot must appear only once

			next_pos = i + 1;
		}
		else break;

		if (strRealName != nullptr) strRealName->Append(c);
		if (strWord != nullptr) strWord->Append(realName ? c : m_strBUFFER[i]);
	}

	const unsigned int first_pos = m_currentPos;

	m_currentPos = next_pos;

	if (first_pos == next_pos) {
		SetError(ERROR_TRANSLATE_WORD, first_pos);
		return false;
	}

	return true;
}

/**
* GetStrToEndLine
* ​​Purpose:
* Get the entire string to the end (character 13 or the end of the program code)
* String
*/

wxString CTranslateCode::GetStrToEndLine() const
{
	unsigned int start_pos = m_currentPos;
	unsigned int i = m_currentPos;
	for (; i < m_bufferSize; i++) {
		if (m_strBuffer[i] == '\r' || m_strBuffer[i] == '\n') {
			i++; break;
		}
	}
	m_currentPos = i;
	return m_strBuffer.substr(start_pos, m_currentPos - start_pos);
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

bool CTranslateCode::GetNumber(wxString* strNumber) const
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
	unsigned int next_pos = m_currentPos, error_pos = m_currentPos, point_pos = 0;
	if (strNumber != nullptr) strNumber->Clear();
	for (unsigned int i = m_currentPos; i < m_bufferSize; i++) {
		const auto& c = m_strBuffer[i];
		if ((c >= '0' && c <= '9') || (c == '.')) {
			if (c == '.')
				point_pos++; //dot must appear only once
			next_pos = i + 1;
		}
		else break;
		error_pos = i + 1;
		if (strNumber != nullptr) strNumber->Append(c);
	}

	const unsigned int first_pos = m_currentPos;
	m_currentPos = next_pos;
	if (first_pos == next_pos) {
		SetError(ERROR_TRANSLATE_NUMBER, first_pos);
		return false;
	}
	else if (IsWord() && m_strBuffer[next_pos] != ' ') {
		SetError(ERROR_TRANSLATE_NUMBER, error_pos);
		return false;
	}
	else if (point_pos > 1) {
		SetError(ERROR_TRANSLATE_NUMBER, error_pos);
		return false;
	}

	return true;
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

bool CTranslateCode::GetString(wxString* strString) const
{
	if (!IsString()) {
		SetError(ERROR_TRANSLATE_STRING, m_currentPos);
		return false;
	}
	unsigned int count_char = 0; bool skip_space = false;
	SkipSpaces();
	if (m_currentPos >= m_bufferSize) {
		SetError(ERROR_TRANSLATE_WORD, m_currentPos);
		return false;
	}
	unsigned int next_pos = m_currentPos, error_pos = m_currentPos;
	if (strString != nullptr) strString->Clear();
	for (unsigned int i = m_currentPos; i < m_bufferSize; i++) {
		const auto& c = m_strBuffer[i];
		if (c == '\n') {
			if (strString != nullptr) strString->Append('\n');
			next_pos = m_currentPos + 1;
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
		error_pos = i + 1;
		if (count_char < 2) {
			next_pos = i + 1;
			if (c == '\"') {
				if (i != m_currentPos && i + 1 < m_bufferSize) {
					if (m_strBuffer[i + 1] == '\"') {
						if (strString != nullptr) strString->Append('\"');
						i++;
						next_pos = i + 1;
						continue;
					}
				}
				count_char++;
				continue;
			}
		}
		else break;
		if (strString != nullptr) strString->Append(c);
	}

	const unsigned int first_pos = m_currentPos;
	m_currentPos = next_pos;

	if (first_pos == next_pos) {
		SetError(ERROR_TRANSLATE_STRING, first_pos);
		return false;
	}
	else if (count_char < 2) {
		SetError(ERROR_TRANSLATE_STRING, error_pos);
		return false;
	}

	return true;
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

bool CTranslateCode::GetDate(wxString* strDate) const
{
	if (!IsDate()) {
		SetError(ERROR_TRANSLATE_DATE, m_currentPos);
		return false;
	}

	unsigned int count_char = 0;
	SkipSpaces();
	if (m_currentPos >= m_bufferSize) {
		SetError(ERROR_TRANSLATE_WORD, m_currentPos);
		return false;
	}

	unsigned int next_pos = m_currentPos, error_pos = m_currentPos;
	if (strDate != nullptr) strDate->Clear();

	for (unsigned int i = m_currentPos; i < m_bufferSize; i++) {
		const auto& c = m_strBuffer[i];
		if (c == '\n') {
			next_pos = m_currentPos + 1;
			m_currentLine++;
			break;
		}
		error_pos = i + 1;
		if (count_char < 2) {
			if (c == '\'') {
				count_char++;
			}
			next_pos = i + 1;
		}
		else break;
		if (strDate != nullptr) strDate->Append(c);
	}

	const unsigned int first_pos = m_currentPos;

	m_currentPos = next_pos;

	if (first_pos == next_pos) {
		SetError(ERROR_TRANSLATE_DATE, first_pos);
		return false;
	}
	else if (count_char < 2) {
		SetError(ERROR_TRANSLATE_DATE, error_pos);
		return false;
	}
	else if (IsWord()) {
		SetError(ERROR_TRANSLATE_DATE, error_pos);
		return false;
	}

	return true;
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

int CTranslateCode::IsKeyWord(const wxString& strKeyWord)
{
	auto it = std::find_if(
		ms_listHashKeyWord.begin(), ms_listHashKeyWord.end(), [strKeyWord](const std::pair<const wxString, void*>& pair) -> bool {
			return stringUtils::CompareString(pair.first, strKeyWord);
		}
	);

	if (it != ms_listHashKeyWord.end())
		return ((int)it->second) - 1;

	return wxNOT_FOUND;
}

wxString CTranslateCode::GetKeyWord(int k)
{
	auto it = std::find_if(
		ms_listHashKeyWord.begin(), ms_listHashKeyWord.end(), [k](const std::pair<const wxString, void*>& pair) -> bool {
			return k == ((int)pair.second) - 1;
		}
	);

	if (it != ms_listHashKeyWord.end())
		return it->first;

	return wxEmptyString;
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

		m_current_lex.m_strModuleName = m_strModuleName;
		m_current_lex.m_strDocPath = m_strDocPath;
		m_current_lex.m_strFileName = m_strFileName;

		m_current_lex.m_numLine = m_currentLine;
		m_current_lex.m_numString = m_currentPos;//if an error occurs later, this is the string that should be returned to the user

		if (IsWord()) {

			wxString strOrig;
			if (GetWord(strCurWord, strOrig)) {

				//processing user definitions (#define)
				if (m_defineList->HasDefine(strCurWord)) {
					CLexemList* pDef = m_defineList->GetDefine(strCurWord);
					for (unsigned int i = 0; i < pDef->size(); i++) {
						CLexem* m_current_lex = pDef[i].data();
						m_current_lex->m_numString = m_currentPos;
						m_current_lex->m_numLine = m_currentLine;//for breakpoints
						m_current_lex->m_strModuleName = m_strModuleName;
						m_current_lex->m_strDocPath = m_strDocPath;
						m_current_lex->m_strFileName = m_strFileName;
						m_listLexem.push_back(*m_current_lex);
					}
					continue;
				}

				const int k = IsKeyWord(strCurWord);

				//undefined
				if (k == KEY_UNDEFINED) {
					m_current_lex.m_lexType = CONSTANT;
					m_current_lex.m_valData.SetType(eValueTypes::TYPE_EMPTY);
				}
				//boolean
				else if (k == KEY_TRUE) {
					m_current_lex.m_lexType = CONSTANT;
					m_current_lex.m_valData.SetBoolean(wxT("true"));
				}
				else if (k == KEY_FALSE) {
					m_current_lex.m_lexType = CONSTANT;
					m_current_lex.m_valData.SetBoolean(wxT("false"));
				}
				//null
				else if (k == KEY_NULL) {
					m_current_lex.m_lexType = CONSTANT;
					m_current_lex.m_valData.SetType(eValueTypes::TYPE_NULL);
				}
				else {
					if (k >= 0) {
						m_current_lex.m_lexType = KEYWORD;
						m_current_lex.m_numData = k;
					}
					else {
						m_current_lex.m_lexType = IDENTIFIER;
					}
					m_current_lex.m_valData = strOrig;
				}
				m_current_lex.m_strData = strCurWord;
			}
		}
		else if (IsNumber() || IsString() || IsDate()) {
			m_current_lex.m_lexType = CONSTANT;
			if (IsNumber()) {
				const int curPos = m_currentPos;
				wxString strData; GetNumber(strData);
				if (!m_current_lex.m_valData.SetNumber(strData)) {
					SetError(ERROR_TRANSLATE_NUMBER, curPos);
				}
				m_current_lex.m_strData = strData;
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
									m_current_lex.m_valData.m_fData = -m_current_lex.m_valData.m_fData;
								m_listLexem[n] = m_current_lex;
								continue;
							}
						}
					}
				}
			}
			else {
				if (IsString()) {
					const int curPos = m_currentPos;
					wxString strData; GetString(strData);
					if (!m_current_lex.m_valData.SetString(strData)) {
						SetError(ERROR_TRANSLATE_STRING, curPos);
					}
					m_current_lex.m_strData = strData;
				}
				else if (IsDate()) {
					const int curPos = m_currentPos;
					wxString strData; GetDate(strData);
					if (!m_current_lex.m_valData.SetDate(strData)) {
						SetError(ERROR_TRANSLATE_DATE, curPos);
					}
					m_current_lex.m_strData = strData;
				}
			}

			m_listLexem.emplace_back(std::move(m_current_lex));
			continue;
		}
		else if (IsByte('~')) {
			strCurWord.Clear();
			GetByte();//skip the separator and auxiliary symbol of the mark (as unnecessary)
			continue;
		}
		else {

			strCurWord.Clear();

			m_current_lex.m_lexType = DELIMITER;
			m_current_lex.m_numData = GetByte();

			if (m_current_lex.m_numData <= 13) continue;		
		}
		m_current_lex.m_strData = strCurWord;
		if (m_current_lex.m_lexType == KEYWORD) {
			if (m_current_lex.m_numData == KEY_DEFINE && m_nModePreparing != LEXEM_ADDDEF) { //setting an arbitrary identifier
				if (!IsWord()) {
					SetError(ERROR_IDENTIFIER_DEFINE, m_currentPos);
				}
				wxString strName; GetWord(strName);
				//add the translation result to the list of definitions
				if (LEXEM_ADD == m_nModePreparing)
					PrepareFromCurrent(LEXEM_ADDDEF, strName);
				else
					PrepareFromCurrent(LEXEM_IGNORE, strName);

				continue;
			}
			else if (m_current_lex.m_numData == KEY_UNDEF) {//removing the identifier
				if (!IsWord()) {
					SetError(ERROR_IDENTIFIER_DEFINE, m_currentPos);
				}
				wxString strName; GetWord(strName);
				m_defineList->RemoveDef(strName);
				continue;
			}
			else if (m_current_lex.m_numData == KEY_IFDEF || m_current_lex.m_numData == KEY_IFNDEF) { //conditional compilation

				if (!IsWord()) {
					SetError(ERROR_IDENTIFIER_DEFINE, m_currentPos);
				}

				wxString strName; GetWord(strName);

				bool bHasDef = m_defineList->HasDefine(strName);
				if (m_current_lex.m_numData == KEY_IFNDEF)
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

				wxString strWord; GetWord(strWord);
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

					GetWord(strWord);
				}
				//Require #endif
				if (IsKeyWord(strWord) != KEY_ENDIFDEF) {
					SetError(ERROR_USE_ENDDEF, m_currentPos);
				}
				continue;
			}
			else if (m_current_lex.m_numData == KEY_ENDIFDEF) {//end of conditional compilation
				m_currentPos = m_current_lex.m_numString;//here we saved the previous value
				break;
			}
			else if (m_current_lex.m_numData == KEY_ELSEDEF) {//"Otherwise" of conditional compilation
				//return to the beginning of the conditional operator
				m_currentPos = m_current_lex.m_numString;//here we saved the previous value
				break;
			}
			else if (m_current_lex.m_numData == KEY_REGION) {
				if (!IsWord()) {
					SetError(ERROR_IDENTIFIER_REGION, m_currentPos);
				}
				/*const wxString &strName = */GetWord();
				PrepareFromCurrent(LEXEM_ADD);
				if (!IsWord()) {
					SetError(ERROR_USE_ENDREGION, m_currentPos);
				}
				wxString strWord; GetWord(strWord);
				//Require #endregion
				if (IsKeyWord(strWord) != KEY_ENDREGION) {
					SetError(ERROR_USE_ENDREGION, m_currentPos);
				}
				continue;
			}
			else if (m_current_lex.m_numData == KEY_ENDREGION) {
				m_currentPos = m_current_lex.m_numString;//here we saved the previous value
				break;
			}
		}
		m_listLexem.emplace_back(std::move(m_current_lex));
	}

	for (auto& translateCode : m_listTranslateCode) {
		translateCode->m_currentLine = translateCode->m_currentPos = 0;
		if (!translateCode->PrepareLexem())
			return false;
		for (auto& m_current_lex : translateCode->m_listLexem) {
			if (m_current_lex.m_lexType != ENDPROGRAM) {
				m_listLexem.push_back(m_current_lex);
			}
		}
		m_currentLine += translateCode->m_currentLine;
		m_currentPos += translateCode->m_currentPos;
	}

	m_current_lex.m_lexType = ENDPROGRAM;
	m_current_lex.m_numData = 0;

	m_current_lex.m_strModuleName = m_strModuleName;
	m_current_lex.m_strDocPath = m_strDocPath;
	m_current_lex.m_strFileName = m_strFileName;

	m_current_lex.m_numLine = m_currentLine;
	m_current_lex.m_numString = m_currentPos;

	m_listLexem.emplace_back(std::move(m_current_lex));
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

size_t CTranslateCode::CalcAllocSize() const {

	m_currentPos = m_currentLine = 0;

	size_t alloc_size = 1;
	while (!IsEnd()) {
		if (IsWord()) {
			try {
				(void)GetWord();
				alloc_size++;
			}
			catch (...) {
			}
		}
		else if (IsNumber() || IsString() || IsDate()) {
			if (IsNumber()) {
				try {
					(void)GetNumber();
					alloc_size++;
				}
				catch (...) {
				}
			}
			else if (IsString()) {
				try {
					(void)GetString();
					alloc_size++;
				}
				catch (...) {
				}
			}
			else if (IsDate()) {
				try {
					(void)GetDate();
					alloc_size++;
				}
				catch (...) {
				}
			}
		}
		else {
			(void)GetByte();
			alloc_size++;
		}
	}

	m_currentPos = m_currentLine = 0;
	return alloc_size;
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