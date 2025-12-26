#ifndef __META_OBJECT_H__
#define __META_OBJECT_H__

#include "backend/propertyManager/propertyManager.h"
#include "backend/metaCtor.h"
#include "backend/backend_metatree.h"

#include "backend/interfaceHelper.h"
#include "backend/roleHelper.h"

//*******************************************************************************
class BACKEND_API IMetaData;
//*******************************************************************************
//*                          define commom clsid                                *
//*******************************************************************************

//COMMON METADATA
const class_identifier_t g_metaCommonMetadataCLSID = string_to_clsid("MD_MTD");

//COMMON OBJECTS
const class_identifier_t g_metaCommonModuleCLSID = string_to_clsid("MD_CMOD");
const class_identifier_t g_metaCommonFormCLSID = string_to_clsid("MD_CFRM");
const class_identifier_t g_metaCommonTemplateCLSID = string_to_clsid("MD_CTMP");

const class_identifier_t g_metaRoleCLSID = string_to_clsid("MD_ROLE");
const class_identifier_t g_metaInterfaceCLSID = string_to_clsid("MD_SSYST");
const class_identifier_t g_metaPictureCLSID = string_to_clsid("MD_PICTR");
const class_identifier_t g_metaLanguageCLSID = string_to_clsid("MD_LANG");

//ADVANCED OBJECTS
const class_identifier_t g_metaAttributeCLSID = string_to_clsid("MD_ATTR");
const class_identifier_t g_metaFormCLSID = string_to_clsid("MD_FRM");
const class_identifier_t g_metaTemplateCLSID = string_to_clsid("MD_TMPL");
const class_identifier_t g_metaModuleCLSID = string_to_clsid("MD_MOD");
const class_identifier_t g_metaManagerCLSID = string_to_clsid("MD_MNGR");
const class_identifier_t g_metaTableCLSID = string_to_clsid("MD_TBL");
const class_identifier_t g_metaEnumCLSID = string_to_clsid("MD_ENUM");
const class_identifier_t g_metaDimensionCLSID = string_to_clsid("MD_DMNT");
const class_identifier_t g_metaResourceCLSID = string_to_clsid("MD_RESS");

//SPECIAL OBJECTS
const class_identifier_t g_metaPredefinedAttributeCLSID = string_to_clsid("MD_DATT");

//MAIN OBJECTS
const class_identifier_t g_metaConstantCLSID = string_to_clsid("MD_CONS");
const class_identifier_t g_metaCatalogCLSID = string_to_clsid("MD_CAT");
const class_identifier_t g_metaDocumentCLSID = string_to_clsid("MD_DOC");
const class_identifier_t g_metaEnumerationCLSID = string_to_clsid("MD_ENM");
const class_identifier_t g_metaDataProcessorCLSID = string_to_clsid("MD_DPR");
const class_identifier_t g_metaReportCLSID = string_to_clsid("MD_RPT");
const class_identifier_t g_metaInformationRegisterCLSID = string_to_clsid("MD_INFR");
const class_identifier_t g_metaAccumulationRegisterCLSID = string_to_clsid("MD_ACCR");

// EXTERNAL
const class_identifier_t g_metaExternalDataProcessorCLSID = string_to_clsid("MD_EDPR");
const class_identifier_t g_metaExternalReportCLSID = string_to_clsid("MD_ERPT");

//*******************************************************************************
//*                             Structure changes                               *
//*******************************************************************************

enum ERestructure {

	restructure_info,
	restructure_warning,
	restructure_error,
};

class BACKEND_API CRestructureInfo {

	struct CRestructureData {

		ERestructure m_type;
		wxString m_strDescr;

		CRestructureData(ERestructure t, const wxString& str) :
			m_type(t), m_strDescr(str)
		{
		}
	};

	std::vector<CRestructureData> m_listRestructure;

public:

	bool HasRestructureInfo() const { return m_listRestructure.size() > 0; }

	void AppendInfo(const wxString& str) {
		m_listRestructure.emplace_back(
			ERestructure::restructure_info, str
		);
	}

	void AppendWarning(const wxString& str) {
		m_listRestructure.emplace_back(
			ERestructure::restructure_warning, str
		);
	}

	void AppendError(const wxString& str) {
		m_listRestructure.emplace_back(
			ERestructure::restructure_error, str
		);
	}

	void ResetRestructureInfo() { return m_listRestructure.clear(); }

	wxString GetDescription(unsigned int idx) const { return m_listRestructure.at(idx).m_strDescr; }
	ERestructure GetType(unsigned int idx) const { return m_listRestructure.at(idx).m_type; }

	unsigned int GetCount() const { return m_listRestructure.size(); }
};

//*******************************************************************************
//*                             IMetaObject                                     *
//*******************************************************************************

#define defaultMetaID 1000

//flags metaobject event 
enum metaObjectFlags {

	defaultFlag = 0x0000,
	onlyLoadFlag = 0x0001,

