#include "backend_spreadsheet.h"
#include "backend/fileSystem/fs.h"

#pragma region __notifier_h__

void CBackendSpreadsheetObject::ResetSpreadsheet(int count)
{
}

//size 
void CBackendSpreadsheetObject::SetRowSize(int row, int height)
{
}

void CBackendSpreadsheetObject::SetColSize(int col, int width)
{
}

//freeze 
void CBackendSpreadsheetObject::SetFreezeRow(int row)
{
}

void CBackendSpreadsheetObject::SetFreezeCol(int col)
{
}

//area 
void CBackendSpreadsheetObject::AddRowArea(const wxString& strAreaName, unsigned int start, unsigned int end)
{
}

void CBackendSpreadsheetObject::DeleteRowArea(const wxString& strAreaName)
{
}

void CBackendSpreadsheetObject::AddColArea(const wxString& strAreaName, unsigned int start, unsigned int end)
{
}

void CBackendSpreadsheetObject::DeleteColArea(const wxString& strAreaName)
{
}

void CBackendSpreadsheetObject::SetRowSizeArea(const wxString& strAreaName, int start, int end)
{
}

void CBackendSpreadsheetObject::SetRowNameArea(size_t idx, const wxString& strAreaName)
{
}

void CBackendSpreadsheetObject::SetColSizeArea(const wxString& strAreaName, int start, int end)
{
}

void CBackendSpreadsheetObject::SetColNameArea(size_t idx, const wxString& strAreaName)
{
}

// ------ row and col formatting
//

void CBackendSpreadsheetObject::SetCellBackgroundColour(int row, int col, const wxColour& colour)
{
}

void CBackendSpreadsheetObject::SetCellTextColour(int row, int col, const wxColour& colour)
{
}

void CBackendSpreadsheetObject::SetCellTextOrient(int row, int col, const int orient)
{
}

void CBackendSpreadsheetObject::SetCellFont(int row, int col, const wxFont& font)
{
}

void CBackendSpreadsheetObject::SetCellAlignment(int row, int col, const int horiz, const int vert)
{
}

void CBackendSpreadsheetObject::SetCellBorderLeft(int row, int col, const CSpreadsheetBorderDescription& desc)
{
}

void CBackendSpreadsheetObject::SetCellBorderRight(int row, int col, const CSpreadsheetBorderDescription& desc)
{
}

void CBackendSpreadsheetObject::SetCellBorderTop(int row, int col, const CSpreadsheetBorderDescription& desc)
{
}

void CBackendSpreadsheetObject::SetCellBorderBottom(int row, int col, const CSpreadsheetBorderDescription& desc)
{
}

void CBackendSpreadsheetObject::SetCellSize(int row, int col, int num_rows, int num_cols)
{
	for (auto notify : m_spreadsheetNotifiers)
		notify->SetCellSize(row, col, num_rows, num_cols);

	m_spreadsheetDesc.SetCellSize(row, col, num_rows, num_cols);
}

void CBackendSpreadsheetObject::SetCellFitMode(int row, int col, CSpreadsheetAttrDescription::EFitMode fitMode)
{
	for (auto notify : m_spreadsheetNotifiers)
		notify->SetCellFitMode(row, col, fitMode);

	m_spreadsheetDesc.SetCellFitMode(row, col, fitMode);
}

void CBackendSpreadsheetObject::SetCellReadOnly(int row, int col, bool isReadOnly)
{
	for (auto notify : m_spreadsheetNotifiers)
		notify->SetCellReadOnly(row, col, isReadOnly);

	m_spreadsheetDesc.SetCellReadOnly(row, col, isReadOnly);
}

// ------ cell brake accessors
//
//support printing 
void CBackendSpreadsheetObject::AddRowBrake(int row)
{
	for (auto notify : m_spreadsheetNotifiers)
		notify->AddRowBrake(row);

	m_spreadsheetDesc.AddRowBrake(row);
}

void CBackendSpreadsheetObject::AddColBrake(int col)
{
	for (auto notify : m_spreadsheetNotifiers)
		notify->AddColBrake(col);

	m_spreadsheetDesc.AddColBrake(col);
}

void CBackendSpreadsheetObject::SetRowBrake(int row)
{
	for (auto notify : m_spreadsheetNotifiers)
		notify->SetRowBrake(row);

	m_spreadsheetDesc.SetRowBrake(row);
}

void CBackendSpreadsheetObject::SetColBrake(int col)
{
	for (auto notify : m_spreadsheetNotifiers)
		notify->SetColBrake(col);

	m_spreadsheetDesc.SetColBrake(col);
}

// ------ cell value accessors
//
void CBackendSpreadsheetObject::SetCellValue(int row, int col, const wxString& s)
{
	for (auto notify : m_spreadsheetNotifiers)
		notify->SetCellValue(row, col, s);

	m_spreadsheetDesc.SetCellValue(row, col, s);
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