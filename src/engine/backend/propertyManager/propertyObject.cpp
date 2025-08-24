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
	wxDELETE(m_category);

	for (auto& property : m_properties)
		wxDELETE(property.second);

	for (auto& event : m_events)
		wxDELETE(event.second);

	IPropertyObject* pobj(this);

	if (backend_mainFrame != nullptr && pobj == backend_mainFrame->GetProperty()) {
		backend_mainFrame->SetProperty(nullptr);
	}

	//LogDebug(wxT("delete IPropertyObject"));
}

wxString IPropertyObject::GetIndentString(int indent) const
{
	wxString s;
	for (int i = 0; i < indent; i++) s += wxT(" ");
	return s;
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
	m_properties.emplace(std::map<wxString, IProperty*>::value_type(prop->GetName(), prop));
}

void IPropertyObject::AddEvent(IEvent* event)
{
	m_events.emplace(std::map<wxString, IEvent*>::value_type(event->GetName(), event));
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

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void CPropertyCategory::AddProperty(IProperty* property)
{
	m_properties.emplace_back(property->GetName());
}

void CPropertyCategory::AddEvent(IEvent* event)
{
	m_events.emplace_back(event->GetName());
}

void CPropertyCategory::AddCategory(CPropertyCategory* cat)
{
	m_categories.emplace_back(cat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////