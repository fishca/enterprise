////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : grid property
////////////////////////////////////////////////////////////////////////////

#include "gridEditor.h"
#include "frontend/mainFrame/mainFrame.h"

void CGridEditor::CPropertyGridEditorSpreadsheet::ShowInspector()
{
	m_selection.clear(); bool hasBlocks = false;

	for (const wxGridExtBlockCoords& coords : m_view->GetSelectedBlocks()) {
		m_selection.push_back(coords); hasBlocks = true;
	}

	if (!hasBlocks) {
		wxGridExtBlockCoords coords(m_view->GetGridCursorRow(), m_view->GetGridCursorCol(),
			m_view->GetGridCursorRow(), m_view->GetGridCursorCol());
		m_selection.push_back(coords);
	}

	if (!objectInspector->IsShownInspector())
		objectInspector->ShowInspector();

	objectInspector->SelectObject(this, true);
}

void CGridEditor::CPropertyGridEditorSpreadsheet::OnPropertyCreated(IProperty* property, const wxGridExtBlockCoords& coords)
{
	if (m_propertyName == property) {
		if (coords.GetTopLeft() != coords.GetBottomRight()) {
			wxString nameField = wxString::Format(wxT("R%iC%i:R%iC%i"),
				coords.GetTopRow() + 1,
				coords.GetLeftCol() + 1,
				coords.GetBottomRow() + 1,
				coords.GetRightCol() + 1
			);
			m_propertyName->SetValue(nameField);
		}
		else {
			wxString nameField = wxString::Format(wxT("R%iC%i"),
				coords.GetTopRow() + 1,
				coords.GetLeftCol() + 1
			);
			m_propertyName->SetValue(nameField);
		}
	}
	else if (m_propertyAlignHorz == property) {
		int horz, vert;
		m_view->GetCellAlignment(coords.GetTopRow(), coords.GetLeftCol(), &horz, &vert);
		m_propertyAlignHorz->SetValue(horz);
		m_propertyAlignVert->SetValue(vert);
	}
	else if (m_propertyOrient == property) {
		m_propertyOrient->SetValue(
			m_view->GetCellTextOrient(coords.GetTopRow(), coords.GetLeftCol())
		);
	}
	else if (m_propertyFont == property) {
		m_propertyFont->SetValue(m_view->GetCellFont(coords.GetTopRow(), coords.GetLeftCol()));
	}
	else if (m_propertyBackgroundColour == property) {
		m_propertyBackgroundColour->SetValue(m_view->GetCellBackgroundColour(coords.GetTopRow(), coords.GetLeftCol()));
	}
	else if (m_propertyTextColour == property) {
		m_propertyTextColour->SetValue(m_view->GetCellTextColour(coords.GetTopRow(), coords.GetLeftCol()));
	}
	else if (m_propertyLeftBorder == property || m_propertyRightBorder == property || m_propertyTopBorder == property || m_propertyBottomBorder == property || m_propertyColourBorder == property) {
		wxGridExtCellBorder borderLeft = m_view->GetCellBorderLeft(coords.GetTopRow(), coords.GetLeftCol());
		wxGridExtCellBorder borderRight = m_view->GetCellBorderRight(coords.GetTopRow(), coords.GetLeftCol());
		wxGridExtCellBorder borderTop = m_view->GetCellBorderTop(coords.GetTopRow(), coords.GetLeftCol());
		wxGridExtCellBorder borderBottom = m_view->GetCellBorderBottom(coords.GetTopRow(), coords.GetLeftCol());
		m_propertyLeftBorder->SetValue(borderLeft.m_style);
		m_propertyRightBorder->SetValue(borderRight.m_style);
		m_propertyTopBorder->SetValue(borderTop.m_style);
		m_propertyBottomBorder->SetValue(borderBottom.m_style);
		m_propertyColourBorder->SetValue(borderLeft.m_colour);
	}
	else if (m_propertyFitMode == property) {
		wxGridExtFitMode fitMode = m_view->GetCellFitMode(coords.GetTopRow(), coords.GetLeftCol());
		if (fitMode.IsOverflow())
			m_propertyFitMode->SetValue(enFitMode_Overflow);
		else if (fitMode.IsClip())
			m_propertyFitMode->SetValue(enFitMode_Clip);
	}
	else if (m_propertyReadOnly == property) {
		m_propertyReadOnly->SetValue(m_view->IsCellReadOnly(coords.GetTopRow(), coords.GetLeftCol()));
	}
	else if (m_propertyFillType == property) {
		if (m_view->GetTable()->CanGetValueAs(coords.GetTopRow(), coords.GetLeftCol(), s_strTypeTextOrString))
			m_propertyFillType->SetValue(enSpreadsheetFillType::enSpreadsheetFillType_StrText);
		else if (m_view->GetTable()->CanGetValueAs(coords.GetTopRow(), coords.GetLeftCol(), s_strTypeTemplate))
			m_propertyFillType->SetValue(enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate);
		else if (m_view->GetTable()->CanGetValueAs(coords.GetTopRow(), coords.GetLeftCol(), s_strTypeParameter))
			m_propertyFillType->SetValue(enSpreadsheetFillType::enSpreadsheetFillType_StrParameter);
	}
	else if (m_propertyText == property) {

		if (m_propertyFillType->GetValueAsEnum() == enSpreadsheetFillType::enSpreadsheetFillType_StrText) {
			void* value = m_view->GetTable()->GetValueAsCustom(coords.GetTopRow(), coords.GetLeftCol(), s_strTypeTextOrString);
			wxSharedPtr<wxString> textString =
				wxSharedPtr<wxString>(static_cast<wxString*>(value));
			m_propertyText->SetValue(*textString);
		}
		else if (m_propertyFillType->GetValueAsEnum() == enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate) {
			void* value = m_view->GetTable()->GetValueAsCustom(coords.GetTopRow(), coords.GetLeftCol(), s_strTypeTemplate);
			wxSharedPtr<wxString> textString =
				wxSharedPtr<wxString>(static_cast<wxString*>(value));
			m_propertyText->SetValue(*textString);
		}
		else if (m_propertyFillType->GetValueAsEnum() == enSpreadsheetFillType::enSpreadsheetFillType_StrParameter) {
			void* value = m_view->GetTable()->GetValueAsCustom(coords.GetTopRow(), coords.GetLeftCol(), s_strTypeParameter);
			wxSharedPtr<wxString> textString =
				wxSharedPtr<wxString>(static_cast<wxString*>(value));
			m_propertyText->SetValue(*textString);
		}
	}
	else if (m_propertyParameter == property) {
		void* value = m_view->GetTable()->GetValueAsCustom(coords.GetTopRow(), coords.GetLeftCol(), s_strTypeParameter);
		wxSharedPtr<wxString> textString =
			wxSharedPtr<wxString>(static_cast<wxString*>(value));
		m_propertyParameter->SetValue(*textString);
	}
}

