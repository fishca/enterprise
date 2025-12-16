#ifndef __DOCUMENT_ENUM_H__
#define __DOCUMENT_ENUM_H__

enum eDocumentWriteMode {
	eDocumentWriteMode_Posting,
	eDocumentWriteMode_UndoPosting,
	eDocumentWriteMode_Write
};

enum eDocumentPostingMode {
	eDocumentPostingMode_RealTime,
	eDocumentPostingMode_Regular
};

#pragma region enumeration
#include "backend/compiler/enumUnit.h"

class CValueEnumDocumentWriteMode : public IEnumeration<eDocumentWriteMode> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumPostingMode);
public:
	CValueEnumDocumentWriteMode() : IEnumeration() {}
	//CValueEnumDocumentWriteMode(eDocumentWriteMode mode) : IEnumeration(mode) {}

	virtual void CreateEnumeration() {
		AddEnumeration(eDocumentWriteMode::eDocumentWriteMode_Posting, wxT("posting"), _("Posting"));
		AddEnumeration(eDocumentWriteMode::eDocumentWriteMode_UndoPosting, wxT("undoPosting"), _("Undo posting"));
		AddEnumeration(eDocumentWriteMode::eDocumentWriteMode_Write, wxT("write"), _("Write"));
	}
};
class CValueEnumDocumentPostingMode : public IEnumeration<eDocumentPostingMode> {
	wxDECLARE_DYNAMIC_CLASS(CValueEnumDocumentPostingMode);
public:
	CValueEnumDocumentPostingMode() : IEnumeration() {}
	//CValueEnumDocumentPostingMode(eDocumentPostingMode mode) : IEnumeration(mode) {}

	virtual void CreateEnumeration() {
		AddEnumeration(eDocumentPostingMode::eDocumentPostingMode_RealTime, wxT("realTime"), _("Real time"));
		AddEnumeration(eDocumentPostingMode::eDocumentPostingMode_Regular, wxT("regular"), _("Regular"));
	}
};
#pragma endregion 

#endif // ! _DOCUMENT_EMUN_H_
