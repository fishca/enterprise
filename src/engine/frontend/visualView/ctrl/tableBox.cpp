#include "tableBox.h"
#include "tableBoxColumnRenderer.h"

#include "form.h"

#include "frontend/visualView/visualHostClient.h"
#include "backend/system/value/valueTable.h"
#include "backend/metaCollection/partial/commonObject.h"
#include "backend/appData.h"

//***********************************************************************************
//*                           IMPLEMENT_DYNAMIC_CLASS                               *
//***********************************************************************************

wxIMPLEMENT_DYNAMIC_CLASS(CValueTableBox, IValueWindow);

wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumTableBoxSelectionMode, CValue);
wxIMPLEMENT_DYNAMIC_CLASS(CValueEnumTableBoxViewMode, CValue);

//***********************************************************************************
//*                                 Special tablebox func                           *
//***********************************************************************************

bool CValueTableBox::GetControlValue(CValue& pvarControlVal) const
{
	if (m_tableModel == nullptr) {
		if (appData->DesignerMode()) {
			if (!m_propertySource->IsEmptyProperty()) {
				ISourceDataObject* srcObject = m_formOwner->GetSourceObject();
				if (srcObject != nullptr) {
					IValueModel* tableModel = nullptr;
					if (srcObject->GetModel(tableModel, m_propertySource->GetValueAsSource())) {
						if (tableModel != m_tableModel) {
							pvarControlVal = tableModel;
							return true;
						}
					}
				}
			}
		}
		return false;
	}
	pvarControlVal = m_tableModel;
	return true;
}

bool CValueTableBox::SetControlValue(const CValue& varControlVal)
{
	m_tableModel = varControlVal.ConvertToType<IValueModel>();
	return true;
}

void CValueTableBox::AddColumn()
{
	wxASSERT(m_formOwner);

	CValueTableBoxColumn* columnTable = wxDynamicCast(m_formOwner->NewObject(g_controlTableBoxColumnCLSID, this), CValueTableBoxColumn);
	g_visualHostContext->InsertControl(columnTable, this);
	if (m_tableModel != nullptr) {
		IValueModel::IValueModelColumnCollection* columnData = m_tableModel->GetColumnCollection();
		wxASSERT(columnData);
		IValueModel::IValueModelColumnCollection::IValueModelColumnInfo* column_info = columnData->AddColumn(
			columnTable->GetControlName(),
			columnTable->GetTypeDesc(),
			columnTable->GetCaption(),
			columnTable->GetWidthColumn()
		);
		if (column_info != nullptr) column_info->SetColumnID(columnTable->GetControlID());
	}

	g_visualHostContext->RefreshEditor();
}

