#include "spreadsheetDescription.h"
#include "backend/fileSystem/fs.h"

#define cell_sign		0x0243565431

#define grid_block		0x10000

#define main_block		0x12000
#define cell_block		0x14000
#define area_block		0x16000
#define data_block		0x18000

#include "backend/typeconv.h"

bool CSpreadsheetDescriptionMemory::LoadData(CMemoryReader& reader, CSpreadsheetDescription& spreadsheetDesc)
{
	wxMemoryBuffer mainBuffer;
	if (!reader.r_chunk(grid_block, mainBuffer))
		return false;
	
	CMemoryReader mainReader(mainBuffer);

	wxMemoryBuffer headerBuffer;
	if (!mainReader.r_chunk(main_block, headerBuffer))
		return false;

	CMemoryReader headerReader(headerBuffer);
	if (headerReader.r_u64() != cell_sign)
		return false;

	wxMemoryBuffer cellBuffer;
	if (!mainReader.r_chunk(cell_block, cellBuffer))
		return false;

	CMemoryReader cellReader(cellBuffer);

	{
		const size_t capacity = cellReader.r_u64();
		spreadsheetDesc.ResetSpreadsheet(capacity);

		for (u64 c = 0; c < capacity; c++)
		{
			int row = cellReader.r_s32();
			int col = cellReader.r_s32();

			CSpreadsheetAttrDescription* cell =
				spreadsheetDesc.GetOrCreateCell(row, col);

			cell->m_value = cellReader.r_stringZ();

			cell->m_alignHorz = cellReader.r_s32();
			cell->m_alignVert = cellReader.r_s32();
			cell->m_textOrient = cellReader.r_s32();

			cell->m_font = typeConv::StringToFont(cellReader.r_stringZ());
			cell->m_backgroundColour = typeConv::StringToColour(cellReader.r_stringZ());
			cell->m_textColour = typeConv::StringToColour(cellReader.r_stringZ());

			cell->m_borderAt[0].m_style = static_cast<wxPenStyle>(cellReader.r_s32());
			cell->m_borderAt[0].m_width = cellReader.r_s32();
			cell->m_borderAt[0].m_colour = typeConv::StringToColour(cellReader.r_stringZ());

			cell->m_borderAt[1].m_style = static_cast<wxPenStyle>(cellReader.r_s32());
			cell->m_borderAt[1].m_width = cellReader.r_s32();
			cell->m_borderAt[1].m_colour = typeConv::StringToColour(cellReader.r_stringZ());

			cell->m_borderAt[2].m_style = static_cast<wxPenStyle>(cellReader.r_s32());
			cell->m_borderAt[2].m_width = cellReader.r_s32();
			cell->m_borderAt[2].m_colour = typeConv::StringToColour(cellReader.r_stringZ());

			cell->m_borderAt[3].m_style = static_cast<wxPenStyle>(cellReader.r_s32());
			cell->m_borderAt[3].m_width = cellReader.r_s32();
			cell->m_borderAt[3].m_colour = typeConv::StringToColour(cellReader.r_stringZ());

			cell->m_row_size = cellReader.r_s32();
			cell->m_col_size = cellReader.r_s32();

			cell->m_fitMode = static_cast<CSpreadsheetAttrDescription::EFitMode>(cellReader.r_s32());
			cell->m_isReadOnly = cellReader.r_u8();
		}
	}

	wxMemoryBuffer areaBuffer;
	if (!mainReader.r_chunk(area_block, areaBuffer))
		return false;

	CMemoryReader areaReader(areaBuffer);

	{
		const size_t capacity = areaReader.r_u64();
		for (u64 c = 0; c < capacity; c++) spreadsheetDesc.AddRowArea(areaReader.r_stringZ(), areaReader.r_s32(), areaReader.r_s32());
	}
	{
		const size_t capacity = areaReader.r_u64();
		for (u64 c = 0; c < capacity; c++) spreadsheetDesc.AddColArea(areaReader.r_stringZ(), areaReader.r_s32(), areaReader.r_s32());
	}

	wxMemoryBuffer dataBuffer;
	if (!mainReader.r_chunk(data_block, dataBuffer))
		return false;

	CMemoryReader dataReader(dataBuffer);

	{
		const size_t capacity = dataReader.r_u64();
		for (u64 c = 0; c < capacity; c++) spreadsheetDesc.AddRowBrake(dataReader.r_s32());
	}
	{
		const size_t capacity = dataReader.r_u64();
		for (u64 c = 0; c < capacity; c++) spreadsheetDesc.AddColBrake(dataReader.r_s32());
	}
	{
		const size_t capacity = dataReader.r_u64();
		for (u64 c = 0; c < capacity; c++) {
			int row = dataReader.r_s32();
			spreadsheetDesc.SetRowSize(row, dataReader.r_s32());
		}
	}
	{
		const size_t capacity = dataReader.r_u64();
		for (u64 c = 0; c < capacity; c++) {
			int col = dataReader.r_s32();
			spreadsheetDesc.SetColSize(col, dataReader.r_s32());
		}
	}

	spreadsheetDesc.SetFreezeRow(dataReader.r_s32());
	spreadsheetDesc.SetFreezeCol(dataReader.r_s32());
	return true;
}

