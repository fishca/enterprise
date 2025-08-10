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
protected:
    CPropertyCategory* m_categoryData = IPropertyObject::CreatePropertyCategory(wxT("data"), _("data"));
    CPropertySource* m_propertySource = IPropertyObject::CreateProperty<CPropertySource>(m_categoryData, wxT("source"), _("source"));
    CPropertyCategory* m_categoryEvent = IPropertyObject::CreatePropertyCategory(wxT("event"), _("event"));
    CEventControl* m_eventSelection = IPropertyObject::CreateEvent<CEventControl>(m_categoryEvent, wxT("selection"), _("selection"), _("On double mouse click or pressing of Enter."), wxArrayString{"control", "rowSelected", "standardProcessing"});
    CEventControl* m_eventOnActivateRow = IPropertyObject::CreateEvent<CEventControl>(m_categoryEvent, wxT("onActivateRow"), _("onActivateRow"), _("When row is activated"), wxArrayString{{"control"}});
    CEventControl* m_eventBeforeAddRow = IPropertyObject::CreateEvent<CEventControl>(m_categoryEvent, wxT("beforeAddRow"), _("beforeAddRow"), _("When row addition mode is called"), wxArrayString{"control", "cancel", "clone"});
    CEventControl* m_eventBeforeDeleteRow = IPropertyObject::CreateEvent<CEventControl>(m_categoryEvent, wxT("beforeDeleteRow"), _("beforeDeleteRow"), _("When row deletion is called"), wxArrayString{"control", "cancel"});

private:
    IValueModel* m_tableModel;
    IValueModel::IValueModelReturnLine* m_tableCurrentLine;
    bool m_dataViewUpdated, m_dataViewSizeChanged;
    wxSize m_dataViewSize;
public:

    ////////////////////////////////////////////////////////////////////////////////////////
    void SetSource(const meta_identifier_t& id) { m_propertySource->SetValue(id); CValueTableBox::RefreshModel(true); }
    meta_identifier_t GetSource() const { return m_propertySource->GetValueAsSource(); }
    ////////////////////////////////////////////////////////////////////////////////////////

    CValueTableBox();
    virtual ~CValueTableBox();

    //Get source attribute  
    virtual IMetaObjectAttribute* GetSourceAttributeObject() const {
        return m_propertySource->GetSourceAttributeObject();
    }

    virtual eSelectorDataType GetFilterDataType() const {
        return eSelectorDataType::eSelectorDataType_table;
    }

    virtual eSourceDataType GetFilterSourceDataType() const {
        return eSourceDataType::eSourceDataVariant_table;
    }

    //Get source object 
    virtual ISourceObject* GetSourceObject() const;

#pragma region _source_data_

    //get metaData from object 
    virtual IMetaObjectSourceData* GetSourceMetaObject() const;

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

    //control factory 
    virtual wxObject* Create(wxWindow* wxparent, IVisualHost* visualHost) override;
    virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost* visualHost, bool first—reated) override;
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
    virtual bool FilterSource(const CSourceExplorer& src, const meta_identifier_t& id);

    //contol value
    virtual bool HasValueInControl() const {
        return true;
    }

    virtual bool GetControlValue(CValue& pvarControlVal) const;
    virtual bool SetControlValue(const CValue& varControlVal = CValue());

    //other
    void AddColumn();
    void CreateColumnCollection(wxDataViewCtrl* tableCtrl = nullptr);

    void CreateTable(bool recreateModel = false);

    void CreateModel(bool recreateModel = false);
    void RefreshModel(bool recreateModel = false);

protected:

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

    friend class CValueForm;
    friend class CValueTableBoxColumn;
};

