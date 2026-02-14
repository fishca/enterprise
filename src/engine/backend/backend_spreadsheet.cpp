#include "backend_spreadsheet.h"
#include "backend/fileSystem/fs.h"

#define spreadsheetNotify \
	for (auto notify : m_spreadsheetNotifiers) notify

#pragma region __notifier_h__

void CBackendSpreadsheetObject::ClearSpreadsheet(int count)
{
	spreadsheetNotify->ClearSpreadsheet();
	m_spreadsheetDesc.ClearSpreadsheet(count);
}

//area 
CSpreadsheetDescription CBackendSpreadsheetObject::GetArea(int rowLeft, int rowRight, int colTop, int colBottom)
{
	CSpreadsheetDescription spreadsheetDesc;

	if (rowLeft >= 0 && colTop >= 0 && rowRight > 0 && colBottom > 0) {
		for (int row = rowLeft; row < rowRight; row++) {
			for (int col = colTop; col < colBottom; col++) {
				CSpreadsheetCellDescription* cell =
					spreadsheetDesc.GetOrCreateCell(row - rowLeft, col - colTop);
				cell->SetCell(m_spreadsheetDesc.GetCell(row, col));
				if (col - colTop == 0) spreadsheetDesc.SetColSize(col - colTop, m_spreadsheetDesc.GetColSize(row));
			}
			if (row - rowLeft == 0) spreadsheetDesc.SetRowSize(row - rowLeft, m_spreadsheetDesc.GetRowSize(row));
		}

		spreadsheetDesc.SetRowBrake(rowLeft - rowRight);
		spreadsheetDesc.SetColBrake(colTop - colBottom);
	}
	else if (rowLeft >= 0 && colTop < 0 && rowRight > 0 && colBottom < 0)
	{
		for (int row = rowLeft; row < rowRight; row++) {
			for (int col = 0; col < GetMaxColBrake(); col++) {
				CSpreadsheetCellDescription* cell =
					spreadsheetDesc.GetOrCreateCell(row - rowLeft, col - colTop);
				cell->SetCell(m_spreadsheetDesc.GetCell(row, col));
				if (col == 0) spreadsheetDesc.SetColSize(col, m_spreadsheetDesc.GetColSize(row));
			}
			if (row - rowLeft == 0) spreadsheetDesc.SetRowSize(row - rowLeft, m_spreadsheetDesc.GetRowSize(row));
		}

		spreadsheetDesc.SetRowBrake(rowLeft - rowRight);
		spreadsheetDesc.SetColBrake(GetMaxColBrake());
	}
	else if (rowLeft < 0 && colTop >= 0 && rowRight < 0 && colBottom > 0) {
		for (int row = 0; row < GetMaxRowBrake(); row++) {
			for (int col = colTop; col < colBottom; col++) {
				CSpreadsheetCellDescription* cell =
					spreadsheetDesc.GetOrCreateCell(row - rowLeft, col - colTop);
				cell->SetCell(m_spreadsheetDesc.GetCell(row, col));
				if (col - colTop == 0) spreadsheetDesc.SetColSize(col - colTop, m_spreadsheetDesc.GetColSize(row));
			}
			if (row == 0) spreadsheetDesc.SetRowSize(row, m_spreadsheetDesc.GetRowSize(row));
		}

		spreadsheetDesc.SetRowBrake(GetMaxRowBrake());
		spreadsheetDesc.SetColBrake(colTop - colBottom);
	}

	return spreadsheetDesc;
}

