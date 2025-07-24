#ifndef __PROPERTY_OBJECT_H__
#define __PROPERTY_OBJECT_H__

#include "backend/backend_core.h"

///////////////////////////////////////////////////////////////////////////////

class BACKEND_API IPropertyObject;

///////////////////////////////////////////////////////////////////////////////

class BACKEND_API IProperty;
class BACKEND_API IEvent;

///////////////////////////////////////////////////////////////////////////////

class BACKEND_API IMetaData;

///////////////////////////////////////////////////////////////////////////////

#include "backend/fileSystem/fs.h"

///////////////////////////////////////////////////////////////////////////////

class BACKEND_API CPropertyCategory final {
	wxString m_catName;
	wxString m_catLabel;
	wxString m_catHelp;
	std::vector<wxString> m_properties;
	std::vector<wxString> m_events;
	std::vector< CPropertyCategory* > m_categories;
	IPropertyObject* m_owner;

private:
	CPropertyCategory(IPropertyObject* object) :
		m_catName("propertyEvent"),
		m_catLabel(_("property and event")),
		m_catHelp(wxEmptyString),
		m_owner(object)
	{
	}
	CPropertyCategory(const wxString& name, IPropertyObject* object, CPropertyCategory* ownerCat = nullptr) :
		m_catName(name),
		m_catLabel(wxEmptyString),
		m_catHelp(wxEmptyString),
		m_owner(object)
	{
		if (ownerCat != nullptr) ownerCat->AddCategory(this);
	}
	CPropertyCategory(const wxString& name, const wxString& label, IPropertyObject* object, CPropertyCategory* ownerCat = nullptr) :
		m_catName(name),
		m_catLabel(label),
		m_catHelp(wxEmptyString),
		m_owner(object)
	{
		if (ownerCat != nullptr) ownerCat->AddCategory(this);
	}
	CPropertyCategory(const wxString& name, const wxString& label, const wxString& helpString, IPropertyObject* object, CPropertyCategory* ownerCat = nullptr) :
		m_catName(name),
		m_catLabel(label),
		m_catHelp(helpString),
		m_owner(object)
	{
		if (ownerCat != nullptr) ownerCat->AddCategory(this);
	}
public:

	~CPropertyCategory() {
		for (auto& category : m_categories) wxDELETE(category);
	}

	void AddProperty(IProperty* property);
	void AddEvent(IEvent* event);
	void AddCategory(CPropertyCategory* cat);

	wxString GetName() const {
		return m_catName;
	}

	wxString GetLabel() const {
		return (m_catLabel.IsEmpty()) ? m_catName : m_catLabel;
	}

	wxString GetHelp() const {
		return m_catHelp;
	}

	wxString GetPropertyName(unsigned int index) const {
		if (index < m_properties.size()) return m_properties[index];
		return wxEmptyString;
	}

	wxString GetEventName(unsigned int index) const {
		if (index < m_events.size()) return m_events[index];
		return wxEmptyString;
	}

	CPropertyCategory* GetCategory(unsigned int index) const {
		if (index < m_categories.size()) return m_categories[index];
		return new CPropertyCategory(m_owner);
	}

	IPropertyObject* GetPropertyObject() const {
		return m_owner;
	}

	unsigned int GetPropertyCount() const {
		return m_properties.size();
	}

	unsigned int GetEventCount() const {
		return m_events.size();
	}

	unsigned int GetCategoryCount() const {
		return m_categories.size();
	}

	friend class IPropertyObject;
};

///////////////////////////////////////////////////////////////////////////////

class BACKEND_API IBackendCellField {
protected:
	IBackendCellField(CPropertyCategory* cat, const wxString& name,
		const wxVariant& value) :
		m_propName(name),
		m_propLabel(name),
		m_propHelp(wxEmptyString),
		m_owner(cat->GetPropertyObject())
	{
	}
	IBackendCellField(CPropertyCategory* cat, const wxString& name,
		const wxString& label, const wxVariant& value) :
		m_propName(name),
		m_propLabel(label.IsEmpty() ? name : label),
		m_propHelp(wxEmptyString),
		m_owner(cat->GetPropertyObject())
	{
	}
	IBackendCellField(CPropertyCategory* cat, const wxString& name,
		const wxString& label, const wxString& helpString, const wxVariant& value) :
		m_propName(name),
		m_propLabel(label.IsEmpty() ? name : label),
		m_propHelp(helpString),
		m_owner(cat->GetPropertyObject())
	{
	}
public:

