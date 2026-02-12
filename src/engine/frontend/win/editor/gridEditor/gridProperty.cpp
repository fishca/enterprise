////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : grid property
////////////////////////////////////////////////////////////////////////////

#include "gridEditor.h"
#include "frontend/mainFrame/mainFrame.h"

static const wxString strTranslateType = wxT("translate");

void CGridEditor::CGridEditorCellProperty::AddSelectedCell(const wxGridExtBlockCoords& coords, bool afterErase)
{
	if (afterErase) ClearSelectedCell();

	m_currentSelection.push_back(coords);

	objectInspector->SelectObject(this, true);
	m_view->ForceRefresh();
}

void CGridEditor::CGridEditorCellProperty::ShowProperty()
{
	CGridEditorCellProperty::ClearSelectedCell(); bool hasBlocks = false;

	for (auto& coords : m_view->GetSelectedBlocks()) {
		m_currentSelection.push_back(coords); hasBlocks = true;
	}
	if (!hasBlocks) {
		m_currentSelection.push_back({
			m_view->GetGridCursorRow(), m_view->GetGridCursorCol(),
			m_view->GetGridCursorRow(), m_view->GetGridCursorCol()
			}
		);
	}

	if (!objectInspector->IsShownProperty())
		objectInspector->ShowProperty();

	objectInspector->SelectObject(this, true);
}

void CGridEditor::CGridEditorCellProperty::OnPropertyCreated(IProperty* property, const wxGridExtBlockCoords& coords)
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
	else if (m_propertyText == property) {

		void* tempval = m_view->GetTable()->GetValueAsCustom(coords.GetTopRow(), coords.GetLeftCol(), strTranslateType);
		if (tempval != nullptr)
		{
			m_propertyText->SetValue(CBackendLocalization::GetRawLocText(*((CBackendLocalizationEntryArray*)tempval)));
			delete (CBackendLocalizationEntryArray*)tempval;
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
	else if (m_propertyLeftBorder == property || m_propertyRightBorder == property || m_propertyTopBorder == property || m_propertyBottomBorder == property || m_propertyColourBorder == property)
	{
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
	else if (m_propertyFitMode == property){
		
		wxGridExtFitMode fitMode = m_view->GetCellFitMode(coords.GetTopRow(), coords.GetLeftCol());

		if (fitMode.IsOverflow())
			m_propertyFitMode->SetValue(enFitMode_Overflow);
		else if (fitMode.IsClip())
			m_propertyFitMode->SetValue(enFitMode_Clip);
	}
	else if (m_propertyReadOnly == property) {
		m_propertyReadOnly->SetValue(m_view->IsCellReadOnly(coords.GetTopRow(), coords.GetLeftCol()));
	}
}

void CGridEditor::CGridEditorCellProperty::OnPropertyChanged(IProperty* property, const wxGridExtBlockCoords& coords)
{
	if (m_propertyText == property) {
		m_view->SetCellValue(coords, m_propertyText->GetValueAsString());
	}
	else if (m_propertyFont == property) {
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
}

#include "backend/metadataConfiguration.h"

IMetaData* CGridEditor::CGridEditorCellProperty::GetMetaData() const
{
	return IMetaDataConfiguration::Get();
}

void CGridEditor::CGridEditorCellProperty::OnPropertyCreated(IProperty* property)
{
	for (const auto coords : m_currentSelection) {
		CGridEditorCellProperty::OnPropertyCreated(property, coords);
	}
}

void CGridEditor::CGridEditorCellProperty::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	int maxRow = m_view->GetGridCursorRow(), maxCol = m_view->GetGridCursorCol();

	for (auto coords : m_currentSelection) {

		if (maxRow < coords.GetBottomRow())
			maxRow = coords.GetBottomRow();

		if (maxCol < coords.GetRightCol())
			maxCol = coords.GetRightCol();

		CGridEditorCellProperty::OnPropertyChanged(property, coords);
	}

	m_view->SendPropertyModify({ maxRow, maxCol });
	m_view->ForceRefresh();
}