#ifndef _ACCUMULATION_REGISTER_ENUM_H__
#define _ACCUMULATION_REGISTER_ENUM_H__

enum ibRegisterType {
	eBalances,
	eTurnovers
};

enum ibRecordType {
	eExpense,
	eReceipt
};

#pragma region enumeration
#include "backend/compiler/enumUnit.h"
class ibValueEnumAccumulationRegisterType : public ibValueEnumeration<ibRegisterType> {
	wxDECLARE_DYNAMIC_CLASS(ibValueEnumAccumulationRegisterType);
public:
	ibValueEnumAccumulationRegisterType() : ibValueEnumeration() {}
	//ibValueEnumAccumulationRegisterType(ibRegisterType mode) : ibValueEnumeration(mode) {}

	virtual void CreateEnumeration() {
		AddEnumeration(ibRegisterType::eBalances, wxT("Balances"), _("Balances"));
		AddEnumeration(ibRegisterType::eTurnovers, wxT("Turnovers"), _("Turnovers"));
	}
};
class ibValueEnumAccumulationRegisterRecordType : public ibValueEnumeration<ibRecordType> {
	wxDECLARE_DYNAMIC_CLASS(ibValueEnumAccumulationRegisterRecordType);
public:
	static ibValue CreateDefEnumValue() {
		return ibValue::CreateEnumObject<ibValueEnumAccumulationRegisterRecordType>(ibRecordType::eExpense);
	}

	ibValueEnumAccumulationRegisterRecordType() : ibValueEnumeration() {}
	//ibValueEnumAccumulationRegisterRecordType(ibRecordType recordType) : ibValueEnumeration(recordType) {}

	virtual void CreateEnumeration() {
		AddEnumeration(eExpense, wxT("Expense"), _("Expense"));
		AddEnumeration(eReceipt, wxT("Receipt"), _("Receipt"));
	}
};
const ibClassID g_enumRecordTypeCLSID = string_to_clsid("EN_RETP");
#pragma endregion 

#endif