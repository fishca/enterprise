////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : autoComplete loader  
////////////////////////////////////////////////////////////////////////////

#include "codeEditor.h"
#include "codeEditorParser.h"

#include "backend/debugger/debugClient.h"
#include "backend/metaCollection/partial/commonObject.h"

#include "frontend/docView/docView.h"

void ibCodeEditor::AddKeywordFromObject(const ibValue& vObject)
{
	if (vObject.GetType() != ibValueTypes::TYPE_EMPTY &&
		vObject.GetType() != ibValueTypes::TYPE_OLE) {
		for (long i = 0; i < vObject.GetNMethods(); i++) {
			if (vObject.HasRetVal(i)) {
				m_ac.Append(
					ibContentType::eFunction,
					vObject.GetMethodName(i),
					vObject.GetMethodHelper(i)
				);
			}
			else {
				m_ac.Append(
					ibContentType::eProcedure,
					vObject.GetMethodName(i),
					vObject.GetMethodHelper(i)
				);
			}
		}
		for (long i = 0; i < vObject.GetNProps(); i++) {
			// Scope-local props (ThisObject / ThisForm) must not
			// surface in autocomplete after a chain walk reaches a
			// foreign object (`Catalogs.Catalog1.CreateElement().` …).
			if (vObject.IsPropScoped(i)) continue;
			m_ac.Append(
				ibContentType::eVariable,
				vObject.GetPropName(i),
				wxEmptyString
			);
		}
		ibRuntimeModuleDataObject* moduleDataObject = dynamic_cast<ibRuntimeModuleDataObject*>(vObject.GetRef());
		if (moduleDataObject != nullptr) {
			const ibValueMetaObjectModuleBase* computeModuleObject = moduleDataObject->GetMetaObject();
			if (computeModuleObject != nullptr) {
				ibParserModule cParser;
				if (cParser.ParseModule(computeModuleObject->GetModuleText())) {
					for (auto code : cParser.GetAllContent()) {
						if (code.m_eType == eExportVariable) {
							m_ac.Append(
								ibContentType::eExportVariable,
								code.m_name,
								wxEmptyString
							);
						}
						else if (code.m_eType == eExportProcedure) {
							m_ac.Append(
								ibContentType::eExportFunction,
								code.m_name,
								code.m_shortDescription
							);
						}
						else if (code.m_eType == eExportFunction) {
							m_ac.Append(
								ibContentType::eExportFunction,
								code.m_name,
								code.m_shortDescription
							);
						}
					}
				}
			}
		}
		ibValueManagerDataObject* managerDataObject = dynamic_cast<ibValueManagerDataObject*>(vObject.GetRef());
		if (managerDataObject != nullptr) {
			ibValueMetaObjectCommonModule* computeManagerModule = managerDataObject->GetModuleManager();
			if (computeManagerModule != nullptr) {
				ibParserModule cParser;
				if (cParser.ParseModule(computeManagerModule->GetModuleText())) {
					for (auto code : cParser.GetAllContent()) {
						if (code.m_eType == eExportVariable) {
							m_ac.Append(ibContentType::eExportVariable, code.m_name, wxEmptyString);
						}
						else if (code.m_eType == eExportProcedure) {
							m_ac.Append(ibContentType::eExportFunction, code.m_name, code.m_shortDescription);
						}
						else if (code.m_eType == eExportFunction) {
							m_ac.Append(ibContentType::eExportFunction, code.m_name, code.m_shortDescription);
						}
					}
				}
			}
		}
	}
	else if (debugClient->IsEnterLoop()) {
		ibPrecompileContext* currContext = m_precompileModule->GetCurrentContext();
		if (currContext && currContext->FindVariable(m_precompileModule->GetLastParentKeyword())) {
			m_ac.Cancel();
			const ibValueMetaObject* metaObject = m_document->GetMetaObject();
			wxASSERT(metaObject);
			debugClient->EvaluateAutocomplete(
				metaObject->GetFileName(),
				metaObject->GetDocPath(),
				m_precompileModule->GetLastExpression(),
				m_precompileModule->GetLastKeyword(),
				GetCurrentPos()
			);
		}
	}
}

