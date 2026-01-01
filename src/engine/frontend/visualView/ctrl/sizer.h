#ifndef _SIZER_H_
#define _SIZER_H_

#include "control.h"

class FRONTEND_API IValueSizer : public IValueControl {
	wxDECLARE_ABSTRACT_CLASS(IValueSizer);
protected:

	CPropertyCategory* m_categorySizer = IPropertyObject::CreatePropertyCategory(wxT("sizerItem"), _("Sizer"));
	CPropertySize* m_propertyMinSize = IPropertyObject::CreateProperty<CPropertySize>(m_categorySizer, wxT("minimumSize"), _("Minimum size"), _("Sets the minimum size of the window, to indicate to the sizer layout mechanism that this is the minimum required size."), wxDefaultSize);

public:

	IValueSizer() : IValueControl() {}

	virtual int GetComponentType() const { return COMPONENT_TYPE_SIZER; }

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

protected:

	void UpdateSizer(wxSizer* sizer);
};

//////////////////////////////////////////////////////////////////////////////

class FRONTEND_API CValueSizerItem : public IValueFrame {
	wxDECLARE_DYNAMIC_CLASS(CValueSizerItem);
private:
	CPropertyCategory* m_categorySizerItem = IPropertyObject::CreatePropertyCategory(wxT("sizerItem"), _("Sizer item"));
	CPropertyUInteger* m_propertyProportion = IPropertyObject::CreateProperty<CPropertyUInteger>(m_categorySizerItem, wxT("proportion"), _("Proportion"), 0);
	CPropertyCategory* m_categorySizerBorder = IPropertyObject::CreatePropertyCategory(wxT("sizerItemBorder"), _("Border"));
	CPropertyUInteger* m_propertyBorder = IPropertyObject::CreateProperty<CPropertyUInteger>(m_categorySizerBorder, wxT("borderSize"), _("Size"), 5);
	CPropertyBoolean* m_propertyFlagBorderLeft = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categorySizerBorder, wxT("borderLeft"), _("Left"), true);
	CPropertyBoolean* m_propertyFlagBorderRight = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categorySizerBorder, wxT("borderRight"), _("Right"), true);
	CPropertyBoolean* m_propertyFlagBorderTop = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categorySizerBorder, wxT("borderTop"), _("Top"), true);
	CPropertyBoolean* m_propertyFlagBorderBottom = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categorySizerBorder, wxT("borderBottom"), _("Bottom"), true);
	CPropertyEnum<CValueEnumStretch>* m_propertyFlagState = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumStretch>>(m_categorySizerItem, wxT("stretch"), _("Stretch"), wxStretch::wxSHRINK);
public:

	void SetProportion(int proportion) {
		m_propertyProportion->SetValue(proportion);
	}

	int GetProportion() const {
		return m_propertyProportion->GetValueAsUInteger();
	}

	void SetFlagBorder(long flag_border) const {
		m_propertyFlagBorderLeft->SetValue((flag_border & (wxLEFT)) != 0);
		m_propertyFlagBorderRight->SetValue((flag_border & (wxRIGHT)) != 0);
		m_propertyFlagBorderTop->SetValue((flag_border & (wxUP)) != 0);
		m_propertyFlagBorderBottom->SetValue((flag_border & (wxDOWN)) != 0);
	}

	long GetFlagBorder() const {
		long flag = 0;
		if (m_propertyFlagBorderLeft->GetValueAsBoolean()) flag |= wxLEFT;
		if (m_propertyFlagBorderRight->GetValueAsBoolean()) flag |= wxRIGHT;
		if (m_propertyFlagBorderTop->GetValueAsBoolean()) flag |= wxUP;
		if (m_propertyFlagBorderBottom->GetValueAsBoolean()) flag |= wxDOWN;
		return flag;
	}

	void SetFlagState(const wxStretch& s) const {
		m_propertyFlagState->SetValue(s);
	}

	wxStretch GetFlagState() const {
		return m_propertyFlagState->GetValueAsEnum();
	}

	int GetBorder() const {
		return m_propertyBorder->GetValueAsUInteger();
	}

	void SetBorder(long border) const {
		m_propertyBorder->SetValue(border);
	}

	CValueSizerItem();

	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) override;

	virtual int GetComponentType() const {
		return COMPONENT_TYPE_SIZERITEM;
	}

	//get metadata
	virtual IMetaData* GetMetaData() const override;

	virtual CValueForm* GetOwnerForm() const { return m_formOwner; }
	virtual void SetOwnerForm(CValueForm* ownerForm) { m_formOwner = ownerForm; }

	// allow getting value in control
	virtual bool HasValueInControl() const { return false; }

	/**
	* Can delete object
	*/
	virtual bool CanDeleteControl() const { return true; }

	//runtime 
	virtual CProcUnit* GetFormProcUnit() const;

	/**
	* Get type form
	*/
	virtual form_identifier_t GetTypeForm() const;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	friend class CVisualEditorDatabase;

