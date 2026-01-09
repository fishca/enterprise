#include "metaModuleObject.h"

/* PNG */
static const wxString s_module_64_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAAclBMVEV+kqOBlKGBk6N4i5yFlqWZp7SbqbaQoK729/j////r8vyxz/Xz9/3R4vnv9f3N4Pn3+v7B2Pf3+PzYhJfkWGTqQk3qQkzbeIrMr8jv9Pza6Pr5+/77/P79/v/w9f3XhpnfbHzdc4Xm6ezt8PKEmKaOnqt4BUlzAAAAZElEQVQYlXXNVw6AIBBF0RFRLIi9Y0Pc/xYtQQMaz+dN5g2AZQBAtgEBdlyNg4F4vsYjZwhCJVCBRgpV4XPC4iS9sCdkeVFWddPeG11vbnA6jBf+F47RaV600ddbsWoEAbkZ5A4AmA3i7wbAzQAAAABJRU5ErkJggg==");

wxIcon IMetaObjectModule::GetIcon() const
{
	return GetIconGroup();
}

wxIcon IMetaObjectModule::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_module_64_png, wxSize(16, 16));

	return icon;
}

wxIcon CMetaObjectCommonModule::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CMetaObjectCommonModule::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_module_64_png, wxSize(16, 16));

	return icon;
}

wxIcon CMetaObjectManagerModule::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CMetaObjectManagerModule::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_module_64_png, wxSize(16, 16));

	return icon;
}