bool ibCodeEditor::PrepareExpression(unsigned int currPos, wxString& expression, wxString& keyword, wxString& currentWord, bool& outHasPoint)
{
	bool hasPoint = false, hasKeyword = false;
	for (unsigned int i = 0; i < m_precompileModule->GetLexems().size(); i++)
	{
		if (m_precompileModule->GetLexems()[i].m_lexType == IDENTIFIER)
		{
			if (hasPoint) expression += m_precompileModule->GetLexems()[i].m_valData.GetString();
			else expression = m_precompileModule->GetLexems()[i].m_valData.GetString();

			currentWord = m_precompileModule->GetLexems()[i].m_valData.GetString();

			if (i < m_precompileModule->GetLexems().size() - 1) {
				if (m_precompileModule->GetLexems()[i + 1].m_numString >= currPos)
					break;
				const ibLexem& lex = m_precompileModule->GetLexems()[i + 1];
				if (lex.m_lexType == DELIMITER && lex.m_numData == '(')
					expression = wxEmptyString;
				if (lex.m_lexType == DELIMITER && lex.m_numData == '(' && !hasPoint)
					keyword = currentWord;

				if (lex.m_lexType != ENDPROGRAM)
					hasPoint = lex.m_lexType == DELIMITER && lex.m_numData == '.';
			}

			hasKeyword = hasKeyword ? i == m_precompileModule->GetLexems().size() - 1 : false;
		}
		else if (m_precompileModule->GetLexems()[i].m_lexType == KEYWORD && m_precompileModule->GetLexems()[i].m_numData == KEY_NEW)
		{
			expression = wxEmptyString; currentWord = wxEmptyString;
			keyword = m_precompileModule->GetLexems()[i].m_valData.GetString(); hasKeyword = true;
		}
		else if (m_precompileModule->GetLexems()[i].m_lexType == CONSTANT)
		{
			// Special-keyword contexts where the autocomplete dropdown
			// should be filtered by the literal currently being typed
			// inside the call's argument string — e.g.
			// showCommonForm("...|") completes against form names.
			//
			// Outside those contexts a literal MUST NOT leak into
			// currentWord — otherwise an unrelated string constant
			// earlier in the source (var = "hello"; <caret>) would
			// poison the autocomplete filter and hide everything.
			const bool inSpecialCall =
				stringUtils::CompareString(keyword, wxT("type"))
				|| stringUtils::CompareString(keyword, wxT("showCommonForm"))
				|| (stringUtils::CompareString(keyword, wxT("getCommonForm")) && !hasPoint);

			if (inSpecialCall) {
				currentWord = m_precompileModule->GetLexems()[i].m_valData.GetString();
				hasKeyword = true;
			}
			else {
				currentWord = wxEmptyString;
				hasKeyword = false;
			}
		}
		else if (m_precompileModule->GetLexems()[i].m_lexType == DELIMITER
			&& m_precompileModule->GetLexems()[i].m_numData == '.')
		{
			if (!expression.IsEmpty())
				expression += '.';

			currentWord = wxEmptyString; hasPoint = true; hasKeyword = false;
		}
		else
		{
			if (m_precompileModule->GetLexems()[i].m_lexType != ENDPROGRAM) {
				expression = wxEmptyString; currentWord = wxEmptyString;
			}

			hasKeyword = false;
		}

		if (i < m_precompileModule->GetLexems().size() - 1 &&
			m_precompileModule->GetLexems()[i + 1].m_numString >= currPos) break;
	}

	outHasPoint = hasPoint; return hasKeyword;
}

