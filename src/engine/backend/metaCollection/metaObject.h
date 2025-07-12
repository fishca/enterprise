#ifndef __META_OBJECT_H__
#define __META_OBJECT_H__

#include "backend/propertyManager/propertyManager.h"
#include "backend/metaCtor.h"
#include "backend/backend_metatree.h"
#include "backend/role.h"

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
const class_identifier_t g_metaInterfaceCLSID = string_to_clsid("MD_INTF");

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
const class_identifier_t g_metaDefaultAttributeCLSID = string_to_clsid("MD_DATT");

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
//*                             IMetaObject                                     *
//*******************************************************************************

#define defaultMetaID 1000

//flags metaobject event 
enum metaObjectFlags {

	defaultFlag = 0x0000,

	onlyLoadFlag = 0x0001,
	saveConfigFlag = 0x0002,
	saveToFileFlag = 0x0004,
	forceCloseFlag = 0x0008,
	forceRunFlag = 0x0010,
	newObjectFlag = 0x0020,

	copyObjectFlag = 0x0040,
	pasteObjectFlag = 0x0080,
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
	public IPropertyObject, public CValue {
	wxDECLARE_ABSTRACT_CLASS(IMetaObject);
protected:

	template <typename... Args>
	inline Role* CreateRole(Args&&... args) {
		return new Role(this, std::forward<Args>(args)...);
	}

protected:

	CPropertyCategory* m_categoryCommon = IPropertyObject::CreatePropertyCategory(wxT("common"), _("common"));
	CPropertyName* m_propertyName = IPropertyObject::CreateProperty<CPropertyName>(m_categoryCommon, wxT("name"), _("name"), wxEmptyString);
	CPropertyCaption* m_propertySynonym = IPropertyObject::CreateProperty<CPropertyCaption>(m_categoryCommon, wxT("synonym"), _("synonym"), wxEmptyString);
	CPropertyString* m_propertyComment = IPropertyObject::CreateProperty<CPropertyString>(m_categoryCommon, wxT("comment"), _("comment"), wxEmptyString);

	CPropertyCategory* m_categorySecondary = IPropertyObject::CreatePropertyCategory(wxT("secondary"), _("secondary"));

protected:

	std::vector<std::pair<wxString, Role*>>& GetRoles() {
		return m_roles;
	}

	friend class Role;

	/**
	* Añade una propiedad al objeto.
	*
	* Este método será usado por el registro de descriptores para crear la
	* instancia del objeto.
	* Los objetos siempre se crearán a través del registro de descriptores.
	*/
	void AddRole(Role* value);

public:

	meta_identifier_t GetMetaID() const { return m_metaId; }
	void SetMetaID(const meta_identifier_t& id) { m_metaId = id; }

	wxString GetName() const { return m_propertyName->GetValueAsString(); }
	void SetName(const wxString& strName) { m_propertyName->SetValue(strName); }

	wxString GetSynonym() const { return m_propertySynonym->IsEmptyProperty() ? GetName() : m_propertySynonym->GetValueAsString(); }
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

#pragma region role 

	bool AccessRight(const Role* role) const {
		return AccessRight(role->GetName());
	}
	bool AccessRight(const wxString& roleName) const {
		return true;
	}
	bool AccessRight(const Role* role, const wxString& userName) const {
		return AccessRight(role->GetName(), userName);
	}
	bool AccessRight(const wxString& roleName, const wxString& userName) const {
		return AccessRight(roleName);
	}
	bool AccessRight(const Role* role, const meta_identifier_t& id) const;
	bool AccessRight(const wxString& roleName, const meta_identifier_t& id) const {
		if (roleName.IsEmpty())
			return false;
		auto& it = std::find_if(m_roles.begin(), m_roles.end(), [roleName](const std::pair<wxString, Role*>& pair)
			{
				return roleName == pair.first;
			}
		);
		if (it == m_roles.end())
			return false;
		return AccessRight(it->second, id);
	}

	bool SetRight(const Role* role, const meta_identifier_t& id, const bool& val);
	bool SetRight(const wxString& roleName, const meta_identifier_t& id, const bool& val) {
		if (roleName.IsEmpty())
			return false;
		auto& it = std::find_if(m_roles.begin(), m_roles.end(), [roleName](const std::pair<wxString, Role*>& pair)
			{
				return roleName == pair.first;
			}
		);
		if (it == m_roles.end())
			return false;
		return SetRight(it->second, id, val);
	}

	/**
	* Obtiene la propiedad identificada por el nombre.
	*
	* @note Notar que no existe el método SetProperty, ya que la modificación
	*       se hace a través de la referencia.
	*/
	Role* GetRole(const wxString& nameParam) const;

	/**
	* Obtiene el número de propiedades del objeto.
	*/
	unsigned int GetRoleCount() const {
		return (unsigned int)m_roles.size();
	}

	Role* GetRole(unsigned int idx) const; // throws ...;

#pragma endregion

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

	Guid GetGuid() const { return m_metaGuid; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	bool IsCopyMode() const {
		return m_metaCopyGuid.isValid();
	}

	bool IsPasteMode() const {
		return m_metaPasteGuid.isValid();
	}

	void SetCommonGuid(const Guid& guid) {
		m_metaCopyGuid.reset(); m_metaPasteGuid.reset(); m_metaGuid = guid;
	}

	Guid GetCommonGuid() const {

		if (m_metaPasteGuid.isValid())
			return m_metaPasteGuid;

		if (m_metaCopyGuid.isValid())
			return m_metaCopyGuid;

		return m_metaGuid;
	}

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	void SetCopyGuid(const Guid& guid) const { m_metaCopyGuid = guid; }

	////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	wxString GetFileName() const;
	wxString GetFullName() const;

	wxString GetModuleName() const;
	wxString GetDocPath() const;

	//create in this metaObject 
	bool AppendChild(IMetaObject* child) {
		if (!FilterChild(child->GetClassType()))
			return false;
		m_listMetaObject.push_back(child);
		return true;
	}

	void RemoveChild(IMetaObject* child) {
		auto it = std::find(m_listMetaObject.begin(), m_listMetaObject.end(), child);
		if (it != m_listMetaObject.end())
			m_listMetaObject.erase(it);
	}

	virtual bool FilterChild(const class_identifier_t& clsid) const {
		return false;
	}

	//process choice 
	virtual bool ProcessChoice(IBackendControlFrame* ownerValue,
		const meta_identifier_t& id, enum eSelectMode selMode) {
		return true;
	}

	std::vector<IMetaObject*> GetObjects() const {
		return m_listMetaObject;
	}

	std::vector<IMetaObject*> GetObjects(const class_identifier_t& clsid) const {
		std::vector<IMetaObject*> metaObjects;
		for (auto& obj : m_listMetaObject) {
			if (clsid == obj->GetClassType()) {
				metaObjects.push_back(obj);
			}
		}
		return metaObjects;
	}

	IMetaObject* FindByName(const wxString& strDocPath) const;
	IMetaObject* FindByName(const class_identifier_t& clsid, const wxString& strDocPath) const;

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
	virtual bool OnSaveMetaObject() { return true; }
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

	// Gets the parent object
	template <typename retType = IMetaObject >
	inline retType* GetParent() const {
		return wxDynamicCast(m_parent, retType);
	}

	/**
	* Obtiene un hijo del objeto.
	*/
	IMetaObject* GetChild(unsigned int idx) const {
		return wxDynamicCast(IPropertyObject::GetChild(idx), IMetaObject);
	}

	IMetaObject* GetChild(unsigned int idx, const wxString& type) const {
		return wxDynamicCast(IPropertyObject::GetChild(idx, type), IMetaObject);
	}

	//check is empty
	virtual inline bool IsEmpty() const {
		return false;
	}

	virtual bool Init() final override;
	virtual bool Init(CValue** paParams, const long lSizeArray) final override;

	//compare object 
	virtual bool CompareObject(IMetaObject* metaObject) const {
		if (GetClassType() != metaObject->GetClassType())
			return false;
		if (m_metaId != metaObject->GetMetaID())
			return false;
		for (unsigned int idx = 0; idx < GetPropertyCount(); idx++) {
			IProperty* propDst = GetProperty(idx);
			wxASSERT(propDst);
			IProperty* propSrc = metaObject->GetProperty(propDst->GetName());
			if (propSrc == nullptr)
				return false;
			if (propDst->GetValue() != propSrc->GetValue())
				return false;
		}
		return true;
	}

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
	virtual bool ChangeChildPosition(IPropertyObject* obj, unsigned int pos);

	//copy & paste object 
	bool CopyObject(CMemoryWriter& writer) const;
	bool PasteObject(CMemoryReader& reader);

protected:

	//load & save event in control 
	virtual bool LoadRole(CMemoryReader& reader);
	virtual bool SaveRole(CMemoryWriter& writer = CMemoryWriter());

	//create and update table 
	virtual bool CreateAndUpdateTableDB(IMetaDataConfiguration* srcMetaData, IMetaObject* srcMetaObject, int flags) { return true; }

	//load & save metaData from DB 
	virtual bool LoadData(CMemoryReader& reader) { return true; }
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter()) { return true; }
	virtual bool DeleteData() { return true; }

	//copy & paste property 
	bool CopyProperty(CMemoryWriter& writer) const;
	bool PasteProperty(CMemoryReader& reader);

protected:

	template<typename retType, typename convType = retType>
	class CMetaVector {
		std::vector<retType*> m_listObject;
	public:
		//////////////////////////////////////////////////////////////////////////
		CMetaVector(const IMetaObject* metaItem, const class_identifier_t& clsid) {
			m_listObject.reserve(metaItem->m_listMetaObject.size());
			std::transform(
				metaItem->m_listMetaObject.begin(),
				metaItem->m_listMetaObject.end(),
				std::back_inserter(m_listObject),
				[](auto const& obj)
				{
					return dynamic_cast<convType*>(obj);
				}
			);
			m_listObject.erase(
				std::remove_if(m_listObject.begin(), m_listObject.end(), [clsid](auto const& obj)
					{
						if (obj != nullptr) return clsid != obj->GetClassType(); return obj == nullptr;
					}), m_listObject.end()
						);
		}
		CMetaVector(const std::vector<IMetaObject*>& listObject) {
			m_listObject.reserve(listObject.size());
			std::transform(
				std::begin(listObject),
				std::end(listObject),
				std::back_inserter(m_listObject),
				[](auto const& obj)
				{
					return dynamic_cast<convType*>(obj);
				}
			);
			m_listObject.erase(
				std::remove_if(m_listObject.begin(), m_listObject.end(), [](auto const& obj) { return obj == nullptr; }), m_listObject.end()
			);
		}
		//////////////////////////////////////////////////////////////////////////
		CMetaVector(const CMetaVector& src) :
			m_listObject(std::move(src.m_listObject)) {
		}
		//////////////////////////////////////////////////////////////////////////
		auto begin() { return m_listObject.begin(); }
		auto end() { return m_listObject.end(); }
		auto cbegin() const { return m_listObject.cbegin(); }
		auto cend() const { returnm_listObject.cend(); }
		//////////////////////////////////////////////////////////////////////////
		std::vector<retType*> GetVector() {
			return m_listObject;
		}
		//////////////////////////////////////////////////////////////////////////
		operator std::vector<retType*>() {
			return m_listObject;
		}
	};

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

protected:

	mutable Guid m_metaCopyGuid, m_metaPasteGuid;

	int m_metaFlags;
	meta_identifier_t m_metaId;			//type id (default is undefined)
	Guid m_metaGuid;

	IMetaData* m_metaData;

	std::vector<std::pair<wxString, Role*>> m_roles;
	std::map<meta_identifier_t,
		std::map<wxString, bool>
	> m_valRoles;

	std::vector<IMetaObject*> m_listMetaObject;

	CMethodHelper* m_methodHelper;
};
#endif