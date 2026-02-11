#ifndef _METAGRIDOBJECT_H__
#define _METAGRIDOBJECT_H__

#include "metaObject.h"

class BACKEND_API IValueMetaObjectSpreadsheet : public IValueMetaObject {
	wxDECLARE_ABSTRACT_CLASS(IValueMetaObjectSpreadsheet);
protected:
	enum
	{
		ID_METATREE_OPEN_TEMPLATE = 19000,
	};

public:

	//set spreadsheet code 
	virtual void SetSpreadsheetDesc(const CSpreadsheetDescription& spreadsheetDescription) = 0;
	virtual CSpreadsheetDescription& GetSpreadsheetDesc() const = 0;

	//support icons
	virtual wxIcon GetIcon() const;
	static wxIcon GetIconGroup();

	//module manager is started or exit 
	virtual bool OnBeforeRunMetaObject(int flags);
	virtual bool OnAfterCloseMetaObject();

	//prepare menu for item
	virtual bool PrepareContextMenu(wxMenu* defaultMenu);
	virtual void ProcessCommand(unsigned int id);
};

class BACKEND_API CValueMetaObjectSpreadsheet : public IValueMetaObjectSpreadsheet {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectCommonSpreadsheet);
public:
	//set spreadsheet code 
	virtual void SetSpreadsheetDesc(const CSpreadsheetDescription& spreadsheetDescription) { m_propertyTemplate->SetValue(spreadsheetDescription); }
	virtual CSpreadsheetDescription& GetSpreadsheetDesc() const { return m_propertyTemplate->GetValueAsSpreadsheetDesc(); }
protected:
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
private:
	CPropertyCategory* m_categoryTemplate = IPropertyObject::CreatePropertyCategory(wxT("template"), _("Template"));
	CPropertySpreadsheet* m_propertyTemplate = IPropertyObject::CreateProperty<CPropertySpreadsheet>(m_categoryTemplate, wxT("templateData"), _("Template data"));
};

class BACKEND_API CValueMetaObjectCommonSpreadsheet : public IValueMetaObjectSpreadsheet {
	wxDECLARE_DYNAMIC_CLASS(CValueMetaObjectCommonSpreadsheet);
public:
	//set spreadsheet code 
	virtual void SetSpreadsheetDesc(const CSpreadsheetDescription& spreadsheetDescription) { m_propertyTemplate->SetValue(spreadsheetDescription); }
	virtual CSpreadsheetDescription& GetSpreadsheetDesc() const { return m_propertyTemplate->GetValueAsSpreadsheetDesc(); }
protected:
	virtual bool LoadData(CMemoryReader& reader);
	virtual bool SaveData(CMemoryWriter& writer = CMemoryWriter());
private:
	CPropertyCategory* m_categoryTemplate = IPropertyObject::CreatePropertyCategory(wxT("commonTemplate"), _("Common template"));
	CPropertySpreadsheet* m_propertyTemplate = IPropertyObject::CreateProperty<CPropertySpreadsheet>(m_categoryTemplate, wxT("templateData"), _("Template data"));
};

#endif 