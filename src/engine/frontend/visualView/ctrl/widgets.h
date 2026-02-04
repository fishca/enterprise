#ifndef _COMMON_H_
#define _COMMON_H_

#include "window.h"
#include "typeControl.h"

/////////////////////////////////////////////////////////////////////////////////////
//                                 COMMON ELEMENTS                                 //
/////////////////////////////////////////////////////////////////////////////////////
#include <wx/button.h>

class CValueButton : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueButton);
public:

	void SetCaption(const wxString& caption) { return m_propertyTitle->SetValue(caption); }
	wxString GetCaption() const { return m_propertyTitle->GetValueAsTranslateString(); }

	CValueButton();

	//get title
	virtual wxString GetControlTitle() const { return GetCaption(); }

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

protected:

	//events 
	void OnButtonPressed(wxCommandEvent& event);

private:
	CPropertyCategory* m_categoryButton = IPropertyObject::CreatePropertyCategory(wxT("button"), _("Button"));
	CPropertyTString* m_propertyTitle = IPropertyObject::CreateProperty<CPropertyTString>(m_categoryButton, wxT("title"), _("Title"), wxT("Button"));
	CPropertyEnum<CValueEnumRepresentation>* m_propertyRepresentation = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumRepresentation>>(m_categoryButton, wxT("representation"), _("Representation"), enRepresentation::eRepresentation_Auto);
	CPropertyPicture* m_propertyPicture = IPropertyObject::CreateProperty<CPropertyPicture>(m_categoryButton, wxT("picture"), _("Picture"));

	//event
	CPropertyCategory* m_categoryEvent = IPropertyObject::CreatePropertyCategory(wxT("Event"), _("Event"));
	CEventControl* m_onButtonPressed = IPropertyObject::CreateEvent<CEventControl>(m_categoryEvent, wxT("onButtonPressed"), _("Button pressed"), wxArrayString{ wxT("control") });
};

#include <wx/stattext.h>

class CValueStaticText : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueStaticText);
public:

	void SetCaption(const wxString& caption) { m_propertyTitle->SetValue(caption); }
	wxString GetCaption() const { return m_propertyTitle->GetValueAsTranslateString(); }

	CValueStaticText();

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated)override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

protected:
	CPropertyCategory* m_categoryStaticText = IPropertyObject::CreatePropertyCategory(wxT("staticText"), _("Static text"));
	CPropertyBoolean* m_propertyMarkup = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryStaticText, wxT("markup"), _("Markup"), true);
	CPropertyUInteger* m_propertyWrap = IPropertyObject::CreateProperty<CPropertyUInteger>(m_categoryStaticText, wxT("wrap"), _("Wrap"), 0);
	CPropertyTString* m_propertyTitle = IPropertyObject::CreateProperty<CPropertyTString>(m_categoryStaticText, wxT("title"), _("Title"), wxT("Static text"));
};

#include <wx/textctrl.h>

