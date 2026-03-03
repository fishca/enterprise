

#include <wx/wx.h>
#include <wx/fontdlg.h>
#include <wx/colordlg.h>

#include "fontcolorsettingspanel.h"
#include "staticTextEx.h"

BEGIN_EVENT_TABLE(CFontColorSettingsPanel, wxPanel)
EVT_BUTTON(CFontColorSettingsPanel::ID_SelectFont, CFontColorSettingsPanel::OnSelectFont)
EVT_BUTTON(CFontColorSettingsPanel::ID_ResetToDefaults, CFontColorSettingsPanel::OnResetToDefaults)
EVT_LISTBOX(CFontColorSettingsPanel::ID_DisplayItemsList, CFontColorSettingsPanel::OnDisplayItemsListItemSelected)
EVT_CHOICE(CFontColorSettingsPanel::ID_ForeColorChoice, CFontColorSettingsPanel::OnForeColorChoiceItemSelected)
EVT_CHOICE(CFontColorSettingsPanel::ID_BackColorChoice, CFontColorSettingsPanel::OnBackColorChoiceItemSelected)
EVT_BUTTON(CFontColorSettingsPanel::ID_ForeColorCustom, CFontColorSettingsPanel::OnForeColorCustom)
EVT_BUTTON(CFontColorSettingsPanel::ID_BackColorCustom, CFontColorSettingsPanel::OnBackColorCustom)
EVT_CHECKBOX(CFontColorSettingsPanel::ID_BoldCheck, CFontColorSettingsPanel::OnBoldCheck)
EVT_CHECKBOX(CFontColorSettingsPanel::ID_ItalicCheck, CFontColorSettingsPanel::OnItalicCheck)
END_EVENT_TABLE()

const wxString CFontColorSettingsPanel::s_defaultColorName[] =
{
	_("Black"),
	_("White"),
	_("Maroon"),
	_("Dark green"),
	_("Olive"),
	_("Dark blue"),
	_("Purple"),
	_("Aquamarine"),
	_("Light gray"),
	_("Dark gray"),
	_("Red"),
	_("Green"),
	_("Yellow"),
	_("Blue"),
	_("Magenta"),
	_("Cyan")
};

const wxColour CFontColorSettingsPanel::s_defaultColor[] =
{
	wxColour(0x00, 0x00, 0x00),     // Black
	wxColour(0xFF, 0xFF, 0xFF),     // White
	wxColour(0x80, 0x00, 0x00),     // Maroon
	wxColour(0x00, 0x80, 0x00),     // Dark green
	wxColour(0x80, 0x80, 0x00),     // Olive
	wxColour(0x00, 0x00, 0x80),     // Dark blue
	wxColour(0x80, 0x00, 0x80),     // Purple
	wxColour(0x00, 0x80, 0x80),     // Aquamarine
	wxColour(0xC0, 0xC0, 0xC0),     // Light grey
	wxColour(0x80, 0x80, 0x80),     // Dark grey
	wxColour(0xFF, 0x00, 0x00),     // Red
	wxColour(0x00, 0xFF, 0x00),     // Green
	wxColour(0xFF, 0xFF, 0x00),     // Yellow
	wxColour(0x00, 0x00, 0xFF),     // Blue
	wxColour(0xFF, 0x00, 0xFF),     // Magenta
	wxColour(0x00, 0xFF, 0xFF),     // Cyan
};

