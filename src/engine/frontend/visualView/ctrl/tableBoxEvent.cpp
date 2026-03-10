#include "tableBox.h" 
#include "tableBoxColumnRenderer.h"

#include "backend/appData.h"

void CValueTableBox::OnColumnClick(wxDataViewExtEvent& event)
{
	CDataViewColumnObject* dataViewColumn =
		dynamic_cast<CDataViewColumnObject*>(event.GetDataViewColumn());
	wxASSERT(dataViewColumn);
	if (g_visualHostContext != nullptr) {
		CValueTableBoxColumn* columnControl = dataViewColumn->GetControl();
		wxASSERT(columnControl);
		g_visualHostContext->SelectControl(columnControl);
	}

	if (m_tableModel != nullptr) {
		CSortOrder::CSortData* sort = m_tableModel->GetSortByID(event.GetColumn());
		if (sort != nullptr && !sort->m_sortSystem) {

			m_tableModel->ResetSort();

			sort->m_sortAscending = !sort->m_sortAscending;
			sort->m_sortEnable = true;

			if (!appData->DesignerMode()) {

				wxDataViewExtCtrl* dataViewCtrl = dataViewColumn->GetOwner();
				wxASSERT(dataViewCtrl);

				try {
					m_tableModel->CallRefreshModel(dataViewCtrl->GetTopItem(), dataViewCtrl->GetCountPerPage());
				}
				catch (const CBackendException* err) {
					dataViewCtrl->AssociateModel(nullptr);
					throw(err);
				}

				if (m_tableCurrentLine != nullptr && !m_tableModel->ValidateReturnLine(m_tableCurrentLine)) {
					const wxDataViewExtItem& currLine = m_tableModel->FindRowValue(&(*m_tableCurrentLine));
					if (currLine.IsOk())
						m_tableCurrentLine = m_tableModel->GetRowAt(currLine);
					else m_tableCurrentLine.Reset();
				}

				if (m_tableCurrentLine != nullptr) {
					dataViewCtrl->Select(
						m_tableCurrentLine->GetLineItem()
					);
				}
			}
		}
		else {
			event.Veto();
			return;
		}
	}

	event.Skip();
}

void CValueTableBox::OnColumnReordered(wxDataViewExtEvent& event)
{
	CDataViewColumnObject* dataViewColumn =
		dynamic_cast<CDataViewColumnObject*>(event.GetDataViewColumn());
	wxASSERT(dataViewColumn);

	CValueTableBoxColumn* columnObject = dataViewColumn->GetControl();
	wxASSERT(columnObject);
	if (ChangeChildPosition(columnObject, event.GetColumn())) {
		if (g_visualHostContext != nullptr) g_visualHostContext->RefreshEditor();
	}

	SetCalculateColumnPos();
	event.Skip();
}

//*********************************************************************
//*                          System event                             *
//*********************************************************************

void CValueTableBox::OnSelectionChanged(wxDataViewExtEvent& event)
{
	// event is a wxDataViewExtEvent
	const wxDataViewExtItem& item = event.GetItem();
	if (!item.IsOk())
		return;
	CValue standardProcessing = true;
	CallAsEvent(m_eventSelection,
		GetValue(), // control
		CValue(m_tableModel->GetRowAt(item)), // rowSelected
		standardProcessing //standardProcessing
	);
	if (standardProcessing.GetBoolean()) {
		m_tableCurrentLine = m_tableModel->GetRowAt(item);
		event.Skip();
	}
	else {
		event.Veto();
	}
}

void CValueTableBox::OnItemActivated(wxDataViewExtEvent& event)
{
	// event is a wxDataViewExtEvent
	const wxDataViewExtItem& item = event.GetItem();
	if (!item.IsOk())
		return;
	if (m_tableModel != nullptr) {
		m_tableModel->ActivateItem(m_formOwner,
			item, event.GetColumn()
		);
	}

	CallAsEvent(m_eventOnActivateRow,
		GetValue() // control
	);

	event.Skip();
}

void CValueTableBox::OnItemCollapsed(wxDataViewExtEvent& event)
{
	event.Skip();
}

void CValueTableBox::OnItemExpanded(wxDataViewExtEvent& event)
{
	event.Skip();
}

void CValueTableBox::OnItemCollapsing(wxDataViewExtEvent& event)
{
	event.Skip();
}

void CValueTableBox::OnItemExpanding(wxDataViewExtEvent& event)
{
	event.Skip();
}

