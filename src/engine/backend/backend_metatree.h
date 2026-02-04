#ifndef  __META_TREE_WRAPPER_H__
#define  __META_TREE_WRAPPER_H__

#include "backend_form.h"

/////////////////////////////////////////////////////////////////////////////////////////////////////
class BACKEND_API IValueMetaObject;
/////////////////////////////////////////////////////////////////////////////////////////////////////

class BACKEND_API IBackendMetadataTree {
public:

	virtual ~IBackendMetadataTree() {}

	virtual form_identifier_t SelectFormType(class CValueMetaObjectForm* metaObject) const = 0;
	virtual void Activate() = 0;

	virtual void SetReadOnly(bool readOnly = true) = 0;
	virtual bool IsEditable() const = 0;

	virtual void Modify(bool modify) = 0;
	virtual void EditModule(const CGuid& moduleName, int lineNumber, bool setRunLine = true) = 0;

	virtual bool OpenFormMDI(IValueMetaObject* obj) = 0;
	virtual bool OpenFormMDI(IValueMetaObject* obj, IBackendMetaDocument*& foundedDoc) = 0;
	virtual bool CloseFormMDI(IValueMetaObject* obj) = 0;

	virtual IBackendMetaDocument* GetDocument(IValueMetaObject* obj) const = 0;

	virtual bool RenameMetaObject(IValueMetaObject* obj, const wxString& strNewName) = 0;
	virtual void CloseMetaObject(IValueMetaObject* obj) = 0;

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
		IValueMetaObject* m_metaObject; //тип элемента
	public:
		CTreeDataMetaItem(IValueMetaObject* metaObject) :
			m_metaObject(metaObject) {
		}
	};
};

/////////////////////////////////////////////////////////////////////////////////////////////////////
class BACKEND_API IMetaDataConfiguration;
/////////////////////////////////////////////////////////////////////////////////////////////////////

#endif // ! _META_TREE_
