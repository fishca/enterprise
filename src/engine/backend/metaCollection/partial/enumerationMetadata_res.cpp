#include "enumeration.h"

/* PNG */
static const wxString s_enum_64_png = wxT("iVBORw0KGgoAAAANSUhEUgAAAEAAAABACAYAAACqaXHeAAAACXBIWXMAAAsTAAALEwEAmpwYAAAAkUlEQVR4nO3YsQ0CQQxFwa3w14NogRbchjuhlw2PlAwdEZxnJDfwAsvyWgBvcu/jzKyriQAtQATorwPcHnX88gjwSQRoAWIJtgARoGcGAADmyvRDKAK0ABGgPUQiQAsQAdoSjAAtQCYFAADmquc+zsy6mhJgC1AC7K8D/P1DpATYApQA2xIsAbYANTEAALCmeAE1bZ2o13JbHQAAAABJRU5ErkJggg==");

wxIcon ibValueMetaObjectEnumeration::GetIcon() const
{
	return GetIconGroup();
}

wxIcon ibValueMetaObjectEnumeration::GetIconGroup()
{
	static wxIcon icon =
		ibBackendPicture::GetIconFromBase64(s_enum_64_png, wxSize(16, 16));

	return icon;
}