void ibCodeEditor::PrepareTooTipExpression(unsigned int currPos, wxString& expression, wxString& currentWord, bool& outHasPoint)
{
	bool hasPoint = false;

	for (unsigned int i = 0; i < m_precompileModule->GetLexems().size(); i++)
	{
		if (m_precompileModule->GetLexems()[i].m_numString > currPos
			&& !hasPoint) break;

		if (m_precompileModule->GetLexems()[i].m_lexType == IDENTIFIER)
		{
			if (hasPoint) expression += m_precompileModule->GetLexems()[i].m_valData.GetString();
			else expression = m_precompileModule->GetLexems()[i].m_valData.GetString();

			currentWord = m_precompileModule->GetLexems()[i].m_valData.GetString();

			if (i < m_precompileModule->GetLexems().size() - 1) {
				const ibLexem& lex = m_precompileModule->GetLexems()[i + 1];
				if (lex.m_lexType == DELIMITER && lex.m_numData == '(')
					expression = wxEmptyString;
				hasPoint = lex.m_lexType == DELIMITER && lex.m_numData == '.';
			}
			else hasPoint = false;
		}
		else if (m_precompileModule->GetLexems()[i].m_lexType == DELIMITER
			&& m_precompileModule->GetLexems()[i].m_numData == '.')
		{
			if (!expression.IsEmpty())
				expression += '.';

			currentWord = wxEmptyString; hasPoint = true;
		}
		else
		{
			expression = wxEmptyString; currentWord = wxEmptyString;
		}
	}

	outHasPoint = hasPoint;
}

namespace {

// Strip EOL markers in-place per the editor's current EOL mode so the
// leading-whitespace scan that follows doesn't trip over `\r`/`\n`.
void StripEOL(std::string& s, int eolMode)
{
	switch (eolMode) {
	case wxSTC_EOL_CRLF:
		s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
		s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
		break;
	case wxSTC_EOL_CR:
		s.erase(std::remove(s.begin(), s.end(), '\r'), s.end());
		break;
	default:
		s.erase(std::remove(s.begin(), s.end(), '\n'), s.end());
		break;
	}
}

// Append the editor's EOL marker(s).
void AppendEOL(std::string& s, int eolMode)
{
	switch (eolMode) {
	case wxSTC_EOL_CRLF: s.push_back('\r'); s.push_back('\n'); break;
	case wxSTC_EOL_CR:   s.push_back('\r'); break;
	default:             s.push_back('\n'); break;
	}
}

// Count leading '\t'/' ' chars; outReplacePos receives one past the last
// whitespace character (the splice end for indent rewrite below).
int CountLeadingIndent(const std::string& s, int& outReplacePos)
{
	int count = 0;
	outReplacePos = 0;
	const int len = (int)s.length();
	for (int i = 0; i < len; ++i) {
		if (s[i] == '\t' || s[i] == ' ') {
			count++;
			outReplacePos = i + 1;
		}
		else break;
	}
	return count;
}

} // namespace

/**
 * PrepareTABs
 *   Auto-indent the current line on Enter:
 *     1. Read the cached fold mask for the line.
 *     2. Compute the target indent for the CURRENT line based on the
 *        fold flag — HEADER aligns to its own level, ELSE/EndXxx (WHITE)
 *        align to the parent indent.
 *     3. Rewrite the current line's leading tabs to match.
 *     4. Append EOL + indent for the NEW line that Enter is about to
 *        produce, and replace the line slice through STC's target API.
 */
