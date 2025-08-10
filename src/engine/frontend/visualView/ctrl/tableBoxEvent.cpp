#include "tableBox.h" 
#include "backend/appData.h"

#include "frontend/visualView/dvc/dvc.h"

void CValueTableBox::OnColumnClick(wxDataViewEvent& event)
{
    ÑDataViewColumnContainer* dataViewColumn =
        dynamic_cast<ÑDataViewColumnContainer*>(event.GetDataViewColumn());
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
                wxDataViewCtrl* dataView = dataViewColumn->GetOwner();
                wxASSERT(dataView);
                try {
                    m_tableModel->CallRefreshModel(dataView->GetTopItem(), dataView->GetCountPerPage());
                }
                catch (const CBackendException* err) {
                    dataView->AssociateModel(nullptr);
                    throw(err);
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

void CValueTableBox::OnColumnReordered(wxDataViewEvent& event)
{
    ÑDataViewColumnContainer* dataViewColumn =
        dynamic_cast<ÑDataViewColumnContainer*>(event.GetDataViewColumn());
    wxASSERT(dataViewColumn);
    if (g_visualHostContext != nullptr) {
        CValueTableBoxColumn* columnControl = dataViewColumn->GetControl();
        wxASSERT(columnControl);
        if (ChangeChildPosition(columnControl, event.GetColumn())) {
            if (g_visualHostContext != nullptr) {
                g_visualHostContext->RefreshTree();
            }
        }
    }
    else if (!appData->DesignerMode()) {
        CValueTableBoxColumn* columnControl = dataViewColumn->GetControl();
        wxASSERT(columnControl);
        ChangeChildPosition(columnControl, event.GetColumn());
    }

    event.Skip();
}

//*********************************************************************
//*                          System event                             *
//*********************************************************************

void CValueTableBox::OnSelectionChanged(wxDataViewEvent& event)
{
    // event is a wxDataViewEvent
    const wxDataViewItem& item = event.GetItem();
    if (!item.IsOk())
        return;
    CValue standardProcessing = true;
    CallAsEvent(m_eventSelection,
        GetValue(), // control
        CValue(m_tableModel->GetRowAt(item)), // rowSelected
        standardProcessing //standardProcessing
    );
    if (standardProcessing.GetBoolean()) {
        if (m_tableCurrentLine != nullptr)
            m_tableCurrentLine->DecrRef();
        m_tableCurrentLine = m_tableModel->GetRowAt(item);
        m_tableCurrentLine->IncrRef();
        event.Skip();
    }
    else {
        event.Veto();
    }
}

void CValueTableBox::OnItemActivated(wxDataViewEvent& event)
{
    // event is a wxDataViewEvent
    const wxDataViewItem& item = event.GetItem();
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

void CValueTableBox::OnItemCollapsed(wxDataViewEvent& event)
{
    event.Skip();
}

void CValueTableBox::OnItemExpanded(wxDataViewEvent& event)
{
    event.Skip();
}

void CValueTableBox::OnItemCollapsing(wxDataViewEvent& event)
{
    event.Skip();
}

void CValueTableBox::OnItemExpanding(wxDataViewEvent& event)
{
    event.Skip();
}

void CValueTableBox::OnItemStartEditing(wxDataViewEvent& event)
{
    // event is a wxDataViewEvent
    const wxDataViewItem& item = event.GetItem();
    if (!item.IsOk())
        return;
    wxDataViewCtrl* dataViewCtrl = dynamic_cast<wxDataViewCtrl*>(event.GetEventObject());
    if (dataViewCtrl != nullptr) {
        wxDataViewColumn* currentColumn = dataViewCtrl->GetCurrentColumn();
        if (currentColumn != nullptr) {
            wxDataViewRenderer* renderer = currentColumn->GetRenderer();
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

void CValueTableBox::OnItemEditingStarted(wxDataViewEvent& event)
{
    event.Skip();
}

void CValueTableBox::OnItemEditingDone(wxDataViewEvent& event)
{
    event.Skip();
}

void CValueTableBox::OnItemValueChanged(wxDataViewEvent& event)
{
    event.Skip();
}

void CValueTableBox::OnItemStartInserting(wxDataViewEvent& event)
{
    if (m_tableModel != nullptr && !m_tableModel->IsCallRefreshModel()) m_formOwner->RefreshForm();
    else if (m_tableModel == nullptr) m_formOwner->RefreshForm();

    event.Skip();
}

void CValueTableBox::OnItemStartDeleting(wxDataViewEvent& event)
{
    // event is a wxDataViewEvent
    const wxDataViewItem& item = event.GetItem();
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
    wxDataModelViewCtrl* dataViewCtrl = dynamic_cast<wxDataModelViewCtrl*>(GetWxObject());
    if (dataViewCtrl != nullptr) {
        ÑDataViewColumnContainer* dataViewColumn =
            dynamic_cast<ÑDataViewColumnContainer*>(dataViewCtrl->GetColumn(event.GetColumn()));
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

void CValueTableBox::OnItemBeginDrag(wxDataViewEvent& event)
{
}

void CValueTableBox::OnItemDropPossible(wxDataViewEvent& event)
{
    if (event.GetDataFormat() != wxDF_UNICODETEXT)
        event.Veto();
    else
        event.SetDropEffect(wxDragMove);	// check 'move' drop effect
}

void CValueTableBox::OnItemDrop(wxDataViewEvent& event)
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

void CValueTableBox::OnContextMenu(wxDataViewEvent& event)
{
    const CActionCollection& actionData =
        CValueTableBox::GetActionCollection(m_formOwner->GetTypeForm());

    wxMenu menu;
    for (unsigned int idx = 0; idx < actionData.GetCount(); idx++) {
        const action_identifier_t& id = actionData.GetID(idx);
        if (id != wxNOT_FOUND) {
            wxMenuItem* menuItem = menu.Append(id, actionData.GetCaptionByID(id));
        }
    }
    wxDataViewCtrl* wnd = wxDynamicCast(
        event.GetEventObject(), wxDataViewCtrl
    );
    wxASSERT(wnd);
    wnd->SetClientData(event.GetDataViewColumn());
    wnd->PopupMenu(&menu, event.GetPosition());
}

void CValueTableBox::OnSize(wxSizeEvent& event)
{
    wxDataModelViewCtrl* dataViewCtrl =
        dynamic_cast<wxDataModelViewCtrl*>(event.GetEventObject());
    if (m_dataViewUpdated)
        m_dataViewSizeChanged = (m_dataViewSize != dataViewCtrl->GetSize()) && (m_dataViewSize != wxDefaultSize);
    event.Skip();
}

void CValueTableBox::OnIdle(wxIdleEvent& event)
{
    wxDataModelViewCtrl* dataViewCtrl =
        dynamic_cast<wxDataModelViewCtrl*>(event.GetEventObject());
    if (m_dataViewSizeChanged && m_tableModel != nullptr) 
        m_tableModel->CallRefreshModel(dataViewCtrl->GetTopItem(), dataViewCtrl->GetCountPerPage());
    m_dataViewSize = dataViewCtrl->GetSize();
    m_dataViewSizeChanged = false;
    event.Skip();
}

void CValueTableBox::HandleOnScroll(wxScrollWinEvent& event)
{
    wxDataModelViewCtrl* dataViewCtrl =
        dynamic_cast<wxDataModelViewCtrl*>(event.GetEventObject());

    const short scroll = (event.GetEventType() == wxEVT_SCROLLWIN_TOP ||
        event.GetEventType() == wxEVT_SCROLLWIN_LINEUP ||
        event.GetEventType() == wxEVT_SCROLLWIN_PAGEUP) ? 1 : -1;

    if (dataViewCtrl != nullptr) {

        if (m_tableModel != nullptr) {

            const int countPerPage = dataViewCtrl->GetCountPerPage();
            const wxDataViewItem& top_item = dataViewCtrl->GetTopItem();
            const wxDataViewItem& focused_item = m_tableCurrentLine != nullptr ?
                m_tableCurrentLine->GetLineItem() : wxDataViewItem(nullptr);

            m_tableModel->CallRefreshItemModel(top_item, focused_item, countPerPage, scroll);

            if (m_tableCurrentLine != nullptr && !m_tableModel->ValidateReturnLine(m_tableCurrentLine)) {
                const wxDataViewItem& currLine = m_tableModel->FindRowValue(m_tableCurrentLine);
                if (currLine.IsOk()) {
                    m_tableCurrentLine->DecrRef();
                    m_tableCurrentLine = nullptr;
                    m_tableCurrentLine = m_tableModel->GetRowAt(currLine);
                    m_tableCurrentLine->IncrRef();
                    dataViewCtrl->Select(m_tableCurrentLine->GetLineItem());
                }
            }

        }
    }
    event.Skip();
}

