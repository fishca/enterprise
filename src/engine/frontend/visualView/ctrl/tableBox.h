#ifndef __TABLE_BOX_H__
#define __TABLE_BOX_H__

#include <wx/headerctrl.h>

#include "frontend/visualView/ctrl/window.h"
#include "frontend/visualView/ctrl/typeControl.h"

#include "frontend/win/ctrls/dataView.h"

//********************************************************************************************
//*                                 define commom clsid									     *
//********************************************************************************************

//COMMON TABLE & COLUMN
const class_identifier_t g_controlTableBoxCLSID = string_to_clsid("CT_TABL");
const class_identifier_t g_controlTableBoxColumnCLSID = string_to_clsid("CT_TBLC");

//********************************************************************************************
//*                                 Value TableBox                                           *
//********************************************************************************************

class CValueTableBox : public IValueWindow,
	public ITypeControlFactory, public ISourceObject {
	wxDECLARE_DYNAMIC_CLASS(CValueTableBox);
public:

	////////////////////////////////////////////////////////////////////////////////////////
	void SetSource(const meta_identifier_t& id) { m_propertySource->SetValue(id); CValueTableBox::RefreshModel(true); }
	meta_identifier_t GetSource() const { return m_propertySource->GetValueAsSource(); }
	////////////////////////////////////////////////////////////////////////////////////////

	CValueTableBox();
	virtual ~CValueTableBox() {}

	//Get source attribute  
	virtual IMetaObjectAttribute* GetSourceAttributeObject() const { return m_propertySource->GetSourceAttributeObject(); }
	virtual eSelectorDataType GetFilterDataType() const { return eSelectorDataType::eSelectorDataType_table; }
	virtual eSourceDataType GetFilterSourceDataType() const { return eSourceDataType::eSourceDataVariant_table; }

	//Get source object 
	virtual ISourceObject* GetSourceObject() const;

#pragma region _source_data_

	//get metaData from object 
	virtual IMetaObjectCompositeData* GetSourceMetaObject() const;
	//get ref class 
	virtual class_identifier_t GetSourceClassType() const;
	//Get presentation 
	virtual wxString GetSourceCaption() const { return GetString(); }

#pragma endregion 

	//get form owner 
	virtual CValueForm* GetOwnerForm() const { return m_formOwner; }

	//get model 
	IValueModel* GetModel() const { return m_tableModel; }

	//get metaData
	virtual IMetaData* GetMetaData() const;

	//get type description 
	virtual CTypeDescription& GetTypeDesc() const {
		return m_propertySource->GetValueAsTypeDesc();
	}

	//methods & attributes
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	// before/after run 
	virtual bool InitializeControl() { CreateModel(); return true; }

	//get title
	virtual wxString GetControlTitle() const {

		if (!m_propertySource->IsEmptyProperty()) {
			CValue pvarPropVal;
			if (m_propertySource->GetDataValue(pvarPropVal))
				return _("TableBox:") + wxT(" ") + stringUtils::GenerateSynonym(pvarPropVal.GetString());
		}

		return _("TableBox:") + _("<empty source>");
	}

	//control factory 
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool firstÑreated) override;
	virtual void Update(wxObject* wxobject, IVisualHost* visualHost) override;
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	//get component type 
	virtual int GetComponentType() const { return COMPONENT_TYPE_WINDOW; }

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	/**
	* Property events
	*/
	virtual void OnPropertyCreated(IProperty* property);
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);


	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());

	/**
	* Override actionData
	*/

	virtual CActionCollection GetActionCollection(const form_identifier_t& formType);
	virtual void ExecuteAction(const action_identifier_t& lNumAction, IBackendValueForm* srcForm);

	/**
	* Support default menu
	*/
	virtual void PrepareDefaultMenu(wxMenu* m_menu);
	virtual void ExecuteMenu(IVisualHost* visualHost, int id);

	// filter data 
	virtual bool FilterSource(const CSourceExplorer& src, const meta_identifier_t& id) const;

	//contol value
	virtual bool HasValueInControl() const {
		return m_propertySource->IsEmptyProperty();
	}

	virtual bool GetControlValue(CValue& pvarControlVal) const;
	virtual bool SetControlValue(const CValue& varControlVal = CValue());

	//other
	void AddColumn();
	void CreateColumnCollection(wxDataViewCtrl* tableCtrl = nullptr);

	void CreateTable(bool recreateModel = false);

	void CreateModel(bool recreateModel = false);
	void RefreshModel(bool recreateModel = false);

	// get current line if exist 
	IValueModel::IValueModelReturnLine* GetCurrentLine() const { return m_tableCurrentLine; }

	void SetCalculateColumnPos() { m_need_calculate_pos = true; }