void ibCodeEditor::PrepareTABs()
{
	const int currPosition = GetCurrentPos();
	const int currLine     = LineFromPosition(currPosition);
	const int startLinePos = PositionFromLine(currLine);
	const int level        = m_fp.GetFoldMask(currLine);
	const int eolMode      = GetEOLMode();

	std::string rawBufferLine;
	if (startLinePos != currPosition)
		rawBufferLine = GetTextRangeRaw(startLinePos, currPosition);

	// Strip any EOL chars upfront so the indent rewrite operates on a
	// pure (whitespace-prefix + text) buffer. Typically the line slice
	// before the caret has none, but this also closes a latent bug where
	// the previous version could leave duplicated EOLs when no fold flag
	// matched (cursor on a free-form line with stray CR/LF).
	StripEOL(rawBufferLine, eolMode);

	int foldLevel = level ^ wxSTC_FOLDLEVELBASE_FLAG;

	auto rewriteIndent = [&](int targetTabs) {
		int replacePos = 0;
		const int currentIndent = CountLeadingIndent(rawBufferLine, replacePos);
		if (currentIndent != targetTabs)
			rawBufferLine.replace(0, replacePos, targetTabs, '\t');
	};

	if ((level & wxSTC_FOLDLEVELHEADER_FLAG) != 0) {
		// Procedure/Function/If/While/Try header — line itself sits at
		// foldLevel; Enter on the header opens its body, +1 indent.
		foldLevel ^= wxSTC_FOLDLEVELHEADER_FLAG;
		rewriteIndent(foldLevel);
		if (startLinePos + foldLevel != currPosition) foldLevel++;
	}
	else if ((level & wxSTC_FOLDLEVELELSE_FLAG) != 0) {
		// Else/ElseIf/Except — pseudo-header at child indent; visually
		// aligns to parent indent (foldLevel - 1).
		foldLevel ^= wxSTC_FOLDLEVELELSE_FLAG;
		if (foldLevel >= 0 && LineLength(currLine) > 0)
			rewriteIndent(std::max(0, foldLevel - 1));
		if (startLinePos + foldLevel - 1 == currPosition) foldLevel--;
	}
	else if ((level & wxSTC_FOLDLEVELWHITE_FLAG) != 0) {
		// EndProcedure/EndFunction/EndIf/EndDo/EndTry — closer at parent
		// indent; the WHITE flag's level is already child, so subtract 1.
		foldLevel = (foldLevel ^ wxSTC_FOLDLEVELWHITE_FLAG) - 1;
		if (foldLevel >= 0 && LineLength(currLine) > 0)
			rewriteIndent(foldLevel);
	}
	else if ((level & wxSTC_FOLDLEVELBASE_FLAG) != 0) {
		// Plain code line inside a block — indent matches foldLevel.
		if (foldLevel >= 0 && LineLength(currLine) > 0)
			rewriteIndent(foldLevel);
	}

	AppendEOL(rawBufferLine, eolMode);
	rawBufferLine.append(foldLevel, '\t');

	SetTargetStart((int)startLinePos);
	SetTargetEnd((int)currPosition);
	ReplaceTargetRaw(rawBufferLine.c_str(), rawBufferLine.length());

	const size_t length = rawBufferLine.length();
	GotoLine(LineFromPosition(startLinePos + length));
	SetEmptySelection(startLinePos + length);
}

void ibCodeEditor::LoadAutoComplete()
{
	int realPos = GetRealPosition();

	// Find the word start
	int currentPos = GetCurrentPos();

	int wordStartPos = WordStartPosition(currentPos, true);
	int wordEndPos = WordEndPosition(currentPos, false);

	// Display the autocompletion list
	int lenEntered = currentPos - wordStartPos;

	wxString expression, keyword, currentWord; bool hasPoint = true;

	if (m_ct.Active())
		m_ct.Cancel();

	if (!PrepareExpression(realPos, expression, keyword, currentWord, hasPoint)) {
		m_ac.Start(currentWord, currentPos, lenEntered, TextHeight(GetCurrentLine()));
		if (hasPoint) {
			LoadIntelliList();
		}
		else {
			LoadSysKeyword();
		}
	}
	else {
		m_ac.Start(currentWord, currentPos, lenEntered, TextHeight(GetCurrentLine()));
		LoadFromKeyWord(keyword);
	}

	wxPoint position = PointFromPosition(wordStartPos);
	position.y += TextHeight(GetCurrentLine());
	m_ac.Show(position);
}

