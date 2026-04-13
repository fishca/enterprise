#include "byteCode.h"

////////////////////////////////////////////////////////////////////////
// ibByteCode ibByteCode ibByteCode ibByteCode						      //
////////////////////////////////////////////////////////////////////////

long ibByteCode::FindMethod(const wxString& strMethodName) const
{
	auto iterator = std::find_if(m_listFunc.begin(), m_listFunc.end(),
		[strMethodName](const auto pair) { return stringUtils::CompareString(strMethodName, pair.first); });
	if (iterator != m_listFunc.end())
		return (long)iterator->second;
	return wxNOT_FOUND;
}

long ibByteCode::FindExportMethod(const wxString& strMethodName) const
{
	auto iterator = std::find_if(m_listExportFunc.begin(), m_listExportFunc.end(),
		[strMethodName](const auto pair) {return stringUtils::CompareString(strMethodName, pair.first); });
	if (iterator != m_listExportFunc.end())
		return (long)iterator->second;
	return wxNOT_FOUND;
}

long ibByteCode::FindFunction(const wxString& funcName) const
{
	auto iterator = std::find_if(m_listFunc.begin(), m_listFunc.end(),
		[funcName](const auto pair) { return stringUtils::CompareString(funcName, pair.first) && pair.second.m_bCodeRet; });
	if (iterator != m_listFunc.end())
		return (long)iterator->second;
	return wxNOT_FOUND;
}

long ibByteCode::FindExportFunction(const wxString& funcName) const
{
	auto iterator = std::find_if(m_listExportFunc.begin(), m_listExportFunc.end(),
		[funcName](const auto pair) { return stringUtils::CompareString(funcName, pair.first) && pair.second.m_bCodeRet; });
	if (iterator != m_listExportFunc.end())
		return (long)iterator->second;
	return wxNOT_FOUND;
}

long ibByteCode::FindProcedure(const wxString& procName) const
{
	auto iterator = std::find_if(m_listFunc.begin(), m_listFunc.end(),
		[procName](const auto pair) { return stringUtils::CompareString(procName, pair.first) && !pair.second.m_bCodeRet; });
	if (iterator != m_listFunc.end())
		return (long)iterator->second;
	return wxNOT_FOUND;
}

long ibByteCode::FindExportProcedure(const wxString& procName) const
{
	auto iterator = std::find_if(m_listExportFunc.begin(), m_listExportFunc.end(),
		[procName](const auto pair) { return stringUtils::CompareString(procName, pair.first) && !pair.second.m_bCodeRet; });
	if (iterator != m_listExportFunc.end())
		return (long)iterator->second;
	return wxNOT_FOUND;
}

long ibByteCode::FindVariable(const wxString& strVarName) const
{
	auto iterator = std::find_if(m_listVar.begin(), m_listVar.end(),
		[strVarName](const std::pair<const wxString, long>& pair) { return stringUtils::CompareString(strVarName, pair.first); });
	if (iterator != m_listVar.end())
		return (long)iterator->second;
	return wxNOT_FOUND;
}

long ibByteCode::FindExportVariable(const wxString& strVarName) const
{
	auto iterator = std::find_if(m_listExportVar.begin(), m_listExportVar.end(),
		[strVarName](const std::pair<const wxString, long>& pair) { return stringUtils::CompareString(strVarName, pair.first); });
	if (iterator != m_listExportVar.end())
		return (long)iterator->second;
	return wxNOT_FOUND;
}