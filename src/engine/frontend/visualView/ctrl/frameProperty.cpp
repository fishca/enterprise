#include "frame.h"

bool ibValueFrame::OnPropertyChanging(ibProperty* property, const wxVariant& newValue)
{
	return true;
}

void ibValueFrame::OnPropertyChanged(ibProperty* property, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (g_visualHostContext != nullptr)
		g_visualHostContext->ModifyProperty(property, oldValue, newValue);
}

bool ibValueFrame::OnEventChanging(IEvent* event, const wxString& newValue)
{
	return true;
}

void ibValueFrame::OnEventChanged(IEvent* event, const wxVariant& oldValue, const wxVariant& newValue)
{
	if (g_visualHostContext != nullptr) 
		g_visualHostContext->ModifyEvent(event, oldValue, newValue);
}