	virtual bool IsOk() const { return !m_propValue.IsNull(); }
	bool IsEditable() const;

	IPropertyObject* GetPropertyObject() const { return m_owner; }

	const wxString& GetName() const { return m_propName; }
	const wxString& GetLabel() const { return m_propLabel; }
	const wxString& GetHelp() const { return m_propHelp; }

	////////////////////

	const wxVariant& GetValue() const { return m_propValue; }

	////////////////////
	void SetValue(const wxVariant& val) { DoSetValue(val); }
	////////////////////

	IBackendCellField& operator =(const wxVariant& val) {
		SetValue(val);
		return *this;
	}

	////////////////////

	template <typename cast_type = wxVariantData>
	inline cast_type* get_cell_variant(const wxVariant& val) const {
		cast_type* ret_type = dynamic_cast<cast_type*>(val.GetRefData());
		wxASSERT(ret_type != nullptr);
		return ret_type;
	}

	template <typename cast_type = wxVariantData>
	inline cast_type* get_cell_variant() const {
		return get_cell_variant<cast_type>(m_propValue);
	}

	////////////////////

	virtual bool IsEmptyProperty() const { return false; }

	//get property for grid 
	virtual wxPGProperty* GetPGProperty() const = 0;
	virtual void RefreshPGProperty(wxPGProperty* pg) {};

	//load & save object in control 
	virtual bool LoadData(CMemoryReader& reader) = 0;
	virtual bool SaveData(CMemoryWriter& writer) = 0;

	//copy & paste object in control 
	virtual bool PasteData(CMemoryReader& reader) { return LoadData(reader); }
	virtual bool CopyData(CMemoryWriter& writer) { return SaveData(writer); }

	//Set/Get property data
	virtual bool SetDataValue(const CValue& varPropVal) = 0;
	virtual bool GetDataValue(CValue& pvarPropVal) const = 0;

protected:
	virtual void DoSetValue(const wxVariant& val) { m_propValue = val; }
protected:

	wxString        m_propName;
	wxString		m_propLabel;
	wxString		m_propHelp;
	IPropertyObject* m_owner; // pointer to the owner object
	wxVariant       m_propValue;
};

class BACKEND_API IProperty : public IBackendCellField {
	void InitProperty(CPropertyCategory* cat, const wxVariant& value = wxNullVariant);
protected:
	IProperty(CPropertyCategory* cat, const wxString& name, const wxVariant& value) : IBackendCellField(cat, name, value) { InitProperty(cat, value); }
	IProperty(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxVariant& value) : IBackendCellField(cat, name, label, value) { InitProperty(cat, value); }
	IProperty(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const wxVariant& value) : IBackendCellField(cat, name, label, helpString, value) { InitProperty(cat, value); }
public:
	virtual bool PasteData(CMemoryReader& reader);
};

class BACKEND_API IEvent : public IBackendCellField {
	void InitEvent(CPropertyCategory* cat, const wxVariant& value = wxNullVariant);
protected:
	IEvent(CPropertyCategory* cat, const wxString& name, const wxString& value) : IBackendCellField(cat, name, value) { InitEvent(cat, value); }
	IEvent(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& value) : IBackendCellField(cat, name, label, value) { InitEvent(cat, value); }
	IEvent(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const wxString& value) : IBackendCellField(cat, name, label, helpString, value) { InitEvent(cat, value); }
	IEvent(CPropertyCategory* cat, const wxString& name, const wxArrayString& args, const wxString& value) : IBackendCellField(cat, name, value), m_args(args) { InitEvent(cat, value); }
	IEvent(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxArrayString& args, const wxString& value) : IBackendCellField(cat, name, label, value), m_args(args) { InitEvent(cat, value); }
	IEvent(CPropertyCategory* cat, const wxString& name, const wxString& label, const wxString& helpString, const wxArrayString& args, const wxString& value) :IBackendCellField(cat, name, label, helpString, value), m_args(args) { InitEvent(cat, value); }
public:
	const wxArrayString& GetArgs() const { return m_args; }
protected:
	wxArrayString m_args;
};

