#ifndef __MYSQL_PARAMETER_H__
#define __MYSQL_PARAMETER_H__

// For compilers that support precompilation, includes "wx.h".
#include <wx/wxprec.h>

#ifdef __BORLANDC__
#pragma hdrstop
#endif

#ifndef WX_PRECOMP
#include <wx/wx.h>
#endif

#include <wx/datetime.h>



#include "backend/databaseLayer/databaseStringConverter.h"
#include "engine/mysql.h"

class ibDatabaseParameterMySQL : public ibDatabaseStringConverter
{
public:
	// ctor
	ibDatabaseParameterMySQL();
	ibDatabaseParameterMySQL(const wxString& strValue);
	ibDatabaseParameterMySQL(int nValue);
	ibDatabaseParameterMySQL(double dblValue);
	ibDatabaseParameterMySQL(const ibNumber& numValue);
	ibDatabaseParameterMySQL(bool bValue);
	ibDatabaseParameterMySQL(const wxDateTime& dateValue);
	ibDatabaseParameterMySQL(const void* pData, long nDataLength);

	// dtor
	virtual ~ibDatabaseParameterMySQL();

	enum {
		PARAM_STRING = 0,
		PARAM_INT,
		PARAM_DOUBLE,
		PARAM_NUMBER,
		PARAM_DATETIME,
		PARAM_BOOL,
		PARAM_BLOB,
		PARAM_NULL
	};

	long unsigned int GetDataLength();
	long unsigned int* GetDataLengthPtr();

	const void* GetDataPtr();
	int GetParameterType();

	enum_field_types GetBufferType();

private:
	int m_nParameterType;

	// A union would probably be better here
	wxString m_strValue;
	int m_nValue;
	double m_dblValue;

	ibNumber m_numValue;

	MYSQL_TIME* m_pDate;
	bool m_bValue;
	wxMemoryBuffer m_BufferValue;
	wxCharBuffer m_CharBufferValue;
	long unsigned int m_nBufferLength;
};


#endif // __MYSQL_PARAMETER_H__