void CValueTableBox::OnItemStartEditing(wxDataViewExtEvent& event)
{
	// event is a wxDataViewExtEvent
	const wxDataViewExtItem& item = event.GetItem();
	if (!item.IsOk())
		return;
	wxDataViewExtCtrl* dataViewCtrl = dynamic_cast<wxDataViewExtCtrl*>(event.GetEventObject());
	if (dataViewCtrl != nullptr) {
		wxDataViewExtColumn* currentColumn = dataViewCtrl->GetCurrentColumn();
		if (currentColumn != nullptr) {
			wxDataViewExtRenderer* renderer = currentColumn->GetRenderer();
			if (renderer != nullptr) renderer->FinishEditing();
		}
	}
	if (!m_tableModel->EditableLine(item, event.GetColumn())) {
		if (m_tableModel != nullptr) m_tableModel->EditValue();
		event.Veto(); /*!!!*/
	}
	else
		event.Skip();
}

void CValueTableBox::OnItemEditingStarted(wxDataViewExtEvent& event)
{
	event.Skip();
}

void CValueTableBox::OnItemEditingDone(wxDataViewExtEvent& event)
{
	event.Skip();
}

void CValueTableBox::OnItemValueChanged(wxDataViewExtEvent& event)
{
	event.Skip();
}

void CValueTableBox::OnItemStartInserting(wxDataViewExtEvent& event)
{
	if (m_tableModel != nullptr && !m_tableModel->IsCallRefreshModel())
		m_formOwner->RefreshForm();
	else if (m_tableModel == nullptr)
		m_formOwner->RefreshForm();

	event.Skip();
}

void CValueTableBox::OnItemStartDeleting(wxDataViewExtEvent& event)
{
	// event is a wxDataViewExtEvent
	const wxDataViewExtItem& item = event.GetItem();
	if (!item.IsOk())
		return;
	CValue cancel = false;
	CallAsEvent(m_eventBeforeDeleteRow,
		GetValue(), // control
		cancel //cancel
	);

	if (m_tableModel != nullptr && !m_tableModel->IsCallRefreshModel()) m_formOwner->RefreshForm();
	else if (m_tableModel == nullptr) m_formOwner->RefreshForm();

	if (cancel.GetBoolean())
		event.Veto();
	else
		event.Skip();
}

void CValueTableBox::OnHeaderResizing(wxHeaderCtrlEvent& event)
{
	wxTableViewCtrl* dataViewCtrl = dynamic_cast<wxTableViewCtrl*>(GetWxObject());
	if (dataViewCtrl != nullptr) {
		CDataViewColumnObject* dataViewColumn =
			dynamic_cast<CDataViewColumnObject*>(dataViewCtrl->GetColumn(event.GetColumn()));
		CValueTableBoxColumn* columnControl = dataViewColumn->GetControl();
		wxASSERT(columnControl);
		columnControl->SetWidthColumn(event.GetWidth());
		if (g_visualHostContext != nullptr)
			g_visualHostContext->SelectControl(columnControl);
	}
	event.Skip();
}

void CValueTableBox::OnMainWindowClick(wxMouseEvent& event)
{
	if (g_visualHostContext != nullptr)
		g_visualHostContext->SelectControl(this);
	event.Skip();
}

#if wxUSE_DRAG_AND_DROP

void CValueTableBox::OnItemBeginDrag(wxDataViewExtEvent& event)
{
}

void CValueTableBox::OnItemDropPossible(wxDataViewExtEvent& event)
{
	if (event.GetDataFormat() != wxDF_UNICODETEXT)
		event.Veto();
	else
		event.SetDropEffect(wxDragMove);	// check 'move' drop effect
}

void CValueTableBox::OnItemDrop(wxDataViewExtEvent& event)
{
	if (event.GetDataFormat() != wxDF_UNICODETEXT) {
		event.Veto();
		return;
	}
}

#endif // wxUSE_DRAG_AND_DROP

void CValueTableBox::OnCommandMenu(wxCommandEvent& event)
{
	CValueTableBox::ExecuteAction(event.GetId(), m_formOwner);
}

void CValueTableBox::OnContextMenu(wxDataViewExtEvent& event)
{
	const CActionCollection& actionData =
		CValueTableBox::GetActionCollection(m_formOwner->GetTypeForm());

	wxMenu menu;
	for (unsigned int idx = 0; idx < actionData.GetCount(); idx++) {
		const action_identifier_t& id = actionData.GetID(idx);
		if (id != wxNOT_FOUND) {
			wxMenuItem* menuItem = menu.Append(id, actionData.GetCaptionByID(id));
			CPictureDescription &pictureDesc = actionData.GetPictureByID(id);
			if (!pictureDesc.IsEmptyPicture())
				menuItem->SetBitmap(CBackendPicture::CreatePicture(pictureDesc));
		}
	}
	wxDataViewExtCtrl* wnd = wxDynamicCast(
		event.GetEventObject(), wxDataViewExtCtrl
	);
	wxASSERT(wnd);
	wnd->SetClientData(event.GetDataViewColumn());
	wnd->PopupMenu(&menu, event.GetPosition());
}

