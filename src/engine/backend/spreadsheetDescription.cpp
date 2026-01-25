#include "spreadsheetDescription.h"
#include "backend/fileSystem/fs.h"

#define cell_sign		0x0243565431

#define main_block		0x12000
#define cell_block		0x14000
#define area_block		0x16000
#define data_block		0x18000

#include "backend/typeconv.h"

bool CSpreadsheetDescriptionMemory::LoadData(CMemoryReader& reader, CSpreadsheetDescription& spreadsheetDesc)
{
	wxMemoryBuffer headerBuffer;
	if (!reader.r_chunk(main_block, headerBuffer))
		return false;

	CMemoryReader headerReader(headerBuffer);
	if (headerReader.r_u64() != cell_sign)
		return false;

	wxMemoryBuffer cellBuffer;
	if (!reader.r_chunk(cell_block, cellBuffer))
		return false;

	CMemoryReader cellReader(cellBuffer);
	spreadsheetDesc.m_cellAt.reserve(cellReader.r_u64());
	for (u64 c = 0; c < spreadsheetDesc.m_cellAt.capacity(); c++)
	{
		CSpreadsheetAttrChunk& cell = spreadsheetDesc.m_cellAt.emplace_back();

		cell.m_row = cellReader.r_s32();
		cell.m_col = cellReader.r_s32();

		cell.m_strCellValue = cellReader.r_stringZ();

		cell.m_cellAlignHorz = cellReader.r_s32();
		cell.m_cellAlignVert = cellReader.r_s32();
		cell.m_textOrient = cellReader.r_s32();

		cell.m_cellFont = typeConv::StringToFont(cellReader.r_stringZ());
		cell.m_cellBackgroundColour = typeConv::StringToColour(cellReader.r_stringZ());
		cell.m_cellTextColour = typeConv::StringToColour(cellReader.r_stringZ());

		cell.m_borderAt[0].m_borderStyle = static_cast<wxPenStyle>(cellReader.r_s32());
		cell.m_borderAt[0].m_borderWidth = cellReader.r_s32();
		cell.m_borderAt[0].m_borderColour = typeConv::StringToColour(cellReader.r_stringZ());

		cell.m_borderAt[1].m_borderStyle = static_cast<wxPenStyle>(cellReader.r_s32());
		cell.m_borderAt[1].m_borderWidth = cellReader.r_s32();
		cell.m_borderAt[1].m_borderColour = typeConv::StringToColour(cellReader.r_stringZ());

		cell.m_borderAt[2].m_borderStyle = static_cast<wxPenStyle>(cellReader.r_s32());
		cell.m_borderAt[2].m_borderWidth = cellReader.r_s32();
		cell.m_borderAt[2].m_borderColour = typeConv::StringToColour(cellReader.r_stringZ());

		cell.m_borderAt[3].m_borderStyle = static_cast<wxPenStyle>(cellReader.r_s32());
		cell.m_borderAt[3].m_borderWidth = cellReader.r_s32();
		cell.m_borderAt[3].m_borderColour = typeConv::StringToColour(cellReader.r_stringZ());

		cell.m_row_size = cellReader.r_s32();
		cell.m_col_size = cellReader.r_s32();
	}

	wxMemoryBuffer areaBuffer;
	if (!reader.r_chunk(area_block, areaBuffer))
		return false;

	CMemoryReader areaReader(areaBuffer);
	spreadsheetDesc.m_rowAreaAt.reserve(areaReader.r_u64());
	for (u64 c = 0; c < spreadsheetDesc.m_rowAreaAt.capacity(); c++) {
		CSpreadsheetAreaChunk& area = spreadsheetDesc.m_rowAreaAt.emplace_back();
		area.m_strAreaName = areaReader.r_stringZ();
		area.m_start = areaReader.r_s32();
		area.m_end = areaReader.r_s32();
	}
	spreadsheetDesc.m_colAreaAt.reserve(areaReader.r_u64());
	for (u64 c = 0; c < spreadsheetDesc.m_colAreaAt.capacity(); c++) {
		CSpreadsheetAreaChunk& area = spreadsheetDesc.m_colAreaAt.emplace_back();
		area.m_strAreaName = areaReader.r_stringZ();
		area.m_start = areaReader.r_s32();
		area.m_end = areaReader.r_s32();
	}

	wxMemoryBuffer dataBuffer;
	if (!reader.r_chunk(data_block, dataBuffer))
		return false;

	CMemoryReader dataReader(dataBuffer);
	spreadsheetDesc.m_rowBrakeAt.reserve(dataReader.r_u64());
	for (u64 c = 0; c < spreadsheetDesc.m_rowBrakeAt.capacity(); c++)
		spreadsheetDesc.m_rowBrakeAt.push_back(dataReader.r_s32());
	spreadsheetDesc.m_colBrakeAt.reserve(dataReader.r_u64());
	for (u64 c = 0; c < spreadsheetDesc.m_colBrakeAt.capacity(); c++)
		spreadsheetDesc.m_colBrakeAt.push_back(dataReader.r_s32());

	spreadsheetDesc.m_rowHeightAt.reserve(dataReader.r_u64());
	for (u64 c = 0; c < spreadsheetDesc.m_rowHeightAt.capacity(); c++)
	{
		CSpreadsheetRowSizeChunk& row = spreadsheetDesc.m_rowHeightAt.emplace_back();
		row.m_row = dataReader.r_s32();
		row.m_height = dataReader.r_s32();
	}
	spreadsheetDesc.m_colWidthAt.reserve(dataReader.r_u64());
	for (u64 c = 0; c < spreadsheetDesc.m_colWidthAt.capacity(); c++)
	{
		CSpreadsheetColSizeChunk& col = spreadsheetDesc.m_colWidthAt.emplace_back();
		col.m_col = dataReader.r_s32();
		col.m_width = dataReader.r_s32();
	}

	spreadsheetDesc.m_freezeRow = dataReader.r_s32();
	spreadsheetDesc.m_freezeCol = dataReader.r_s32();
	return true;
}

