#include "backend_form.h"
#include "backend/backend_exception.h"
#include "backend/backend_mainFrame.h"
#include "backend/session/session.h"

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
	if (ibSession::CurrentFrame() != nullptr) {
		ibBackendValueForm* createdForm = ibSession::CurrentFrame()->CreateNewForm(
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
	if (ibSession::CurrentFrame() != nullptr)  return ibSession::CurrentFrame()->CreateFormUniqueKey(ownerControl, sourceObject, formGuid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return ibUniqueKey();
}

ibBackendValueForm* ibBackendValueForm::FindFormByUniqueKey(ibBackendControlFrame* ownerControl, ibSourceDataObject* sourceObject, const ibUniqueKey& formGuid)
{
	if (ibSession::CurrentFrame() != nullptr) return ibSession::CurrentFrame()->FindFormByUniqueKey(ownerControl, sourceObject, formGuid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

ibBackendValueForm* ibBackendValueForm::FindFormByUniqueKey(const ibUniqueKey& guid)
{
	if (ibSession::CurrentFrame() != nullptr)  return ibSession::CurrentFrame()->FindFormByUniqueKey(guid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

ibBackendValueForm* ibBackendValueForm::FindFormByControlUniqueKey(const ibUniqueKey& guid)
{
	if (ibSession::CurrentFrame() != nullptr)  return ibSession::CurrentFrame()->FindFormByControlUniqueKey(guid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

ibBackendValueForm* ibBackendValueForm::FindFormBySourceUniqueKey(const ibUniqueKey& guid)
{
	if (ibSession::CurrentFrame() != nullptr)  return ibSession::CurrentFrame()->FindFormBySourceUniqueKey(guid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return nullptr;
}

bool ibBackendValueForm::UpdateFormUniqueKey(const ibUniqueKeyPair& guid)
{
	if (ibSession::CurrentFrame() != nullptr) return ibSession::CurrentFrame()->UpdateFormUniqueKey(guid);
	ibBackendCoreException::Error(_("Context functions are not available!"));
	return false;
}