CSpreadsheetDescription CBackendSpreadsheetObject::GetAreaByName(const wxString& strAreaLeftName, const wxString& strAreaTopName)
{
	const CSpreadsheetAreaDescription* r = m_spreadsheetDesc.GetRowAreaByName(strAreaLeftName);
	const CSpreadsheetAreaDescription* c = m_spreadsheetDesc.GetColAreaByName(strAreaTopName);

	CSpreadsheetDescription spreadsheetDesc;

	if (r != nullptr && c != nullptr) {
		for (int row = r->m_start; row <= (int)r->m_end; row++) {
			for (int col = c->m_start; col <= (int)c->m_end; col++) {
				CSpreadsheetCellDescription* cell =
					spreadsheetDesc.GetOrCreateCell(row - r->m_start, col - c->m_start);
				cell->SetCell(m_spreadsheetDesc.GetCell(row, col));
				if (col - c->m_start == 0) spreadsheetDesc.SetColSize(col - c->m_start, m_spreadsheetDesc.GetColSize(row));
			}
			if (row - r->m_start == 0) spreadsheetDesc.SetRowSize(row - r->m_start, m_spreadsheetDesc.GetRowSize(row));
		}

		spreadsheetDesc.SetRowBrake(r->m_end - r->m_start);
		spreadsheetDesc.SetColBrake(c->m_end - c->m_start);
	}
	else if (r != nullptr) {
		for (int row = r->m_start; row <= (int)r->m_end; row++) {
			for (int col = 0; col <= GetMaxColBrake(); col++) {
				CSpreadsheetCellDescription* cell =
					spreadsheetDesc.GetOrCreateCell(row - r->m_start, col);
				cell->SetCell(m_spreadsheetDesc.GetCell(row, col));
				if (col == 0) spreadsheetDesc.SetColSize(col, m_spreadsheetDesc.GetColSize(row));
			}
			if (row - r->m_start == 0) spreadsheetDesc.SetRowSize(row - r->m_start, m_spreadsheetDesc.GetRowSize(row));
		}

		spreadsheetDesc.SetRowBrake(r->m_end - r->m_start);
		spreadsheetDesc.SetColBrake(GetMaxColBrake());
	}
	else if (c != nullptr) {
		for (int row = 0; row <= GetMaxRowBrake(); row++) {
			for (int col = c->m_start; col <= (int)c->m_end; col++) {
				CSpreadsheetCellDescription* cell =
					spreadsheetDesc.GetOrCreateCell(row, col - c->m_start);
				cell->SetCell(m_spreadsheetDesc.GetCell(row, col));
				if (col - c->m_start == 0) spreadsheetDesc.SetColSize(col - c->m_start, m_spreadsheetDesc.GetColSize(row));
			}
			if (row == 0) spreadsheetDesc.SetRowSize(row, m_spreadsheetDesc.GetRowSize(row));
		}

		spreadsheetDesc.SetRowBrake(GetMaxRowBrake());
		spreadsheetDesc.SetColBrake(c->m_end - c->m_start);
	}

	return spreadsheetDesc;
}

#include "backend_localization.h"

void CBackendSpreadsheetObject::PutArea(const wxObjectDataPtr<CBackendSpreadsheetObject>& doc)
{
	const int maxRowBrake = m_spreadsheetDesc.GetNumberRows();
	const int maxColBrake = m_spreadsheetDesc.GetNumberCols();

	for (int row = 0; row < doc->GetNumberRows(); row++) {
		for (int col = 0; col < doc->GetNumberCols(); col++) {

			CSpreadsheetCellDescription* cell =
				m_spreadsheetDesc.GetOrCreateCell(maxRowBrake + row, col);

			cell->SetCell(doc->GetSpreadsheetDesc().GetCell(row, col));

			if (cell->m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate || cell->m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrParameter) {
				cell->m_value = doc->ComputeStringValueFromParameters(cell->m_value, cell->m_fillSetType);
				cell->m_fillSetType = enSpreadsheetFillType::enSpreadsheetFillType_StrText;
			}

			if (maxColBrake + col == 0) m_spreadsheetDesc.SetColSize(maxColBrake + col, doc->GetColSize(col));
		}
		if (row == 0) m_spreadsheetDesc.SetRowSize(maxRowBrake + row, doc->GetRowSize(row));
	}

	m_spreadsheetDesc.SetRowBrake(maxRowBrake + doc->GetNumberRows() - 1);

	if (maxColBrake < doc->GetNumberCols())
		m_spreadsheetDesc.SetColBrake(maxColBrake + doc->GetNumberCols() - 1);
}

