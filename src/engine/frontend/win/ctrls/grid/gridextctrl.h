///////////////////////////////////////////////////////////////////////////
// Name:        wx/generic/gridctrl.h
// Purpose:     wxGridExt controls
// Author:      Paul Gammans, Roger Gammans
// Modified by:
// Created:     11/04/2001
// Copyright:   (c) The Computer Surgery (paul@compsurg.co.uk)
// Licence:     wxWindows licence
/////////////////////////////////////////////////////////////////////////////

#ifndef _WX_GENERIC_GRIDCTRL_H_
#define _WX_GENERIC_GRIDCTRL_H_

#include "gridext.h"

#if wxUSE_GRID

#define wxGRID_VALUE_CHOICEINT    wxT("choiceint")
#define wxGRID_VALUE_DATETIME     wxT("datetime")


	// the default renderer for the cells containing string data
class wxGridExtCellStringRenderer : public wxGridExtCellRenderer
{
public:
	wxGridExtCellStringRenderer()
		: wxGridExtCellRenderer()
	{
	}

	wxGridExtCellStringRenderer(const wxGridExtCellStringRenderer& other)
		: wxGridExtCellRenderer(other)
	{
	}

	// draw the string
	virtual void Draw(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		const wxRect& rect,
		int row, int col,
		bool isSelected) wxOVERRIDE;

	// return the string extent
	virtual wxSize GetBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col) wxOVERRIDE;

	virtual wxGridExtCellRenderer* Clone() const wxOVERRIDE
	{
		return new wxGridExtCellStringRenderer(*this);
	}

protected:
	// calc the string extent for given string/font
	wxSize DoGetBestSize(const wxGridExtCellAttr& attr,
		wxDC& dc,
		const wxString& text);
};

// the default renderer for the cells containing numeric (long) data
class wxGridExtCellNumberRenderer : public wxGridExtCellStringRenderer
{
public:
	explicit wxGridExtCellNumberRenderer(long minValue = LONG_MIN,
		long maxValue = LONG_MAX)
		: wxGridExtCellStringRenderer(),
		m_minValue(minValue),
		m_maxValue(maxValue)
	{
	}

	wxGridExtCellNumberRenderer(const wxGridExtCellNumberRenderer& other)
		: wxGridExtCellStringRenderer(other),
		m_minValue(other.m_minValue),
		m_maxValue(other.m_maxValue)
	{
	}

	// draw the string right aligned
	virtual void Draw(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		const wxRect& rect,
		int row, int col,
		bool isSelected) wxOVERRIDE;

	virtual wxSize GetBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col) wxOVERRIDE;

	virtual wxSize GetMaxBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc) wxOVERRIDE;

	// Optional parameters for this renderer are "<min>,<max>".
	virtual void SetParameters(const wxString& params) wxOVERRIDE;

	virtual wxGridExtCellRenderer* Clone() const wxOVERRIDE
	{
		return new wxGridExtCellNumberRenderer(*this);
	}

protected:
	wxString GetString(const wxGridExt& grid, int row, int col);

	long m_minValue,
		m_maxValue;
};

class wxGridExtCellFloatRenderer : public wxGridExtCellStringRenderer
{
public:
	wxGridExtCellFloatRenderer(int width = -1,
		int precision = -1,
		int format = 16 /*wxGRID_FLOAT_FORMAT_DEFAULT*/);

	wxGridExtCellFloatRenderer(const wxGridExtCellFloatRenderer& other)
		: wxGridExtCellStringRenderer(other),
		m_width(other.m_width),
		m_precision(other.m_precision),
		m_style(other.m_style),
		m_format(other.m_format)
	{
	}

	// get/change formatting parameters
	int GetWidth() const { return m_width; }
	void SetWidth(int width) { m_width = width; m_format.clear(); }
	int GetPrecision() const { return m_precision; }
	void SetPrecision(int precision) { m_precision = precision; m_format.clear(); }
	int GetFormat() const { return m_style; }
	void SetFormat(int format) { m_style = format; m_format.clear(); }

