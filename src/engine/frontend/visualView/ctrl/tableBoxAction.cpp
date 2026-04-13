#include "tableBox.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "form.h"

//****************************************************************************
//*                              actionData                                     *
//****************************************************************************

ibValueModelTableBox::ibActionCollection ibValueModelTableBox::GetActionCollection(const ibFormID& formType)
{
	if (m_tableModel == nullptr) {
		//if (m_dataSource.isValid()) {
		//	if (srcObject != nullptr) {
		//		ibValueModel* tableModel = nullptr;
		//		if (srcObject->GetModel(tableModel, GetIdByGuid(m_dataSource))) {
		//			return tableModel->GetActionCollection(formType);
		//		}
		//	}
		//}

		if (!m_propertySource->IsEmptyProperty()) {
			ibSourceDataObject* srcObject = m_formOwner->GetSourceObject();
			if (srcObject != nullptr) {
				ibValueModel* tableModel = nullptr;
				if (srcObject->GetModel(tableModel, m_propertySource->GetValueAsSource())) {
					return tableModel->GetActionCollection(formType);
				}
			}
		}

		return ibActionCollection();
	}

	return m_tableModel->GetActionCollection(formType);
}

#include "backend/appData.h"

void ibValueModelTableBox::ExecuteAction(const ibActionID& lNumAction, ibBackendValueForm* srcForm)
{
	if (m_tableModel == nullptr) return;
	if (!appData->DesignerMode()) {
		m_tableModel->ExecuteAction(lNumAction, srcForm);
	}
}