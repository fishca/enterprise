#include "metaFormObject.h"

/* PNG */
static const wxString s_form_16_png = wxT("iVBORw0KGgoAAAANSUhEUgAAABAAAAAQCAMAAAAoLQ9TAAAAA3NCSVQICAjb4U/gAAAAzFBMVEUAAAGHnLKQp7iDlqd4i5x5jZ54i516jZ55jJ2Hmaqqw9SkxeK12O+bvd2tz+quz+u83/W83vO73vO52u+21Oi00eStx9mmu8yQo7TK6v3C6P/P6/2SorHg6fPe6PPd5/Pi6vOSoq/9/f3g2urZ0ub5+fri5uvj5+vv8fP+/v79/f7c1fL////6+vvk6Ovb1fLi5+yTorDa1fLc1vLd1vLa0+b4+fru7fLl4evl4Ov8/f37+/yPoKywu8Szvca0vse0v8e2wMiyvMWxvMVQ8sjXAAAAAXRSTlMAQObYZgAAAKZJREFUGJVlz9cSgjAQQNFFsGDvomKUEMEAomABFRD0///JUGZs5/HO7s4swC+u9IUDXhDKlWpNKPAg1hvNVrvT7fUHw9F4IoI0nX2YSyAvEELLHEIrGRSsEkZda5qub6gCikFMhliZNNA8bG0mm8DE3GVB37PgZKFYodQqgntw0wndfh89nvIbmIWz5/v+xaMUX7FxcyAIoyi6x0kSR49nGAZ/378AP8kduERIZWoAAAAASUVORK5CYII=");

wxIcon CValueMetaObjectForm::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CValueMetaObjectForm::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_form_16_png, wxSize(16, 16));

	return icon;
}

wxIcon CValueMetaObjectCommonForm::GetIcon() const
{
	return GetIconGroup();
}

wxIcon CValueMetaObjectCommonForm::GetIconGroup()
{
	static wxIcon icon =
		CBackendPicture::GetIconFromBase64(s_form_16_png, wxSize(16, 16));

	return icon;
}