	// draw the string right aligned with given width/precision
	virtual void Draw(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		const wxRect& rect,
		int row, int col,
		bool isSelected) wxOVERRIDE;

	virtual wxSize GetBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col) wxOVERRIDE;

	// parameters string format is "width[,precision[,format]]"
	// with format being one of f|e|g|E|F|G
	virtual void SetParameters(const wxString& params) wxOVERRIDE;

	virtual wxGridExtCellRenderer* Clone() const wxOVERRIDE
	{
		return new wxGridExtCellFloatRenderer(*this);
	}

protected:
	wxString GetString(const wxGridExt& grid, int row, int col);

private:
	// formatting parameters
	int m_width,
		m_precision;

	int m_style;
	wxString m_format;
};

// renderer for boolean fields
class wxGridExtCellBoolRenderer : public wxGridExtCellRenderer
{
public:
	wxGridExtCellBoolRenderer()
		: wxGridExtCellRenderer()
	{
	}

	wxGridExtCellBoolRenderer(const wxGridExtCellBoolRenderer& other)
		: wxGridExtCellRenderer(other)
	{
	}

	// draw a check mark or nothing
	virtual void Draw(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		const wxRect& rect,
		int row, int col,
		bool isSelected) wxOVERRIDE;

	// return the checkmark size
	virtual wxSize GetBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col) wxOVERRIDE;

	virtual wxSize GetMaxBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc) wxOVERRIDE;

	virtual wxGridExtCellRenderer* Clone() const wxOVERRIDE
	{
		return new wxGridExtCellBoolRenderer(*this);
	}
};


#if wxUSE_DATETIME

#include "wx/datetime.h"

namespace wxGridExtPrivate { class DateParseParams; }

// renderer for the cells containing dates only, without time component
class wxGridExtCellDateRenderer : public wxGridExtCellStringRenderer
{
public:
	explicit wxGridExtCellDateRenderer(const wxString& outformat = wxString());

	wxGridExtCellDateRenderer(const wxGridExtCellDateRenderer& other)
		: wxGridExtCellStringRenderer(other),
		m_oformat(other.m_oformat),
		m_tz(other.m_tz)
	{
	}

	// draw the string right aligned
	virtual void Draw(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		const wxRect& rect,
		int row, int col,
		bool isSelected) wxOVERRIDE;

	virtual wxSize GetBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col) wxOVERRIDE;

	virtual wxSize GetMaxBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc) wxOVERRIDE;

	virtual wxGridExtCellRenderer* Clone() const wxOVERRIDE
	{
		return new wxGridExtCellDateRenderer(*this);
	}

	// output strptime()-like format string
	virtual void SetParameters(const wxString& params) wxOVERRIDE;

protected:
	wxString GetString(const wxGridExt& grid, int row, int col);

	// This is overridden in wxGridExtCellDateTimeRenderer which uses a separate
	// input format and forbids fallback to ParseDate().
	virtual void
		GetDateParseParams(wxGridExtPrivate::DateParseParams& params) const;

	wxString m_oformat;
	wxDateTime::TimeZone m_tz;
};

// the default renderer for the cells containing times and dates
class wxGridExtCellDateTimeRenderer : public wxGridExtCellDateRenderer
{
public:
	wxGridExtCellDateTimeRenderer(const wxString& outformat = wxASCII_STR(wxDefaultDateTimeFormat),
		const wxString& informat = wxASCII_STR(wxDefaultDateTimeFormat));

	wxGridExtCellDateTimeRenderer(const wxGridExtCellDateTimeRenderer& other)
		: wxGridExtCellDateRenderer(other),
		m_iformat(other.m_iformat)
	{
	}

	virtual wxGridExtCellRenderer* Clone() const wxOVERRIDE
	{
		return new wxGridExtCellDateTimeRenderer(*this);
	}

protected:
	virtual void
		GetDateParseParams(wxGridExtPrivate::DateParseParams& params) const wxOVERRIDE;

