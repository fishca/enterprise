#ifndef __DVC_H__
#define __DVC_H__

#include "frontend/win/ctrls/dataview/dataview.h"

// ----------------------------------------------------------------------------
// CDataViewValueRenderer
// ----------------------------------------------------------------------------

#include "frontend/visualView/ctrl/form.h"
#include "frontend/visualView/ctrl/tableBox.h"

class CDataViewValueRenderer :
	public wxDataViewExtCustomRenderer {
public:

	virtual void FinishSelecting() {

		if (m_tableBoxColumn != nullptr) {
			CValueForm* valueForm = m_tableBoxColumn->GetOwnerForm();
			if (valueForm != nullptr) valueForm->RefreshForm();
		}

		if (m_editorCtrl != nullptr) {
			// Remove our event handler first to prevent it from (recursively) calling
			// us again as it would do via a call to FinishEditing() when the editor
			// loses focus when we hide it below.
			wxEvtHandler* const handler = m_editorCtrl->PopEventHandler();

			// Hide the control immediately but don't delete it yet as there could be
			// some pending messages for it.
			m_editorCtrl->Hide();

			wxPendingDelete.Append(handler);
			wxPendingDelete.Append(m_editorCtrl);

			// Ensure that DestroyEditControl() is not called again for this control.
			m_editorCtrl.Release();
		}

		DoHandleEditingDone(nullptr);
	}

	virtual void CancelEditing() {

		if (m_tableBoxColumn != nullptr) {
			CValueForm* valueForm = m_tableBoxColumn->GetOwnerForm();
			if (valueForm != nullptr) valueForm->RefreshForm();
		}

		wxDataViewExtCustomRenderer::CancelEditing();
	}

	virtual bool FinishEditing() {

		if (m_tableBoxColumn != nullptr) {
			CValueForm* valueForm = m_tableBoxColumn->GetOwnerForm();
			if (valueForm != nullptr) valueForm->RefreshForm();
		}

		return wxDataViewExtCustomRenderer::FinishEditing();
	}

	// This renderer can be either activatable or editable, for demonstration
	// purposes. In real programs, you should select whether the user should be
	// able to activate or edit the cell and it doesn't make sense to switch
	// between the two -- but this is just an example, so it doesn't stop us.
	explicit CDataViewValueRenderer(CValueTableBoxColumn* tableBoxColumn)
		: wxDataViewExtCustomRenderer(wxT("string"), wxDATAVIEW_CELL_EDITABLE, wxALIGN_LEFT), m_tableBoxColumn(tableBoxColumn)
	{
	}

	virtual bool IsCompatibleVariantType(const wxString& variantType) const { return true; }

	virtual bool Render(wxRect rect, wxDC* dc, int state) override
	{
		RenderText(m_valueVariant,
			0, // no offset
			rect,
			dc,
			state);

		return true;
	}

	virtual bool ActivateCell(const wxRect& cell,
		wxDataViewExtModel* model,
		const wxDataViewExtItem& item,
		unsigned int col,
		const wxMouseEvent* mouseEvent) override
	{
		return false;
	}

	virtual wxSize GetSize() const override {
		if (!m_valueVariant.IsNull()) {
			return GetTextExtent(m_valueVariant);
		}
		else {
			return GetView()->FromDIP(wxSize(wxDVC_DEFAULT_RENDERER_SIZE,
				wxDVC_DEFAULT_RENDERER_SIZE));
		}
	}

	virtual bool SetValue(const wxVariant& value) override {

		if (value.GetType() == wxT("number"))
			SetAlignment(wxALIGN_RIGHT);
		else
			SetAlignment(wxALIGN_LEFT);

		m_valueVariant = value;
		return true;
	}

	virtual bool GetValue(wxVariant& WXUNUSED(value)) const override
	{
		return true;
	}

#if wxUSE_ACCESSIBILITY
	virtual wxString GetAccessibleDescription() const override { return m_valueVariant; }
#endif // wxUSE_ACCESSIBILITY

	virtual bool HasEditorCtrl() const override {
		return true;
	}

	virtual wxWindow* CreateEditorCtrl(wxWindow* parent,
		wxRect labelRect,
		const wxVariant& value) override;

	virtual bool GetValueFromEditorCtrl(wxWindow* ctrl, wxVariant& value) override;

private:

	CValueTableBoxColumn* m_tableBoxColumn;
	wxVariant m_valueVariant;
};

// ----------------------------------------------------------------------------
// CDataViewColumnObject
// ----------------------------------------------------------------------------

class CDataViewColumnObject :
	public wxDataViewExtColumn, public wxObject {
public:

	CDataViewColumnObject(CValueTableBoxColumn* col,
		const wxString& title,
		unsigned int model_column,
		int width = wxDVC_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE)
		:
		wxDataViewExtColumn(title, new CDataViewValueRenderer(col), model_column, width, align, flags)
	{
	}

	CDataViewColumnObject(CValueTableBoxColumn* col,
		const wxBitmap& bitmap,
		unsigned int model_column,
		int width = wxDVC_DEFAULT_WIDTH,
		wxAlignment align = wxALIGN_CENTER,
		int flags = wxDATAVIEW_COL_RESIZABLE)
		:
		wxDataViewExtColumn(bitmap, new CDataViewValueRenderer(col), model_column, width, align, flags)
	{
	}

	CDataViewValueRenderer* GetRenderer() const { return static_cast<CDataViewValueRenderer*>(m_renderer); }

	void SetControl(CValueTableBoxColumn* control) { m_tableBoxColumn = control; }
	CValueTableBoxColumn* GetControl() const { return m_tableBoxColumn; }

	void SetColumnModel(unsigned int col_model) { m_model_column = col_model; }

private:

	CValueTableBoxColumn* m_tableBoxColumn;
};

#endif // !_DVC_H__