void CValueTableBox::CreateColumnCollection(wxDataViewExtCtrl* dataViewCtrl)
{
	if (appData->DesignerMode())
		return;

	wxDataViewExtCtrl* tc = dataViewCtrl ?
		dataViewCtrl : dynamic_cast<wxDataViewExtCtrl*>(GetWxObject());
	wxASSERT(tc);

	CFormVisualDocument* visualDocument = m_formOwner->GetVisualDocument();
	//clear all controls 
	for (unsigned int idx = 0; idx < GetChildCount(); idx++) {
		IValueFrame* childColumn = GetChild(idx);
		wxASSERT(childColumn);
		if (visualDocument != nullptr) {
			CVisualClientHost* visualView = visualDocument->GetFirstView() ?
				visualDocument->GetFirstView()->GetVisualHost() : nullptr;
			wxASSERT(visualView);
			visualView->RemoveControl(childColumn, this);
		}

		RemoveChild(childColumn);

		childColumn->SetParent(nullptr);
		childColumn->DecrRef();
	}

	//clear all children
	RemoveAllChildren();

	//clear all old columns
	tc->ClearColumns();

	//create new columns
	IValueModel::IValueModelColumnCollection* tableColumns = m_tableModel->GetColumnCollection();
	wxASSERT(tableColumns);
	for (unsigned int idx = 0; idx < tableColumns->GetColumnCount(); idx++) {

		IValueModel::IValueModelColumnCollection::IValueModelColumnInfo* columnInfo = tableColumns->GetColumnInfo(idx);
		CValueTableBoxColumn* newTableBoxColumn =
			m_formOwner->NewObject<CValueTableBoxColumn>(g_controlTableBoxColumnCLSID, this);

		const CTypeDescription& typeDescription = columnInfo->GetColumnType();
		if (typeDescription.IsOk())
			newTableBoxColumn->SetDefaultMetaType(typeDescription);
		else
			newTableBoxColumn->SetDefaultMetaType(eValueTypes::TYPE_STRING);

		newTableBoxColumn->SetCaption(columnInfo->GetColumnCaption());
		newTableBoxColumn->SetWidthColumn(columnInfo->GetColumnWidth());
		newTableBoxColumn->SetModelColumn(columnInfo->GetColumnID());

		if (visualDocument != nullptr) {

			CVisualClientHost* visualView = visualDocument->GetFirstView() ?
				visualDocument->GetFirstView()->GetVisualHost() : nullptr;

			wxASSERT(visualView);
			visualView->CreateControl(newTableBoxColumn, this);
		}
	}

	if (visualDocument != nullptr) {

		CVisualClientHost* visualView = visualDocument->GetFirstView() ?
			visualDocument->GetFirstView()->GetVisualHost() : nullptr;

		wxASSERT(visualView);
		//fix size in parent window 
		wxWindow* wndParent = visualView->GetParent();
		if (wndParent != nullptr) {
			wndParent->Layout();
		}
	}
}

void CValueTableBox::CreateTable(bool recreateModel) {

	if (recreateModel && m_tableModel != nullptr) m_tableModel = nullptr;

	if (m_tableModel == nullptr) {

		m_tableModel = ITypeControlFactory::CreateAndConvertValueRef<IValueModel>();

		if (m_tableModel != nullptr) {
			for (unsigned int idx = 0; idx < GetChildCount(); idx++) {
				CValueTableBoxColumn* columnTable = wxDynamicCast(GetChild(idx), CValueTableBoxColumn);
				if (columnTable != nullptr) {
					IValueModel::IValueModelColumnCollection* columnData = m_tableModel->GetColumnCollection();
					wxASSERT(columnData);
					IValueModel::IValueModelColumnCollection::IValueModelColumnInfo* column_info = columnData->AddColumn(
						columnTable->GetControlName(),
						columnTable->GetTypeDesc(),
						columnTable->GetCaption(),
						columnTable->GetWidthColumn()
					);

					if (column_info != nullptr) column_info->SetColumnID(columnTable->GetControlID());
				}
			}
		}
	}
}

void CValueTableBox::CreateModel(bool recreateModel)
{
	if (!m_propertySource->IsEmptyProperty()) {
		ISourceDataObject* srcObject = m_formOwner->GetSourceObject();
		if (srcObject != nullptr) {
			IValueModel* tableModel = nullptr;
			if (srcObject->GetModel(tableModel, m_propertySource->GetValueAsSource())) {
				if (tableModel != m_tableModel) m_tableModel = tableModel;
			}
		}
		CreateTable(false);
	}
	else if (m_tableModel != nullptr && m_propertySource->GetValueAsTypeDesc() != m_tableModel->GetSourceClassType()) {
		CreateTable(true);
	}
	else {
		CreateTable(recreateModel);
	}
}

void CValueTableBox::RefreshModel(bool recreateModel)
{
	CValueTableBox::CreateModel(recreateModel);
}

////////////////////////////////////////////////////////////////////////////////////

ISourceObject* CValueTableBox::GetSourceObject() const
{
	return m_formOwner ? m_formOwner->GetSourceObject() : nullptr;
}

IValueMetaObjectCompositeData* CValueTableBox::GetSourceMetaObject() const
{
	wxASSERT(m_tableModel);
	if (m_tableModel == nullptr) return nullptr;
	return m_tableModel->GetSourceMetaObject();
}

class_identifier_t CValueTableBox::GetSourceClassType() const
{
	wxASSERT(m_tableModel);
	if (m_tableModel == nullptr) return 0;
	return m_tableModel->GetSourceClassType();
}

