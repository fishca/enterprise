#include "backend_type.h"
#include "backend/compiler/enumUnit.h"

//***********************************************************************
//*                         Type factory                                * 
//***********************************************************************

CValue IBackendTypeFactory::CreateValue() const
{
	CValue* refData = CreateValueRef();
	return refData ?
		refData : CValue();
}

CValue* IBackendTypeFactory::CreateValueRef() const
{
	const CTypeDescription& typeDesc = GetTypeDesc();
	if (typeDesc.GetClsidCount() == 1) {
		const class_identifier_t& clsid = typeDesc.GetFirstClsid();
		if (CValue::IsRegisterCtor(clsid)) {
			const IAbstractTypeCtor* so = CValue::GetAvailableCtor(clsid);
			if (so->GetObjectTypeCtor() == eCtorObjectType::eCtorObjectType_object_enum) {
				try {
					std::shared_ptr<IEnumerationWrapper> enumVal(
						CValue::CreateAndConvertObjectRef<IEnumerationWrapper>(so->GetClassName())
					);
					return enumVal->GetEnumVariantValue();
				}
				catch (...) {
				}
				return nullptr;
			}
			try {
				return CValue::CreateObjectRef(so->GetClassType());
			}
			catch (...) {
				return nullptr;
			}
		}
	}
	return nullptr;
}

#include "backend/system/value/valueType.h"

CValue IBackendTypeFactory::AdjustValue() const
{
	return CValueTypeDescription::AdjustValue(GetTypeDesc());
}

CValue IBackendTypeFactory::AdjustValue(const CValue& varValue) const
{
	return CValueTypeDescription::AdjustValue(GetTypeDesc(), varValue);
}

/////////////////////////////////////////////////////////////////////////////////////

CValue IBackendTypeConfigFactory::CreateValue() const
{
	CValue* refData = CreateValueRef();
	if (refData == nullptr)
		return CValue();
	return refData;
}

#include "backend/metadata.h"
#include "backend/objCtor.h"

CValue* IBackendTypeConfigFactory::CreateValueRef() const
{
	IMetaData const* metaData = GetMetaData();
	wxASSERT(metaData);
	const CTypeDescription& typeDesc = GetTypeDesc();
	if (typeDesc.GetClsidCount() == 1) {
		const IMetaValueTypeCtor* so = metaData->GetTypeCtor(typeDesc.GetFirstClsid());
		if (so != nullptr) {
			try {
				return metaData->CreateObjectRef(so->GetClassType());
			}
			catch (...) {
				return nullptr;
			}
		}
	}
	return IBackendTypeFactory::CreateValueRef();
}

CValue IBackendTypeConfigFactory::AdjustValue() const
{
	return CValueTypeDescription::AdjustValue(
		GetTypeDesc(),
		GetMetaData()
	);
}

CValue IBackendTypeConfigFactory::AdjustValue(const CValue& varValue) const
{
	return CValueTypeDescription::AdjustValue(
		GetTypeDesc(),
		varValue,
		GetMetaData()
	);
}

/////////////////////////////////////////////////////////////////////////////////////

bool IBackendTypeSourceFactory::FilterSource(const CSourceExplorer& src, const meta_identifier_t& id)
{
	return !src.IsTableSection();
}