	loadConfigFlag = 0x0002,
	saveConfigFlag = 0x0004,

	newObjectFlag = 0x0008,
	forceRunFlag = 0x0010,
	forceCloseFlag = 0x0020,

	loadFileFlag = 0x0040,
	saveToFileFlag = 0x0080,

	copyObjectFlag = 0x0100,
	pasteObjectFlag = 0x0200,
};

//flags metaobject 
#define metaDeletedFlag 0x0001000
#define metaCanSaveFlag 0x0002000
#define metaDisableFlag 0x0008000

#define metaDefaultFlag metaCanSaveFlag

//flags save
#define createMetaTable  0x0001000
#define updateMetaTable	 0x0002000	
#define deleteMetaTable	 0x0004000	

#define repairMetaTable  0x0008000

class BACKEND_API IMetaObject :

	public CValue,

	public IPropertyObjectHelper<IMetaObject>,
	public IAccessObject, public IInterfaceObject {

	wxDECLARE_ABSTRACT_CLASS(IMetaObject);

protected:

	CPropertyCategory* m_categoryCommon = IPropertyObject::CreatePropertyCategory(wxT("common"), _("Common"));
	CPropertyName* m_propertyName = IPropertyObject::CreateProperty<CPropertyName>(m_categoryCommon, wxT("name"), _("Name"), wxEmptyString);
	CPropertyCaption* m_propertySynonym = IPropertyObject::CreateProperty<CPropertyCaption>(m_categoryCommon, wxT("synonym"), _("Synonym"), wxEmptyString);
	CPropertyString* m_propertyComment = IPropertyObject::CreateProperty<CPropertyString>(m_categoryCommon, wxT("comment"), _("Comment"), wxEmptyString);

	CPropertyCategory* m_categorySecondary = IPropertyObject::CreatePropertyCategory(wxT("secondary"), _("Secondary"));

public:

	// get object name as string 
	bool GetObjectNameAsString(wxString& result) const {
		return m_propertyName->GetValueAsString(result);
	}

	//system attributes 
	meta_identifier_t GetMetaID() const { return m_metaId; }
	void SetMetaID(const meta_identifier_t& id) { m_metaId = id; }

	wxString GetName() const { return m_propertyName->GetValueAsString(); }
	void SetName(const wxString& strName) { m_propertyName->SetValue(strName); }

	wxString GetSynonym() const { return m_propertySynonym->IsEmptyProperty() ? stringUtils::GenerateSynonym(GetName()) : m_propertySynonym->GetValueAsString(); }
	void SetSynonym(const wxString& synonym) { m_propertySynonym->SetValue(synonym); }

	wxString GetComment() const { return m_propertyComment->GetValueAsString(); }
	void SetComment(const wxString& comment) { m_propertyComment->SetValue(comment); }

	virtual void SetMetaData(IMetaData* metaData) { m_metaData = metaData; }
	virtual IMetaData* GetMetaData() const { return m_metaData; }

	void ResetGuid();
	void ResetId();

	void ResetAll() {
		ResetGuid(); ResetId();
	}

	void GenerateGuid() {
		wxASSERT(!m_metaGuid.isValid());
		if (!m_metaGuid.isValid()) {
			ResetGuid();
		}
	}

	inline bool CompareId(const meta_identifier_t& id) const { return m_metaId == id; }
	inline bool CompareGuid(const CGuid& guid) const { return m_metaGuid == guid; }

	operator meta_identifier_t() const { return m_metaId; }

	IBackendMetadataTree* GetMetaDataTree() const;

public:

	bool IsAllowed() const {
		return IsEnabled()
			&& !IsDeleted();
	}

	bool IsEnabled() const {
		return (m_metaFlags & metaDisableFlag) == 0;
	}

	bool IsDeleted() const {
		return (m_metaFlags & metaDeletedFlag) != 0;
	}

public:

	void MarkAsDeleted() {
		m_metaFlags |= metaDeletedFlag;
	}

public:

	void SetFlag(int flag) {
		m_metaFlags |= flag;
	}

	void ClearFlag(int flag) {
		m_metaFlags &= ~(flag);
	}

public:

	bool BuildNewName();

	IMetaObject(
		const wxString& strName = wxEmptyString,
		const wxString& synonym = wxEmptyString,
		const wxString& comment = wxEmptyString
	);

	virtual ~IMetaObject();

	//system override 
	virtual int GetComponentType() const final { return COMPONENT_TYPE_METADATA; }

	virtual wxString GetClassName() const final { return CValue::GetClassName(); }
	virtual wxString GetObjectTypeName() const final { return CValue::GetClassName(); }

	CGuid GetGuid() const { return m_metaGuid; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool IsCopyMode() const {
		return m_metaCopyGuid.isValid();
	}

	bool IsPasteMode() const {
		return m_metaPasteGuid.isValid();
	}

	void SetCommonGuid(const CGuid& guid) {
		m_metaCopyGuid.reset(); m_metaPasteGuid.reset(); m_metaGuid = guid;
	}

	CGuid GetCommonGuid() const {

		if (m_metaPasteGuid.isValid())
			return m_metaPasteGuid;

		if (m_metaCopyGuid.isValid())
			return m_metaCopyGuid;

		return m_metaGuid;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SetCopyGuid(const CGuid& guid) const { m_metaCopyGuid = guid; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	wxString GetFileName() const;
	wxString GetFullName() const;

	wxString GetModuleName() const;
	wxString GetDocPath() const { return m_metaGuid.str(); }

	//filter children element
	virtual bool FilterChild(const class_identifier_t& clsid) const { return false; }

	//process choice 
	virtual bool ProcessChoice(IBackendControlFrame* ownerValue,
		const wxString& strFormName, enum eSelectMode selMode) {
		return true;
	}

	//find child by guid 
	IMetaObject* FindChildByGuid(const CGuid& guid) const {
	
		for (const auto object : m_children) {
			if (object->CompareGuid(guid))
				return object;
		}
		
		return nullptr;
	}

	//methods 
	virtual CMethodHelper* GetPMethods() const { // get a reference to the class helper for parsing attribute and method names
		//PrepareNames();
		return m_methodHelper;
	}

	virtual void PrepareNames() const; // this method is automatically called to initialize attribute and method names.

	//attributes 
	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	//support icons
	virtual wxIcon GetIcon() const { return wxNullIcon; }
	static wxIcon GetIconGroup() { return wxNullIcon; }

	//load & save object in metaObject 
	bool LoadMeta(CMemoryReader& dataReader);
	bool SaveMeta(CMemoryWriter& dataWritter = CMemoryWriter());

	//load & save object
	bool LoadMetaObject(IMetaData* metaData, CMemoryReader& dataReader);
	bool SaveMetaObject(IMetaData* metaData, CMemoryWriter& dataWritter, int flags = defaultFlag);
	bool DeleteMetaObject(IMetaData* metaData);

	// save & delete object in DB 
	bool CreateMetaTable(IMetaDataConfiguration* srcMetaData, int flags = createMetaTable);
	bool UpdateMetaTable(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject);
	bool DeleteMetaTable(IMetaDataConfiguration* srcMetaData);

	//events: 
	virtual bool OnCreateMetaObject(IMetaData* metaData, int flags);
	virtual bool OnLoadMetaObject(IMetaData* metaData);
	virtual bool OnSaveMetaObject(int flags) { return true; }
	virtual bool OnDeleteMetaObject();
	virtual bool OnRenameMetaObject(const wxString& sNewName) { return true; }

	//for designer 
	virtual bool OnReloadMetaObject() { return true; }

	//module manager is started or exit 
	//after and before for designer 
	virtual bool OnBeforeRunMetaObject(int flags) { return true; }
	virtual bool OnAfterRunMetaObject(int flags) { return true; }

	virtual bool OnBeforeCloseMetaObject() { return true; }
	virtual bool OnAfterCloseMetaObject();

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu) { return false; }
	virtual void ProcessCommand(unsigned int id) {}

	//check is empty
	virtual inline bool IsEmpty() const { return false; }

	virtual bool Init() final override;
	virtual bool Init(CValue** paParams, const long lSizeArray) final override;

	//Is editable object? 
	virtual bool IsEditable() const;

	//compare object 
	virtual bool CompareObject(IMetaObject* metaObject) const;

	/**
	* Property events
	*/
	virtual void OnPropertyCreated(IProperty* property);
	virtual void OnPropertySelected(IProperty* property);
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue);
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue);

	/**
	* Devuelve la posicion del hijo o GetChildCount() en caso de no encontrarlo
	*/
	bool ChangeChildPosition(IMetaObject* obj, unsigned int pos);

	//copy & paste object 
	bool CopyObject(CMemoryWriter& writer) const;
	bool PasteObject(CMemoryReader& reader);

protected:

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags) { return true; }

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader) { return true; }
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter()) { return true; }
	virtual bool DeleteData() { return true; }

protected:

	template<typename T, typename... Args>
	T* CreateMetaObjectAndSetParent(Args&&... args) {
		T* createdObject = CValue::CreateAndConvertObjectValueRef<T>(args...);
		wxASSERT(createdObject);
		//set child/parent
		createdObject->SetParent(this);
		this->AddChild(createdObject);
		return createdObject;
	}

#pragma region interface_h
	virtual void DoSetInterface(const meta_identifier_t& id, const bool& val = true);
#pragma endregion

#pragma region role_h
	virtual void DoSetRight(const CRole* role, const bool& val = true);
#pragma endregion

protected:

	mutable CGuid m_metaCopyGuid, m_metaPasteGuid;

	int m_metaFlags;
	meta_identifier_t m_metaId;			//type id (default is undefined)
	CGuid m_metaGuid;

	IMetaData* m_metaData;
	CMethodHelper* m_methodHelper;
};

extern BACKEND_API CRestructureInfo s_restructureInfo;

#endif