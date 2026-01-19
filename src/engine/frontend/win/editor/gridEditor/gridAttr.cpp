#include "gridEditor.h"
#include "frontend/visualView/ctrl/frame.h"

void CGridEditor::CGridEditorDrawHelper::GetTextBoxSize(const wxDC& dc,
	const wxArrayString& lines,
	wxArrayInt& arrWidth, wxArrayInt& arrHeight,
	long* width, long* height)
{
	wxCoord w = 0;
	wxCoord h = 0;
	wxCoord lineW = 0, lineH = 0;

	arrWidth.reserve(lines.GetCount());
	arrHeight.reserve(lines.GetCount());

	size_t i;
	for (i = 0; i < lines.GetCount(); i++)
	{
		if (lines[i].empty()) {

			// GetTextExtent() would return 0 for empty lines, but we still
			// need to account for their height.
			h += dc.GetCharHeight();

			arrWidth.Add(0);
			arrHeight.Add(dc.GetCharHeight());
		}
		else
		{
			dc.GetTextExtent(lines[i], &lineW, &lineH);

			w = wxMax(w, lineW);
			h += lineH;

			arrWidth.Add(lineW);
			arrHeight.Add(lineH);
		}
	}

	*width = w;
	*height = h;
}

wxArrayString CGridEditor::CGridEditorDrawHelper::GetTextLines(wxDC& dc, const wxString& data, const wxFont& font, const wxRect& rect)
{
	wxArrayString lines;

	wxCoord x = 0, y = 0, curr_x = 0;
	wxCoord max_x = rect.GetWidth();

	dc.SetFont(font);

	//wxStringTokenizer tk(data, _T(" \n\t\r"));
	//wxString thisline = wxEmptyString;

	//while (tk.HasMoreTokens())
	//{
	//	wxString tok = tk.GetNextToken();
	//	//FIXME: this causes us to print an extra unnecesary
	//	//       space at the end of the line. But it
	//	//       is invisible , simplifies the size calculation
	//	//       and ensures tokens are separated in the display
	//	tok += _T(" ");

	//	dc.GetTextExtent(tok, &x, &y);
	//	if (curr_x + x > max_x)
	//	{
	//		lines.Add(wxString(thisline));
	//		thisline = tok;
	//		curr_x = x;
	//	}
	//	else
	//	{
	//		thisline += tok;
	//		curr_x += x;
	//	}
	//}
	//
	////Add last line
	//lines.Add(wxString(thisline));

	lines.Add(data);
	return lines;
}

void CGridEditor::CGridEditorDrawHelper::DrawTextRectangle(wxDC& dc,
	const wxArrayString& lines,
	const wxRect& rect,
	int horizAlign,
	int vertAlign,
	int textOrientation)
{
	if (lines.empty())
		return;

	wxDCClipper clip(dc, rect);

	long textWidth = 0,
		textHeight = 0;

	static wxArrayInt arrWidth, arrHeight;
	{
		arrWidth.resize(0); arrHeight.resize(0);
	}

	if (textOrientation == wxHORIZONTAL)
		GetTextBoxSize(dc, lines, arrWidth, arrHeight, &textWidth, &textHeight);
	else
		GetTextBoxSize(dc, lines, arrWidth, arrHeight, &textHeight, &textWidth);

	int x = 0,
		y = 0;

	switch (vertAlign)
	{
	case wxALIGN_BOTTOM:
		if (textOrientation == wxHORIZONTAL)
			y = rect.y + (rect.height - textHeight - GRID_TEXT_MARGIN);
		else
			x = rect.x + (rect.width - textWidth - GRID_TEXT_MARGIN);
		break;

	case wxALIGN_CENTRE:
		if (textOrientation == wxHORIZONTAL)
			y = rect.y + ((rect.height - textHeight) / 2);
		else
			x = rect.x + ((rect.width - textWidth) / 2);
		break;

	case wxALIGN_TOP:
	default:
		if (textOrientation == wxHORIZONTAL)
			y = rect.y + GRID_TEXT_MARGIN;
		else
			x = rect.x + GRID_TEXT_MARGIN;
		break;
	}

	// Align each line of a multi-line label
	size_t nLines = lines.GetCount();
	for (size_t l = 0; l < nLines; l++)
	{
		const wxString& line = lines[l];

		if (line.empty())
		{
			*(textOrientation == wxHORIZONTAL ? &y : &x) += dc.GetCharHeight();
			continue;
		}

		wxCoord lineWidth = arrWidth[l],
			lineHeight = arrHeight[l];

		switch (horizAlign)
		{
		case wxALIGN_RIGHT:
			if (textOrientation == wxHORIZONTAL)
				x = rect.x + (rect.width - lineWidth - GRID_TEXT_MARGIN);
			else
				y = rect.y + lineWidth + GRID_TEXT_MARGIN;
			break;

		case wxALIGN_CENTRE:
			if (textOrientation == wxHORIZONTAL)
				x = rect.x + ((rect.width - lineWidth) / 2);
			else
				y = rect.y + rect.height - ((rect.height - lineWidth) / 2);
			break;

		case wxALIGN_LEFT:
		default:
			if (textOrientation == wxHORIZONTAL)
				x = rect.x + GRID_TEXT_MARGIN;
			else
				y = rect.y + rect.height - GRID_TEXT_MARGIN;
			break;
		}

		if (textOrientation == wxHORIZONTAL)
		{
			dc.DrawText(line, x, y);
			y += lineHeight;
		}
		else
		{
			dc.DrawRotatedText(line, x, y, 90.0);
			x += lineHeight;
		}
	}
}