protected:

	//frame owner 
	CValueForm* m_formOwner;
};

//////////////////////////////////////////////////////////////////////////////

class CValueBoxSizer : public IValueSizer {
	wxDECLARE_DYNAMIC_CLASS(CValueBoxSizer);
public:
	CPropertyEnum<CValueEnumOrient>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumOrient>>(m_categorySizer, wxT("orient"), _("Orient"), wxVERTICAL);
public:

	CValueBoxSizer();

	//control factory
	virtual wxObject* Create(wxWindow* /*parent*/, IVisualHost* /*visualHost*/) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

#include <wx/wrapsizer.h>

class CValueWrapSizer : public IValueSizer {
	wxDECLARE_DYNAMIC_CLASS(CValueWrapSizer);
protected:
	CPropertyEnum<CValueEnumOrient>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumOrient>>(m_categorySizer, wxT("orient"), _("Orient"), wxHORIZONTAL);
public:

	CValueWrapSizer();

	//control factory
	virtual wxObject* Create(wxWindow* /*parent*/, IVisualHost* /*visualHost*/) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

class CValueStaticBoxSizer : public IValueSizer {
	wxDECLARE_DYNAMIC_CLASS(CValueStaticBoxSizer);
protected:
	CPropertyEnum<CValueEnumOrient>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumOrient>>(m_categorySizer, wxT("orient"), _("Orient"), wxHORIZONTAL);
	CPropertyTString* m_propertyCaption = IPropertyObject::CreateProperty<CPropertyTString>(m_categorySizer, wxT("caption"), _("Caption"), wxT(""));
	CPropertyFont* m_propertyFont = IPropertyObject::CreateProperty<CPropertyFont>(m_categorySizer, wxT("font"), _("Font"), _("Sets the font for this window. This should not be use for a parent window if you don't want its font to be inherited by its children"));
	CPropertyColour* m_propertyFG = IPropertyObject::CreateProperty<CPropertyColour>(m_categorySizer, wxT("fg"), _("Foreground"), _("Sets the foreground colour of the window."), wxDefaultStypeFGColour);
	CPropertyColour* m_propertyBG = IPropertyObject::CreateProperty<CPropertyColour>(m_categorySizer, wxT("bg"), _("Background"), _("Sets the background colour of the window."), wxDefaultStypeBGColour);
	CPropertyString* m_propertyTooltip = IPropertyObject::CreateProperty<CPropertyString>(m_categorySizer, wxT("tooltip"), _("Tooltip"), _("Attach a tooltip to the window."), wxT(""));
	CPropertyBoolean* m_propertyContextMenu = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categorySizer, wxT("contextMenu"), _("Context menu"), _("Generates event handler for displaying of menu assigned to this widgets as a context menu."));
	CPropertyString* m_propertyContextHelp = IPropertyObject::CreateProperty<CPropertyString>(m_categorySizer, wxT("contextHelp"), _("Context help"), _("Attach context-sensitive help to the window. Note: The Project's &quot;help_provider&quot; property must be set for context-sensitive help to work."), wxEmptyString);
	CPropertyBoolean* m_propertyEnabled = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categorySizer, wxT("enabled"), _("Enabled"), _("Enable or disable the window for user input.Note that when a parent window is disabled, all of its children are disabled as well and they are reenabled again when the parent is."), true);
	CPropertyBoolean* m_propertyVisible = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categorySizer, wxT("visible"), _("Visible"), _("Indicates that a pane caption should be visible."), true);
public:

	CValueStaticBoxSizer();

	//get caption 
	virtual wxString GetControlCaption() const { return m_propertyCaption->GetValueAsTranslateString(); }

	//control factory
	virtual wxObject* Create(wxWindow* parent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

class CValueGridSizer : public IValueSizer {
	wxDECLARE_DYNAMIC_CLASS(CValueGridSizer);
protected:
	CPropertyUInteger* m_propertyRows = IPropertyObject::CreateProperty<CPropertyUInteger>(m_categorySizer, wxT("rows"), _("Rows"), 0);
	CPropertyUInteger* m_propertyCols = IPropertyObject::CreateProperty<CPropertyUInteger>(m_categorySizer, wxT("cols"), _("Cols"), 2);
public:

	CValueGridSizer();

	//control factory
	virtual wxObject* Create(wxWindow* /*parent*/, IVisualHost* /*visualHost*/) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

#endif 