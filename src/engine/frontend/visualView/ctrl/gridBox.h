#ifndef _GRID_H__
#define _GRID_H__

#include "window.h"
#include "frontend/win/editor/gridEditor/gridEditor.h"
#include "backend/system/value/valueSpreadsheet.h"

class CValueGridBox : public IValueWindow {
	wxDECLARE_DYNAMIC_CLASS(CValueGridBox);
public:

	CValueGridBox();

	virtual wxObject* Create(wxWindow* wxparent, IVisualHost *visualHost) override;
	virtual void OnCreated(wxObject* wxobject, wxWindow* wxparent, IVisualHost *visualHost, bool first—reated) override;
	virtual void OnSelected(wxObject* wxobject) override;
	virtual void Update(wxObject* wxobject, IVisualHost *visualHost) override;
	virtual void Cleanup(wxObject* obj, IVisualHost *visualHost) override;

	//support printing 
	virtual wxPrintout* CreatePrintout() const;

	//methods & attributes
	virtual void PrepareNames() const;                         // this method is automatically called to initialize attribute and method names.

	virtual bool SetPropVal(const long lPropNum, const CValue& varPropVal);        //setting attribute
	virtual bool GetPropVal(const long lPropNum, CValue& pvarPropVal);                   //attribute value

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//load & save object in control 
	virtual bool LoadData(CMemoryReader &reader);
	virtual bool SaveData(CMemoryWriter &writer = CMemoryWriter());

private:

	CValuePtr<CValueSpreadsheetDocument> m_valueSpreadsheet;
};

#endif