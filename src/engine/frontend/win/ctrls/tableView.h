#ifndef __DATA_VIEW_BOX_H__
#define __DATA_VIEW_BOX_H__

#include "dataview/dataview.h"

class wxTableViewCtrl :
	public wxDataViewExtCtrl {
public:

	wxTableViewCtrl() : wxDataViewExtCtrl() {}
	wxTableViewCtrl(wxWindow* parent, wxWindowID id,
		const wxPoint& pos = wxDefaultPosition,
		const wxSize& size = wxDefaultSize, long style = 0,
		const wxValidator& validator = wxDefaultValidator,
		const wxString& name = wxASCII_STR(wxDataViewExtCtrlNameStr)) : wxDataViewExtCtrl(parent, id, pos, size, style, validator, name)
	{
	}

	//show data filter
	virtual bool ShowFilter(struct CFilterRow& filter);
	virtual bool ShowViewMode();

private:

	wxDECLARE_DYNAMIC_CLASS(wxTableViewCtrl);
	wxDECLARE_NO_COPY_CLASS(wxTableViewCtrl);
};

#endif 