bool CValueTableBox::FilterSource(const CSourceExplorer& src, const meta_identifier_t& id) const
{
	return src.IsTableSection();
}

//***********************************************************************************
//*                              CValueTableBox                                     *
//***********************************************************************************

CValueTableBox::CValueTableBox() : IValueWindow(), ITypeControlFactory(),
m_tableModel(nullptr), m_tableCurrentLine(nullptr),
m_dataViewCreated(false), m_dataViewSelected(false), m_dataViewUpdated(false), m_dataViewSizeChanged(false),
m_need_calculate_pos(false)
{
	m_propertySource->SetValue(CTypeDescription(g_valueTableCLSID));

	//set default params
	m_propertyMinSize->SetValue(wxSize(150, 75));
	m_propertyBG->SetValue(wxColour(255, 255, 255));
}

/////////////////////////////////////////////////////////////////////////////////////

void CValueTableBox::CalculateColumnPos()
{
	wxTableViewCtrl* dataViewCtrl = dynamic_cast<wxTableViewCtrl*>(GetWxObject());
	if (dataViewCtrl != nullptr) {

		dataViewCtrl->SetExpanderColumn(nullptr);

		wxHeaderGenericCtrl* headerCtrl = dataViewCtrl->GenericGetHeader();
		if (headerCtrl != nullptr) {

			bool need_reset_columns_order = false;

			for (unsigned int idx = 0; idx < GetChildCount(); idx++) {

				const IValueFrame* valueFrame = GetChild(idx);
				wxASSERT(valueFrame);

				wxDataViewExtColumn* column = dynamic_cast<wxDataViewExtColumn*>(valueFrame->GetWxObject());

				const unsigned int column_model_index = dataViewCtrl->GetColumnIndex(column);
				const unsigned int column_index = headerCtrl->GetColumnPos(column_model_index);

				const unsigned int real_column_index = valueFrame->GetParentPosition();

				if (column_index != real_column_index) {
					dataViewCtrl->DeleteColumn(column);
					dataViewCtrl->InsertColumn(real_column_index, column);
					need_reset_columns_order = true;
				}

				if (column->IsShown() && dataViewCtrl->GetExpanderColumn() == nullptr) {
					dataViewCtrl->SetExpanderColumn(column);
				}
			}

			if (need_reset_columns_order) headerCtrl->ResetColumnsOrder();
		}
		else
		{
			for (unsigned int idx = 0; idx < GetChildCount(); idx++) {

				const IValueFrame* valueFrame = GetChild(idx);
				wxASSERT(valueFrame);

				wxDataViewExtColumn* column = dynamic_cast<wxDataViewExtColumn*>(valueFrame->GetWxObject());

				const unsigned int column_model_index = dataViewCtrl->GetColumnIndex(column);
				const unsigned int real_column_index = valueFrame->GetParentPosition();

				if (column_model_index != real_column_index) {
					dataViewCtrl->DeleteColumn(column);
					dataViewCtrl->InsertColumn(real_column_index, column);
				}

				if (column->IsShown() && dataViewCtrl->GetExpanderColumn() == nullptr) {
					dataViewCtrl->SetExpanderColumn(column);
				}
			}
		}
	}
}

/////////////////////////////////////////////////////////////////////////////////////