void CBackendSpreadsheetObject::JoinArea(const wxObjectDataPtr<CBackendSpreadsheetObject>& doc)
{
	const int maxRowBrake = m_spreadsheetDesc.GetNumberRows();
	const int maxColBrake = m_spreadsheetDesc.GetNumberCols();

	for (int col = 0; col < doc->GetNumberCols(); col++) {
		for (int row = 0; row < doc->GetNumberRows(); row++) {

			CSpreadsheetCellDescription* cell =
				m_spreadsheetDesc.GetOrCreateCell(row, maxColBrake + col);

			cell->SetCell(doc->GetSpreadsheetDesc().GetCell(row, col));

			if (cell->m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate || cell->m_fillSetType == enSpreadsheetFillType::enSpreadsheetFillType_StrParameter) {
				cell->m_value = doc->ComputeStringValueFromParameters(cell->m_value, cell->m_fillSetType);
				cell->m_fillSetType = enSpreadsheetFillType::enSpreadsheetFillType_StrText;
			}

			if (maxRowBrake + row == 0) m_spreadsheetDesc.SetRowSize(maxRowBrake + row, doc->GetRowSize(row));
		}
		if (col == 0) m_spreadsheetDesc.SetColSize(maxColBrake + col, doc->GetColSize(col));
	}

	if (maxRowBrake < doc->GetNumberRows())
		m_spreadsheetDesc.SetRowBrake(maxRowBrake + doc->GetNumberRows() - 1);

	m_spreadsheetDesc.SetColBrake(maxColBrake + doc->GetNumberCols() - 1);
}

//size 
void CBackendSpreadsheetObject::SetRowSize(int row, int height)
{
	spreadsheetNotify->SetRowSize(row, height);
	m_spreadsheetDesc.SetRowSize(row, height);
}

void CBackendSpreadsheetObject::SetColSize(int col, int width)
{
	spreadsheetNotify->SetColSize(col, width);
	m_spreadsheetDesc.SetColSize(col, width);
}

//freeze 
void CBackendSpreadsheetObject::SetRowFreeze(int row)
{
	spreadsheetNotify->SetRowFreeze(row);
	m_spreadsheetDesc.SetRowFreeze(row);
}

void CBackendSpreadsheetObject::SetColFreeze(int col)
{
	spreadsheetNotify->SetColFreeze(col);
	m_spreadsheetDesc.SetColFreeze(col);
}

// ------ row and col formatting
//

void CBackendSpreadsheetObject::SetCellBackgroundColour(int row, int col, const wxColour& colour)
{
	spreadsheetNotify->SetCellBackgroundColour(row, col, colour);
	m_spreadsheetDesc.SetCellBackgroundColour(row, col, colour);
}

void CBackendSpreadsheetObject::SetCellTextColour(int row, int col, const wxColour& colour)
{
	spreadsheetNotify->SetCellTextColour(row, col, colour);
	m_spreadsheetDesc.SetCellTextColour(row, col, colour);
}

void CBackendSpreadsheetObject::SetCellTextOrient(int row, int col, const int orient)
{
	spreadsheetNotify->SetCellTextOrient(row, col, orient);
	m_spreadsheetDesc.SetCellTextOrient(row, col, orient);
}

void CBackendSpreadsheetObject::SetCellFont(int row, int col, const wxFont& font)
{
	spreadsheetNotify->SetCellFont(row, col, font);
	m_spreadsheetDesc.SetCellFont(row, col, font);
}

