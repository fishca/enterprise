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
	property->AppendItem(wxT("default"), _("default"), wxNOT_FOUND);

	const IMetaData* metaData = GetMetaData();
	if (metaData != nullptr) {
		IMetaObjectRecordDataRef* metaObjectRefValue = nullptr;
		//if (m_dataSource.isValid()) {
		if (!m_propertySource->IsEmptyProperty()) {

			IMetaObjectGenericData* metaObjectValue =
				m_formOwner->GetMetaObject();

			if (metaObjectValue != nullptr) {
				IMetaObject* metaobject =
					metaObjectValue->FindMetaObjectByID(m_propertySource->GetValueAsSource());
				//	metaObjectValue->FindMetaObjectByID(m_dataSource);

				IMetaObjectAttribute* metaAttribute = wxDynamicCast(
					metaobject, IMetaObjectAttribute
				);
				if (metaAttribute != nullptr) {

					IMetaValueTypeCtor* so = metaData->GetTypeCtor(metaAttribute->GetFirstClsid());
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
			IMetaValueTypeCtor* so = metaData->GetTypeCtor(ITypeControlFactory::GetFirstClsid());
			if (so != nullptr) {
				metaObjectRefValue = wxDynamicCast(so->GetMetaObject(), IMetaObjectRecordDataRef);
			}
		}

		if (metaObjectRefValue != nullptr) {
			for (auto form : metaObjectRefValue->GetObjectForms()) {
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
	IValueControl(), ITypeControlFactory()
{
}

IMetaData* CValueTableBoxColumn::GetMetaData() const
{
	return m_formOwner ?
		m_formOwner->GetMetaData() : nullptr;
}

wxObject* CValueTableBoxColumn::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	—DataViewColumnContainer* columnObject = new —DataViewColumnContainer(this, m_propertyCaption->GetValueAsString(),
		wxNOT_FOUND,
		m_propertyWidth->GetValueAsUInteger(),
		m_propertyAlign->GetValueAsEnum(),
		wxDATAVIEW_COL_REORDERABLE
	);

	columnObject->SetControl(this);
	columnObject->SetControlID(m_controlId);

	columnObject->SetTitle(m_propertyCaption->GetValueAsString());
	columnObject->SetWidth(m_propertyWidth->GetValueAsUInteger());
	columnObject->SetAlignment(m_propertyAlign->GetValueAsEnum());

	columnObject->SetBitmap(m_propertyIcon->GetValueAsBitmap());
	columnObject->SetHidden(!m_propertyVisible->GetValueAsBoolean());
	columnObject->SetSortable(false);
	columnObject->SetResizeable(m_propertyResizable->GetValueAsBoolean());

	CValueViewRenderer* colRenderer = columnObject->GetRenderer();
	wxASSERT(colRenderer);
	return columnObject;
}

void CValueTableBoxColumn::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
	wxDataViewCtrl* tableCtrl = dynamic_cast<wxDataViewCtrl*>(wxparent);
	wxASSERT(tableCtrl);
	—DataViewColumnContainer* columnObject = dynamic_cast<—DataViewColumnContainer*>(wxobject);
	wxASSERT(columnObject);

	wxHeaderCtrl* headerCtrl = tableCtrl->GenericGetHeader();
	if (headerCtrl != nullptr)
		headerCtrl->ResetColumnsOrder();
	tableCtrl->AppendColumn(columnObject);
}

#include "backend/appData.h"

void CValueTableBoxColumn::OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost)
{
	IValueFrame* parentControl = GetParent(); int idx = wxNOT_FOUND;

	for (unsigned int i = 0; i < parentControl->GetChildCount(); i++) {
		CValueTableBoxColumn* child = dynamic_cast<CValueTableBoxColumn*>(parentControl->GetChild(i));
		wxASSERT(child);
		if (child->GetSourceColumn() == GetSourceColumn()) { idx = i; break; }
	}

	wxDataViewCtrl* tableCtrl = dynamic_cast<wxDataViewCtrl*>(wxparent);
	wxASSERT(tableCtrl);
	—DataViewColumnContainer* columnObject = dynamic_cast<—DataViewColumnContainer*>(wxobject);
	wxASSERT(columnObject);

	columnObject->SetControl(this);

	wxString textCaption = m_propertyName->GetValueAsString();

	if (!m_propertySource->IsEmptyProperty()) {
		const IMetaObject* metaObject = m_propertySource->GetSourceAttributeObject();
		if (metaObject != nullptr) textCaption = metaObject->GetSynonym();
	}

	columnObject->SetTitle(m_propertyCaption->IsEmptyProperty() ?
		textCaption : m_propertyCaption->GetValueAsString());
	columnObject->SetWidth(m_propertyWidth->GetValueAsUInteger());
	columnObject->SetAlignment(m_propertyAlign->GetValueAsEnum());

	IValueModel* modelValue = GetOwner()->GetModel();
	CSortOrder::CSortData* sort = modelValue != nullptr ? modelValue->GetSortByID(GetSourceColumn()) : nullptr;

	columnObject->SetBitmap(m_propertyIcon->GetValueAsBitmap());
	columnObject->SetHidden(!m_propertyVisible->GetValueAsBoolean());
	columnObject->SetSortable(sort != nullptr && !appData->DesignerMode());
	columnObject->SetResizeable(m_propertyResizable->GetValueAsBoolean());

	if (sort != nullptr && sort->m_sortEnable && !sort->m_sortSystem && !appData->DesignerMode())
		columnObject->SetSortOrder(sort->m_sortAscending);

	columnObject->SetColModel(GetSourceColumn());

	wxHeaderCtrl* headerCtrl = tableCtrl->GenericGetHeader();
	if (headerCtrl != nullptr) {
		unsigned int model_index = tableCtrl->GetColumnIndex(columnObject);
		unsigned int col_header_index = headerCtrl->GetColumnPos(model_index);
		if (col_header_index != idx && tableCtrl->DeleteColumn(columnObject))
			tableCtrl->InsertColumn(idx, columnObject);
	}
	else {
		unsigned int model_index = tableCtrl->GetColumnIndex(columnObject);
		if (model_index != idx && tableCtrl->DeleteColumn(columnObject))
			tableCtrl->InsertColumn(idx, columnObject);
	}
}

void CValueTableBoxColumn::Cleanup(wxObject* obj, IVisualHost* visualHost)
{
	wxDataViewCtrl* tableCtrl = dynamic_cast<wxDataViewCtrl*>(visualHost->GetWxObject(GetOwner()));
	wxASSERT(tableCtrl);
	—DataViewColumnContainer* columnObject = dynamic_cast<—DataViewColumnContainer*>(obj);
	wxASSERT(columnObject);
	wxHeaderCtrl* headerCtrl = tableCtrl->GenericGetHeader();
	if (headerCtrl != nullptr) {
		columnObject->SetHidden(false);
		headerCtrl->ResetColumnsOrder();
	}
	tableCtrl->DeleteColumn(columnObject);
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
	IValueTable::IValueModelReturnLine* retLine = GetReturnLine();
	if (retLine != nullptr) {
		retLine->SetValueByMetaID(
			GetSourceColumn(), varControlVal
		);
	}
	—DataViewColumnContainer* columnObject =
		dynamic_cast<—DataViewColumnContainer*>(GetWxObject());
	if (columnObject != nullptr) {
		CValueViewRenderer* renderer = columnObject->GetRenderer();
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
	IValueTable::IValueModelReturnLine* retLine = GetReturnLine();
	if (retLine != nullptr) {
		return retLine->GetValueByMetaID(
			GetSourceColumn(), pvarControlVal
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