wxObject* CValueTableBox::Create(wxWindow* wxparent, IVisualHost* visualHost)
{
	wxTableViewCtrl* dataViewCtrl = new wxTableViewCtrl(wxparent, wxID_ANY,
		wxDefaultPosition,
		wxDefaultSize,
		wxDV_SINGLE | wxDV_HORIZ_RULES | wxDV_VERT_RULES | wxDV_ROW_LINES | wxDV_VARIABLE_LINE_HEIGHT | wxBORDER_SIMPLE);

	const CFormVisualDocument* visualDoc = CValueTableBox::GetVisualDocument();

	if (visualDoc == nullptr || (visualDoc != nullptr && !visualDoc->IsVisualDemonstrationDoc())) {

		dataViewCtrl->Bind(wxEVT_DATAVIEW_COLUMN_HEADER_CLICK, &CValueTableBox::OnColumnClick, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_COLUMN_REORDERED, &CValueTableBox::OnColumnReordered, this);

		//system events:
		dataViewCtrl->Bind(wxEVT_DATAVIEW_SELECTION_CHANGED, &CValueTableBox::OnSelectionChanged, this);

		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_ACTIVATED, &CValueTableBox::OnItemActivated, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_COLLAPSED, &CValueTableBox::OnItemCollapsed, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_EXPANDED, &CValueTableBox::OnItemExpanded, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_COLLAPSING, &CValueTableBox::OnItemCollapsing, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_EXPANDING, &CValueTableBox::OnItemExpanding, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_START_EDITING, &CValueTableBox::OnItemStartEditing, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_EDITING_STARTED, &CValueTableBox::OnItemEditingStarted, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_EDITING_DONE, &CValueTableBox::OnItemEditingDone, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_VALUE_CHANGED, &CValueTableBox::OnItemValueChanged, this);

		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_START_INSERTING, &CValueTableBox::OnItemStartInserting, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_START_DELETING, &CValueTableBox::OnItemStartDeleting, this);

#if wxUSE_DRAG_AND_DROP 
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_BEGIN_DRAG, &CValueTableBox::OnItemBeginDrag, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_DROP_POSSIBLE, &CValueTableBox::OnItemDropPossible, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_DROP, &CValueTableBox::OnItemDrop, this);
#endif // wxUSE_DRAG_AND_DROP

		dataViewCtrl->Bind(wxEVT_DATAVIEW_VIEW_SET, &CValueTableBox::OnViewSet, this);

		dataViewCtrl->GenericGetHeader()->Bind(wxEVT_HEADER_RESIZING, &CValueTableBox::OnHeaderResizing, this);

		dataViewCtrl->Bind(wxEVT_SCROLLWIN_TOP, &CValueTableBox::HandleOnScroll, this);
		dataViewCtrl->Bind(wxEVT_SCROLLWIN_BOTTOM, &CValueTableBox::HandleOnScroll, this);
		dataViewCtrl->Bind(wxEVT_SCROLLWIN_LINEUP, &CValueTableBox::HandleOnScroll, this);
		dataViewCtrl->Bind(wxEVT_SCROLLWIN_LINEDOWN, &CValueTableBox::HandleOnScroll, this);
		dataViewCtrl->Bind(wxEVT_SCROLLWIN_PAGEUP, &CValueTableBox::HandleOnScroll, this);
		dataViewCtrl->Bind(wxEVT_SCROLLWIN_PAGEDOWN, &CValueTableBox::HandleOnScroll, this);

		dataViewCtrl->Bind(wxEVT_SCROLLWIN_THUMBTRACK, &CValueTableBox::HandleOnScroll, this);
		dataViewCtrl->Bind(wxEVT_SCROLLWIN_THUMBRELEASE, &CValueTableBox::HandleOnScroll, this);

		dataViewCtrl->GetMainWindow()->Bind(wxEVT_LEFT_DOWN, &CValueTableBox::OnMainWindowClick, this);

#if wxUSE_DRAG_AND_DROP && wxUSE_UNICODE
		dataViewCtrl->EnableDragSource(wxDF_UNICODETEXT);
		dataViewCtrl->EnableDropTarget(wxDF_UNICODETEXT);
#endif // wxUSE_DRAG_AND_DROP && wxUSE_UNICODE

		dataViewCtrl->Bind(wxEVT_SIZE, &CValueTableBox::OnSize, this);
		dataViewCtrl->Bind(wxEVT_IDLE, &CValueTableBox::OnIdle, this);

		dataViewCtrl->Bind(wxEVT_MENU, &CValueTableBox::OnCommandMenu, this);
		dataViewCtrl->Bind(wxEVT_DATAVIEW_ITEM_CONTEXT_MENU, &CValueTableBox::OnContextMenu, this);
	}

	return dataViewCtrl;
}

