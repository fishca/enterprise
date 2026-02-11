#include "backend_spreadsheet.h"
#include "backend/fileSystem/fs.h"

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