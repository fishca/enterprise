////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuider-team
//	Description : base object for property objects
////////////////////////////////////////////////////////////////////////////

#include "propertyObject.h"

bool IBackendCellField::IsEditable() const
{
	return m_owner ? m_owner->IsEditable() : false;
}

///////////////////////////////////////////////////////////////////////////////

void IProperty::InitProperty(CPropertyCategory* cat, const wxVariant& value)
{
	m_owner->AddProperty(this);
	if (cat != nullptr) cat->AddProperty(this);
	DoSetValue(value);
}

void IEvent::InitEvent(CPropertyCategory* cat, const wxVariant& value)
{
	m_owner->AddEvent(this);
	if (cat != nullptr) cat->AddEvent(this);
	DoSetValue(value);
}

///////////////////////////////////////////////////////////////////////////////

bool IProperty::PasteData(CMemoryReader& reader)
{
	if (!LoadData(reader))
		return false;
	m_owner->OnPropertyPasted(this);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

#include "backend/backend_mainFrame.h"

IPropertyObject::~IPropertyObject()
{
	//wxDELETE(m_category);

	//for (auto& property : m_properties)
	//	wxDELETE(property.second);

	//for (auto& event : m_events)
	//	wxDELETE(event.second);

	IPropertyObject* pobj(GetThis());

	if (backend_mainFrame != nullptr && pobj == backend_mainFrame->GetProperty()) {
		backend_mainFrame->SetProperty(nullptr);
	}

	// remove the reference in the parent
	if (m_parent != nullptr)
		m_parent->RemoveChild(pobj);

	for (auto& child : m_children)
		child->SetParent(nullptr);

	//LogDebug(wxT("delete IPropertyObject"));
}

wxString IPropertyObject::GetIndentString(int indent) const
{
	wxString s;
	for (int i = 0; i < indent; i++) s += wxT(" ");
	return s;
}

unsigned int IPropertyObject::GetParentPosition() const
{
	if (m_parent == nullptr)
		return 0;
	unsigned int pos = 0;
	while (pos < m_parent->GetChildCount() && m_parent->m_children[pos] != this)
		pos++;
	return pos;
}

IProperty* IPropertyObject::GetProperty(const wxString& nameParam) const
{
	std::map<wxString, IProperty*>::const_iterator it = std::find_if(m_properties.begin(), m_properties.end(),
		[nameParam](const std::pair<wxString, IProperty*>& pair) {
			return stringUtils::CompareString(nameParam, pair.first);
		}
	);

	if (it != m_properties.end())
		return it->second;

	//LogDebug(wxT("[IPropertyObject::GetProperty] IProperty %s not found!"),name.c_str());
	  // este aserto falla siempre que se crea un sizerItem
	  // assert(false);

	return nullptr;
}

IProperty* IPropertyObject::GetProperty(unsigned int idx) const
{
	assert(idx < m_properties.size());

	std::map<wxString, IProperty*>::const_iterator it = m_properties.begin();
	unsigned int i = 0;
	while (i < idx && it != m_properties.end()) {
		i++;
		it++;
	}

	if (it != m_properties.end())
		return it->second;

	return nullptr;
}

IEvent* IPropertyObject::GetEvent(const wxString& nameParam) const
{
	std::map<wxString, IEvent*>::const_iterator it = std::find_if(m_events.begin(), m_events.end(),
		[nameParam](const std::pair<wxString, IEvent*>& pair) {
			return stringUtils::CompareString(nameParam, pair.first);
		}
	);

	if (it != m_events.end())
		return it->second;

	//LogDebug("[IPropertyObject::GetEvent] IEvent " + name + " not found!");
	return nullptr;
}

IEvent* IPropertyObject::GetEvent(unsigned int idx) const
{
	assert(idx < m_events.size());

	std::map<wxString, IEvent*>::const_iterator it = m_events.begin();
	unsigned int i = 0;
	while (i < idx && it != m_events.end()) {
		i++; it++;
	}

	if (it != m_events.end())
		return it->second;

	return nullptr;
}

void IPropertyObject::AddProperty(IProperty* prop)
{
	m_properties.insert(std::map<wxString, IProperty*>::value_type(prop->GetName(), prop));
}

void IPropertyObject::AddEvent(IEvent* event)
{
	m_events.insert(std::map<wxString, IEvent*>::value_type(event->GetName(), event));
}

IPropertyObject* IPropertyObject::FindNearAncestor(const wxString& type) const
{
	IPropertyObject* result = nullptr;
	IPropertyObject* parent = GetParent();
	if (parent != nullptr) {
		if (stringUtils::CompareString(parent->GetObjectTypeName(), type))
			result = parent;
		else
			result = parent->FindNearAncestor(type);
	}

	return result;
}

IPropertyObject* IPropertyObject::FindNearAncestorByBaseClass(const wxString& type) const
{
	IPropertyObject* result = nullptr;
	IPropertyObject* parent = GetParent();
	if (parent != nullptr) {
		if (stringUtils::CompareString(parent->GetObjectTypeName(), type))
			result = parent;
		else
			result = parent->FindNearAncestorByBaseClass(type);
	}

	return result;
}

bool IPropertyObject::AddChild(IPropertyObject* obj)
{
	m_children.push_back(obj);
	return true;
}

bool IPropertyObject::AddChild(unsigned int idx, IPropertyObject* obj)
{
	m_children.insert(m_children.begin() + idx, obj);
	return true;
}

void IPropertyObject::RemoveChild(IPropertyObject* obj)
{
	std::vector< IPropertyObject* >::iterator it = m_children.begin();
	while (it != m_children.end() && *it != obj)
		it++;

	if (it != m_children.end())
		m_children.erase(it);
}

void IPropertyObject::RemoveChild(unsigned int idx)
{
	assert(idx < m_children.size());

	std::vector< IPropertyObject* >::iterator it = m_children.begin() + idx;
	m_children.erase(it);
}

IPropertyObject* IPropertyObject::GetChild(unsigned int idx) const
{
	assert(idx < m_children.size());
	return m_children[idx];
}

IPropertyObject* IPropertyObject::GetChild(unsigned int idx, const wxString& type) const
{
	unsigned int cnt = 0;
	for (std::vector< IPropertyObject* >::const_iterator it = m_children.begin(); it != m_children.end(); ++it) {
		if (stringUtils::CompareString((*it)->GetObjectTypeName(), type) && ++cnt == idx)
			return *it;
	}
	return nullptr;
}

unsigned int IPropertyObject::GetPropertyIndex(const wxString& nameParam) const {
	return std::distance(m_properties.begin(),
		std::find_if(m_properties.begin(), m_properties.end(),
			[nameParam](const  std::pair<wxString, IProperty*>& pair) {
				return stringUtils::CompareString(nameParam, pair.first);
			}
		)
	);
}

unsigned int IPropertyObject::GetChildPosition(IPropertyObject* obj) const
{
	unsigned int pos = 0;
	while (pos < GetChildCount() && m_children[pos] != obj)
		pos++;
	return pos;
}

bool IPropertyObject::ChangeChildPosition(IPropertyObject* obj, unsigned int pos)
{
	unsigned int obj_pos = GetChildPosition(obj);

	if (obj_pos == GetChildCount() || pos >= GetChildCount())
		return false;

	if (pos == obj_pos)
		return true;

	// Procesamos el cambio de posición
	RemoveChild(obj);
	AddChild(pos, obj);
	return true;
}

bool IPropertyObject::IsSubclassOf(const wxString& clsName) const
{
	bool found = false;

	if (stringUtils::CompareString(clsName, GetClassName())) {
		found = true;
	}
	else {
		IPropertyObject* parent = GetParent();
		while (parent) {
			found = parent->IsSubclassOf(clsName);
			if (found)
				break;
			else
				parent = parent->GetParent();
		}
	}

	return found;
}

void IPropertyObject::DeleteRecursive()
{
	for (auto objChild : m_children) {
		objChild->SetParent(nullptr);
		objChild->DeleteRecursive();
		wxDELETE(objChild);
	}
	m_children.clear();
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CPropertyCategory::AddProperty(IProperty* property)
{
	m_properties.push_back(property->GetName());
}

void CPropertyCategory::AddEvent(IEvent* event)
{
	m_events.push_back(event->GetName());
}

void CPropertyCategory::AddCategory(CPropertyCategory* cat)
{
	m_categories.push_back(cat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////