void CValueTableBox::OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)
{
	wxTableViewCtrl* dataViewCtrl = dynamic_cast<wxTableViewCtrl*>(wxobject);

	if (dataViewCtrl != nullptr) CValueTableBox::CreateModel();

	if (visualHost->IsDesignerHost() && GetChildCount() == 0
		&& first—reated) {
		CValueTableBox::AddColumn();
	}
}

#include <wx/itemattr.h>

void CValueTableBox::Update(wxObject* wxobject, IVisualHost* visualHost)
{
	wxTableViewCtrl* dataViewCtrl =
		dynamic_cast<wxTableViewCtrl*>(wxobject);

	if (dataViewCtrl != nullptr) {
		UpdateWindow(dataViewCtrl);
	}
}

void CValueTableBox::OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost)
{
	wxTableViewCtrl* dataViewCtrl = dynamic_cast<wxTableViewCtrl*>(wxobject);

	if (dataViewCtrl != nullptr) {

		wxDataViewExtModel* dataViewOldModel = dataViewCtrl->GetModel();
		wxDataViewExtModel* dataViewNewModel = m_tableModel != nullptr ?
			m_tableModel->GetDataViewModel() : nullptr;

		if (dataViewNewModel != dataViewOldModel) {
			if (dataViewOldModel == nullptr) dataViewCtrl->SetFocus();
			dataViewCtrl->AssociateModel(dataViewNewModel);
			m_tableCurrentLine.Reset();
		}

		if (appData->DesignerMode()) {
			if (!visualHost->IsDesignerHost() || m_propertyHeader->GetValueAsBoolean()) {
				dataViewCtrl->ShowHeaderWindow(m_propertyHeader->GetValueAsBoolean());
				dataViewCtrl->SetHeaderHeight(m_propertyHeaderHeight->GetValueAsUInteger());
			}
			else {
				dataViewCtrl->ShowHeaderWindow(true);
				dataViewCtrl->SetHeaderHeight(1);
				dataViewCtrl->SetForegroundColour(*wxLIGHT_GREY);
			}
		}
		else {
			dataViewCtrl->ShowHeaderWindow(m_propertyHeader->GetValueAsBoolean());
			dataViewCtrl->SetHeaderHeight(m_propertyHeaderHeight->GetValueAsUInteger());
		}

		dataViewCtrl->ShowFooterWindow(m_propertyFooter->GetValueAsBoolean());
		dataViewCtrl->SetFooterHeight(m_propertyFooterHeight->GetValueAsUInteger());

		dataViewCtrl->FreezeTo(
			m_propertyFreezeRow->GetValueAsUInteger(), m_propertyFreezeCol->GetValueAsUInteger());

		dataViewCtrl->SetSelectionMode(m_propertyRowSelectionMode->GetValueAsEnum());
		dataViewCtrl->SetViewMode(m_propertyViewMode->GetValueAsEnum());

		wxItemAttr attr(
			dataViewCtrl->GetForegroundColour(),
			dataViewCtrl->GetBackgroundColour(),
			dataViewCtrl->GetFont()
		);

		dataViewCtrl->SetHeaderAttr(attr);

		if (!appData->DesignerMode())
			m_dataViewCreated = m_dataViewUpdated = true;
	}
}

void CValueTableBox::Cleanup(wxObject* obj, IVisualHost* visualHost)
{
	wxTableViewCtrl* dataViewCtrl = dynamic_cast<wxTableViewCtrl*>(obj);
	m_dataViewCreated = m_dataViewUpdated = false;
	if (dataViewCtrl != nullptr) dataViewCtrl->AssociateModel(nullptr);
	m_tableCurrentLine.Reset();
}

//***********************************************************************************
//*                                  Property                                       *
//***********************************************************************************

