#ifndef __PICTURE_PREDEFINED_H__
#define __PICTURE_PREDEFINED_H__

#include "backend/backend_core.h"

//*******************************************************************************
//*                          define common pic							        *
//*******************************************************************************

//COMMON PICTURES
const ibPictureID g_picStructureCLSID = string_to_clsid("PC_STRCT");
const ibPictureID g_picErrorCLSID = string_to_clsid("PC_ERROR");

const ibPictureID g_picCloseFormCLSID = string_to_clsid("PC_CLOSE");
const ibPictureID g_picUpdateFormCLSID = string_to_clsid("PC_REFRE");
const ibPictureID g_picHelpFormCLSID = string_to_clsid("PC_HELP");

const ibPictureID g_picChangeFormCLSID = string_to_clsid("PC_CHAGF");

const ibPictureID g_picAddCLSID = string_to_clsid("PC_ADDVL");
const ibPictureID g_picEditCLSID = string_to_clsid("PC_EDITV");
const ibPictureID g_picCopyCLSID = string_to_clsid("PC_COPYV");
const ibPictureID g_picDeleteCLSID = string_to_clsid("PC_DELVL");

const ibPictureID g_picAddFolderCLSID = string_to_clsid("PC_ADDFV");
const ibPictureID g_picSelectCLSID = string_to_clsid("PC_SELVL");

const ibPictureID g_picFilterCLSID = string_to_clsid("PC_FLTER");
const ibPictureID g_picFilterSetCLSID = string_to_clsid("PC_FLTES");
const ibPictureID g_picFilterClearCLSID = string_to_clsid("PC_FLTEC");

const ibPictureID g_picCloneCLSID = string_to_clsid("PC_CLONE");
const ibPictureID g_picSaveCLSID = string_to_clsid("PC_SAVE");
const ibPictureID g_picPostCLSID = string_to_clsid("PC_POST");
const ibPictureID g_picMarkAsDeleteCLSID = string_to_clsid("PC_MDEL");
const ibPictureID g_picGenerateCLSID = string_to_clsid("PC_GENTE");
const ibPictureID g_picPrintCLSID = string_to_clsid("PC_PRINT");
const ibPictureID g_picHierarchyCLSID = string_to_clsid("PC_HRCHY");

const ibPictureID g_picUserCLSID = string_to_clsid("PC_USER");
const ibPictureID g_picUserActiveCLSID = string_to_clsid("PC_USRAC");
const ibPictureID g_picUserListCLSID = string_to_clsid("PC_USRLS");

const ibPictureID g_picAuthenticationCLSID = string_to_clsid("PC_ATTON");

#endif 