///////////////////////////////////////////////////////////////////////////////

#define property_cast(val, type) dynamic_cast<type*>(val.GetData())

///////////////////////////////////////////////////////////////////////////////

class BACKEND_API IPropertyObject {
protected:

	template <typename typeProp, typename... Args>
	inline typeProp* CreateProperty(CPropertyCategory* cat, Args&&... args) {
		return new typeProp(cat, std::forward<Args>(args)...);
	}
	template <typename typeEvent, typename... Args>
	inline typeEvent* CreateEvent(CPropertyCategory* cat, Args&&... args) {
		return new typeEvent(cat, std::forward<Args>(args)...);
	}
	inline CPropertyCategory* CreatePropertyCategory(const wxString& catName) {
		return new CPropertyCategory(catName, this, m_category);
	}
	inline CPropertyCategory* CreatePropertyCategory(const wxString& catName, const wxString& catLabel) {
		return new CPropertyCategory(catName, catLabel, this, m_category);
	}
	inline CPropertyCategory* CreatePropertyCategory(const wxString& catName, const wxString& catLabel, const wxString& catHelp) {
		return new CPropertyCategory(catName, catLabel, catHelp, this, m_category);
	}
	inline CPropertyCategory* CreatePropertyCategory(CPropertyCategory* ownerCat, const wxString& catName) {
		return new CPropertyCategory(catName, this, ownerCat);
	}
	inline CPropertyCategory* CreatePropertyCategory(CPropertyCategory* ownerCat, const wxString& catName, const wxString& catLabel) {
		return new CPropertyCategory(catName, catLabel, this, ownerCat);
	}
	inline CPropertyCategory* CreatePropertyCategory(CPropertyCategory* ownerCat, const wxString& catName, const wxString& catLabel, const wxString& catHelp) {
		return new CPropertyCategory(catName, catLabel, catHelp, this, ownerCat);
	}

protected:

	wxString GetIndentString(int indent) const; // obtiene la cadena con el indentado

	std::vector<IPropertyObject*>& GetChildren() { return m_children; }
	std::map<wxString, IProperty*>& GetProperties() { return m_properties; }

	// devuelve el puntero "this"
	IPropertyObject* GetThis() { return this; }

	IPropertyObject() : m_parent(nullptr) { m_category = new CPropertyCategory(this); }

	friend class IProperty;
	friend class IEvent;

	/**
	* Añade una propiedad al objeto.
	*
	* Este método será usado por el registro de descriptores para crear la
	* instancia del objeto.
	* Los objetos siempre se crearán a través del registro de descriptores.
	*/
	void AddProperty(IProperty* property);
	void AddEvent(IEvent* event);

public:

	virtual ~IPropertyObject();

	virtual bool IsEditable() const { return m_propEnabled; }
	virtual void SetReadOnly(bool enabled = true) { m_propEnabled = enabled; }

	/**
	* Obtiene el nombre del objeto.
	*
	* @note No confundir con la propiedad nombre que tienen algunos objetos.
	*       Cada objeto tiene un nombre, el cual será el mismo que el usado
	*       como clave en el registro de descriptores.
	*/
	virtual wxString GetClassName() const = 0;

	/// Gets the parent object
	virtual IPropertyObject* GetParent() const { return m_parent; }

	/// Links the object to a parent
	virtual void SetParent(IPropertyObject* parent) { m_parent = parent; }

	/**
	* Devuelve la posicion del hijo o GetParentPosition() en caso de no encontrarlo
	*/
	virtual unsigned int GetParentPosition() const;

	/**
	* Obtiene la propiedad identificada por el nombre.
	*
	* @note Notar que no existe el método SetProperty, ya que la modificación
	*       se hace a través de la referencia.
	*/
	IProperty* GetProperty(const wxString& nameParam) const;
	IEvent* GetEvent(const wxString& nameParam) const;

	/**
	* Obtiene el número de propiedades del objeto.
	*/
	unsigned int GetPropertyCount() const { return (unsigned int)m_properties.size(); }

	unsigned int GetEventCount() const { return m_events.size(); }

	IProperty* GetProperty(unsigned int idx) const; // throws ...;
	IEvent* GetEvent(unsigned int idx) const; // throws ...;

