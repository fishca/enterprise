#ifndef _CORE_OES_H__
#define _CORE_OES_H__

#include <wx/wx.h>

#include "backend.h"

extern BACKEND_API unsigned int GetBuildId();

#include "guid.h"
#include "clsid.h"
#include "number.h"
#include "typeconv.h"
#include "stringUtils.h"

//*******************************************************************************************

#define oes_clipboard_metadata	wxT("oes_clipboard_metadata")
#define oes_clipboard_frame		wxT("oes_clipboard_frame")
#define oes_clipboard_interface	wxT("oes_clipboard_interface")
#define oes_clipboard_role		wxT("oes_clipboard_role")
#define oes_clipboard_template	wxT("oes_clipboard_template")

//*******************************************************************************************
//*                                 Special structures                                      *
//*******************************************************************************************

#define emptyDate -62135604000000ll

typedef int role_identifier_t;
typedef int meta_identifier_t;
typedef int form_identifier_t;
typedef int action_identifier_t;

typedef unsigned wxLongLong_t picture_identifier_t;
typedef unsigned int version_identifier_t;

typedef std::map<
	meta_identifier_t, class BACKEND_API CValue
> valueArray_t;


//*******************************************************************************************
//*                                 Special enumeration                                     *
//*******************************************************************************************

enum eValueTypes {

	TYPE_EMPTY = 0,
	TYPE_BOOLEAN = 1,
	TYPE_NUMBER = 2,
	TYPE_DATE = 3,
	TYPE_STRING = 4,
	TYPE_NULL = 5,

	TYPE_REFFER = 100, // object reference

	TYPE_VALUE = 200, // value
	TYPE_ENUM = 201, // enumeration
	TYPE_OLE = 202, // ole object

	TYPE_LAST,
};

//*******************************************************************************************
//*                                 Declare special var                                     *
//*******************************************************************************************

#define _USE_CONTROL_VALUECAST 1 
//firebird doesn't support multiple transaction 
#define _USE_SAVE_METADATA_IN_TRANSACTION 1 
// full parser is very slowly ...
#define _USE_OLD_TEXT_PARSER_IN_CODE_EDITOR 0
//debugger
#define _USE_64_BIT_POINT_IN_DEBUGGER 1
//have bugs....
#define _USE_NET_COMPRESSOR 0
//use dynamic linking 
#define _USE_DYNAMIC_DATABASE_LAYER_LINKING 1
//don't use exception in db layer
#define _USE_DATABASE_LAYER_EXCEPTIONS 0

//max precision 
#define MAX_PRECISION_NUMBER 32
//max precision 
#define MAX_LENGTH_STRING 1024
//stack guard
#define MAX_OBJECTS_LEVEL 100
//max record level 
#define MAX_REC_COUNT 200

#if defined(_LP64) || defined(__LP64__) || defined(__arch64__) || defined(_WIN64)
#define MAX_STATIC_VAR 25ll
#else 
#define MAX_STATIC_VAR 10ll
#endif 

//*******************************************************************************************
//*                                 Versions support									    *
//*******************************************************************************************

#define version_generate(major, minor, release) \
		( (major * 1000) + (minor * 100) + release )

enum eProgramVersion {
	version_oes_1_0_0 = version_generate(1, 0, 0),
	version_oes_1_0_1 = version_generate(1, 0, 1),
	version_oes_last  = version_oes_1_0_1
};

//*******************************************************************************************

#define COMPONENT_TYPE_ABSTRACT		 0
#define COMPONENT_TYPE_METADATA		 COMPONENT_TYPE_ABSTRACT

#endif 