void ibCodeEditor::LoadToolTip(const wxPoint& pos)
{
	if (debugClient->IsEnterLoop()) {

		int currentPos = GetRealPositionFromPoint(pos);
		wxString expression, currentWord; bool hasPoint = false;
		PrepareTooTipExpression(currentPos, expression, currentWord, hasPoint);

		expression.Trim(true).Trim(false);

		if (expression.IsEmpty()) {
			SetToolTip(nullptr); return;
		}

		const ibValueMetaObject* metaObject = m_document->GetMetaObject();
		wxASSERT(metaObject);
		debugClient->EvaluateToolTip(
			metaObject->GetFileName(),
			metaObject->GetDocPath(),
			expression
		);
	}
}

void ibCodeEditor::LoadCallTip()
{
	// Find the word start
	int currentPos = GetRealPosition();

	wxString expression, keyword, currentWord, sDescription; bool hasPoint = true;

	if (!PrepareExpression(currentPos, expression, keyword, currentWord, hasPoint)) {
		if (hasPoint) {
			m_precompileModule->SetCurrentPos(GetRealPosition());
			//Collect text
			if (m_precompileModule->Compile()) {
				ibValue vObject = m_precompileModule->GetComputeValue();
				for (long i = 0; i < vObject.GetNMethods(); i++) {
					wxString sMethod = vObject.GetMethodName(i);
					if (stringUtils::CompareString(sMethod, currentWord)) {
						sDescription = vObject.GetMethodHelper(i);
						break;
					}
				}

				ibRuntimeModuleDataObject* moduleDataObject = dynamic_cast<ibRuntimeModuleDataObject*>(vObject.GetRef());
				if (moduleDataObject) {
					const ibValueMetaObjectModuleBase* computeModuleObject = moduleDataObject->GetMetaObject();
					if (computeModuleObject) {
						ibParserModule cParser;
						if (cParser.ParseModule(computeModuleObject->GetModuleText())) {
							for (auto code : cParser.GetAllContent()) {
								if (code.m_eType == eExportProcedure || code.m_eType == eExportFunction) {
									if (stringUtils::CompareString(code.m_name, currentWord)) {
										sDescription = code.m_shortDescription;
										break;
									}
								}
							}
						}
					}
				}

				ibValueManagerDataObject* managerDataObject = dynamic_cast<ibValueManagerDataObject*>(vObject.GetRef());
				if (managerDataObject) {
					ibValueMetaObjectCommonModule* computeManagerModule = managerDataObject->GetModuleManager();
					if (computeManagerModule) {
						ibParserModule cParser;
						if (cParser.ParseModule(computeManagerModule->GetModuleText())) {
							for (auto code : cParser.GetAllContent()) {
								if (stringUtils::CompareString(code.m_name, currentWord)) {
									sDescription = code.m_shortDescription;
									break;
								}
							}
						}
					}
				}
			}
		}
		else
		{
			//Collect text
			m_precompileModule->SetCurrentPos(GetRealPosition());

			if (m_precompileModule->Compile()) {
				ibPrecompileContext* rootContext = m_precompileModule->GetContext();
				for (auto function : rootContext->m_functions) {
					ibPrecompileFunction* functionContext = function.second;
					if (stringUtils::CompareString(function.first, currentWord)) {
						sDescription = functionContext->m_shortDescription;
						break;
					}
				}
			}
		}
	}
	else {

		if (stringUtils::CompareString(keyword, wxT("new"))) {
			if (ibValue::IsRegisterCtor(expression)) {
				const ibCtorAbstractType* objectValueAbstract =
					ibValue::GetAvailableCtor(expression);
				ibValue* newObject = objectValueAbstract->CreateObject();
				ibValue::ibValueMethodHelper* methodHelper = newObject->GetPMethods();
				if (methodHelper != nullptr) {
					for (long idx = 0; idx < methodHelper->GetNConstructors(); idx++) {
						sDescription = methodHelper->GetConstructorHelper(idx);
					}
				}
				wxDELETE(newObject);
			}
		}
	}

	if (!sDescription.IsEmpty()) {
		m_ct.Show(currentPos, sDescription);
	}

	m_precompileModule->Clear();
}