	/**
	* Devuelve el primer antecesor cuyo tipo coincida con el que se pasa
	* como parámetro.
	*
	* Será útil para encontrar el widget padre.
	*/
	IPropertyObject* FindNearAncestor(const wxString& type) const;
	IPropertyObject* FindNearAncestorByBaseClass(const wxString& type) const;

	/**
	* Añade un hijo al objeto.
	* Esta función es virtual, debido a que puede variar el comportamiento
	* según el tipo de objeto.
	*
	* @return true si se añadió el hijo con éxito y false en caso contrario.
	*/
	virtual bool AddChild(IPropertyObject*);
	virtual bool AddChild(unsigned int idx, IPropertyObject* obj);

	/**
	* Devuelve la posicion del hijo o GetChildCount() en caso de no encontrarlo
	*/
	virtual unsigned int GetChildPosition(IPropertyObject* obj) const;
	virtual bool ChangeChildPosition(IPropertyObject* obj, unsigned int pos);

	/**
	* Elimina un hijo del objeto.
	*/
	virtual void RemoveChild(IPropertyObject* obj);
	virtual void RemoveChild(unsigned int idx);

	virtual void RemoveAllChildren() { m_children.clear(); }

	/**
	* Obtiene un hijo del objeto.
	*/
	IPropertyObject* GetChild(unsigned int idx) const;
	IPropertyObject* GetChild(unsigned int idx, const wxString& type) const;

	/**
	* Obtiene el número de hijos del objeto.
	*/
	unsigned int GetChildCount() const { return (unsigned int)m_children.size(); }

	unsigned int GetPropertyIndex(const wxString& nameParam) const;

	IProperty* GetPropertyByIndex(unsigned int idx) const {
		if (m_properties.size() < idx)
			return nullptr;
		auto properties_iterator = m_properties.begin();
		std::advance(properties_iterator, idx);
		return (*properties_iterator).second;
	}

	/**
	* Devuelve el tipo de objeto.
	*
	* Deberá ser redefinida en cada clase derivada.
	*/
	virtual wxString GetObjectTypeName() const = 0;

	/**
	* Devuelve la profundidad  del objeto en el arbol.
	*/
	virtual int GetComponentType() const = 0;

	/**
	* IBackendCellField events
	*/

	virtual void OnCellChanged(IBackendCellField* cell, 
		const wxVariant& oldValue, const wxVariant& newValue) {}

	/**
	* IProperty events
	*/
	virtual void OnPropertyCreated() {}
	virtual void OnPropertyCreated(IProperty* property) {}
	virtual void OnPropertyRefresh(class wxPropertyGridManager* pg, class wxPGProperty* pgProperty, IProperty* property) {}
	virtual void OnPropertySelected(IProperty* property) {}	
	virtual bool OnPropertyChanging(IProperty* property, const wxVariant& newValue) { return true; }
	virtual void OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue) {}

	//paste property from data
	virtual void OnPropertyPasted(IProperty* property) {}

	/**
	* IEvent events
	*/
	virtual void OnEventCreated() {}
	virtual void OnEventCreated(IEvent* event) {}
	virtual void OnEventRefresh(class wxPropertyGridManager* pg, class wxPGProperty* pgProperty, IEvent* event) {}
	virtual void OnEventSelected(IEvent* event) {}
	virtual bool OnEventChanging(IEvent* event, const wxVariant& newValue) { return true; }
	virtual void OnEventChanged(IEvent* property, const wxVariant& oldValue, const wxVariant& newValue) {}

	/**
	* Comprueba si el tipo es derivado del que se pasa como parámetro.
	*/

	bool IsSubclassOf(const wxString& clsName) const;

	CPropertyCategory* GetCategory() const { return m_category; }
	IPropertyObject* GetChildPtr(unsigned int idx) const { return GetChild(idx); }

	virtual void DeleteRecursive();

	// get metaData from object
	//virtual IMetaData* GetMetaData() const { return nullptr; }

protected:

	bool m_propEnabled = true;
	IPropertyObject* m_parent = nullptr;
	CPropertyCategory* m_category;
	std::vector<IPropertyObject*> m_children;

private:

	std::map<wxString, IProperty*> m_properties;
	std::map<wxString, IEvent*> m_events;
};

#include "backend/compiler/value.h"

#endif