#include "variantNumber.h"

wxString wxVariantDataNumber::MakeString() const
{
	return m_number.ToString();
}