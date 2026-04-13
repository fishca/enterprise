#include "frame.h"

bool IValueFrame::OnPropertyChanging(IProperty* property, const wxVariant& newValue)
{
	return true;
}

void IValueFrame::OnPropertyChanged(IProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (g_visualHostContext != nullptr)
		g_visualHostContext->ModifyProperty(property, oldValue, newValue);
}

bool IValueFrame::OnEventChanging(IEvent* event, const wxString& newValue)
{
	return true;
}

void IValueFrame::OnEventChanged(IEvent* event, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (g_visualHostContext != nullptr) 
		g_visualHostContext->ModifyEvent(event, oldValue, newValue);
}
