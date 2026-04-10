#include "tableBox.h"
#include "tableBoxColumnRenderer.h"

//***********************************************************************************
//*                           IMPLEMENT_DYNAMIC_CLASS                               *
//***********************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueTableBoxColumn, IValueControl);

//****************************************************************************
#include "backend/metaData.h"
#include "backend/objCtor.h"

bool CValueTableBoxColumn::GetChoiceForm(CPropertyList* property)
{
	const IMetaData* metaData = GetMetaData();
	if (metaData != nullptr) {
		IValueMetaObjectRecordDataRef* metaObjectRefValue = nullptr;
		if (!m_propertySource->IsEmptyProperty()) {

			IValueMetaObjectGenericData* metaObjectValue =
				m_formOwner->GetMetaObject();

			if (metaObjectValue != nullptr) {
				IValueMetaObject* metaobject =
					metaObjectValue->FindAnyObjectByFilter(m_propertySource->GetValueAsSource());

				IValueMetaObjectAttribute* attribute = wxDynamicCast(
					metaobject, IValueMetaObjectAttribute
				);
				if (attribute != nullptr) {

					const IMetaValueTypeCtor* so = metaData->GetTypeCtor(attribute->GetFirstClsid());
					if (so != nullptr) {
						metaObjectRefValue = wxDynamicCast(so->GetMetaObject(), IValueMetaObjectRecordDataRef);
					}
				}
				else
				{
					metaObjectRefValue = wxDynamicCast(
						metaobject, IValueMetaObjectRecordDataRef);
				}
			}
		}
		else {
			const IMetaValueTypeCtor* so = metaData->GetTypeCtor(ITypeControlFactory::GetFirstClsid());
			if (so != nullptr) {
				metaObjectRefValue = wxDynamicCast(so->GetMetaObject(), IValueMetaObjectRecordDataRef);
			}
		}

		if (metaObjectRefValue != nullptr) {
			for (auto form : metaObjectRefValue->GetFormArrayObject()) {
				property->AppendItem(
					form->GetSynonym(),
					form->GetMetaID(),
					form->GetIcon(),
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

wxString CValueTableBoxColumn::GetControlTitle() const
{
	if (!m_propertyTitle->IsEmptyProperty()) {
		return m_propertyTitle->GetValueAsTranslateString();
	}
	else if (!m_propertySource->IsEmptyProperty()) {
		const IValueMetaObject* metaObject = m_propertySource->GetSourceAttributeObject();
		wxASSERT(metaObject);
		return metaObject->GetSynonym();
	}

	return m_propertyName->GetValueAsString();
}

wxObject* CValueTableBoxColumn::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	CDataViewColumnObject* dataViewColumn = new CDataViewColumnObject(this, wxT(""),
		wxNOT_FOUND, wxDVC_DEFAULT_WIDTH, wxALIGN_CENTER, wxDATAVIEW_COL_REORDERABLE);

	dataViewColumn->SetControl(this);
	return dataViewColumn;
}

void CValueTableBoxColumn::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
	wxDataViewExtCtrl* dataViewCtrl = dynamic_cast<wxDataViewExtCtrl*>(wxparent);
	wxASSERT(dataViewCtrl);
	CDataViewColumnObject* dataViewColumn = dynamic_cast<CDataViewColumnObject*>(wxobject);
	wxASSERT(dataViewColumn);

	dataViewCtrl->AppendColumn(dataViewColumn);
	GetOwner()->SetCalculateColumnPos();
}

#include "backend/appData.h"

void CValueTableBoxColumn::OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost)
{
	wxDataViewExtCtrl* dataViewCtrl = dynamic_cast<wxDataViewExtCtrl*>(wxparent);
	wxASSERT(dataViewCtrl);
	CDataViewColumnObject* dataViewColumn = dynamic_cast<CDataViewColumnObject*>(wxobject);
	wxASSERT(dataViewColumn);

	const unsigned int order_position = GetParentPosition();

	if (m_propertyRepresentation->GetValueAsEnum() == enRepresentation::eRepresentation_Auto) {
		dataViewColumn->SetTitle(GetControlTitle());
		dataViewColumn->SetBitmap(m_propertyHeaderPicture->GetValueAsBitmap());
		dataViewColumn->SetFooterTitle(m_propertyFooterText->GetValueAsTranslateString());
		dataViewColumn->SetFooterBitmap(m_propertyFooterPicture->GetValueAsBitmap());
	}
	else if (m_propertyRepresentation->GetValueAsEnum() == enRepresentation::eRepresentation_PictureAndText) {
		dataViewColumn->SetTitle(GetControlTitle());
		dataViewColumn->SetBitmap(m_propertyHeaderPicture->GetValueAsBitmap());
		dataViewColumn->SetFooterTitle(m_propertyFooterText->GetValueAsTranslateString());
		dataViewColumn->SetFooterBitmap(m_propertyFooterPicture->GetValueAsBitmap());
	}
	else if (m_propertyRepresentation->GetValueAsEnum() == enRepresentation::eRepresentation_Picture) {
		dataViewColumn->SetTitle(wxEmptyString);
		dataViewColumn->SetBitmap(m_propertyHeaderPicture->GetValueAsBitmap());
		dataViewColumn->SetFooterTitle(wxEmptyString);
		dataViewColumn->SetFooterBitmap(m_propertyFooterPicture->GetValueAsBitmap());
	}
	else if (m_propertyRepresentation->GetValueAsEnum() == enRepresentation::eRepresentation_Text) {
		dataViewColumn->SetTitle(GetControlTitle());
		dataViewColumn->SetBitmap(wxNullBitmap);
		dataViewColumn->SetFooterTitle(m_propertyFooterText->GetValueAsTranslateString());
		dataViewColumn->SetFooterBitmap(wxNullBitmap);
	}

	dataViewColumn->SetWidth(m_propertyWidth->GetValueAsUInteger());
	dataViewColumn->SetAlignment(m_propertyHeaderAlign->GetValueAsEnum());
	dataViewColumn->SetFooterAlignment(m_propertyFooterAlign->GetValueAsEnum());

	const form_identifier_t source_column = GetModelColumn();

	IValueModel* modelValue = GetOwner()->GetModel();
	CSortOrder::CSortData* sort = modelValue != nullptr ? modelValue->GetSortByID(source_column) : nullptr;

	dataViewColumn->SetHidden(!m_propertyVisible->GetValueAsBoolean());
	dataViewColumn->SetSortable(sort != nullptr && !appData->DesignerMode());
	dataViewColumn->SetResizeable(m_propertyResizable->GetValueAsBoolean());

	if (sort != nullptr && sort->m_sortEnable && !sort->m_sortSystem && !appData->DesignerMode())
		dataViewColumn->SetSortOrder(sort->m_sortAscending);

	dataViewColumn->SetColumnModel(source_column);
}

void CValueTableBoxColumn::Cleanup(wxObject* obj, IVisualHost* visualHost)
{
	wxDataViewExtCtrl* dataViewCtrl = dynamic_cast<wxDataViewExtCtrl*>(visualHost->GetWxObject(GetOwner()));
	wxASSERT(dataViewCtrl);
	CDataViewColumnObject* dataViewColumn = dynamic_cast<CDataViewColumnObject*>(obj);
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

bool CValueTableBoxColumn::FilterSource(const CSourceExplorer& src, const meta_identifier_t& id) const
{
	return id == GetOwner()->GetSource();
}

#include "frontend/win/ctrls/controlTextEditor.h"

bool CValueTableBoxColumn::SetControlValue(const CValue& varControlVal)
{
	IValueTable::IValueModelReturnLine* currentLine = GetCurrentLine();
	if (currentLine != nullptr) {
		currentLine->SetValueByMetaID(
			GetModelColumn(), varControlVal
		);
	}

	CDataViewColumnObject* dataViewColumn =
		dynamic_cast<CDataViewColumnObject*>(GetWxObject());

	if (dataViewColumn != nullptr) {
		CDataViewValueRenderer* renderer = dataViewColumn->GetRenderer();
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
	m_propertyTitle->LoadData(reader);
	m_propertyRepresentation->LoadData(reader);
	m_propertyFooterText->LoadData(reader);

	m_propertyHeaderPicture->LoadData(reader);
	m_propertyFooterPicture->LoadData(reader);

	m_propertyPasswordMode->LoadData(reader);
	m_propertyMultilineMode->LoadData(reader);
	m_propertyTexteditMode->LoadData(reader);

	m_propertySelectButton->LoadData(reader);
	m_propertyOpenButton->LoadData(reader);
	m_propertyClearButton->LoadData(reader);

	m_propertyHeaderAlign->LoadData(reader);
	m_propertyFooterAlign->LoadData(reader);

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
	m_propertyTitle->SaveData(writer);
	m_propertyRepresentation->SaveData(writer);
	m_propertyFooterText->SaveData(writer);

	m_propertyHeaderPicture->SaveData(writer);
	m_propertyFooterPicture->SaveData(writer);

	m_propertyPasswordMode->SaveData(writer);
	m_propertyMultilineMode->SaveData(writer);
	m_propertyTexteditMode->SaveData(writer);

	m_propertySelectButton->SaveData(writer);
	m_propertyOpenButton->SaveData(writer);
	m_propertyClearButton->SaveData(writer);

	m_propertyHeaderAlign->SaveData(writer);
	m_propertyFooterAlign->SaveData(writer);

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

S_CONTROL_TYPE_REGISTER(CValueTableBoxColumn, "TableboxColumn", "TableboxColumn", g_controlTableBoxColumnCLSID);