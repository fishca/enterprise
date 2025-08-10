#ifndef  __META_TREE_WRAPPER_H__
#define  __META_TREE_WRAPPER_H__

#include "backend_form.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
class BACKEND_API IMetaObject;
/////////////////////////////////////////////////////////////////////////////////////////////////////

class BACKEND_API IBackendMetadataTree {
public:

	virtual ~IBackendMetadataTree() {}

	virtual form_identifier_t SelectFormType(class CMetaObjectForm* metaObject) const = 0;

	virtual void SetReadOnly(bool readOnly = true) = 0;
	virtual bool IsEditable() const = 0;
	
	virtual void Modify(bool modify) = 0;

	virtual void EditModule(const wxString& fullName, int lineNumber, bool setRunLine = true) = 0;

	virtual bool OpenFormMDI(IMetaObject* obj) = 0;
	virtual bool OpenFormMDI(IMetaObject* obj, IBackendMetaDocument*& foundedDoc) = 0;
	virtual bool CloseFormMDI(IMetaObject* obj) = 0;

	virtual IBackendMetaDocument* GetDocument(IMetaObject* obj) const = 0;

	virtual bool RenameMetaObject(IMetaObject* obj, const wxString& sNewName) = 0;

	virtual void CloseMetaObject(IMetaObject* obj) = 0;
	virtual void OnCloseDocument(IBackendMetaDocument* doc) = 0;

	virtual void UpdateChoiceSelection() {}

protected:

	struct CTreeData {
		bool m_expanded = false;
	};

	struct CTreeDataClassIdentifier : CTreeData {
		class_identifier_t m_clsid; //тип элемента
	public:
		CTreeDataClassIdentifier(const class_identifier_t& clsid) :
			m_clsid(clsid) {
		}
	};

	struct CTreeDataMetaItem : CTreeData {
		IMetaObject* m_metaObject; //тип элемента
	public:
		CTreeDataMetaItem(IMetaObject* metaObject) :
			m_metaObject(metaObject) {
		}
	};
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
class BACKEND_API IMetaDataConfiguration;
/////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // ! _META_TREE_
