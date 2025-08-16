#ifndef _METADATA_DOC_H__
#define _METADATA_DOC_H__

#include "frontend/docView/docView.h"
#include "mainFrame/metatree/metatreeWnd.h"

// The view using a standard wxTextCtrl to show its contents
class CMetadataView : public CMetaView
{
	CMetadataTree* m_metaTree;

public:

	CMetadataView() : CMetaView() {}

	virtual bool OnCreate(CMetaDocument* doc, long flags) override;
	virtual void OnDraw(wxDC* dc) override;
	virtual bool OnClose(bool deleteWindow = true) override;

	CMetadataTree* GetMetaTree() const {  return m_metaTree; }

protected:

	wxDECLARE_DYNAMIC_CLASS(CMetadataView);
};

// ----------------------------------------------------------------------------
// CMetadataDocument: wxDocument and wxTextCtrl married
// ----------------------------------------------------------------------------

class CMetadataBrowserDocument : public CMetaDocument {
	
	virtual CMetaView* DoCreateView() {
		return new CMetadataView(); 
	}

public:
	
	CMetadataBrowserDocument(IMetaDataConfiguration* metaData = nullptr) : 
		CMetaDocument(), m_metaData(metaData) { m_childDoc = false; }
	
	virtual wxIcon GetIcon() const {
		if (m_metaData != nullptr) {
			IMetaObject* metaObject = m_metaData->GetCommonMetaObject();
			wxASSERT(metaObject);
			return metaObject->GetIcon();
		}
		return wxNullIcon;
	}

	virtual bool OnCreate(const wxString& path, long flags) override;
	virtual bool OnCloseDocument() override;

	virtual void Modify(bool mod) override {}

	virtual CMetadataTree* GetMetaTree() const;

protected:

	IMetaDataConfiguration* m_metaData;

	wxDECLARE_NO_COPY_CLASS(CMetadataBrowserDocument);
	wxDECLARE_DYNAMIC_CLASS(CMetadataBrowserDocument);
};

class CMetadataFileDocument : public CMetadataBrowserDocument {

	virtual CMetaView* DoCreateView() {
		return CMetaDocument::DoCreateView();
	}

public:

	CMetadataFileDocument() : CMetadataBrowserDocument() {}
	virtual ~CMetadataFileDocument() { wxDELETE(m_metaData); }

	virtual bool OnCreate(const wxString& path, long flags) override;
	virtual bool OnCloseDocument() override;

	virtual bool IsModified() const override;
	virtual void Modify(bool mod) override;

protected:

	virtual bool DoOpenDocument(const wxString& filename) override;
	virtual bool DoSaveDocument(const wxString& filename) override;

	wxDECLARE_NO_COPY_CLASS(CMetadataFileDocument);
	wxDECLARE_DYNAMIC_CLASS(CMetadataFileDocument);
};


#endif 