void CBackendSpreadsheetObject::SetCellAlignment(int row, int col, const int horiz, const int vert)
{
	spreadsheetNotify->SetCellAlignment(row, col, horiz, vert);
	m_spreadsheetDesc.SetCellAlignment(row, col, horiz, vert);
}

void CBackendSpreadsheetObject::SetCellBorderLeft(int row, int col, const CSpreadsheetBorderDescription& desc)
{
	spreadsheetNotify->SetCellBorderLeft(row, col, desc);
	m_spreadsheetDesc.SetCellBorderLeft(row, col, desc);
}

void CBackendSpreadsheetObject::SetCellBorderRight(int row, int col, const CSpreadsheetBorderDescription& desc)
{
	spreadsheetNotify->SetCellBorderRight(row, col, desc);
	m_spreadsheetDesc.SetCellBorderRight(row, col, desc);
}

void CBackendSpreadsheetObject::SetCellBorderTop(int row, int col, const CSpreadsheetBorderDescription& desc)
{
	spreadsheetNotify->SetCellBorderTop(row, col, desc);
	m_spreadsheetDesc.SetCellBorderTop(row, col, desc);
}

void CBackendSpreadsheetObject::SetCellBorderBottom(int row, int col, const CSpreadsheetBorderDescription& desc)
{
	spreadsheetNotify->SetCellBorderBottom(row, col, desc);
	m_spreadsheetDesc.SetCellBorderBottom(row, col, desc);
}

void CBackendSpreadsheetObject::SetCellSize(int row, int col, int num_rows, int num_cols)
{
	spreadsheetNotify->SetCellSize(row, col, num_rows, num_cols);
	m_spreadsheetDesc.SetCellSize(row, col, num_rows, num_cols);
}

void CBackendSpreadsheetObject::SetCellFitMode(int row, int col, CSpreadsheetCellDescription::EFitMode fitMode)
{
	spreadsheetNotify->SetCellFitMode(row, col, fitMode);
	m_spreadsheetDesc.SetCellFitMode(row, col, fitMode);
}

void CBackendSpreadsheetObject::SetCellReadOnly(int row, int col, bool isReadOnly)
{
	spreadsheetNotify->SetCellReadOnly(row, col, isReadOnly);
	m_spreadsheetDesc.SetCellReadOnly(row, col, isReadOnly);
}

// ------ cell brake accessors
//
//support printing 
void CBackendSpreadsheetObject::AddRowBrake(int row)
{
	spreadsheetNotify->AddRowBrake(row);
	m_spreadsheetDesc.AddRowBrake(row);
}

void CBackendSpreadsheetObject::AddColBrake(int col)
{
	spreadsheetNotify->AddColBrake(col);
	m_spreadsheetDesc.AddColBrake(col);
}

void CBackendSpreadsheetObject::DeleteRowBrake(int row)
{
	spreadsheetNotify->DeleteRowBrake(row);
	m_spreadsheetDesc.DeleteRowBrake(row);
}

void CBackendSpreadsheetObject::DeleteColBrake(int col)
{
	spreadsheetNotify->DeleteColBrake(col);
	m_spreadsheetDesc.DeleteColBrake(col);
}

void CBackendSpreadsheetObject::SetRowBrake(int row)
{
	spreadsheetNotify->SetRowBrake(row);
	m_spreadsheetDesc.SetRowBrake(row);
}

void CBackendSpreadsheetObject::SetColBrake(int col)
{
	spreadsheetNotify->SetColBrake(col);
	m_spreadsheetDesc.SetColBrake(col);
}

// ------ cell value accessors
//

void CBackendSpreadsheetObject::SetCellFillType(int row, int col, enSpreadsheetFillType type)
{
	m_spreadsheetDesc.SetCellFillType(row, col, type);
}

void CBackendSpreadsheetObject::SetCellValue(int row, int col, const wxString& s)
{
	spreadsheetNotify->SetCellValue(row, col, s);
	m_spreadsheetDesc.SetCellValue(row, col, s);
}