bool CValueTableBox::LoadData(CMemoryReader& reader)
{
	if (!m_propertySource->LoadData(reader))
		return false;

	m_propertyHeader->LoadData(reader);
	m_propertyHeaderHeight->LoadData(reader);
	m_propertyFooter->LoadData(reader);
	m_propertyFooterHeight->LoadData(reader);

	m_propertyFreezeRow->LoadData(reader);
	m_propertyFreezeCol->LoadData(reader);

	m_propertyRowSelectionMode->LoadData(reader);

	//events
	m_eventSelection->LoadData(reader);
	m_eventBeforeAddRow->LoadData(reader);
	m_eventBeforeDeleteRow->LoadData(reader);
	m_eventOnActivateRow->LoadData(reader);

	return IValueWindow::LoadData(reader);
}

bool CValueTableBox::SaveData(CMemoryWriter& writer)
{
	if (!m_propertySource->SaveData(writer))
		return false;

	m_propertyHeader->SaveData(writer);
	m_propertyHeaderHeight->SaveData(writer);
	m_propertyFooter->SaveData(writer);
	m_propertyFooterHeight->SaveData(writer);

	m_propertyFreezeRow->SaveData(writer);
	m_propertyFreezeCol->SaveData(writer);

	m_propertyRowSelectionMode->SaveData(writer);

	//events
	m_eventSelection->SaveData(writer);
	m_eventBeforeAddRow->SaveData(writer);
	m_eventBeforeDeleteRow->SaveData(writer);
	m_eventOnActivateRow->SaveData(writer);

	return IValueWindow::SaveData(writer);
}

//***********************************************************************************

enum prop {
	eTableValue,
	eCurrentRow,
};

IMetaData* CValueTableBox::GetMetaData() const
{
	return m_formOwner != nullptr ?
		m_formOwner->GetMetaData() : nullptr;
}

void CValueTableBox::PrepareNames() const
{
	IValueFrame::PrepareNames();

	m_methodHelper->AppendProp(wxT("Value"), eTableValue, eControl);
	m_methodHelper->AppendProp(wxT("CurrentRow"), eCurrentRow, eControl);
}

bool CValueTableBox::SetPropVal(const long lPropNum, const CValue& varPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum); bool refreshColumn = false;
	if (lPropAlias == eControl) {
		const long lPropData = m_methodHelper->GetPropData(lPropNum);
		if (lPropData == eTableValue) {
			m_tableModel = varPropVal.ConvertToType<IValueTable>();
			m_tableCurrentLine.Reset();
			refreshColumn = true;
		}
		else if (lPropData == eCurrentRow) {
			IValueTable::IValueModelReturnLine* tableReturnLine = nullptr;
			if (varPropVal.ConvertToValue(tableReturnLine)) {
				if (m_tableModel == tableReturnLine->GetOwnerModel())
					m_tableCurrentLine = tableReturnLine;
				else m_tableCurrentLine.Reset();
			}
			else {
				m_tableCurrentLine.Reset();
			}
		}
	}

	bool result = IValueFrame::SetPropVal(lPropNum, varPropVal);

	if (refreshColumn && m_tableModel != nullptr && m_tableModel->AutoCreateColumn()) {
		CValueTableBox::CreateColumnCollection();
	}

	return result;
}

bool CValueTableBox::GetPropVal(const long lPropNum, CValue& pvarPropVal)
{
	const long lPropAlias = m_methodHelper->GetPropAlias(lPropNum);
	if (lPropAlias == eControl) {
		const long lPropData = m_methodHelper->GetPropData(lPropNum);
		if (lPropData == eTableValue) {
			pvarPropVal = m_tableModel;
			return true;
		}
		else if (lPropData == eCurrentRow) {
			pvarPropVal = m_tableCurrentLine;
			return true;
		}
	}
	return IValueFrame::GetPropVal(lPropNum, pvarPropVal);
}

//***********************************************************************
//*                       Register in runtime                           *
//***********************************************************************

ENUM_TYPE_REGISTER(CValueEnumTableBoxSelectionMode, "TableboxRowSelectionMode", string_to_clsid("EN_TBXSL"));
ENUM_TYPE_REGISTER(CValueEnumTableBoxViewMode, "TableboxViewMode", string_to_clsid("EN_TBXVM"));
CONTROL_TYPE_REGISTER(CValueTableBox, "Tablebox", "Container", g_controlTableBoxCLSID);