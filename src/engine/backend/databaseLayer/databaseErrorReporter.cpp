#include "databaseErrorReporter.h"
#include "databaseErrorCodes.h"
#include "databaseLayerException.h"

ibDatabaseErrorReporter::ibDatabaseErrorReporter()
{
	ResetErrorCodes();
}

ibDatabaseErrorReporter::~ibDatabaseErrorReporter()
{
}

const wxString& ibDatabaseErrorReporter::GetErrorMessage()
{
	return m_strErrorMessage;
}

int ibDatabaseErrorReporter::GetErrorCode()
{
	return m_nErrorCode;
}

void ibDatabaseErrorReporter::SetErrorMessage(const wxString& strErrorMessage)
{
	m_strErrorMessage = strErrorMessage;
}

void ibDatabaseErrorReporter::SetErrorCode(int nErrorCode)
{
	m_nErrorCode = nErrorCode;
}

void ibDatabaseErrorReporter::ResetErrorCodes()
{
	m_strErrorMessage.Clear();
	m_nErrorCode = DATABASE_LAYER_OK;
}

#include "backend/backend_exception.h"
#include "backend/system/systemManager.h"

void ibDatabaseErrorReporter::ThrowDatabaseException()
{
	ibValueSystemFunction::Message(GetErrorMessage());

#if _USE_DATABASE_LAYER_EXCEPTIONS == 0
	try {
#endif
		ibBackendCoreException::Error(GetErrorMessage());
#if _USE_DATABASE_LAYER_EXCEPTIONS == 0
	}
	catch (...) {
	}
#endif
}

