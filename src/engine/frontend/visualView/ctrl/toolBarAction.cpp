#include "toolbar.h"
#include "form.h"

bool CValueToolbar::GetActionSource(CPropertyList* property)
{
	property->AppendItem(
		wxT("form"),
		_("Form"),
		FORM_ACTION,
		m_formOwner->GetIcon(),
		(IValueFrame*)m_formOwner
	);

	class CValueToolbarActionParser {
	public:
		static inline void FillActionSource(IValueFrame* element, CPropertyList* property) {
			if (element->GetClassName() == wxT("tablebox")) {
				property->AppendItem(
					element->GetControlName(),
					stringUtils::GenerateSynonym(element->GetControlName()),
					element->GetControlID(),
					element->GetIcon(),
					element);
			}

			for (unsigned int i = 0; i < element->GetChildCount(); i++) {
				FillActionSource(element->GetChild(i), property);
			}
		}
	};

	CValueToolbarActionParser::FillActionSource(m_formOwner, property);
	return true;
}