class CValueTextCtrl : public IValueWindow,
	public ITypeControlFactory {
	wxDECLARE_DYNAMIC_CLASS(CValueTextCtrl);
public:

	////////////////////////////////////////////////////////////////////////////////////////
	void SetSource(const meta_identifier_t& id) { m_propertySource->SetValue(id); }
	meta_identifier_t GetSource(const meta_identifier_t& id) { return m_propertySource->GetValueAsSource(); }
	////////////////////////////////////////////////////////////////////////////////////////

	void SetCaption(const wxString& caption) { return m_propertyTitle->SetValue(caption); }
	wxString GetCaption() const { return m_propertyTitle->GetValueAsTranslateString(); }

	void SetSelectButton(bool caption) { return m_propertySelectButton->SetValue(caption); }
	bool GetSelectButton() const { return m_propertySelectButton->GetValueAsBoolean(); }

	void SetOpenButton(bool caption) { return m_propertyOpenButton->SetValue(caption); }
	bool GetOpenButton() const { return m_propertyOpenButton->GetValueAsBoolean(); }

	void SetClearButton(bool caption) { return m_propertyClearButton->SetValue(caption); }
	bool GetClearButton() const { return m_propertyClearButton->GetValueAsBoolean(); }

	CValueTextCtrl();

	//Get source object 
	virtual ISourceObject* GetSourceObject() const;

	//Get source attribute  
	virtual IValueMetaObjectAttribute* GetSourceAttributeObject() const {
		return m_propertySource->GetSourceAttributeObject();
	}

	//get form owner 
	virtual CValueForm* GetOwnerForm() const { return m_formOwner; }

	//get metaData
	virtual IMetaData* GetMetaData() const;

	//get type description 
	virtual CTypeDescription& GetTypeDesc() const { return m_propertySource->GetValueAsTypeDesc(); }

	//methods & attributes
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	//get title
	virtual wxString GetControlTitle() const;

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	/**
	* Property events
	*/
	virtual void OnPropertyCreated(IProperty* property);
	virtual void OnPropertyRefresh(class wxPropertyGridManager* pg, class wxPGProperty* pgProperty, IProperty* property);

	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

public:

	virtual bool HasValueInControl() const {
		return m_propertySource->IsEmptyProperty();
	}

	virtual bool GetControlValue(CValue& pvarControlVal) const;
	virtual bool SetControlValue(const CValue& varControlVal = CValue());

public:

	virtual void ChoiceProcessing(CValue& vSelected);

protected:

	bool TextProcessing(wxTextCtrl* textCtrl, const wxString& strData);

	//Events:
	void OnTextEnter(wxCommandEvent& event);
	void OnTextUpdated(wxCommandEvent& event);

	void OnKillFocus(wxFocusEvent& event);

	void OnSelectButtonPressed(wxCommandEvent& event);
	void OnOpenButtonPressed(wxCommandEvent& event);
	void OnClearButtonPressed(wxCommandEvent& event);

private:

	bool GetChoiceForm(CPropertyList* property);
	
	bool m_textModified;
	
	CValue m_selValue;

	CPropertyCategory* m_categoryText = IPropertyObject::CreatePropertyCategory(wxT("textbox"), _("Textbox"));
	CPropertyTString* m_propertyTitle = IPropertyObject::CreateProperty<CPropertyTString>(m_categoryText, wxT("title"), _("Title"), wxT(""));
	CPropertyBoolean* m_propertyPasswordMode = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryText, wxT("passwordMode"), _("Password mode"), _("Mode in which typed characters are replaced with a special character"), false);
	CPropertyBoolean* m_propertyMultilineMode = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryText, wxT("multilineMode"), _("Multiline mode"), _("Multiline mode"), false);
	CPropertyBoolean* m_propertyTexteditMode = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryText, wxT("texteditMode"), _("Textedit mode"), _("Whether or not text editing is enabled in the text box "), true);

	CPropertyCategory* m_categoryData = IPropertyObject::CreatePropertyCategory(wxT("data"), _("Data"));
	CPropertySource* m_propertySource = IPropertyObject::CreateProperty<CPropertySource>(m_categoryData, wxT("source"), _("Source"), eValueTypes::TYPE_STRING);
	CPropertyList* m_propertyChoiceForm = IPropertyObject::CreateProperty<CPropertyList>(m_categoryData, wxT("choiceForm"), _("Choice form"), &CValueTextCtrl::GetChoiceForm, wxNOT_FOUND);

	CPropertyCategory* m_categoryButton = IPropertyObject::CreatePropertyCategory(wxT("button"), _("Button"));
	CPropertyBoolean* m_propertySelectButton = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryButton, wxT("buttonSelect"), _("Select button"), true);
	CPropertyBoolean* m_propertyClearButton = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryButton, wxT("buttonClear"), _("Clear button"), true);
	CPropertyBoolean* m_propertyOpenButton = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryButton, wxT("buttonOpen"), _("Open button"), false);

	CPropertyCategory* m_propertyEvent = IPropertyObject::CreatePropertyCategory(wxT("event"), _("Event"));
	CEventControl* m_eventOnChange = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("onChange"), _("Change"), wxArrayString{ wxT("control") });
	CEventControl* m_eventStartChoice = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("startChoice"), _("Start choice"), wxArrayString{ wxT("control"), wxT("standartProcessing") });
	CEventControl* m_eventStartListChoice = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("startListChoice"), _("Start list choice"), wxArrayString{ wxT("control"), wxT("standartProcessing") });
	CEventControl* m_eventClearing = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("clearing"), _("Clearing"), wxArrayString{ wxT("control"), wxT("standartProcessing") });
	CEventControl* m_eventOpening = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("opening"), _("Opening"), wxArrayString{ wxT("control"), wxT("standartProcessing") });
	CEventControl* m_eventChoiceProcessing = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("choiceProcessing"), _("Choice processing"), wxArrayString{ wxT("control"), wxT("valueSelected"), wxT("standartProcessing") });

	friend class CValueForm;
};