bool CSpreadsheetDescriptionMemory::SaveData(CMemoryWriter& writer, CSpreadsheetDescription& spreadsheetDesc)
{
	CMemoryWriter mainWriter;

	CMemoryWriter headerWriter;
	headerWriter.w_u64(cell_sign); //sign
	headerWriter.w_u64(0); //reserved
	mainWriter.w_chunk(main_block, headerWriter.buffer());

	CMemoryWriter cellWriter;
	cellWriter.w_u64(spreadsheetDesc.GetCellCount());

	for (int idx = 0; idx < spreadsheetDesc.GetCellCount(); idx++)
	{
		const CSpreadsheetAttrDescription* cell = spreadsheetDesc.GetCellByIdx(idx);

		cellWriter.w_s32(cell->m_row);
		cellWriter.w_s32(cell->m_col);

		cellWriter.w_stringZ(cell->m_value);

		cellWriter.w_s32(cell->m_alignHorz);
		cellWriter.w_s32(cell->m_alignVert);
		cellWriter.w_s32(cell->m_textOrient);

		cellWriter.w_stringZ(typeConv::FontToString(cell->m_font));
		cellWriter.w_stringZ(typeConv::ColourToString(cell->m_backgroundColour));
		cellWriter.w_stringZ(typeConv::ColourToString(cell->m_textColour));

		cellWriter.w_s32(cell->m_borderAt[0].m_style);
		cellWriter.w_s32(cell->m_borderAt[0].m_width);
		cellWriter.w_stringZ(typeConv::ColourToString(cell->m_borderAt[0].m_colour));

		cellWriter.w_s32(cell->m_borderAt[1].m_style);
		cellWriter.w_s32(cell->m_borderAt[1].m_width);
		cellWriter.w_stringZ(typeConv::ColourToString(cell->m_borderAt[1].m_colour));

		cellWriter.w_s32(cell->m_borderAt[2].m_style);
		cellWriter.w_s32(cell->m_borderAt[2].m_width);
		cellWriter.w_stringZ(typeConv::ColourToString(cell->m_borderAt[2].m_colour));

		cellWriter.w_s32(cell->m_borderAt[3].m_style);
		cellWriter.w_s32(cell->m_borderAt[3].m_width);
		cellWriter.w_stringZ(typeConv::ColourToString(cell->m_borderAt[3].m_colour));

		cellWriter.w_s32(cell->m_row_size);
		cellWriter.w_s32(cell->m_col_size);

		cellWriter.w_s32(cell->m_fitMode);
		cellWriter.w_s8(cell->m_isReadOnly);
	}

	mainWriter.w_chunk(cell_block, cellWriter.buffer());

	CMemoryWriter areaWriter;

	areaWriter.w_u64(spreadsheetDesc.GetAreaNumberRows());

	for (int idx = 0; idx < spreadsheetDesc.GetAreaNumberRows(); idx++)
	{
		const CSpreadsheetAreaDescription* area = spreadsheetDesc.GetRowAreaByIdx(idx);

		areaWriter.w_stringZ(area->m_label);
		areaWriter.w_s32(area->m_start);
		areaWriter.w_s32(area->m_end);
	}

	areaWriter.w_u64(spreadsheetDesc.GetAreaNumberCols());

	for (int idx = 0; idx < spreadsheetDesc.GetAreaNumberCols(); idx++)
	{
		const CSpreadsheetAreaDescription* area = spreadsheetDesc.GetColAreaByIdx(idx);

		areaWriter.w_stringZ(area->m_label);
		areaWriter.w_s32(area->m_start);
		areaWriter.w_s32(area->m_end);
	}

	mainWriter.w_chunk(area_block, areaWriter.buffer());

	CMemoryWriter dataWriter;

	dataWriter.w_u64(spreadsheetDesc.GetBrakeNumberRows());
	for (int idx = 0; idx < spreadsheetDesc.GetBrakeNumberRows(); idx++) dataWriter.w_s32(spreadsheetDesc.GetRowBrakeByIdx(idx));

	dataWriter.w_u64(spreadsheetDesc.GetBrakeNumberCols());
	for (int idx = 0; idx < spreadsheetDesc.GetBrakeNumberCols(); idx++) dataWriter.w_s32(spreadsheetDesc.GetColBrakeByIdx(idx));

	dataWriter.w_u64(spreadsheetDesc.GetSizeNumberRows());
	for (int idx = 0; idx < spreadsheetDesc.GetSizeNumberRows(); idx++) {
		const CSpreadsheetRowSizeDescription* desc = spreadsheetDesc.GetRowSizeByIdx(idx);
		dataWriter.w_s32(desc->m_row);
		dataWriter.w_s32(desc->m_height);
	}

	dataWriter.w_u64(spreadsheetDesc.GetSizeNumberCols());
	for (int idx = 0; idx < spreadsheetDesc.GetSizeNumberCols(); idx++) {
		const CSpreadsheetColSizeDescription* desc = spreadsheetDesc.GetColSizeByIdx(idx);
		dataWriter.w_s32(desc->m_col);
		dataWriter.w_s32(desc->m_width);
	}

	dataWriter.w_s32(spreadsheetDesc.GetFreezeRow());
	dataWriter.w_s32(spreadsheetDesc.GetFreezeCol());

	mainWriter.w_chunk(data_block, dataWriter.buffer());

	writer.w_chunk(grid_block, mainWriter.buffer());
	return true;
}
