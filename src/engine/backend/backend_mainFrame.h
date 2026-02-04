#ifndef __MAIN_FRAME_CORE_H__
#define __MAIN_FRAME_CORE_H__

#include "backend/uniqueKey.h"
#include "backend/backend_spreadsheet.h"
#include "backend/system/systemEnum.h"

#define backend_mainFrame \
	IBackendDocMDIFrame::GetDocMDIFrame() \

class BACKEND_API IBackendDocMDIFrame {
protected:
	IBackendDocMDIFrame();
public:

	static IBackendDocMDIFrame* GetDocMDIFrame();

	virtual ~IBackendDocMDIFrame();
	virtual wxFrame* GetFrameHandler() const = 0;

	virtual class IMetaData* FindMetadataByPath(const wxString& strFileName) const { return nullptr; }
	virtual void BackendError(const wxString& strFileName, const wxString& strDocPath, const long line, const wxString& strErrorMessage) const {}

#pragma region _frontend_call_h__

	// Form support
	virtual class IBackendValueForm* ActiveWindow() const { return nullptr; }
	virtual class IBackendValueForm* CreateNewForm(const class IValueMetaObjectForm* creator, class IBackendControlFrame* ownerControl = nullptr,
		class ISourceDataObject* srcObject = nullptr, const CUniqueKey& formGuid = wxNullUniqueKey) {
		return nullptr;
	}

	virtual CUniqueKey CreateFormUniqueKey(const IBackendControlFrame* ownerControl,
		const ISourceDataObject* sourceObject, const CUniqueKey& formGuid) {
		return wxNullUniqueKey;
	}

	virtual class IBackendValueForm* FindFormByUniqueKey(const IBackendControlFrame* ownerControl,
		const ISourceDataObject* sourceObject, const CUniqueKey& formGuid) {
		return nullptr;
	}

	virtual class IBackendValueForm* FindFormByUniqueKey(const CUniqueKey& guid) { return nullptr; }
	virtual class IBackendValueForm* FindFormByControlUniqueKey(const CUniqueKey& guid) { return nullptr; }
	virtual class IBackendValueForm* FindFormBySourceUniqueKey(const CUniqueKey& guid) { return nullptr; }

	virtual bool UpdateFormUniqueKey(const CUniquePairKey& guid) { return nullptr; }

	// Grid support
	virtual bool ShowSpreadSheetDocument(const wxString& strTitle, const CBackendSpreadSheetDocument& doc) { return false; }
	virtual bool PrintSpreadSheetDocument(const CBackendSpreadSheetDocument& doc) { return false; }

#pragma endregion 

#pragma region property
	virtual class BACKEND_API IPropertyObject* GetProperty() const { return nullptr; }
	virtual bool SetProperty(class BACKEND_API IPropertyObject* prop) { return false; }
#pragma endregion 

	virtual void SetTitle(const wxString& strTitle) = 0;
	virtual void SetStatusText(const wxString& strTitle, int number = 0) = 0;

	virtual void Message(const wxString& strMessage, eStatusMessage status) {}
	virtual void ClearMessage() {}

	virtual void RefreshFrame() = 0;
	virtual void RaiseFrame() = 0;

	virtual bool AuthenticationUser(const wxString& userName, const wxString& userPassword) const { return false; }

public:
	virtual void OnInitializeConfiguration(enum eConfigType cfg) {}
	virtual void OnDestroyConfiguration(enum eConfigType cfg) {}
private:
	static IBackendDocMDIFrame* ms_mainFrame;
};

#endif