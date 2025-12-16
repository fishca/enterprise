#include "tableBox.h"
#include "frontend/visualView/dvc/dvc.h"

//***********************************************************************************
//*                           IMPLEMENT_DYNAMIC_CLASS                               *
//***********************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueTableBoxColumn, IValueControl);

//****************************************************************************
#include "backend/metaData.h"
#include "backend/objCtor.h"

bool CValueTableBoxColumn::GetChoiceForm(CPropertyList* property)
{
	property->AppendItem(wxT("default"), _("Default"), wxNOT_FOUND);

	const IMetaData* metaData = GetMetaData();
	if (metaData != nullptr) {
		IMetaObjectRecordDataRef* metaObjectRefValue = nullptr;
		if (!m_propertySource->IsEmptyProperty()) {

			IMetaObjectGenericData* metaObjectValue =
				m_formOwner->GetMetaObject();

			if (metaObjectValue != nullptr) {
				IMetaObject* metaobject =
					metaObjectValue->FindAnyObjectByFilter(m_propertySource->GetValueAsSource());

				IMetaObjectAttribute* attribute = wxDynamicCast(
					metaobject, IMetaObjectAttribute
				);
				if (attribute != nullptr) {

					const IMetaValueTypeCtor* so = metaData->GetTypeCtor(attribute->GetFirstClsid());
					if (so != nullptr) {
						metaObjectRefValue = wxDynamicCast(so->GetMetaObject(), IMetaObjectRecordDataRef);
					}
				}
				else
				{
					metaObjectRefValue = wxDynamicCast(
						metaobject, IMetaObjectRecordDataRef);
				}
			}
		}
		else {
			const IMetaValueTypeCtor* so = metaData->GetTypeCtor(ITypeControlFactory::GetFirstClsid());
			if (so != nullptr) {
				metaObjectRefValue = wxDynamicCast(so->GetMetaObject(), IMetaObjectRecordDataRef);
			}
		}

		if (metaObjectRefValue != nullptr) {
			for (auto form : metaObjectRefValue->GetFormArrayObject()) {
				property->AppendItem(
					form->GetSynonym(),
					form->GetMetaID(),
					form
				);
			}
		}
	}

	return true;
}

//***********************************************************************************
//*                            CValueTableBoxColumn                                 *
//***********************************************************************************

CValueTableBoxColumn::CValueTableBoxColumn() :
	IValueControl(), ITypeControlFactory(), m_model_id(wxNOT_FOUND)
{
}

IMetaData* CValueTableBoxColumn::GetMetaData() const
{
	return m_formOwner ?
		m_formOwner->GetMetaData() : nullptr;
}

wxString CValueTableBoxColumn::GetControlCaption() const
{
	if (!m_propertyCaption->IsEmptyProperty()) {
		return m_propertyCaption->GetValueAsString();
	}
	else if (!m_propertySource->IsEmptyProperty()) {
		const IMetaObject* metaObject = m_propertySource->GetSourceAttributeObject();
		wxASSERT(metaObject);
		return metaObject->GetSynonym();
	}

	return m_propertyName->GetValueAsString();
}

wxObject* CValueTableBoxColumn::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	—DataViewColumnContainer* dataViewColumn = new —DataViewColumnContainer(this, m_propertyCaption->GetValueAsString(),
		wxNOT_FOUND,
		m_propertyWidth->GetValueAsUInteger(),
		m_propertyAlign->GetValueAsEnum(),
		wxDATAVIEW_COL_REORDERABLE
	);

	dataViewColumn->SetControl(this);
	return dataViewColumn;
}

void CValueTableBoxColumn::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
	wxDataViewCtrl* dataViewCtrl = dynamic_cast<wxDataViewCtrl*>(wxparent);
	wxASSERT(dataViewCtrl);
	—DataViewColumnContainer* dataViewColumn = dynamic_cast<—DataViewColumnContainer*>(wxobject);
	wxASSERT(dataViewColumn);

	dataViewCtrl->AppendColumn(dataViewColumn);
	GetOwner()->SetCalculateColumnPos();
}

#include "backend/appData.h"

void CValueTableBoxColumn::OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost)
{
	wxDataViewCtrl* dataViewCtrl = dynamic_cast<wxDataViewCtrl*>(wxparent);
	wxASSERT(dataViewCtrl);
	—DataViewColumnContainer* dataViewColumn = dynamic_cast<—DataViewColumnContainer*>(wxobject);
	wxASSERT(dataViewColumn);

	const unsigned int order_position = GetParentPosition();

	dataViewColumn->SetTitle(GetControlCaption());
	dataViewColumn->SetWidth(m_propertyWidth->GetValueAsUInteger());
	dataViewColumn->SetAlignment(m_propertyAlign->GetValueAsEnum());

	const form_identifier_t source_column = GetModelColumn();

	IValueModel* modelValue = GetOwner()->GetModel();
	CSortOrder::CSortData* sort = modelValue != nullptr ? modelValue->GetSortByID(source_column) : nullptr;

	dataViewColumn->SetBitmap(m_propertyIcon->GetValueAsBitmap());
	dataViewColumn->SetHidden(!m_propertyVisible->GetValueAsBoolean());
	dataViewColumn->SetSortable(sort != nullptr && !appData->DesignerMode());
	dataViewColumn->SetResizeable(m_propertyResizable->GetValueAsBoolean());

	if (sort != nullptr && sort->m_sortEnable && !sort->m_sortSystem && !appData->DesignerMode())
		dataViewColumn->SetSortOrder(sort->m_sortAscending);

	dataViewColumn->SetColModel(source_column);
}