#include <wx/combobox.h>

class CValueComboBox : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueComboBox);
public:

	CValueComboBox();

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

#include <wx/choice.h>

class CValueChoice : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueChoice);
public:

	CValueChoice();

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

#include <wx/listbox.h>

class CValueListBox : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueListBox);

public:

	CValueListBox();

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
};

#include <wx/checkbox.h>

class CValueCheckbox : public IValueWindow,
	public ITypeControlFactory {
	wxDECLARE_DYNAMIC_CLASS(CValueCheckbox);
public:

	////////////////////////////////////////////////////////////////////////////////////////
	void SetSource(const meta_identifier_t& id) { m_propertySource->SetValue(id); }
	meta_identifier_t GetSource() const { return m_propertySource->GetValueAsSource(); }
	////////////////////////////////////////////////////////////////////////////////////////

	void SetCaption(const wxString& caption) { return m_propertyTitle->SetValue(caption); }
	wxString GetCaption() const { return m_propertyTitle->GetValueAsTranslateString(); }

	CValueCheckbox();

	//get source object 
	virtual ISourceObject* GetSourceObject() const;

	//get source attribute  
	virtual IValueMetaObjectAttribute* GetSourceAttributeObject() const {
		return m_propertySource->GetSourceAttributeObject();
	}

	//get form owner 
	virtual CValueForm* GetOwnerForm() const { return m_formOwner; }

	//get metaData
	virtual IMetaData* GetMetaData() const;

	//get type description 
	virtual CTypeDescription& GetTypeDesc() const { return m_propertySource->GetValueAsTypeDesc(); }

	virtual eSelectorDataType GetFilterDataType() const {
		return eSelectorDataType::eSelectorDataType_boolean;
	}

	//methods & attributes
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	//get title
	virtual wxString GetControlTitle() const;

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	/**
	* Property events
	*/
	virtual void OnPropertyCreated(IProperty* property);
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);


	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

public:

	virtual bool HasValueInControl() const {
		return m_propertySource->IsEmptyProperty();
	}

	virtual bool GetControlValue(CValue& pvarControlVal) const;
	virtual bool SetControlValue(const CValue& varControlVal = CValue());

protected:

	//events 
	void OnClickedCheckbox(wxCommandEvent& event);

