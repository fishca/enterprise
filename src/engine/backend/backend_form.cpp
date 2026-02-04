#include "backend_form.h"
#include "backend/backend_exception.h"
#include "backend/backend_mainFrame.h"

#if !wxUSE_EXTENDED_RTTI
wxClassInfo IBackendControlFrame::ms_classInfo(wxT("IBackendControlFrame"), 0, 0,
	(int)sizeof(wxObject),
	(wxObjectConstructorFn)nullptr);

wxClassInfo* IBackendControlFrame::GetClassInfo() const {
	return &IBackendControlFrame::ms_classInfo;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

IBackendValueForm* IBackendValueForm::CreateNewForm(
	const IValueMetaObjectForm* creator,
	IBackendControlFrame* ownerControl,
	ISourceDataObject* srcObject,
	const CUniqueKey& formGuid
)
{
	if (backend_mainFrame != nullptr) {
		IBackendValueForm* createdForm = backend_mainFrame->CreateNewForm(
			creator,
			ownerControl,
			srcObject,
			formGuid
		);

		if (createdForm == nullptr) {
			CBackendCoreException::Error(_("Context functions are not available in this frontend library!"));
			return nullptr;
		}
		return createdForm;
	}

	CBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

CUniqueKey IBackendValueForm::CreateFormUniqueKey(IBackendControlFrame* ownerControl, ISourceDataObject* sourceObject, const CUniqueKey& formGuid)
{
	if (backend_mainFrame != nullptr)  return backend_mainFrame->CreateFormUniqueKey(ownerControl, sourceObject, formGuid);
	CBackendCoreException::Error(_("Context functions are not available!"));
	return CUniqueKey();
}

IBackendValueForm* IBackendValueForm::FindFormByUniqueKey(IBackendControlFrame* ownerControl, ISourceDataObject* sourceObject, const CUniqueKey& formGuid)
{
	if (backend_mainFrame != nullptr) return backend_mainFrame->FindFormByUniqueKey(ownerControl, sourceObject, formGuid);
	CBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

IBackendValueForm* IBackendValueForm::FindFormByUniqueKey(const CUniqueKey& guid)
{
	if (backend_mainFrame != nullptr)  return backend_mainFrame->FindFormByUniqueKey(guid);
	CBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

IBackendValueForm* IBackendValueForm::FindFormByControlUniqueKey(const CUniqueKey& guid)
{
	if (backend_mainFrame != nullptr)  return backend_mainFrame->FindFormByControlUniqueKey(guid);
	CBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

IBackendValueForm* IBackendValueForm::FindFormBySourceUniqueKey(const CUniqueKey& guid)
{
	if (backend_mainFrame != nullptr)  return backend_mainFrame->FindFormBySourceUniqueKey(guid);
	CBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

bool IBackendValueForm::UpdateFormUniqueKey(const CUniquePairKey& guid)
{
	if (backend_mainFrame != nullptr) return backend_mainFrame->UpdateFormUniqueKey(guid);
	CBackendCoreException::Error(_("Context functions are not available!"));
	return false;
}