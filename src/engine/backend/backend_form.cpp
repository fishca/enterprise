#include "backend_form.h"
#include "backend/backend_exception.h"
#include "backend/backend_mainFrame.h"

#if !wxUSE_EXTENDED_RTTI
wxClassInfo ibBackendControlFrame::ms_classInfo(wxT("ibBackendControlFrame"), 0, 0,
	(int)sizeof(wxObject),
	(wxObjectConstructorFn)nullptr);

wxClassInfo* ibBackendControlFrame::GetClassInfo() const {
	return &ibBackendControlFrame::ms_classInfo;
}
#endif

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

ibBackendValueForm* ibBackendValueForm::CreateNewForm(
	const ibValueMetaObjectFormBase* creator,
	ibBackendControlFrame* ownerControl,
	ibSourceDataObject* srcObject,
	const ibUniqueKey& formGuid
)
{
	if (backend_mainFrame != nullptr) {
		ibBackendValueForm* createdForm = backend_mainFrame->CreateNewForm(
			creator,
			ownerControl,
			srcObject,
			formGuid
		);

		if (createdForm == nullptr) {
			ibBackendCoreException::Error(_("Context functions are not available in this frontend library!"));
			return nullptr;
		}
		return createdForm;
	}

	ibBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

ibUniqueKey ibBackendValueForm::CreateFormUniqueKey(ibBackendControlFrame* ownerControl, ibSourceDataObject* sourceObject, const ibUniqueKey& formGuid)
{
	if (backend_mainFrame != nullptr)  return backend_mainFrame->CreateFormUniqueKey(ownerControl, sourceObject, formGuid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return ibUniqueKey();
}

ibBackendValueForm* ibBackendValueForm::FindFormByUniqueKey(ibBackendControlFrame* ownerControl, ibSourceDataObject* sourceObject, const ibUniqueKey& formGuid)
{
	if (backend_mainFrame != nullptr) return backend_mainFrame->FindFormByUniqueKey(ownerControl, sourceObject, formGuid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

ibBackendValueForm* ibBackendValueForm::FindFormByUniqueKey(const ibUniqueKey& guid)
{
	if (backend_mainFrame != nullptr)  return backend_mainFrame->FindFormByUniqueKey(guid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

ibBackendValueForm* ibBackendValueForm::FindFormByControlUniqueKey(const ibUniqueKey& guid)
{
	if (backend_mainFrame != nullptr)  return backend_mainFrame->FindFormByControlUniqueKey(guid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

ibBackendValueForm* ibBackendValueForm::FindFormBySourceUniqueKey(const ibUniqueKey& guid)
{
	if (backend_mainFrame != nullptr)  return backend_mainFrame->FindFormBySourceUniqueKey(guid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

bool ibBackendValueForm::UpdateFormUniqueKey(const CUniquePairKey& guid)
{
	if (backend_mainFrame != nullptr) return backend_mainFrame->UpdateFormUniqueKey(guid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return false;
}