	wxString m_iformat;
};

#endif // wxUSE_DATETIME

// Renderer for fields taking one of a limited set of values: this is the same
// as the renderer for strings, except that it can implement GetMaxBestSize().
class wxGridExtCellChoiceRenderer : public wxGridExtCellStringRenderer
{
public:
	explicit wxGridExtCellChoiceRenderer(const wxString& choices = wxString());

	wxGridExtCellChoiceRenderer(const wxGridExtCellChoiceRenderer& other);

	virtual wxSize GetMaxBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc) wxOVERRIDE;

	// Parameters string is a comma-separated list of values.
	virtual void SetParameters(const wxString& params) wxOVERRIDE;

	virtual wxGridExtCellRenderer* Clone() const wxOVERRIDE
	{
		return new wxGridExtCellChoiceRenderer(*this);
	}

protected:

	wxArrayString m_choices;
};


// renders a number using the corresponding text string
class wxGridExtCellEnumRenderer : public wxGridExtCellChoiceRenderer
{
public:
	explicit wxGridExtCellEnumRenderer(const wxString& choices = wxString())
		: wxGridExtCellChoiceRenderer(choices)
	{
	}

	wxGridExtCellEnumRenderer(const wxGridExtCellEnumRenderer& other)
		: wxGridExtCellChoiceRenderer(other)
	{
	}

	// draw the string right aligned
	virtual void Draw(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		const wxRect& rect,
		int row, int col,
		bool isSelected) wxOVERRIDE;

	virtual wxSize GetBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col) wxOVERRIDE;

	virtual wxGridExtCellRenderer* Clone() const wxOVERRIDE
	{
		return new wxGridExtCellEnumRenderer(*this);
	}

protected:
	wxString GetString(const wxGridExt& grid, int row, int col);
};


class wxGridExtCellAutoWrapStringRenderer : public wxGridExtCellStringRenderer
{
public:
	wxGridExtCellAutoWrapStringRenderer()
		: wxGridExtCellStringRenderer()
	{
	}

	wxGridExtCellAutoWrapStringRenderer(const wxGridExtCellAutoWrapStringRenderer& other)
		: wxGridExtCellStringRenderer(other)
	{
	}

	virtual void Draw(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		const wxRect& rect,
		int row, int col,
		bool isSelected) wxOVERRIDE;

	virtual wxSize GetBestSize(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col) wxOVERRIDE;

	virtual int GetBestHeight(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col,
		int width) wxOVERRIDE;

	virtual int GetBestWidth(wxGridExt& grid,
		wxGridExtCellAttr& attr,
		wxDC& dc,
		int row, int col,
		int height) wxOVERRIDE;

	virtual wxGridExtCellRenderer* Clone() const wxOVERRIDE
	{
		return new wxGridExtCellAutoWrapStringRenderer(*this);
	}

private:
	wxArrayString GetTextLines(wxGridExt& grid,
		wxDC& dc,
		const wxGridExtCellAttr& attr,
		const wxRect& rect,
		int row, int col);

	// Helper methods of GetTextLines()

	// Break a single logical line of text into several physical lines, all of
	// which are added to the lines array. The lines are broken at maxWidth and
	// the dc is used for measuring text extent only.
	void BreakLine(wxDC& dc,
		const wxString& logicalLine,
		wxCoord maxWidth,
		wxArrayString& lines);

	// Break a word, which is supposed to be wider than maxWidth, into several
	// lines, which are added to lines array and the last, incomplete, of which
	// is returned in line output parameter.
	//
	// Returns the width of the last line.
	wxCoord BreakWord(wxDC& dc,
		const wxString& word,
		wxCoord maxWidth,
		wxArrayString& lines,
		wxString& line);
};

#endif  // wxUSE_GRID
#endif // _WX_GENERIC_GRIDCTRL_H_