class CValueTableBoxColumn : public IValueControl,
    public ITypeControlFactory {
    wxDECLARE_DYNAMIC_CLASS(CValueTableBoxColumn);
protected:

    bool GetChoiceForm(CPropertyList* property);

    CPropertyCategory* m_categoryInfo = IPropertyObject::CreatePropertyCategory(wxT("info"), _("info"));
    CPropertyCaption* m_propertyCaption = IPropertyObject::CreateProperty<CPropertyCaption>(m_categoryInfo, wxT("caption"), wxEmptyString);
    CPropertyBoolean* m_propertyPasswordMode = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryInfo, wxT("passwordMode"), _("password mode"), false);
    CPropertyBoolean* m_propertyMultilineMode = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryInfo, wxT("multilineMode"), _("multiline mode"), false);
    CPropertyBoolean* m_propertyTexteditMode = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryInfo, wxT("texteditMode"), _("textedit mode"), true);

    CPropertyCategory* m_categoryData = IPropertyObject::CreatePropertyCategory(wxT("data"), _("data"));
    CPropertySource* m_propertySource = IPropertyObject::CreateProperty<CPropertySource>(m_categoryData, wxT("source"), _("source"), eValueTypes::TYPE_STRING);
    CPropertyList* m_propertyChoiceForm = IPropertyObject::CreateProperty<CPropertyList>(m_categoryData, wxT("choiceForm"), _("choice form"), &CValueTableBoxColumn::GetChoiceForm, wxNOT_FOUND);

    CPropertyCategory* m_categoryButton = IPropertyObject::CreatePropertyCategory(wxT("button"), _("button"));
    CPropertyBoolean* m_propertySelectButton = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryButton, wxT("buttonSelect"), _("select"), true);
    CPropertyBoolean* m_propertyClearButton = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryButton, wxT("buttonClear"), _("clear"), true);
    CPropertyBoolean* m_propertyOpenButton = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryButton, wxT("buttonOpen"), _("open"), false);

    CPropertyCategory* m_categoryStyle = IPropertyObject::CreatePropertyCategory(wxT("style"), _("style"));
    CPropertyUInteger* m_propertyWidth = IPropertyObject::CreateProperty<CPropertyUInteger>(m_categoryStyle, wxT("width"), _("width"), wxDVC_DEFAULT_WIDTH);
    CPropertyEnum<CValueEnumHorizontalAlignment>* m_propertyAlign = IPropertyObject::CreateProperty<CPropertyEnum<CValueEnumHorizontalAlignment>>(m_categoryStyle, wxT("align"), _("align"), wxALIGN_LEFT);
    CPropertyPicture* m_propertyIcon = IPropertyObject::CreateProperty<CPropertyPicture>(m_categoryStyle, wxT("icon"), _("icon"));
    CPropertyBoolean* m_propertyVisible = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryStyle, wxT("visible"), _("visible"), true);
    CPropertyBoolean* m_propertyResizable = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryStyle, wxT("resizable"), _("resizable"), true);
    //CPropertyBoolean* m_propertySortable = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryStyle, wxT("sortable"), _("sortable"), false);
    CPropertyBoolean* m_propertyReorderable = IPropertyObject::CreateProperty<CPropertyBoolean>(m_categoryStyle, wxT("reorderable"), _("reorderable"), true);

    CPropertyCategory* m_propertyEvent = IPropertyObject::CreatePropertyCategory(wxT("event"), _("event"));
    CEventControl* m_eventOnChange = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("onChange"), wxArrayString{wxT("control")});
    CEventControl* m_eventStartChoice = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("startChoice"), wxArrayString{wxT("control"), wxT("standartProcessing")});
    CEventControl* m_eventStartListChoice = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("startListChoice"), wxArrayString{wxT("control"), wxT("standartProcessing")});
    CEventControl* m_eventClearing = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("clearing"), wxArrayString{wxT("control"), wxT("standartProcessing")});
    CEventControl* m_eventOpening = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("opening"), wxArrayString{wxT("control"), wxT("standartProcessing")});
    CEventControl* m_eventChoiceProcessing = IPropertyObject::CreateEvent<CEventControl>(m_propertyEvent, wxT("choiceProcessing"), wxArrayString{wxT("control"), wxT("valueSelected"), wxT("standartProcessing")});

public:

    ////////////////////////////////////////////////////////////////////////////////////////

    form_identifier_t GetSourceColumn() const {
        const form_identifier_t& id = GetSource();
        return id != wxNOT_FOUND ? id : m_controlId;
    }

    ////////////////////////////////////////////////////////////////////////////////////////
    void SetSource(const meta_identifier_t& id) { m_propertySource->SetValue(id); }
    meta_identifier_t GetSource() const { return m_propertySource->GetValueAsSource(); }
    ////////////////////////////////////////////////////////////////////////////////////////

    void SetCaption(const wxString& caption) { return m_propertyCaption->SetValue(caption); }
    wxString GetCaption() const { return m_propertyCaption->GetValueAsString(); }

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

    CValueTableBox* GetOwner() const { return dynamic_cast<CValueTableBox*>(m_parent); }

    IValueTable::IValueModelReturnLine* GetReturnLine() const {
        CValueTableBox* tableBox = GetOwner();
        wxASSERT(tableBox);
        return tableBox->m_tableCurrentLine;
    }

    ///////////////////////////////////////////////////////////////////////

    CValueTableBoxColumn();

    //Get source object 
    virtual ISourceObject* GetSourceObject() const { return GetOwner(); }

    //Get source attribute  
    virtual IMetaObjectAttribute* GetSourceAttributeObject() const {
        return m_propertySource->GetSourceAttributeObject();
    }

    virtual eSelectorDataType GetFilterDataType() const {
        return eSelectorDataType::eSelectorDataType_reference;
    }

    virtual eSourceDataType GetFilterSourceDataType() const {
        return eSourceDataType::eSourceDataVariant_tableColumn;
    }

    //get form owner 
    virtual CValueForm* GetOwnerForm() const { return m_formOwner; }

    //get metaData
    virtual IMetaData* GetMetaData() const;

    //get type description 
    virtual CTypeDescription& GetTypeDesc() const { return m_propertySource->GetValueAsTypeDesc(); }

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
    virtual bool FilterSource(const CSourceExplorer& src, const meta_identifier_t& id);

    //get control value
    virtual bool SetControlValue(const CValue& varControlVal = CValue());
    virtual bool GetControlValue(CValue& pvarControlVal) const;

    //choice processing
    virtual void ChoiceProcessing(CValue& vSelected);

protected:

    // text processing
    bool TextProcessing(wxTextCtrl* textCtrl, const wxString& strData);

    //events
    void OnSelectButtonPressed(wxCommandEvent& event);
    void OnOpenButtonPressed(wxCommandEvent& event);
    void OnClearButtonPressed(wxCommandEvent& event);

    void OnTextEnter(wxCommandEvent& event);
    void OnKillFocus(wxFocusEvent& event);

    friend class CValueForm;
    friend class CValueTableBox;
    friend class CValueViewRenderer;
};

#endif 