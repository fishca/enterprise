#ifndef __WINDOW_BASE_H__
#define __WINDOW_BASE_H__

#include "control.h"

#define FORM_ACTION 1

class IValueWindow : public IValueControl {
    wxDECLARE_ABSTRACT_CLASS(IValueWindow);
protected:
    CPropertyCategory* m_categoryWindow = IPropertyObject::CreatePropertyCategory(wxT("window"), _("Window"));
    CPropertySize* m_propertyMinSize = IPropertyObject::CreateProperty<CPropertySize>(m_categoryWindow, wxT("minimumSize"), _("Minimum size"), _("Sets the minimum size of the window, to indicate to the sizer layout mechanism that this is the minimum required size."), wxDefaultSize);
    CPropertySize* m_propertyMaxSize = IPropertyObject::CreateProperty<CPropertySize>(m_categoryWindow, wxT("maximumSize"), _("Maximum size"), _("Sets the maximum size of the window, to indicate to the sizer layout mechanism that this is the maximum allowable size."), wxDefaultSize);
    CPropertyFont* m_propertyFont = IPropertyObject::CreateProperty<CPropertyFont>(m_categoryWindow, wxT("font"), _("Font"), _("Sets the font for this window. This should not be use for a parent window if you don't want its font to be inherited by its children"));
    CPropertyColour* m_propertyFG = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryWindow, wxT("fg"), _("Foreground"), _("Sets the foreground colour of the window."), wxDefaultStypeFGColour);
    CPropertyColour* m_propertyBG = IPropertyObject::CreateProperty<CPropertyColour>(m_categoryWindow, wxT("bg"), _("Background"), _("Sets the background colour of the window."), wxDefaultStypeBGColour);
    CPropertyTString* m_propertyTooltip = IPropertyObject::CreateProperty<CPropertyTString>(m_categoryWindow, wxT("tooltip"), _("Tooltip"), _("Attach a tooltip to the window."), wxT(""));
    CPropertyBoolean* m_propertyEnabled = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryWindow, wxT("enabled"), _("Enabled"), _("Enable or disable the window for user input.Note that when a parent window is disabled, all of its children are disabled as well and they are reenabled again when the parent is."), true);
    CPropertyBoolean* m_propertyVisible = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryWindow, wxT("visible"), _("Visible"), _("Indicates that a pane caption should be visible."), true);
public:

    void EnableWindow(bool enable = true) const { m_propertyEnabled->SetValue(enable); }
    void VisibleWindow(bool visible = true) const { m_propertyVisible->SetValue(visible); }

    IValueWindow();

    //load & save object in control 
    virtual bool LoadData(CMemoryReader& reader);
    virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

    virtual int GetComponentType() const { return COMPONENT_TYPE_WINDOW; }

protected:

    void UpdateWindow(wxWindow* window);
};

#endif 