protected:

	virtual void OnChangeChildPosition(IValueFrame* obj, unsigned int pos) { SetCalculateColumnPos(); }

	void CalculateColumnPos();

	//events 
	void OnColumnClick(wxDataViewEvent& event);
	void OnColumnReordered(wxDataViewEvent& event);

	void OnSelectionChanged(wxDataViewEvent& event);

	void OnItemActivated(wxDataViewEvent& event);
	void OnItemCollapsed(wxDataViewEvent& event);
	void OnItemExpanded(wxDataViewEvent& event);
	void OnItemCollapsing(wxDataViewEvent& event);
	void OnItemExpanding(wxDataViewEvent& event);
	void OnItemStartEditing(wxDataViewEvent& event);
	void OnItemEditingStarted(wxDataViewEvent& event);
	void OnItemEditingDone(wxDataViewEvent& event);
	void OnItemValueChanged(wxDataViewEvent& event);

	void OnItemStartInserting(wxDataViewEvent& event);
	void OnItemStartDeleting(wxDataViewEvent& event);

	void OnHeaderResizing(wxHeaderCtrlEvent& event);
	void OnMainWindowClick(wxMouseEvent& event);

#if wxUSE_DRAG_AND_DROP
	void OnItemBeginDrag(wxDataViewEvent& event);
	void OnItemDropPossible(wxDataViewEvent& event);
	void OnItemDrop(wxDataViewEvent& event);
#endif // wxUSE_DRAG_AND_DROP

	void OnCommandMenu(wxCommandEvent& event);
	void OnContextMenu(wxDataViewEvent& event);

	void OnSize(wxSizeEvent& event);
	void OnIdle(wxIdleEvent& event);

	// the methods to be called from the window event handlers
	void HandleOnScroll(wxScrollWinEvent& event);

private:

#pragma region __property_define_h__
	CPropertyCategory* m_categoryData = IPropertyObject::CreatePropertyCategory(wxT("data"), _("Data"));
	CPropertySource* m_propertySource = IPropertyObject::CreateProperty<CPropertySource>(m_categoryData, wxT("source"), _("Source"));
	CPropertyCategory* m_categoryEvent = IPropertyObject::CreatePropertyCategory(wxT("event"), _("Event"));
	CEventControl* m_eventSelection = IPropertyObject::CreateEvent<CEventControl>(m_categoryEvent, wxT("selection"), _("Selection"), _("On double mouse click or pressing of Enter."), wxArrayString{ "control", "rowSelected", "standardProcessing" });
	CEventControl* m_eventOnActivateRow = IPropertyObject::CreateEvent<CEventControl>(m_categoryEvent, wxT("onActivateRow"), _("Activate row"), _("When row is activated"), wxArrayString{ {"control"} });
	CEventControl* m_eventBeforeAddRow = IPropertyObject::CreateEvent<CEventControl>(m_categoryEvent, wxT("beforeAddRow"), _("Before add row"), _("When row addition mode is called"), wxArrayString{ "control", "cancel", "clone" });
	CEventControl* m_eventBeforeDeleteRow = IPropertyObject::CreateEvent<CEventControl>(m_categoryEvent, wxT("beforeDeleteRow"), _("Before delete row"), _("When row deletion is called"), wxArrayString{ "control", "cancel" });
#pragma endregion 

	bool m_dataViewCreated, m_dataViewUpdated,
		m_dataViewSelected, m_dataViewSizeChanged;

	bool m_need_calculate_pos;

	wxSize m_dataViewSize;

	CValuePtr<IValueModel> m_tableModel;
	CValuePtr<IValueModel::IValueModelReturnLine> m_tableCurrentLine;
};

