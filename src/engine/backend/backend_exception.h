#ifndef _backend_exception_h__
#define _backend_exception_h__

enum { //Error message numbers
	ERROR_USAGE = 0,
	ERROR_FILE_READ,
	ERROR_FILE_OPEN,
	ERROR_ASSERT,
	ERROR_ASSERT_VALID,
	ERROR_SYS1,
	ERROR_TRANSLATE_BYTE,
	ERROR_TRANSLATE_BOOLEAN,
	ERROR_TRANSLATE_WORD,
	ERROR_TRANSLATE_NUMBER,
	ERROR_TRANSLATE_STRING,
	ERROR_TRANSLATE_DATE,
	ERROR_IDENTIFIER_DUPLICATE,
	ERROR_LABEL_DEFINE,
	ERROR_KEYWORD_DEFINE,
	ERROR_CODE_DEFINE,
	ERROR_IDENTIFIER_DEFINE,
	ERROR_IDENTIFIER_REGION,
	ERROR_CODE,
	ERROR_DELIMETER,
	ERROR_FUNC_DELIMETER,
	ERROR_FUNC_DEFINE,
	ERROR_RETURN,
	ERROR_CONST_DEFINE,
	ERROR_ENDFUNC_DEFINE,
	ERROR_FILE_WRITE,
	ERROR_EXPRESSION,
	ERROR_KEYWORD,
	ERROR_IDENTIFIER_NOT_FOUND,
	ERROR_USE_BREAK,
	ERROR_USE_CONTINUE,
	ERROR_USE_RETURN,
	ERROR_USE_BLOCK,
	ERROR_EXPRESSION_REQUIRE,
	ERROR_CALL_FUNCTION,
	ERROR_DEF_VARIABLE,
	ERROR_DEF_FUNCTION,
	ERROR_MANY_PARAMS,
	ERROR_FEW_PARAMS,
	ERROR_VAR_NOT_FOUND,
	ERROR_END_PROGRAM,
	ERROR_ONLY_FUNCTION,
	ERROR_USE_PROCEDURE_AS_FUNCTION,
	ERROR_ARRAY_SIZE_CONST,
	ERROR_DUBLICATE_IMPORT,
	ERROR_MODULE_NOT_FOUND,
	ERROR_USE_IMPORT,
	ERROR_USE_ENDDEF,
	ERROR_USE_ENDREGION,
	ERROR_CALL_CONSTRUCTOR,
	ERROR_TYPE_DEF,
	ERROR_BAD_TYPE,
	ERROR_BAD_TYPE_EXPRESSION,
	ERROR_NUMBER_TYPE,
	ERROR_BAD_TYPE_EXPRESSION_B,
	ERROR_BAD_TYPE_EXPRESSION_N,
	ERROR_BAD_TYPE_EXPRESSION_S,
	ERROR_BAD_TYPE_EXPRESSION_D,
	ERROR_TYPE_OPERATION,

	LastError
};

#include "backend/backend.h"

class BACKEND_API CBackendException {
protected:

	class wxFormatErrorString : public wxFormatString {
	public:
		wxFormatErrorString(int codeError)
			: wxFormatString(CBackendException::GetErrorDesc(codeError)) {
		}

#ifndef wxNO_IMPLICIT_WXSTRING_ENCODING
		wxFormatErrorString(const char* str)
			: wxFormatString(str) {
		}
#endif
		wxFormatErrorString(const wchar_t* str)
			: wxFormatString(str) {
		}
		wxFormatErrorString(const wxString& str)
			: wxFormatString(str) {
		}
		wxFormatErrorString(const wxCStrData& str)
			: wxFormatString(str) {
		}
#ifndef wxNO_IMPLICIT_WXSTRING_ENCODING
		wxFormatErrorString(const wxScopedCharBuffer& str)
			: wxFormatString(str) {
		}
#endif
		wxFormatErrorString(const wxScopedWCharBuffer& str)
			: wxFormatString(str) {
		}
	};

	CBackendException(const wxString& strErrorDescription);

public:

	WX_DEFINE_VARARG_FUNC(static wxString, Format, 1, (const wxFormatErrorString&),
		DoFormatWchar, DoFormatUtf8);

	//get error description
	const wxString GetErrorDescription() const { return m_strErrorDescription; }

	//error from proc unit/compile module 
	static void ProcessError(const CBackendException* err, const struct CByteUnit& error);
	static void ProcessError(const wxString& strFileName,
		const wxString& strModuleName, const wxString& strDocPath,
		const unsigned int currPos, const unsigned int currLine,
		const wxString& strCodeLineError, const int codeError, const wxString& strErrorDesc // error code from compile module
	);

	static wxString FindErrorCodeLine(const wxString& sBuffer, unsigned int currPos);
	static wxString GetLastError() {
		const wxString strLastError = ms_strError;
		ms_strError = wxEmptyString;
		return strLastError;
	}

	static void SetEvalMode(bool mode = true) { ms_evalMode = mode; }
	static bool IsEvalMode() { return ms_evalMode; }

protected:

	static wxString FormatV(const wxString& fmt, va_list& list);
	static wxString ms_strError;

private:

	//error handling routines
	static const wxString& GetErrorDesc(int codeError);	
	static wxString ProcessExceptionError(const wxString& strFileName,
		const wxString& strModuleName, const wxString& strDocPath,
		const unsigned int currPos, const unsigned int currLine,
		const wxString& strCodeLineError, const int codeError, const wxString& strErrorDesc // error code from compile module
	);

#if !wxUSE_UTF8_LOCALE_ONLY
	static wxString DoFormatWchar(const wxChar* format, ...);
#endif
#if wxUSE_UNICODE_UTF8
	static wxString DoFormatWchar(const wxChar* format, ...);
#endif

	static bool ms_evalMode;
	wxString m_strErrorDescription;
};

#pragma region _exception_h_

class BACKEND_API CBackendCoreException : public CBackendException {
	CBackendCoreException(const wxString& strErrorDescription) : CBackendException(strErrorDescription) {}
public:

	WX_DEFINE_VARARG_FUNC(static void, Error, 1, (const wxFormatErrorString&),
		DoErrorWchar, DoErrorUtf8);

private:

#if !wxUSE_UTF8_LOCALE_ONLY
	static void DoErrorWchar(const wxChar* format, ...);
#endif
#if wxUSE_UNICODE_UTF8
	static void DoErrorUtf8(const wxChar* format, ...);
#endif
};

class BACKEND_API CBackendInterruptException : public CBackendException {
	CBackendInterruptException() : CBackendException(_("The program was stopped by the user!")) {}
public:
	static void Error();
};

class BACKEND_API CBackendAccessException : public CBackendException {
	CBackendAccessException() : CBackendException(_("Not enough access rights for this user!")) {}
public:
	static void Error();
};
#pragma endregion 

#endif 