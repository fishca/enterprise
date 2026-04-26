////////////////////////////////////////////////////////////////////////////
//	Author		: Maxim Kornienko, wxFormBuider-team
//	Description : base object for property objects
////////////////////////////////////////////////////////////////////////////

#include "propertyObject.h"

#define propBlock 0x00023456
#define eventBlock 0x00023457

////////////////////////////////////////////////////////////////////////

bool ibBackendProperty::IsEditable() const
{
	return m_owner ? m_owner->IsEditable() : false;
}

///////////////////////////////////////////////////////////////////////////////

void ibProperty::InitProperty(ibPropertyCategory* cat, const wxVariant& value)
{
	m_owner->AddProperty(this);
	if (cat != nullptr) cat->AddProperty(this);
	DoSetValue(value);
}

void ibEvent::InitEvent(ibPropertyCategory* cat, const wxVariant& value)
{
	m_owner->AddEvent(this);
	if (cat != nullptr) cat->AddEvent(this);
	DoSetValue(value);
}

///////////////////////////////////////////////////////////////////////////////

bool ibProperty::PasteData(ibReaderMemory& reader)
{
	return LoadData(reader);
}

///////////////////////////////////////////////////////////////////////////////

#include "backend/backend_mainFrame.h"
#include "backend/appData.h"
#include "backend/session/session.h"

ibPropertyObject::~ibPropertyObject()
{
	wxDELETE(m_category);

	for (auto& property : m_properties)
		wxDELETE(property.second);

	for (auto& event : m_events)
		wxDELETE(event.second);

	// Clear dangling reference on the main frame's property slot if this
	// is the current selection. Explicit chain through appData → main
	// session → frame; null-guarded for headless (no UI). TODO: replace
	// this with an observer pattern so ibPropertyObject doesn't reach
	// the frame at destruction time.
	if (auto* frame = ibSession::CurrentFrame()) {
		if (this == frame->GetProperty()) {
			frame->SetProperty(nullptr);
		}
	}
}

wxString ibPropertyObject::GetIndentString(int indent) const
{
	wxString s;
	for (int i = 0; i < indent; i++) s += wxT(" ");
	return s;
}

ibProperty* ibPropertyObject::GetProperty(const wxString& nameParam) const
{
	std::map<wxString, ibProperty*>::const_iterator it = std::find_if(m_properties.begin(), m_properties.end(),
		[nameParam](const std::pair<wxString, ibProperty*>& pair) {
			return stringUtils::CompareString(nameParam, pair.first);
		}
	);

	if (it != m_properties.end())
		return it->second;

	//LogDebug(wxT("[ibPropertyObject::GetProperty] ibProperty %s not found!"),name.c_str());
	  // este aserto falla siempre que se crea un sizerItem
	  // assert(false);

	return nullptr;
}

ibProperty* ibPropertyObject::GetProperty(unsigned int idx) const
{
	assert(idx < m_properties.size());

	if (idx < m_properties.size()) {
		std::map<wxString, ibProperty*>::const_iterator iterator = m_properties.begin();
		std::advance(iterator, idx);
		return iterator->second;
	}

	return nullptr;
}

ibEvent* ibPropertyObject::GetEvent(const wxString& nameParam) const
{
	std::map<wxString, ibEvent*>::const_iterator it = std::find_if(m_events.begin(), m_events.end(),
		[nameParam](const std::pair<wxString, ibEvent*>& pair) {
			return stringUtils::CompareString(nameParam, pair.first);
		}
	);

	if (it != m_events.end())
		return it->second;

	//LogDebug("[ibPropertyObject::GetEvent] ibEvent " + name + " not found!");
	return nullptr;
}

ibEvent* ibPropertyObject::GetEvent(unsigned int idx) const
{
	assert(idx < m_events.size());

	std::map<wxString, ibEvent*>::const_iterator it = m_events.begin();
	unsigned int i = 0;
	while (i < idx && it != m_events.end()) {
		i++; it++;
	}

	if (it != m_events.end())
		return it->second;

	return nullptr;
}

void ibPropertyObject::AddProperty(ibProperty* prop)
{
	m_properties.emplace(std::map<wxString, ibProperty*>::value_type(prop->GetName(), prop));
}

void ibPropertyObject::AddEvent(ibEvent* event)
{
	m_events.emplace(std::map<wxString, ibEvent*>::value_type(event->GetName(), event));
}

unsigned int ibPropertyObject::GetPropertyIndex(const wxString& nameParam) const {
	return std::distance(m_properties.begin(),
		std::find_if(m_properties.begin(), m_properties.end(),
			[nameParam](const  std::pair<wxString, ibProperty*>& pair) {
				return stringUtils::CompareString(nameParam, pair.first);
			}
		)
	);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool ibPropertyObject::PasteProperty(ibReaderMemory& reader)
{
	std::shared_ptr <ibReaderMemory>propReader(reader.open_chunk(propBlock));
	if (propReader != nullptr) {
		for (u64 iter_pos = 0; ; iter_pos++) {
			std::shared_ptr <ibReaderMemory>propDataReader(propReader->open_chunk(iter_pos));
			if (propDataReader == nullptr)
				break;
			ibProperty* prop = GetProperty(propDataReader->r_stringZ());
			if (prop != nullptr && !prop->PasteData(*propDataReader))
				return false;
		}
	}
	std::shared_ptr <ibReaderMemory>eventReader(reader.open_chunk(eventBlock));
	if (eventReader != nullptr) {
		for (u64 iter_pos = 0; ; iter_pos++) {
			std::shared_ptr <ibReaderMemory>eventDataReader(eventReader->open_chunk(iter_pos));
			if (eventDataReader == nullptr)
				break;
			ibEvent* event = GetEvent(eventDataReader->r_stringZ());
			if (event != nullptr && !event->PasteData(*eventDataReader))
				return false;
		};
	}

	return true;
}

bool ibPropertyObject::CopyProperty(ibWriterMemory& writer) const
{
	ibWriterMemory propWritter;
	for (unsigned int idx = 0; idx < GetPropertyCount(); idx++) {
		ibProperty* prop = GetProperty(idx);
		wxASSERT(prop);
		ibWriterMemory propDataWritter;
		propDataWritter.w_stringZ(prop->GetName());
		if (!prop->CopyData(propDataWritter))
			return false;
		propWritter.w_chunk(idx, propDataWritter.pointer(), propDataWritter.size());
	}

	writer.w_chunk(propBlock, propWritter.pointer(), propWritter.size());

	ibWriterMemory eventWritter;
	for (unsigned int idx = 0; idx < GetEventCount(); idx++) {
		ibEvent* event = GetEvent(idx);
		wxASSERT(event);
		ibWriterMemory eventDataWritter;
		eventDataWritter.w_stringZ(event->GetName());
		if (!event->CopyData(eventDataWritter))
			return false;
		eventWritter.w_chunk(idx, eventDataWritter.pointer(), eventDataWritter.size());
	}

	writer.w_chunk(eventBlock, eventWritter.pointer(), eventWritter.size());
	return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void ibPropertyCategory::AddProperty(ibProperty* property)
{
	m_properties.emplace_back(property->GetName());
}

void ibPropertyCategory::AddEvent(ibEvent* event)
{
	m_events.emplace_back(event->GetName());
}

void ibPropertyCategory::AddCategory(ibPropertyCategory* cat)
{
	m_categories.emplace_back(cat);
}

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////