void CGridEditor::CPropertyGridEditorSpreadsheet::OnPropertyChanged(IProperty* property, const wxGridExtBlockCoords& coords)
{
	if (m_propertyFont == property) {
		m_view->SetCellFont(coords, m_propertyFont->GetValueAsFont());
	}
	else if (m_propertyBackgroundColour == property) {
		m_view->SetCellBackgroundColour(coords, m_propertyBackgroundColour->GetValueAsColour());
	}
	else if (m_propertyTextColour == property) {
		m_view->SetCellTextColour(coords, m_propertyTextColour->GetValueAsColour());
	}
	else if (m_propertyAlignHorz == property || m_propertyAlignVert == property) {
		m_view->SetCellAlignment(coords, m_propertyAlignHorz->GetValueAsInteger(), m_propertyAlignVert->GetValueAsInteger());
	}
	else if (m_propertyOrient == property) {
		m_view->SetCellTextOrient(coords, m_propertyOrient->GetValueAsInteger());
	}
	else if (m_propertyLeftBorder == property) {
		m_view->SetCellBorderLeft(coords, (wxPenStyle)m_propertyLeftBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
	}
	else if (m_propertyRightBorder == property) {
		m_view->SetCellBorderRight(coords, (wxPenStyle)m_propertyRightBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
	}
	else if (m_propertyTopBorder == property) {
		m_view->SetCellBorderTop(coords, (wxPenStyle)m_propertyTopBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
	}
	else if (m_propertyBottomBorder == property) {
		m_view->SetCellBorderBottom(coords, (wxPenStyle)m_propertyBottomBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
	}
	else if (m_propertyColourBorder == property) {
		m_view->SetCellBorderLeft(coords, (wxPenStyle)m_propertyLeftBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
		m_view->SetCellBorderRight(coords, (wxPenStyle)m_propertyRightBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
		m_view->SetCellBorderTop(coords, (wxPenStyle)m_propertyTopBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
		m_view->SetCellBorderBottom(coords, (wxPenStyle)m_propertyBottomBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
	}
	else if (m_propertyFitMode == property) {
		m_view->SetCellFitMode(coords, m_propertyFitMode->GetValueAsEnum() == enFitMode_Overflow ? wxGridExtFitMode::Overflow() : wxGridExtFitMode::Clip());
	}
	else if (m_propertyReadOnly == property) {
		m_view->SetCellReadOnly(coords, m_propertyReadOnly->GetValueAsBoolean());
	}
	else if (m_propertyFillType == property) {
		enSpreadsheetFillType type = m_propertyFillType->GetValueAsEnum();
		for (int row = coords.GetTopRow(); row <= coords.GetBottomRow(); row++) {
			for (int col = coords.GetLeftCol(); col <= coords.GetRightCol(); col++) {
				if (type == enSpreadsheetFillType::enSpreadsheetFillType_StrText) {
					wxSharedPtr<wxString> ptr;
					ptr = static_cast<wxString*>(m_view->GetTable()->GetValueAsCustom(row, col, s_strTypeTextOrString));
					m_view->GetTable()->SetValueAsCustom(row, col, s_strTypeTextOrString, ptr.get());
				}
				else if (type == enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate) {
					wxSharedPtr<wxString> ptr;
					ptr = static_cast<wxString*>(m_view->GetTable()->GetValueAsCustom(row, col, s_strTypeTemplate));
					m_view->GetTable()->SetValueAsCustom(row, col, s_strTypeTemplate, ptr.get());
				}
				else if (type == enSpreadsheetFillType::enSpreadsheetFillType_StrParameter) {
					wxSharedPtr<wxString> ptr;
					ptr = static_cast<wxString*>(m_view->GetTable()->GetValueAsCustom(row, col, s_strTypeParameter));
					m_view->GetTable()->SetValueAsCustom(row, col, s_strTypeParameter, ptr.get());
				}
			}
		}
	}
	else if (m_propertyText == property) {
		m_view->SetCellValue(coords, m_propertyText->GetValueAsString());
	}
	else if (m_propertyParameter == property) {
		m_view->SetCellValue(coords, m_propertyParameter->GetValueAsString());
	}
}

#include "backend/metadataConfiguration.h"

IMetaData* CGridEditor::CPropertyGridEditorSpreadsheet::GetMetaData() const
{
	return IMetaDataConfiguration::Get();
}

void CGridEditor::CPropertyGridEditorSpreadsheet::OnPropertyCreated(IProperty* property)
{
	for (const auto& coords : m_selection)
		CPropertyGridEditorSpreadsheet::OnPropertyCreated(property, coords);
}

void CGridEditor::CPropertyGridEditorSpreadsheet::OnPropertyRefresh(wxPropertyGridManager* pg, wxPGProperty* pgProperty, IProperty* property)
{
	if (m_propertyText == property) {
		pg->HideProperty(pgProperty, m_propertyFillType->GetValueAsEnum() == enSpreadsheetFillType::enSpreadsheetFillType_StrParameter);
	}
	else if (m_propertyParameter == property) {
		pg->HideProperty(pgProperty, m_propertyFillType->GetValueAsEnum() != enSpreadsheetFillType::enSpreadsheetFillType_StrParameter);
	}

}

void CGridEditor::CPropertyGridEditorSpreadsheet::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	const int row = m_view->GetGridCursorRow(),
		col = m_view->GetGridCursorCol();

	for (const wxGridExtBlockCoords& coords : m_selection)
		CPropertyGridEditorSpreadsheet::OnPropertyChanged(property, coords);

	m_view->ForceRefresh();
}

void CGridEditor::CPropertyGridEditorSpreadsheet::OnSelectCell(wxGridExtEvent& event)
{
	if (event.Selecting()) {
		wxGridExtBlockCoords coords(event.GetRow(), event.GetCol(),
			event.GetRow(), event.GetCol());
		m_selection.push_back(coords);
	}
	else {
		m_selection.clear();
	}

	objectInspector->SelectObject(this, true);
	m_view->ForceRefresh();
	event.Skip();
}

void CGridEditor::CPropertyGridEditorSpreadsheet::OnSelectCells(wxGridExtRangeSelectEvent& event)
{
	if (event.Selecting()) {
		wxGridExtBlockCoords coords(event.GetTopRow(), event.GetLeftCol(),
			event.GetBottomRow(), event.GetRightCol());
		m_selection.push_back(coords);
	}
	else {
		m_selection.clear();
	}

	objectInspector->SelectObject(this, true);
	m_view->ForceRefresh();
	event.Skip();
}