class CValueTableBoxColumn : public IValueControl,
	public ITypeControlFactory {
	wxDECLARE_DYNAMIC_CLASS(CValueTableBoxColumn);
protected:

	bool GetChoiceForm(CPropertyList* property);

public:

	////////////////////////////////////////////////////////////////////////////////////////

	form_identifier_t GetModelColumn() const {
		const form_identifier_t& id = m_model_id != wxNOT_FOUND ? m_model_id : GetSource();
		return id != wxNOT_FOUND ? id : m_controlId;
	}
	void SetModelColumn(const form_identifier_t& id) { m_model_id = id; }

	////////////////////////////////////////////////////////////////////////////////////////

	void SetSource(const meta_identifier_t& id) { m_propertySource->SetValue(id); }
	meta_identifier_t GetSource() const { return m_propertySource->GetValueAsSource(); }

	////////////////////////////////////////////////////////////////////////////////////////

	void SetCaption(const wxString& caption) { return m_propertyTitle->SetValue(caption); }
	wxString GetCaption() const { return m_propertyTitle->GetValueAsTranslateString(); }

	void SetPasswordMode(bool caption) { return m_propertyPasswordMode->SetValue(caption); }
	bool GetPasswordMode() const { return m_propertyPasswordMode->GetValueAsBoolean(); }

	void SetMultilineMode(bool caption) { return m_propertyMultilineMode->SetValue(caption); }
	bool GetMultilineMode() const { return m_propertyMultilineMode->GetValueAsBoolean(); }

	void SetTexteditMode(bool caption) { return m_propertyTexteditMode->SetValue(caption); }
	bool GetTextEditMode() const { return m_propertyTexteditMode->GetValueAsBoolean(); }

	void SetSelectButton(bool caption) { return m_propertySelectButton->SetValue(caption); }
	bool GetSelectButton() const { return m_propertySelectButton->GetValueAsBoolean(); }

	void SetOpenButton(bool caption) { return m_propertyOpenButton->SetValue(caption); }
	bool GetOpenButton() const { return m_propertyOpenButton->GetValueAsBoolean(); }

	void SetClearButton(bool caption) { return m_propertyClearButton->SetValue(caption); }
	bool GetClearButton() const { return m_propertyClearButton->GetValueAsBoolean(); }

	void SetVisibleColumn(bool visible = true) const { m_propertyVisible->SetValue(visible); }
	bool GetVisibleColumn() const { return m_propertyVisible->GetValueAsBoolean(); }

	void SetWidthColumn(int width) const { m_propertyWidth->SetValue(width); }
	int GetWidthColumn() const { return m_propertyWidth->GetValueAsUInteger(); }

	///////////////////////////////////////////////////////////////////////

	CValueTableBox* GetOwner() const { return m_parent->ConvertToType<CValueTableBox>(); }

	IValueTable::IValueModelReturnLine* GetCurrentLine() const {
		const CValueTableBox* tableBox = GetOwner();
		return tableBox != nullptr ?
			tableBox->GetCurrentLine() : nullptr;
	}

	///////////////////////////////////////////////////////////////////////

	CValueTableBoxColumn();

	//Get source object 
	virtual ISourceObject* GetSourceObject() const { return GetOwner(); }

	//Get source attribute  
	virtual IMetaObjectAttribute* GetSourceAttributeObject() const { return m_propertySource->GetSourceAttributeObject(); }
	virtual eSelectorDataType GetFilterDataType() const { return eSelectorDataType::eSelectorDataType_reference; }
	virtual eSourceDataType GetFilterSourceDataType() const { return eSourceDataType::eSourceDataVariant_tableColumn; }

	//get form owner 
	virtual CValueForm* GetOwnerForm() const { return m_formOwner; }

	//get metaData
	virtual IMetaData* GetMetaData() const;

	//get type description 
	virtual CTypeDescription& GetTypeDesc() const { return m_propertySource->GetValueAsTypeDesc(); }

	//get title
	virtual wxString GetControlTitle() const;

	//control factory
	virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool firstCreated) override;
	virtual void OnUpdated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost* visualHost) override;

	virtual bool CanDeleteControl() const;

	//get component type 
	virtual int GetComponentType() const { return COMPONENT_TYPE_ABSTRACT; }

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

	//filter source
	virtual bool FilterSource(const CSourceExplorer& src, const meta_identifier_t& id) const;

	//get control value
	virtual bool SetControlValue(const CValue& varControlVal = CValue());
	virtual bool GetControlValue(CValue& pvarControlVal) const;

	//choice processing
	virtual void ChoiceProcessing(CValue& vSelected);

	//events
	void OnSelectButtonPressed(wxCommandEvent& event);
	void OnOpenButtonPressed(wxCommandEvent& event);
	void OnClearButtonPressed(wxCommandEvent& event);

	void OnTextEnter(wxCommandEvent& event);
	void OnKillFocus(wxFocusEvent& event);