bool CBackendSpreadsheetObject::GetParameter(const wxString& strParameter, CValue& valueParam) const
{
	auto iterator = std::find_if(m_paramVector.begin(), m_paramVector.end(),
		[strParameter](const auto& pair) { return stringUtils::CompareString(strParameter, pair.first); });

	if (iterator == m_paramVector.end())
		return false;

	valueParam = iterator->second;
	return true;
}

void CBackendSpreadsheetObject::SetParameter(const wxString& strParameter, const CValue& valueParam)
{
	m_paramVector.insert_or_assign(strParameter, valueParam);
}

wxString CBackendSpreadsheetObject::ComputeStringValueFromParameters(const wxString& strValue, enSpreadsheetFillType type) const
{
	if (type == enSpreadsheetFillType::enSpreadsheetFillType_StrTemplate) {

		if (!strValue.IsEmpty()) {

			wxString strTemplateValue;
			CBackendLocalization::GetTranslateGetRawLocText(m_docLangCode, strValue, strTemplateValue);

			size_t start_pos = 0, end_pos = 0;

			// Find the first opening or closing bracket
			start_pos = strTemplateValue.find_first_of(wxT("[]"), start_pos);

			while (start_pos != wxString::npos) {

				// Find the next bracket of any type
				end_pos = strTemplateValue.find_first_of(wxT("[]"), start_pos + 1);

				if (end_pos != wxString::npos) {
					// Extract the substring between the brackets
					// +1 to start after the opening bracket
					const wxString& token =
						strTemplateValue.substr(start_pos + 1, end_pos - start_pos - 1);
					if (!token.empty()) {
						strTemplateValue.replace(start_pos, end_pos - start_pos, GetParameter(token).GetString());
					}
					else {
						strTemplateValue.replace(start_pos, end_pos - start_pos, wxT(""));
					}

					// Move start_pos to the character after the closing bracket for the next iteration
					start_pos = end_pos + 1;
				}
				else {
					// No matching end bracket found, stop
					break;
				}

				// Find the next opening bracket for the next iteration
				start_pos = strTemplateValue.find_first_of(wxT("[]"), start_pos);
			}

			return CBackendLocalization::CreateLocalizationRawLocText(strTemplateValue);
		}
	}
	else if (type == enSpreadsheetFillType::enSpreadsheetFillType_StrParameter) {

		if (!strValue.IsEmpty())
			return CBackendLocalization::CreateLocalizationRawLocText(GetParameter(strValue).GetString());
	}

	return strValue;
}

#pragma endregion 

#pragma region __fs_h__

#include <fstream>

bool CBackendSpreadsheetObject::LoadFromFile(const wxString& strFileName)
{
	std::ifstream in(strFileName.ToStdWstring(), std::ios::in | std::ios::binary);

	if (!in.is_open())
		return false;

	//go to end
	in.seekg(0, in.end);
	//get size of file
	std::streamsize fsize = in.tellg();
	//go to beginning
	in.seekg(0, in.beg);

	wxMemoryBuffer tempBuffer(fsize);
	in.read((char*)tempBuffer.GetWriteBuf(fsize), fsize);

	CMemoryReader readerData(tempBuffer.GetData(), tempBuffer.GetBufSize());

	if (readerData.eof())
		return false;

	in.close();

	return CSpreadsheetDescriptionMemory::LoadData(readerData, m_spreadsheetDesc);
}

bool CBackendSpreadsheetObject::SaveToFile(const wxString& strFileName)
{
	//common data
	CMemoryWriter writterData;

	if (!CSpreadsheetDescriptionMemory::SaveData(writterData, m_spreadsheetDesc))
		return false;

	std::ofstream datafile;
	datafile.open(strFileName.ToStdWstring(), std::ios::binary);
	datafile.write(reinterpret_cast <char*> (writterData.pointer()), writterData.size());
	datafile.close();

	return true;
}

#pragma endregion 