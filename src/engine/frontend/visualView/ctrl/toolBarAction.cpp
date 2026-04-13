#include "toolbar.h"
#include "form.h"

bool ibValueToolbar::GetActionSource(ibPropertyList* property)
{
	property->AppendItem(
		wxT("form"),
		_("Form"),
		FORM_ACTION,
		m_formOwner->GetIcon(),
		(ibValueFrame*)m_formOwner
	);

	class ibValueToolbarActionParser {
	public:
		static inline void FillActionSource(ibValueFrame* element, ibPropertyList* property) {
			if (element->GetClassName() == wxT("Tablebox")) {
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

	ibValueToolbarActionParser::FillActionSource(m_formOwner, property);
	return true;
}