CFontColorSettingsPanel::CFontColorSettingsPanel(wxWindow* parent, int id, wxPoint pos, wxSize size, int style) : wxPanel(parent, id, pos, size, style)
{

	wxFlexGridSizer* fgSizer6;
	fgSizer6 = new wxFlexGridSizer(4, 1, 0, 0);
	fgSizer6->AddGrowableCol(0);
	fgSizer6->AddGrowableRow(1);
	fgSizer6->SetFlexibleDirection(wxBOTH);

	wxFlexGridSizer* fgSizer3;
	fgSizer3 = new wxFlexGridSizer(1, 2, 0, 0);
	fgSizer3->AddGrowableCol(0);
	fgSizer3->SetFlexibleDirection(wxHORIZONTAL);

	wxBoxSizer* bSizer1;
	bSizer1 = new wxBoxSizer(wxVERTICAL);

	m_staticText3 = new wxStaticText(this, wxID_ANY, _("Font:"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer1->Add(m_staticText3, 0, wxALL, 5);

	m_selectedFontTextCtrl = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_READONLY);
	bSizer1->Add(m_selectedFontTextCtrl, 0, wxALL | wxEXPAND, 5);

	fgSizer3->Add(bSizer1, 1, wxEXPAND, 5);

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer(wxVERTICAL);

	bSizer2->Add(0, 0, 1, wxEXPAND, 0);

	m_button6 = new wxButton(this, ID_SelectFont, _("Select..."), wxDefaultPosition, wxDefaultSize, 0);
	bSizer2->Add(m_button6, 0, wxALL, 5);

	fgSizer3->Add(bSizer2, 1, wxEXPAND, 5);

	fgSizer6->Add(fgSizer3, 1, wxEXPAND, 5);

	wxFlexGridSizer* fgSizer8;
	fgSizer8 = new wxFlexGridSizer(1, 2, 0, 0);
	fgSizer8->AddGrowableCol(0);
	fgSizer8->AddGrowableRow(0);
	fgSizer8->SetFlexibleDirection(wxBOTH);

	wxFlexGridSizer* fgSizer10;
	fgSizer10 = new wxFlexGridSizer(2, 1, 0, 0);
	fgSizer10->AddGrowableCol(0);
	fgSizer10->AddGrowableRow(1);
	fgSizer10->SetFlexibleDirection(wxBOTH);

	m_staticText9 = new wxStaticText(this, wxID_ANY, _("Display items:"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer10->Add(m_staticText9, 0, wxALL, 5);

	m_displayItemsListBox = new wxListBox(this, ID_DisplayItemsList, wxDefaultPosition, wxDefaultSize, 0, nullptr, wxLB_SINGLE);
	fgSizer10->Add(m_displayItemsListBox, 1, wxALL | wxEXPAND, 5);

	fgSizer8->Add(fgSizer10, 1, wxEXPAND, 5);

	wxFlexGridSizer* fgSizer13 = new wxFlexGridSizer(6, 2, 0, 0);
	fgSizer13->AddGrowableCol(0);
	fgSizer13->SetFlexibleDirection(wxBOTH);

	m_staticText12 = new wxStaticText(this, wxID_ANY, _("Item foreground:"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer13->Add(m_staticText12, 0, wxALL, 5);

	fgSizer13->Add(0, 0, 1, wxEXPAND, 0);

	m_foreColorChoice = new wxChoice(this, ID_ForeColorChoice, wxDefaultPosition, wxDefaultSize, s_numDefaultColors, s_defaultColorName, 0);
	fgSizer13->Add(m_foreColorChoice, 0, wxALL | wxEXPAND, 5);

	m_button4 = new wxButton(this, ID_ForeColorCustom, _("Custom..."), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer13->Add(m_button4, 0, wxALL, 5);

	m_staticText13 = new wxStaticText(this, wxID_ANY, _("Item background:"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer13->Add(m_staticText13, 0, wxALL, 5);

	fgSizer13->Add(0, 0, 1, wxEXPAND, 0);

	m_backColorChoice = new wxChoice(this, ID_BackColorChoice, wxDefaultPosition, wxDefaultSize, s_numDefaultColors, s_defaultColorName, 0);
	fgSizer13->Add(m_backColorChoice, 0, wxALL | wxEXPAND, 5);

	m_button5 = new wxButton(this, ID_BackColorCustom, _("Custom..."), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer13->Add(m_button5, 0, wxALL, 5);

	// Bold check box.
	m_boldCheck = new wxCheckBox(this, ID_BoldCheck, _("Bold"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer13->Add(m_boldCheck, 0, wxALL, 5);
	fgSizer13->Add(0, 0, wxALL, 5);

	// Italic check box.
	m_italicCheck = new wxCheckBox(this, ID_ItalicCheck, _("Italic"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer13->Add(m_italicCheck, 0, wxALL, 5);
	fgSizer13->Add(0, 0, wxALL, 5);

	fgSizer8->Add(fgSizer13, 1, wxEXPAND, 5);

	fgSizer6->Add(fgSizer8, 1, wxEXPAND, 5);

	wxBoxSizer* bSizer7;
	bSizer7 = new wxBoxSizer(wxVERTICAL);

	m_staticText10 = new wxStaticText(this, wxID_ANY, _("Sample:"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer7->Add(m_staticText10, 0, wxALL, 5);

	m_previewPanel = new CStaticTextEx(this, wxID_ANY, wxT("AaBbCcDdXxYyZz"), wxDefaultPosition, wxSize(-1, 80), wxSUNKEN_BORDER | wxALIGN_CENTRE);
	m_previewPanel->SetBackgroundColour(wxSystemSettings::GetColour(wxSYS_COLOUR_WINDOW));

	bSizer7->Add(m_previewPanel, 1, wxEXPAND | wxALL, 5);

	fgSizer6->Add(bSizer7, 1, wxEXPAND, 5);

	m_button51 = new wxButton(this, ID_ResetToDefaults, _("Reset to defaults"), wxDefaultPosition, wxDefaultSize, 0);
	fgSizer6->Add(m_button51, 0, wxALL, 5);

	this->SetSizer(fgSizer6);
	this->Layout();

}

void CFontColorSettingsPanel::Initialize()
{

	for (unsigned int i = 0; i < m_settings.GetNumDisplayItems(); ++i)
	{
		m_displayItemsListBox->Append(m_settings.GetDisplayItemName(i));
	}

	m_displayItemsListBox->SetSelection(0);

	UpdateSelectFontTextCtrl();
	UpdatePreview();

}

void CFontColorSettingsPanel::SetSettings(const CFontColorSettings& settings)
{
	m_settings = settings;
}

const CFontColorSettings& CFontColorSettingsPanel::GetSettings() const
{
	return m_settings;
}

void CFontColorSettingsPanel::OnSelectFont(wxCommandEvent& event)
{

	wxFontData fontData;

	fontData.EnableEffects(false);
	fontData.SetInitialFont(m_settings.GetFont());

	wxFontDialog dialog(this, fontData);

	if (dialog.ShowModal() == wxID_OK)
	{
		m_settings.SetFont(dialog.GetFontData().GetChosenFont());
		UpdateSelectFontTextCtrl();
		UpdatePreview();
	}

}

void CFontColorSettingsPanel::OnResetToDefaults(wxCommandEvent& event)
{

	m_settings = CFontColorSettings();

	UpdateSelectFontTextCtrl();
	UpdatePreview();

}

void CFontColorSettingsPanel::OnDisplayItemsListItemSelected(wxCommandEvent& event)
{
	UpdatePreview();
}

void CFontColorSettingsPanel::OnForeColorChoiceItemSelected(wxCommandEvent& event)
{

	CFontColorSettings::DisplayItem item = static_cast<CFontColorSettings::DisplayItem>(m_displayItemsListBox->GetSelection());

	int selection = m_foreColorChoice->GetSelection();

	if (selection < s_numDefaultColors)
	{

		// Update the settings.
		CFontColorSettings::Colors colors = m_settings.GetColors(item);
		colors.foreColor = s_defaultColor[selection];
		m_settings.SetColors(item, colors);

		UpdatePreview();

	}

}

void CFontColorSettingsPanel::OnBackColorChoiceItemSelected(wxCommandEvent& event)
{

	CFontColorSettings::DisplayItem item = static_cast<CFontColorSettings::DisplayItem>(m_displayItemsListBox->GetSelection());

	int selection = m_backColorChoice->GetSelection();

	if (selection < s_numDefaultColors)
	{

		// Update the settings.
		CFontColorSettings::Colors colors = m_settings.GetColors(item);
		colors.backColor = s_defaultColor[selection];
		m_settings.SetColors(item, colors);

		UpdatePreview();

	}

}

void CFontColorSettingsPanel::OnForeColorCustom(wxCommandEvent& event)
{

	CFontColorSettings::DisplayItem item = static_cast<CFontColorSettings::DisplayItem>(m_displayItemsListBox->GetSelection());
	CFontColorSettings::Colors colors = m_settings.GetColors(item);

	colors.foreColor = wxGetColourFromUser(this, colors.foreColor);

	if (colors.foreColor.IsOk())
	{
		m_settings.SetColors(item, colors);
		UpdatePreview();
	}

}

void CFontColorSettingsPanel::OnBackColorCustom(wxCommandEvent& event)
{

	CFontColorSettings::DisplayItem item = static_cast<CFontColorSettings::DisplayItem>(m_displayItemsListBox->GetSelection());
	CFontColorSettings::Colors colors = m_settings.GetColors(item);

	colors.backColor = wxGetColourFromUser(this, colors.backColor);

	if (colors.backColor.IsOk())
	{
		m_settings.SetColors(item, colors);
		UpdatePreview();
	}

}

void CFontColorSettingsPanel::OnBoldCheck(wxCommandEvent& event)
{

	CFontColorSettings::DisplayItem item = static_cast<CFontColorSettings::DisplayItem>(m_displayItemsListBox->GetSelection());
	CFontColorSettings::Colors colors = m_settings.GetColors(item);

	colors.bold = m_boldCheck->GetValue();
	m_settings.SetColors(item, colors);

	UpdatePreview();

}

void CFontColorSettingsPanel::OnItalicCheck(wxCommandEvent& event)
{

	CFontColorSettings::DisplayItem item = static_cast<CFontColorSettings::DisplayItem>(m_displayItemsListBox->GetSelection());
	CFontColorSettings::Colors colors = m_settings.GetColors(item);

	colors.italic = m_italicCheck->GetValue();
	m_settings.SetColors(item, colors);

	UpdatePreview();

}

void CFontColorSettingsPanel::UpdateSelectFontTextCtrl()
{

	wxString text;

	if (m_settings.GetFont().IsOk())
	{
		text = m_settings.GetFont().GetFaceName();
	}

	m_selectedFontTextCtrl->SetValue(text);

}

void CFontColorSettingsPanel::UpdatePreview()
{
	if (m_settings.GetFont().IsOk())
	{
		m_previewPanel->SetFont(m_settings.GetFont());
	}

	int selection = m_displayItemsListBox->GetSelection();
	assert(selection != wxNOT_FOUND);

	CFontColorSettings::DisplayItem item = static_cast<CFontColorSettings::DisplayItem>(selection);

	m_previewPanel->SetFont(m_settings.GetFont(item));
	m_previewPanel->SetForegroundColour(m_settings.GetColors(item).foreColor);
	m_previewPanel->SetBackgroundColour(m_settings.GetColors(item).backColor);

	unsigned int foreColorIndex = GetMatchingDefaultColor(m_settings.GetColors(item).foreColor);
	unsigned int backColorIndex = GetMatchingDefaultColor(m_settings.GetColors(item).backColor);

	SetColorSelection(m_foreColorChoice, foreColorIndex);
	SetColorSelection(m_backColorChoice, backColorIndex);

	m_boldCheck->SetValue(m_settings.GetColors(item).bold);
	m_italicCheck->SetValue(m_settings.GetColors(item).italic);

	// Force a repaint.
	m_previewPanel->Refresh();

}

unsigned int CFontColorSettingsPanel::GetMatchingDefaultColor(const wxColor& color) const
{
	for (unsigned int i = 0; i < s_numDefaultColors; ++i)
	{
		if (s_defaultColor[i] == color)
		{
			return i;
		}
	}
	return -1;
}

void CFontColorSettingsPanel::SetColorSelection(wxChoice* colorChoice, unsigned int index)
{

	const wxString customString = "Custom";
	int customIndex = colorChoice->FindString(customString);

	if (index != -1)
	{
		colorChoice->SetSelection(index);
		if (customIndex != wxNOT_FOUND)
		{
			// Remove the custom choice.
			colorChoice->Delete(customIndex);
		}
	}
	else
	{
		if (customIndex == -1)
		{
			// Add the custom option.
			customIndex = colorChoice->Append(customString);
		}
		colorChoice->SetSelection(customIndex);
	}

}
