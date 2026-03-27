#ifndef _DESIGNER_MANAGER_H__
#define _DESIGNER_MANAGER_H__

#include "frontend/docView/docManager.h"

class ibMetaDocManagerEnterprise : public ibMetaDocManager {
public:
	ibMetaDocManagerEnterprise();

protected:
	wxDECLARE_DYNAMIC_CLASS(ibMetaDocManagerEnterprise);
	wxDECLARE_NO_COPY_CLASS(ibMetaDocManagerEnterprise);
};

#endif
