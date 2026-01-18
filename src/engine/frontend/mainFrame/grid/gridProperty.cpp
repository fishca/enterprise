////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko
//	Description : grid property
////////////////////////////////////////////////////////////////////////////

#include "gridWindow.h"
#include "frontend/mainFrame/mainFrame.h"

static const wxString strTranslateType = wxT("translate");

void CGridExtCtrl::CGridExtCellProperty::AddSelectedCell(const wxGridExtBlockCoords& coords, bool afterErase)
{
	if (afterErase) ClearSelectedCell();

	m_currentSelection.push_back(coords);

	objectInspector->SelectObject(this, true);
	m_view->ForceRefresh();
}

void CGridExtCtrl::CGridExtCellProperty::ShowProperty()
{
	CGridExtCellProperty::ClearSelectedCell(); bool hasBlocks = false;

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

void CGridExtCtrl::CGridExtCellProperty::OnPropertyCreated(IProperty* property, const wxGridExtBlockCoords& coords)
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
}

void CGridExtCtrl::CGridExtCellProperty::OnPropertyChanged(IProperty* property, const wxGridExtBlockCoords& coords)
{
	for (int col = coords.GetLeftCol(); col <= coords.GetRightCol(); col++) {
		for (int row = coords.GetTopRow(); row <= coords.GetBottomRow(); row++) {
			if (m_propertyText == property) {
				m_view->SetCellValue(row, col, m_propertyText->GetValueAsString());
			}
			else if (m_propertyFont == property) {
				m_view->SetCellFont(row, col, m_propertyFont->GetValueAsFont());
			}
			else if (m_propertyBackgroundColour == property) {
				m_view->SetCellBackgroundColour(row, col, m_propertyBackgroundColour->GetValueAsColour());
			}
			else if (m_propertyTextColour == property) {
				m_view->SetCellTextColour(row, col, m_propertyTextColour->GetValueAsColour());
			}
			else if (m_propertyAlignHorz == property || m_propertyAlignVert == property) {
				m_view->SetCellAlignment(row, col, m_propertyAlignHorz->GetValueAsInteger(), m_propertyAlignVert->GetValueAsInteger());
			}
			else if (m_propertyOrient == property) {
				m_view->SetCellTextOrient(row, col, m_propertyOrient->GetValueAsInteger());
			}
			else if (m_propertyLeftBorder == property || m_propertyRightBorder == property || m_propertyTopBorder == property || m_propertyBottomBorder == property || m_propertyColourBorder == property) 
			{
				m_view->SetCellBorderLeft(row, col, m_propertyLeftBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
				m_view->SetCellBorderRight(row, col, m_propertyRightBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
				m_view->SetCellBorderTop(row, col, m_propertyTopBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
				m_view->SetCellBorderBottom(row, col, m_propertyBottomBorder->GetValueAsEnum(), m_propertyColourBorder->GetValueAsColour());
			}
		}
	}
}

#include "backend/metadataConfiguration.h"

IMetaData* CGridExtCtrl::CGridExtCellProperty::GetMetaData() const
{
	return IMetaDataConfiguration::Get();
}

void CGridExtCtrl::CGridExtCellProperty::OnPropertyCreated(IProperty* property)
{
	for (auto coords : m_currentSelection) {
		CGridExtCellProperty::OnPropertyCreated(property, coords);
	}
}

void CGridExtCtrl::CGridExtCellProperty::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	int maxRow = m_view->GetGridCursorRow(), maxCol = m_view->GetGridCursorCol();

	for (auto coords : m_currentSelection) {

		if (maxRow < coords.GetBottomRow())
			maxRow = coords.GetBottomRow();

		if (maxCol < coords.GetRightCol())
			maxCol = coords.GetRightCol();

		CGridExtCellProperty::OnPropertyChanged(property, coords);
	}

	m_view->SendPropertyModify({ maxRow, maxCol });
	m_view->ForceRefresh();
}