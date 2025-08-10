#include "backend_form.h"
#include "backend/system/systemManager.h"
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
    const IMetaObjectForm* creator,
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
            CSystemFunction::Raise(_("Context functions are not available in this frontend library!"));
            return nullptr;
        }
        return createdForm;
    }

    CSystemFunction::Raise(_("Context functions are not available!"));
    return nullptr;
}

CUniqueKey IBackendValueForm::CreateFormUniqueKey(IBackendControlFrame* ownerControl, ISourceDataObject* sourceObject, const CUniqueKey& formGuid)
{
    if (backend_mainFrame != nullptr)  return backend_mainFrame->CreateFormUniqueKey(ownerControl, sourceObject, formGuid);
    CSystemFunction::Raise(_("Context functions are not available!"));
    return CUniqueKey();
}

IBackendValueForm* IBackendValueForm::FindFormByUniqueKey(IBackendControlFrame* ownerControl, ISourceDataObject* sourceObject, const CUniqueKey& formGuid)
{
    if (backend_mainFrame != nullptr) return backend_mainFrame->FindFormByUniqueKey(ownerControl, sourceObject, formGuid);
    CSystemFunction::Raise(_("Context functions are not available!"));
    return nullptr;
}

IBackendValueForm* IBackendValueForm::FindFormByUniqueKey(const CUniqueKey& guid)
{
    if (backend_mainFrame != nullptr)  return backend_mainFrame->FindFormByUniqueKey(guid);
    CSystemFunction::Raise(_("Context functions are not available!"));
    return nullptr;
}

IBackendValueForm* IBackendValueForm::FindFormByControlUniqueKey(const CUniqueKey& guid)
{
    if (backend_mainFrame != nullptr)  return backend_mainFrame->FindFormByControlUniqueKey(guid);
    CSystemFunction::Raise(_("Context functions are not available!"));
    return nullptr;
}

IBackendValueForm* IBackendValueForm::FindFormBySourceUniqueKey(const CUniqueKey& guid)
{
    if (backend_mainFrame != nullptr)  return backend_mainFrame->FindFormBySourceUniqueKey(guid);
    CSystemFunction::Raise(_("Context functions are not available!"));
    return nullptr;
}

bool IBackendValueForm::UpdateFormUniqueKey(const CUniquePairKey& guid)
{
    if (backend_mainFrame != nullptr) return backend_mainFrame->UpdateFormUniqueKey(guid);
    CSystemFunction::Raise(_("Context functions are not available!"));
    return false;
}