////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuider-team
//	Description : base object for property objects
////////////////////////////////////////////////////////////////////////////

#include "propertyObject.h"

#define propBlock 0x00023456
#define eventBlock 0x00023457

////////////////////////////////////////////////////////////////////////

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
	return LoadData(reader);
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

	if (idx < m_properties.size()) {
		std::map<wxString, IProperty*>::const_iterator iterator = m_properties.begin();
		std::advance(iterator, idx);
		return iterator->second;
	}

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

bool IPropertyObject::PasteProperty(CMemoryReader& reader)
{
	std::shared_ptr <CMemoryReader>propReader(reader.open_chunk(propBlock));
	if (propReader != nullptr) {
		for (u64 iter_pos = 0; ; iter_pos++) {
			std::shared_ptr <CMemoryReader>propDataReader(propReader->open_chunk(iter_pos));
			if (propDataReader == nullptr)
				break;
			IProperty* prop = GetProperty(propDataReader->r_stringZ());
			if (prop != nullptr && !prop->PasteData(*propDataReader))
				return false;
		}
	}
	std::shared_ptr <CMemoryReader>eventReader(reader.open_chunk(eventBlock));
	if (eventReader != nullptr) {
		for (u64 iter_pos = 0; ; iter_pos++) {
			std::shared_ptr <CMemoryReader>eventDataReader(eventReader->open_chunk(iter_pos));
			if (eventDataReader == nullptr)
				break;
			IEvent* event = GetEvent(eventDataReader->r_stringZ());
			if (event != nullptr && !event->PasteData(*eventDataReader))
				return false;
		};
	}

	return true;
}

bool IPropertyObject::CopyProperty(CMemoryWriter& writer) const
{
	CMemoryWriter propWritter;
	for (unsigned int idx = 0; idx < GetPropertyCount(); idx++) {
		IProperty* prop = GetProperty(idx);
		wxASSERT(prop);
		CMemoryWriter propDataWritter;
		propDataWritter.w_stringZ(prop->GetName());
		if (!prop->CopyData(propDataWritter))
			return false;
		propWritter.w_chunk(idx, propDataWritter.pointer(), propDataWritter.size());
	}

	writer.w_chunk(propBlock, propWritter.pointer(), propWritter.size());

	CMemoryWriter eventWritter;
	for (unsigned int idx = 0; idx < GetEventCount(); idx++) {
		IEvent* event = GetEvent(idx);
		wxASSERT(event);
		CMemoryWriter eventDataWritter;
		eventDataWritter.w_stringZ(event->GetName());
		if (!event->CopyData(eventDataWritter))
			return false;
		eventWritter.w_chunk(idx, eventDataWritter.pointer(), eventDataWritter.size());
	}

	writer.w_chunk(eventBlock, eventWritter.pointer(), eventWritter.size());
	return true;
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