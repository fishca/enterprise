#ifndef __PROP_H__
#define __PROP_H__

// Flags used only internally

// wxBoolProperty, wxFlagsProperty specific flags
constexpr wxPGFlags wxPGPropertyFlags_UseCheckBox = wxPGFlags::Reserved_1;
// DCC = Double Click Cycles
constexpr wxPGFlags wxPGPropertyFlags_UseDCC = wxPGFlags::Reserved_2;

// wxStringProperty flag
constexpr wxPGFlags wxPGPropertyFlags_Password = wxPGFlags::Reserved_2;

// wxColourProperty flag - if set, then match from list is searched for a custom colour.
constexpr wxPGFlags wxPGPropertyFlags_TranslateCustom = wxPGFlags::Reserved_1;

// wxCursorProperty, wxSystemColourProperty - If set, then selection of choices is static
// and should not be changed (i.e. returns nullptr in GetPropertyChoices).
constexpr wxPGFlags wxPGPropertyFlags_StaticChoices = wxPGFlags::Reserved_1;

// wxSystemColourProperty - wxEnumProperty based classes cannot use wxPGFlags::Reserved_1
constexpr wxPGFlags wxPGPropertyFlags_HideCustomColour = wxPGFlags::Reserved_2;
constexpr wxPGFlags wxPGPropertyFlags_ColourHasAlpha = wxPGFlags::Reserved_3;

// wxFileProperty - if set, full path is shown in wxFileProperty.
constexpr wxPGFlags wxPGPropertyFlags_ShowFullFileName = wxPGFlags::Reserved_1;

// wxLongStringProperty - flag used to mark that edit button
// should be enabled even in the read-only mode.
constexpr wxPGFlags wxPGPropertyFlags_ActiveButton = wxPGFlags::Reserved_3;

#endif 