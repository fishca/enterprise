#include "toolbar.h"
#include "form.h"

bool CValueToolbar::GetActionSource(CPropertyList* property)
{
	property->AppendItem(wxT("notSelected"), _("<not selected>"), wxNOT_FOUND);
	property->AppendItem(wxT("form"), _("form"), FORM_ACTION, (IValueFrame*)m_formOwner);

	class CValueToolbarActionParser {
	public:
		static inline void FillActionSource(IValueFrame* element, CPropertyList* property)
		{
			if (element->GetClassName() == wxT("tablebox")) {
				property->AppendItem(element->GetControlName(), element->GetControlID(), element);
			}

			for (unsigned int i = 0; i < element->GetChildCount(); i++) {
				FillActionSource(element->GetChild(i), property);
			}
		}
	};

	CValueToolbarActionParser::FillActionSource(m_formOwner, property);
	return true;
}