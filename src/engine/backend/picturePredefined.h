#ifndef __PICTURE_PREDEFINED_H__
#define __PICTURE_PREDEFINED_H__

#include "backend/backend_core.h"

//*******************************************************************************
//*                          define common pic							        *
//*******************************************************************************

//COMMON PICTURES
const picture_identifier_t g_picStructureCLSID = string_to_clsid("PC_STRCT");
const picture_identifier_t g_picErrorCLSID = string_to_clsid("PC_ERROR");

const picture_identifier_t g_picCloseFormCLSID = string_to_clsid("PC_CLOSE");
const picture_identifier_t g_picUpdateFormCLSID = string_to_clsid("PC_REFRE");
const picture_identifier_t g_picHelpFormCLSID = string_to_clsid("PC_HELP");

const picture_identifier_t g_picChangeFormCLSID = string_to_clsid("PC_CHAGF");

const picture_identifier_t g_picAddCLSID = string_to_clsid("PC_ADDVL");
const picture_identifier_t g_picEditCLSID = string_to_clsid("PC_EDITV");
const picture_identifier_t g_picCopyCLSID = string_to_clsid("PC_COPYV");
const picture_identifier_t g_picDeleteCLSID = string_to_clsid("PC_DELVL");

const picture_identifier_t g_picAddFolderCLSID = string_to_clsid("PC_ADDFV");
const picture_identifier_t g_picSelectCLSID = string_to_clsid("PC_SELVL");

const picture_identifier_t g_picFilterCLSID = string_to_clsid("PC_FLTER");
const picture_identifier_t g_picFilterSetCLSID = string_to_clsid("PC_FLTES");
const picture_identifier_t g_picFilterClearCLSID = string_to_clsid("PC_FLTEC");

const picture_identifier_t g_picCloneCLSID = string_to_clsid("PC_CLONE");
const picture_identifier_t g_picSaveCLSID = string_to_clsid("PC_SAVE");
const picture_identifier_t g_picPostCLSID = string_to_clsid("PC_POST");
const picture_identifier_t g_picMarkAsDeleteCLSID = string_to_clsid("PC_MDEL");
const picture_identifier_t g_picGenerateCLSID = string_to_clsid("PC_GENTE");

const picture_identifier_t g_picUserCLSID = string_to_clsid("PC_USER");
const picture_identifier_t g_picUserActiveCLSID = string_to_clsid("PC_USRAC");
const picture_identifier_t g_picUserListCLSID = string_to_clsid("PC_USRLS");

const picture_identifier_t g_picAuthenticationCLSID = string_to_clsid("PC_ATTON");
#endif 