void CValueTableBox::OnSize(wxSizeEvent& event)
{
	wxTableViewCtrl* dataViewCtrl = dynamic_cast<wxTableViewCtrl*>(event.GetEventObject());

	if (m_dataViewCreated)
		m_dataViewSizeChanged = m_dataViewUpdated || (m_dataViewSize != dataViewCtrl->GetSize()) && (m_dataViewSize != wxDefaultSize);

	event.Skip();
}

void CValueTableBox::OnIdle(wxIdleEvent& event)
{
	wxTableViewCtrl* dataViewCtrl = dynamic_cast<wxTableViewCtrl*>(event.GetEventObject());

	if (m_dataViewSizeChanged && m_tableModel != nullptr) {

		try {
			m_tableModel->CallRefreshModel(dataViewCtrl->GetTopItem(), dataViewCtrl->GetCountPerPage());
		}
		catch (const CBackendException* err) {
			dataViewCtrl->AssociateModel(nullptr);
			throw(err);
		}

		const CValue& createdValue = m_formOwner->GetCreatedValue();
		if (!createdValue.IsEmpty()) {
			const wxDataViewExtItem& currLine = m_tableModel->FindRowValue(createdValue);
			if (currLine.IsOk()) m_tableCurrentLine = m_tableModel->GetRowAt(currLine);
			else m_tableCurrentLine.Reset();
		}
		else if (!m_dataViewSelected) {
			IValueFrame* ownerControl = m_formOwner->GetOwnerControl();
			if (ownerControl != nullptr && m_tableCurrentLine == nullptr) {
				CValue retValue; ownerControl->GetControlValue(retValue);
				const wxDataViewExtItem& currLine = m_tableModel->FindRowValue(retValue);
				if (currLine.IsOk()) m_tableCurrentLine = m_tableModel->GetRowAt(currLine);
				else m_tableCurrentLine.Reset();
			}
			m_dataViewSelected = true;
		}

		if (m_tableCurrentLine != nullptr && !m_tableModel->ValidateReturnLine(m_tableCurrentLine)) {

			const wxDataViewExtItem& currLine = m_tableModel->FindRowValue(&(*m_tableCurrentLine));
			if (currLine.IsOk()) {
				m_tableCurrentLine = m_tableModel->GetRowAt(currLine);
			}
			else {
				const CValue& changedValue = m_formOwner->GetChangedValue();
				if (!changedValue.IsEmpty()) {
					const wxDataViewExtItem& currLine = m_tableModel->FindRowValue(changedValue);
					if (currLine.IsOk()) m_tableCurrentLine = m_tableModel->GetRowAt(currLine);
					else m_tableCurrentLine.Reset();
				}
				else {
					m_tableCurrentLine.Reset();
				}
			}
		}

		if (m_tableCurrentLine != nullptr) {
			dataViewCtrl->Select(
				m_tableCurrentLine->GetLineItem()
			);
		}
	}

	if (m_need_calculate_pos) {
		CalculateColumnPos();
		m_need_calculate_pos = false;
	}

	m_dataViewSize = dataViewCtrl->GetSize();
	m_dataViewUpdated = m_dataViewSizeChanged = false;
	event.Skip();
}

void CValueTableBox::HandleOnScroll(wxScrollWinEvent& event)
{
	wxTableViewCtrl* dataViewCtrl = dynamic_cast<wxTableViewCtrl*>(event.GetEventObject());

	const short scroll = (event.GetEventType() == wxEVT_SCROLLWIN_TOP ||
		event.GetEventType() == wxEVT_SCROLLWIN_LINEUP ||
		event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP) ? 1 : -1;

	if (dataViewCtrl != nullptr) {

		if (m_tableModel != nullptr) {

			const int countPerPage = dataViewCtrl->GetCountPerPage();
			const wxDataViewExtItem& top_item = dataViewCtrl->GetTopItem();
			const wxDataViewExtItem& focused_item = m_tableCurrentLine != nullptr ?
				m_tableCurrentLine->GetLineItem() : wxDataViewExtItem(nullptr);

			m_tableModel->CallRefreshItemModel(top_item, focused_item, countPerPage, scroll);

			if (m_tableCurrentLine != nullptr && !m_tableModel->ValidateReturnLine(m_tableCurrentLine)) {
				const wxDataViewExtItem& currLine = m_tableModel->FindRowValue(m_tableCurrentLine);
				if (currLine.IsOk()) {
					m_tableCurrentLine = m_tableModel->GetRowAt(currLine);
					dataViewCtrl->Select(m_tableCurrentLine->GetLineItem());
				}
				else {
					m_tableCurrentLine.Reset();
				}
			}
		}
	}

	event.Skip();
}

