#include "variantAction.h"

wxString wxVariantDataAction::MakeString() const
{
	return m_actionData.GetCustomAction();
}