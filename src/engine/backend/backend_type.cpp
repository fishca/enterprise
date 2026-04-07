#include "backend_type.h"
#include "backend/compiler/enumUnit.h"

//***********************************************************************
//*                         Type factory                                * 
//***********************************************************************

ibValue ibBackendTypeFactory::CreateValue() const
{
	ibValue* refData = CreateValueRef();
	return refData ?
		refData : ibValue();
}

ibValue* ibBackendTypeFactory::CreateValueRef() const
{
	const ibTypeDescription& typeDesc = GetTypeDesc();
	if (typeDesc.GetClsidCount() == 1) {
		const ibClassID& clsid = typeDesc.GetFirstClsid();
		if (ibValue::IsRegisterCtor(clsid)) {
			const ibCtorAbstractType* so = ibValue::GetAvailableCtor(clsid);
			if (so->GetObjectTypeCtor() == ibCtorObjectType::ibCtorObjectType_object_enum) {
				try {
					std::shared_ptr<ibValueEnumerationWrapper> enumVal(
						ibValue::CreateAndConvertObjectRef<ibValueEnumerationWrapper>(so->GetClassName())
					);
					return enumVal->GetEnumVariantValue();
				}
				catch (...) {
				}
				return nullptr;
			}
			try {
				return ibValue::CreateObjectRef(so->GetClassType());
			}
			catch (...) {
				return nullptr;
			}
		}
	}
	return nullptr;
}

#include "backend/system/value/valueType.h"

ibValue ibBackendTypeFactory::AdjustValue() const
{
	return ibValueTypeDescription::AdjustValue(GetTypeDesc());
}

ibValue ibBackendTypeFactory::AdjustValue(const ibValue& varValue) const
{
	return ibValueTypeDescription::AdjustValue(GetTypeDesc(), varValue);
}

/////////////////////////////////////////////////////////////////////////////////////

ibValue ibBackendTypeConfigFactory::CreateValue() const
{
	ibValue* refData = CreateValueRef();
	if (refData == nullptr)
		return ibValue();
	return refData;
}

#include "backend/metadata.h"
#include "backend/objCtor.h"

ibValue* ibBackendTypeConfigFactory::CreateValueRef() const
{
	ibMetaData const* metaData = GetMetaData();
	wxASSERT(metaData);
	const ibTypeDescription& typeDesc = GetTypeDesc();
	if (typeDesc.GetClsidCount() == 1) {
		const ibCtorMetaValueType* so = metaData->GetTypeCtor(typeDesc.GetFirstClsid());
		if (so != nullptr) {
			try {
				return metaData->CreateObjectRef(so->GetClassType());
			}
			catch (...) {
				return nullptr;
			}
		}
	}
	return ibBackendTypeFactory::CreateValueRef();
}

ibValue ibBackendTypeConfigFactory::AdjustValue() const
{
	return ibValueTypeDescription::AdjustValue(
		GetTypeDesc(),
		GetMetaData()
	);
}

ibValue ibBackendTypeConfigFactory::AdjustValue(const ibValue& varValue) const
{
	return ibValueTypeDescription::AdjustValue(
		GetTypeDesc(),
		varValue,
		GetMetaData()
	);
}

/////////////////////////////////////////////////////////////////////////////////////

bool ibBackendTypeSourceFactory::FilterSource(const ibSourceExplorer& src, const ibMetaID& id) const
{
	return !src.IsTableSection();
}