void ibCodeEditor::LoadSysKeyword()
{
	m_precompileModule->SetCurrentPos(GetRealPosition());

	for (int i = 0; i < LastKeyWord; i++) {
		m_ac.Append(ibContentType::eVariable,
			s_listKeyWord[i].m_strKeyWord,
			s_listKeyWord[i].m_strShortDescription
		);
	}

	if (m_precompileModule->Compile()) {
		ibPrecompileContext* rootContext = m_precompileModule->GetContext();
		for (const auto& variable : rootContext->m_variables) {
			const ibPrecompileVariable& v = variable.second;
			if (v.m_isTempVar)
				continue;
			m_ac.Append(v.m_isExport ?
				ibContentType::eExportVariable : ibContentType::eVariable, v.m_realName, wxEmptyString
			);
		}

		for (auto function : rootContext->m_functions) {
			ibPrecompileFunction* functionContext = function.second;
			if (functionContext->m_context) {
				if (functionContext->m_context->m_returnKind == RETURN_FUNCTION) {
					m_ac.Append(functionContext->m_isExport ? ibContentType::eExportFunction : ibContentType::eFunction, functionContext->m_realName, functionContext->m_shortDescription);
				}
				else {
					m_ac.Append(functionContext->m_isExport ? ibContentType::eExportProcedure : ibContentType::eProcedure, functionContext->m_realName, functionContext->m_shortDescription);
				}
			}
			else {
				m_ac.Append(functionContext->m_isExport ? ibContentType::eExportFunction : ibContentType::eFunction, functionContext->m_realName, functionContext->m_shortDescription);
			}

			if (m_precompileModule->GetCurrentContext() && m_precompileModule->GetCurrentContext() == functionContext->m_context) {
				for (const auto& variable : m_precompileModule->GetCurrentContext()->m_variables) {
					const ibPrecompileVariable& v = variable.second;
					if (v.m_isTempVar)
						continue;
					m_ac.Append(v.m_isExport ?
						ibContentType::eExportVariable : ibContentType::eVariable, v.m_realName, wxEmptyString
					);
				}
			}
		}
	}

	m_precompileModule->Clear();
}

void ibCodeEditor::LoadIntelliList()
{
	m_precompileModule->SetCurrentPos(GetRealPosition());
	m_precompileModule->SetCalcValue(true);

	//Collect text
	if (m_precompileModule->Compile()) {
		AddKeywordFromObject(m_precompileModule->GetComputeValue());
	}

	m_precompileModule->SetCalcValue(false);
	m_precompileModule->Clear();
}

#include "backend/metaData.h"
#include "backend/objCtor.h"