protected:

	// text processing
	bool TextProcessing(wxTextCtrl* textCtrl, const wxString& strData);

	form_identifier_t m_model_id;

private:

	CPropertyCategory* m_categoryInfo = IPropertyObject::CreatePropertyCategory(wxT("info"), _("Info"));
	CPropertyTString* m_propertyTitle = IPropertyObject::CreateProperty<CPropertyTString>(m_categoryInfo, wxT("title"), _("Title"), wxT(""));
	CPropertyBoolean* m_propertyPasswordMode = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryInfo, wxT("passwordMode"), _("Password mode"), _("Mode in which typed characters are replaced with a special character"), false);
	CPropertyBoolean* m_propertyMultilineMode = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryInfo, wxT("multilineMode"), _("Multiline mode"), _("Multiline mode"), false);
	CPropertyBoolean* m_propertyTexteditMode = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryInfo, wxT("texteditMode"), _("Textedit mode"), _("Whether or not text editing is enabled in the text box "), true);
	
	CPropertyCategory* m_categoryData = IPropertyObject::CreatePropertyCategory(wxT("data"), _("Data"));
	CPropertySource* m_propertySource = IPropertyObject::CreateProperty<CPropertySource>(m_categoryData, wxT("source"), _("Source"), eValueTypes::TYPE_STRING);
	CPropertyList* m_propertyChoiceForm = IPropertyObject::CreateProperty<CPropertyList>(m_categoryData, wxT("choiceForm"), _("Choice form"), &CValueTableBoxColumn::GetChoiceForm);

	CPropertyCategory* m_categoryButton = IPropertyObject::CreatePropertyCategory(wxT("button"), _("Button"));
	CPropertyBoolean* m_propertySelectButton = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryButton, wxT("buttonSelect"), _("Select button"), true);
	CPropertyBoolean* m_propertyClearButton = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryButton, wxT("buttonClear"), _("Clear button"), true);
	CPropertyBoolean* m_propertyOpenButton = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryButton, wxT("buttonOpen"), _("Open button"), false);

	CPropertyCategory* m_categoryStyle = IPropertyObject::CreatePropertyCategory(wxT("style"), _("Style"));
	CPropertyUInteger* m_propertyWidth = IPropertyObject::CreateProperty<CPropertyUInteger>(m_categoryStyle, wxT("width"), _("Width"), wxDVC_DEFAULT_WIDTH);
	CPropertyEnum<CValueEnumHorizontalAlignment>* m_propertyAlign = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumHorizontalAlignment>>(m_categoryStyle, wxT("align"), _("Align"), wxALIGN_LEFT);
	CPropertyEnum<CValueEnumRepresentation>* m_propertyRepresentation = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumRepresentation>>(m_categoryStyle, wxT("representation"), _("Representation"), enRepresentation::eRepresentation_Auto);
	CPropertyPicture* m_propertyPicture = IPropertyObject::CreateProperty<CPropertyPicture>(m_categoryStyle, wxT("picture"), _("Picture"));

	CPropertyBoolean* m_propertyVisible = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryStyle, wxT("visible"), _("Visible"), true);
	CPropertyBoolean* m_propertyResizable = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryStyle, wxT("resizable"), _("Resizable"), true);
	//CPropertyBoolean* m_propertySortable = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryStyle, wxT("sortable"), _("Sortable"), false);
	CPropertyBoolean* m_propertyReorderable = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryStyle, wxT("reorderable"), _("Reorderable"), true);

	CPropertyCategory* m_propertyEvent = IPropertyObject::CreatePropertyCategory(wxT("event"), _("Event"));
	CEventControl* m_eventOnChange = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("onChange"), _("Change"), wxArrayString{ wxT("control") });
	CEventControl* m_eventStartChoice = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("startChoice"), _("Start choice"), wxArrayString{ wxT("control"), wxT("standartProcessing") });
	CEventControl* m_eventStartListChoice = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("startListChoice"), wxArrayString{ wxT("control"), wxT("standartProcessing") });
	CEventControl* m_eventClearing = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("clearing"), _("Clearing"), wxArrayString{ wxT("control"), wxT("standartProcessing") });
	CEventControl* m_eventOpening = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("opening"), _("Opening"), wxArrayString{ wxT("control"), wxT("standartProcessing") });
	CEventControl* m_eventChoiceProcessing = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("choiceProcessing"), _("Choice processing"), wxArrayString{ wxT("control"), wxT("valueSelected"), wxT("standartProcessing") });
};

#endif 