bool CSpreadsheetDescriptionMemory::SaveData(CMemoryWriter& writer, CSpreadsheetDescription& spreadsheetDesc)
{
	CMemoryWriter headerWritter;
	headerWritter.w_u64(cell_sign); //sign
	headerWritter.w_u64(0); //reserved
	writer.w_chunk(main_block, headerWritter.buffer());

	CMemoryWriter cellWritter;
	cellWritter.w_u64(spreadsheetDesc.m_cellAt.size());
	for (const auto& c : spreadsheetDesc.m_cellAt)
	{
		cellWritter.w_s32(c.m_row);
		cellWritter.w_s32(c.m_col);

		cellWritter.w_stringZ(c.m_strCellValue);

		cellWritter.w_s32(c.m_cellAlignHorz);
		cellWritter.w_s32(c.m_cellAlignVert);
		cellWritter.w_s32(c.m_textOrient);

		cellWritter.w_stringZ(typeConv::FontToString(c.m_cellFont));
		cellWritter.w_stringZ(typeConv::ColourToString(c.m_cellBackgroundColour));
		cellWritter.w_stringZ(typeConv::ColourToString(c.m_cellTextColour));

		cellWritter.w_s32(c.m_borderAt[0].m_borderStyle);
		cellWritter.w_s32(c.m_borderAt[0].m_borderWidth);
		cellWritter.w_stringZ(typeConv::ColourToString(c.m_borderAt[0].m_borderColour));

		cellWritter.w_s32(c.m_borderAt[1].m_borderStyle);
		cellWritter.w_s32(c.m_borderAt[1].m_borderWidth);
		cellWritter.w_stringZ(typeConv::ColourToString(c.m_borderAt[1].m_borderColour));

		cellWritter.w_s32(c.m_borderAt[2].m_borderStyle);
		cellWritter.w_s32(c.m_borderAt[2].m_borderWidth);
		cellWritter.w_stringZ(typeConv::ColourToString(c.m_borderAt[2].m_borderColour));

		cellWritter.w_s32(c.m_borderAt[3].m_borderStyle);
		cellWritter.w_s32(c.m_borderAt[3].m_borderWidth);
		cellWritter.w_stringZ(typeConv::ColourToString(c.m_borderAt[3].m_borderColour));

		cellWritter.w_s32(c.m_row_size);
		cellWritter.w_s32(c.m_col_size);
	}
	writer.w_chunk(cell_block, cellWritter.buffer());

	CMemoryWriter areaWritter;
	areaWritter.w_u64(spreadsheetDesc.m_rowAreaAt.size());
	for (const auto& c : spreadsheetDesc.m_rowAreaAt)
	{
		areaWritter.w_stringZ(c.m_strAreaName);
		areaWritter.w_s32(c.m_start);
		areaWritter.w_s32(c.m_end);
	}
	areaWritter.w_u64(spreadsheetDesc.m_colAreaAt.size());
	for (const auto& c : spreadsheetDesc.m_colAreaAt)
	{
		areaWritter.w_stringZ(c.m_strAreaName);
		areaWritter.w_s32(c.m_start);
		areaWritter.w_s32(c.m_end);
	}
	writer.w_chunk(area_block, areaWritter.buffer());

	CMemoryWriter dataWritter;

	dataWritter.w_u64(spreadsheetDesc.m_rowBrakeAt.size());
	for (const auto& c : spreadsheetDesc.m_rowBrakeAt) dataWritter.w_s32(c);

	dataWritter.w_u64(spreadsheetDesc.m_colBrakeAt.size());
	for (const auto& c : spreadsheetDesc.m_colBrakeAt) dataWritter.w_s32(c);

	dataWritter.w_u64(spreadsheetDesc.m_rowHeightAt.size());
	for (const auto& c : spreadsheetDesc.m_rowHeightAt)
	{
		dataWritter.w_s32(c.m_row);
		dataWritter.w_s32(c.m_height);
	}

	dataWritter.w_u64(spreadsheetDesc.m_colWidthAt.size());
	for (const auto& c : spreadsheetDesc.m_colWidthAt)
	{
		dataWritter.w_s32(c.m_col);
		dataWritter.w_s32(c.m_width);
	}

	dataWritter.w_s32(spreadsheetDesc.m_freezeRow);
	dataWritter.w_s32(spreadsheetDesc.m_freezeCol);
	writer.w_chunk(data_block, dataWritter.buffer());

	return true;
}