private:

	CValue m_selValue = false;

	CPropertyCategory* m_categoryCheckBox = IPropertyObject::CreatePropertyCategory(wxT("checkbox"), _("Checkbox"));
	CPropertyTString* m_propertyTitle = IPropertyObject::CreateProperty<CPropertyTString>(m_categoryCheckBox, wxT("title"), _("Title"), wxT("Checkbox"));
	CPropertyEnum<CValueEnumTitleLocation>* m_propertyTitleLocation = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumTitleLocation>>(m_categoryCheckBox, wxT("titleLocation"), _("Title location"), enTitleLocation::eLeft);

	CPropertyCategory* m_categorySource = IPropertyObject::CreatePropertyCategory(wxT("data"), _("Data"));
	CPropertySource* m_propertySource = IPropertyObject::CreateProperty<CPropertySource>(m_categoryCheckBox, wxT("source"), _("Source"), eValueTypes::TYPE_BOOLEAN);

	CPropertyCategory* m_categoryEvent = IPropertyObject::CreatePropertyCategory(wxT("event"), _("Event"));
	CEventControl* m_onCheckboxClicked = IPropertyObject::CreateEvent<CEventControl>(m_categoryEvent, wxT("onCheckboxClicked"), _("Checkbox clicked"), wxArrayString{ wxT("control") });

	friend class CValueForm;
};

#include <wx/radiobut.h>

class CValueRadioButton : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueRadioButton);
public:

	void SetCaption(const wxString& caption) { return m_propertyTitle->SetValue(caption); }
	wxString GetCaption() const { return m_propertyTitle->GetValueAsTranslateString(); }

	CValueRadioButton();

	//get title
	virtual wxString GetControlTitle() const { return GetCaption(); }

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:
	CPropertyCategory* m_categoryRadioButton = IPropertyObject::CreatePropertyCategory(wxT("radioButton"), _("Radio button"));
	CPropertyTString* m_propertyTitle = IPropertyObject::CreateProperty<CPropertyTString>(m_categoryRadioButton, wxT("title"), _("Title"), wxT("Radio button"));
	CPropertyBoolean* m_propertySelected = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryRadioButton, wxT("selected"), _("Selected"));
};

#include <wx/statline.h>

class CValueStaticLine : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueStaticLine);
public:

	CValueStaticLine();

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:
	CPropertyCategory* m_categoryStaticLine = IPropertyObject::CreatePropertyCategory(wxT("staticLine"), _("Static line"));
	CPropertyEnum<CValueEnumOrient>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumOrient>>(m_categoryStaticLine, wxT("orient"), _("Orient"), wxHORIZONTAL);
};

#include <wx/slider.h>

class CValueSlider : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueSlider);
public:

	CValueSlider();

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:
	CPropertyCategory* m_categorySlider = IPropertyObject::CreatePropertyCategory(wxT("slider"), _("Slider"));
	CPropertyInteger* m_propertyMinValue = IPropertyObject::CreateProperty<CPropertyInteger>(m_categorySlider, wxT("minValue"), _("Min value"), 0);
	CPropertyInteger* m_propertyMaxValue = IPropertyObject::CreateProperty<CPropertyInteger>(m_categorySlider, wxT("maxValue"), _("Max value"), 100);
	CPropertyInteger* m_propertyValue = IPropertyObject::CreateProperty<CPropertyInteger>(m_categorySlider, wxT("value"), _("Value"), 50);
	CPropertyEnum<CValueEnumOrient>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumOrient>>(m_categorySlider, wxT("orient"), _("Orient"), wxHORIZONTAL);
};

#include <wx/gauge.h>

class CValueGauge : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueGauge);
public:

	CValueGauge();

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

private:
	CPropertyCategory* m_categoryGauge = IPropertyObject::CreatePropertyCategory(wxT("gauge"), _("Gauge"));
	CPropertyInteger* m_propertyRange = IPropertyObject::CreateProperty<CPropertyInteger>(m_categoryGauge, wxT("range"), _("Range"), 100);
	CPropertyInteger* m_propertyValue = IPropertyObject::CreateProperty<CPropertyInteger>(m_categoryGauge, wxT("value"), _("Value"), 30);
	CPropertyEnum<CValueEnumOrient>* m_propertyOrient = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumOrient>>(m_categoryGauge, wxT("orient"), _("Orient"), wxHORIZONTAL);
};

#endif