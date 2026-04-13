#ifndef __METAOBJECT_METADATA_ENUM_H__
#define __METAOBJECT_METADATA_ENUM_H__

#include "backend/compiler/enumUnit.h"
class ibValueEnumVersion : public ibValueEnumeration<ibProgramVersion> {
	wxDECLARE_DYNAMIC_CLASS(ibValueEnumVersion);
public:

	ibValueEnumVersion() : ibValueEnumeration() {}
	//ibValueEnumVersion(ibProgramVersion v) : ibValueEnumeration(v) {}

	virtual void CreateEnumeration() {
		AddEnumeration(version_oes_1_0_0, wxT("OES_1_0_0"), _("1.0.0"));
		AddEnumeration(version_oes_last, wxT("OES_Last"), _("Don't use compatibility"));
	};
};

class ibValueEnumSyntax : public ibValueEnumeration<ibProgramSyntax> {
	wxDECLARE_DYNAMIC_CLASS(ibValueEnumSyntax);
public:

	ibValueEnumSyntax() : ibValueEnumeration() {}
	//ibValueEnumSyntax(ibProgramSyntax v) : ibValueEnumeration(v) {}

	virtual void CreateEnumeration() {
		AddEnumeration(syntax_vbs, wxT("vbs"), _("vbs"));
		AddEnumeration(syntax_ces, wxT("ces"), _("ces"));
	};
};

#endif