void ibCodeEditor::LoadFromKeyWord(const wxString& keyword)
{
	if (stringUtils::CompareString(keyword, wxT("new"))) {
		for (auto class_obj : ibValue::GetListCtorsByType(ibCtorObjectType::ibCtorObjectType_object_value))
			m_ac.Append(ibContentType::eVariable, class_obj->GetClassName(), wxEmptyString);
	}
	else if (stringUtils::CompareString(keyword, wxT("type")))
	{
		for (auto class_obj : ibValue::GetListCtorsByType(ibCtorObjectType::ibCtorObjectType_object_primitive))
			m_ac.Append(ibContentType::eVariable, class_obj->GetClassName(), wxEmptyString);

		for (auto class_obj : ibValue::GetListCtorsByType(ibCtorObjectType::ibCtorObjectType_object_value))
			m_ac.Append(ibContentType::eVariable, class_obj->GetClassName(), wxEmptyString);

		for (auto class_obj : ibValue::GetListCtorsByType(ibCtorObjectType::ibCtorObjectType_object_control))
			m_ac.Append(ibContentType::eVariable, class_obj->GetClassName(), wxEmptyString);

		for (auto class_obj : ibValue::GetListCtorsByType(ibCtorObjectType::ibCtorObjectType_object_system))
			m_ac.Append(ibContentType::eVariable, class_obj->GetClassName(), wxEmptyString);

		for (auto class_obj : ibValue::GetListCtorsByType(ibCtorObjectType::ibCtorObjectType_object_enum))
			m_ac.Append(ibContentType::eVariable, class_obj->GetClassName(), wxEmptyString);

		if (m_document) {
			const ibValueMetaObject* metaObject = m_document->GetMetaObject();
			if (metaObject) {
				ibMetaData* metaData = metaObject->GetMetaData();
				wxASSERT(metaData);

				for (auto class_obj : metaData->GetListCtorsByType(ibCtorObjectMetaType::ibCtorObjectMetaType_Object))
					m_ac.Append(ibContentType::eVariable, class_obj->GetClassName(), wxEmptyString);
				for (auto class_obj : metaData->GetListCtorsByType(ibCtorObjectMetaType::ibCtorObjectMetaType_Reference))
					m_ac.Append(ibContentType::eVariable, class_obj->GetClassName(), wxEmptyString);
				for (auto class_obj : metaData->GetListCtorsByType(ibCtorObjectMetaType::ibCtorObjectMetaType_List))
					m_ac.Append(ibContentType::eVariable, class_obj->GetClassName(), wxEmptyString);
				for (auto class_obj : metaData->GetListCtorsByType(ibCtorObjectMetaType::ibCtorObjectMetaType_Manager))
					m_ac.Append(ibContentType::eVariable, class_obj->GetClassName(), wxEmptyString);
				for (auto class_obj : metaData->GetListCtorsByType(ibCtorObjectMetaType::ibCtorObjectMetaType_Selection))
					m_ac.Append(ibContentType::eVariable, class_obj->GetClassName(), wxEmptyString);
			}
		}
	}
	else if (stringUtils::CompareString(keyword, wxT("showCommonForm"))
		|| stringUtils::CompareString(keyword, wxT("getCommonForm")))
	{
		const ibValueMetaObject* metaObject = m_document->GetMetaObject();
		wxASSERT(metaObject);
		ibMetaData* metaData = metaObject->GetMetaData();
		wxASSERT(metaData);

		for (const auto object : metaData->GetAnyArrayObject(g_metaCommonFormCLSID))
			m_ac.Append(ibContentType::eVariable, object->GetName(), wxEmptyString);
	}
}

#include "backend/fileSystem/fs.h"

void ibCodeEditor::ShowAutoComplete(const ibDebugAutoCompleteData& autoCompleteData)
{
	m_ac.Cancel();

	for (unsigned int i = 0; i < autoCompleteData.m_arrVar.size(); i++) {
		m_ac.Append(ibContentType::eVariable, autoCompleteData.m_arrVar[i].m_variableName, wxEmptyString);
	}

	for (unsigned int i = 0; i < autoCompleteData.m_arrMeth.size(); i++) {
		m_ac.Append(autoCompleteData.m_arrMeth[i].m_methodRet ? ibContentType::eFunction : ibContentType::eProcedure,
			autoCompleteData.m_arrMeth[i].m_methodName,
			autoCompleteData.m_arrMeth[i].m_methodHelper
		);
	}

	m_ac.Start(autoCompleteData.m_keyword,
		autoCompleteData.m_currentPos,
		autoCompleteData.m_keyword.Length(),
		TextHeight(GetCurrentLine())
	);

	wxPoint position = PointFromPosition(autoCompleteData.m_currentPos);
	position.y += TextHeight(GetCurrentLine());
	m_ac.Show(position);
}