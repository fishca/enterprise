#ifndef _FRONTEND_OES_H__
#define _FRONTEND_OES_H__

#include <wx/wx.h>

// The desktop frontend.dll sets FRONTEND_EXPORTS; wfrontend.dll sets
// WFRONTEND_EXPORTS and compiles the same sources, so both need
// FRONTEND_API to resolve to dllexport. Either define flags the classes
// as "being built here".
#if defined(FRONTEND_EXPORTS) || defined(WFRONTEND_EXPORTS)
#define FRONTEND_API WXEXPORT
#else
#define FRONTEND_API WXIMPORT
#endif

#endif 