void CValueTableBoxColumn::Cleanup(wxObject* obj, IVisualHost* visualHost)
{
	wxDataViewCtrl* dataViewCtrl = dynamic_cast<wxDataViewCtrl*>(visualHost->GetWxObject(GetOwner()));
	wxASSERT(dataViewCtrl);
	—DataViewColumnContainer* dataViewColumn = dynamic_cast<—DataViewColumnContainer*>(obj);
	wxASSERT(dataViewColumn);

	dataViewCtrl->DeleteColumn(dataViewColumn);
	GetOwner()->SetCalculateColumnPos();
}

bool CValueTableBoxColumn::CanDeleteControl() const
{
	return m_parent->GetChildCount() > 1;
}

#include "backend/metaCollection/partial/commonObject.h"

//*******************************************************************
//*							 Control value	                        *
//*******************************************************************

bool CValueTableBoxColumn::FilterSource(const CSourceExplorer& src, const meta_identifier_t& id)
{
	return id == GetOwner()->GetSource();
}

#include "frontend/visualView/dvc/dvc.h"
#include "frontend/win/ctrls/textEditor.h"

bool CValueTableBoxColumn::SetControlValue(const CValue& varControlVal)
{
	IValueTable::IValueModelReturnLine* currentLine = GetCurrentLine();
	if (currentLine != nullptr) {
		currentLine->SetValueByMetaID(
			GetModelColumn(), varControlVal
		);
	}

	—DataViewColumnContainer* dataViewColumn =
		dynamic_cast<—DataViewColumnContainer*>(GetWxObject());

	if (dataViewColumn != nullptr) {
		CValueViewRenderer* renderer = dataViewColumn->GetRenderer();
		wxASSERT(renderer);
		wxControlTextEditor* textEditor = dynamic_cast<wxControlTextEditor*>(renderer->GetEditorCtrl());
		if (textEditor != nullptr) {
			textEditor->SetValue(varControlVal.GetString());
			textEditor->SetInsertionPointEnd();
		}
		else {
			renderer->FinishSelecting();
		}
	}

	m_formOwner->RefreshForm();
	return true;
}

bool CValueTableBoxColumn::GetControlValue(CValue& pvarControlVal) const
{
	IValueTable::IValueModelReturnLine* currentLine = GetCurrentLine();
	if (currentLine != nullptr) {
		return currentLine->GetValueByMetaID(
			GetModelColumn(), pvarControlVal
		);
	}

	return false;
}

//***********************************************************************************
//*                                  Data											*
//***********************************************************************************

bool CValueTableBoxColumn::LoadData(CMemoryReader& reader)
{
	m_propertyCaption->LoadData(reader);
	m_propertyIcon->LoadData(reader);

	m_propertyPasswordMode->LoadData(reader);
	m_propertyMultilineMode->LoadData(reader);
	m_propertyTexteditMode->LoadData(reader);

	m_propertySelectButton->LoadData(reader);
	m_propertyOpenButton->LoadData(reader);
	m_propertyClearButton->LoadData(reader);

	m_propertyAlign->LoadData(reader);
	m_propertyWidth->LoadData(reader);
	m_propertyVisible->LoadData(reader);
	m_propertyResizable->LoadData(reader);
	//m_propertySortable->LoadData(reader);
	m_propertyReorderable->LoadData(reader);

	m_propertyChoiceForm->LoadData(reader);

	if (!m_propertySource->LoadData(reader))
		return false;

	//events
	m_eventOnChange->LoadData(reader);
	m_eventStartChoice->LoadData(reader);
	m_eventStartListChoice->LoadData(reader);
	m_eventClearing->LoadData(reader);
	m_eventOpening->LoadData(reader);
	m_eventChoiceProcessing->LoadData(reader);
	return IValueControl::LoadData(reader);
}

bool CValueTableBoxColumn::SaveData(CMemoryWriter& writer)
{
	m_propertyCaption->SaveData(writer);
	m_propertyIcon->SaveData(writer);

	m_propertyPasswordMode->SaveData(writer);
	m_propertyMultilineMode->SaveData(writer);
	m_propertyTexteditMode->SaveData(writer);

	m_propertySelectButton->SaveData(writer);
	m_propertyOpenButton->SaveData(writer);
	m_propertyClearButton->SaveData(writer);

	m_propertyAlign->SaveData(writer);
	m_propertyWidth->SaveData(writer);
	m_propertyVisible->SaveData(writer);
	m_propertyResizable->SaveData(writer);
	//m_propertySortable->SaveData(writer);
	m_propertyReorderable->SaveData(writer);

	m_propertyChoiceForm->SaveData(writer);

	if (!m_propertySource->SaveData(writer))
		return false;

	//events
	m_eventOnChange->SaveData(writer);
	m_eventStartChoice->SaveData(writer);
	m_eventStartListChoice->SaveData(writer);
	m_eventClearing->SaveData(writer);
	m_eventOpening->SaveData(writer);
	m_eventChoiceProcessing->SaveData(writer);
	return IValueControl::SaveData(writer);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

S_CONTROL_TYPE_REGISTER(CValueTableBoxColumn, "tableboxColumn", "tableboxColumn", g_controlTableBoxColumnCLSID);