#ifndef _DESIGNER_MANAGER_H__
#define _DESIGNER_MANAGER_H__

#include "mainFrame/mainFrameDesigner.h"
#include "frontend/docView/docManager.h"

class ibMetaDocManagerDesigner : public ibMetaDocManager {
public:
	ibMetaDocManagerDesigner();
protected:

	void OnUpdateSaveMetadata(wxUpdateUIEvent& event);

	wxDECLARE_DYNAMIC_CLASS(ibMetaDocManagerDesigner);
	wxDECLARE_NO_COPY_CLASS(ibMetaDocManagerDesigner);

	wxDECLARE_EVENT_TABLE();
};

#endif
