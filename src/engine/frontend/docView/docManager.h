#ifndef _REPORT_MANAGER_H__
#define _REPORT_MANAGER_H__

#include "docView.h"

#define docManager ibMetaDocManager::GetDocumentManager()

#include <wx/fdrepdlg.h>

// Document template flags
enum
{
	wxTEMPLATE_ONLY_OPEN = 4,
	wxTEMPLATE_SAVE_AS_FILE = 8
};

class FRONTEND_API ibMetaDocManager : public wxDocManager {

	class ibMetaDocTemplate : public wxDocTemplate {
	public:
		// Associate document and view types. They're for identifying what view is
		// associated with what template/document type
		ibMetaDocTemplate(wxDocManager* manager,
			const wxString& descr,
			const wxString& filter,
			const wxString& dir,
			const wxString& ext,
			const wxString& docTypeName,
			const wxString& viewTypeName,
			wxClassInfo* docClassInfo = nullptr,
			wxClassInfo* viewClassInfo = nullptr,
			long flags = wxDEFAULT_TEMPLATE_FLAGS) : wxDocTemplate(manager,
				descr,
				filter,
				dir,
				ext,
				docTypeName,
				viewTypeName,
				docClassInfo,
				viewClassInfo,
				flags)
		{
		}

		// Helper method for CreateDocument; also allows you to do your own document
		// creation
		virtual bool InitDocument(wxDocument* doc,
			const wxString& path,
			long flags = 0);
	};

	struct ibMetaDocManagerItem {
		ibClassID m_clsid;
		wxString m_className;
		wxString m_classDescr;
		ibGuid m_guidTemplate;
		ibMetaDocTemplate* m_docTemplate;
		wxIcon m_classIcon;
	};

	std::vector <ibMetaDocManagerItem> m_templateVector;

	wxFindReplaceData m_findData;
	wxFindReplaceDialog* m_findDialog;

private:

	ibMetaDocument* OpenForm(ibValueMetaObject* metaObject, ibMetaDocument* docParent, long flags);

	// Handlers for common user commands
	void OnFileClose(wxCommandEvent& event);
	void OnFileCloseAll(wxCommandEvent& event);
	void OnFileNew(wxCommandEvent& event);
	void OnFileOpen(wxCommandEvent& event);
	void OnFileRevert(wxCommandEvent& event);
	void OnFileSave(wxCommandEvent& event);
	void OnFileSaveAs(wxCommandEvent& event);
	void OnMRUFile(wxCommandEvent& event);
#if wxUSE_PRINTING_ARCHITECTURE
	void OnPrint(wxCommandEvent& event);
	void OnPreview(wxCommandEvent& event);
	void OnPageSetup(wxCommandEvent& event);
#endif // wxUSE_PRINTING_ARCHITECTURE
	void OnUndo(wxCommandEvent& event);
	void OnRedo(wxCommandEvent& event);

	// Handlers for UI update commands
	void OnUpdateFileOpen(wxUpdateUIEvent& event);
	void OnUpdateDisableIfNoDoc(wxUpdateUIEvent& event);
	void OnUpdateFileRevert(wxUpdateUIEvent& event);
	void OnUpdateFileNew(wxUpdateUIEvent& event);
	void OnUpdateFileSave(wxUpdateUIEvent& event);
	void OnUpdateFileSaveAs(wxUpdateUIEvent& event);
	void OnUpdateUndo(wxUpdateUIEvent& event);
	void OnUpdateRedo(wxUpdateUIEvent& event);

	void OnUpdateSaveMetadata(wxUpdateUIEvent& event);

	//find dialog
	void OnFindDialog(wxCommandEvent& event);
	void OnFind(wxFindDialogEvent& event);
	void OnFindClose(wxFindDialogEvent& event);

public:

	template <typename T, typename... Args>
	T* CreateDocument(Args&&... args) const {
		wxDocTemplate* docTemplate = FindTemplateByDocClassInfo(CLASSINFO(T));
		if (docTemplate != nullptr) {
			T* doc = new T(std::forward<Args>(args)...);
			doc->SetDocumentTemplate(docTemplate);
			return doc;
		}
		return nullptr;
	}

	static ibMetaDocument* OpenFormMDI(ibValueMetaObject* metaObject, long flags = wxDOC_NEW);
	static ibMetaDocument* OpenFormMDI(ibValueMetaObject* metaObject, ibMetaDocument* docParent, long flags = wxDOC_NEW);

	// Get the current document manager
	static ibMetaDocManager* GetDocumentManager() {
		return dynamic_cast<ibMetaDocManager*>(sm_docManager);
	}

	ibMetaDocManager();
	virtual ~ibMetaDocManager();

	void AddDocTemplate(const ibPictureID& id, const wxString& descr,
		const wxString& filter,
		const wxString& dir,
		const wxString& ext,
		const wxString& docTypeName,
		const wxString& viewTypeName,
		wxClassInfo* docClassInfo,
		wxClassInfo* viewClassInfo,
		long flags = wxTEMPLATE_VISIBLE
	);

	void AddDocTemplate(const ibPictureID& id, const wxString& descr,
		const wxString& filter,
		const wxString& ext,
		const wxString& docTypeName,
		const wxString& viewTypeName,
		wxClassInfo* docClassInfo,
		wxClassInfo* viewClassInfo,
		long flags = wxTEMPLATE_VISIBLE
	);

	void AddDocTemplate(const ibClassID& id,
		const wxString& descr,
		const wxString& filter,
		const wxString& ext,
		wxClassInfo* docClassInfo,
		wxClassInfo* viewClassInfo
	);

	void AddDocTemplate(const ibClassID& id,
		wxClassInfo* docClassInfo,
		wxClassInfo* viewClassInfo
	);

	ibMetaDocument* GetCurrentDocument() const;

	virtual wxDocument* CreateDocument(const wxString& pathOrig, long flags) override;
	virtual wxDocTemplate* SelectDocumentPath(wxDocTemplate** templates,
		int noTemplates, wxString& path, long flags, bool save = false) override;
	virtual wxDocTemplate* SelectDocumentType(wxDocTemplate** templates,
		int noTemplates, bool sort = false) override;

	wxDocTemplate* FindTemplateByDocClassInfo(const wxClassInfo* classInfo) const;

	bool CloseDocument(wxDocument* doc, bool force = false);
	bool CloseDocuments(bool force);
	bool Clear(bool force);

protected:

	wxDECLARE_DYNAMIC_CLASS(ibMetaDocManager);
	wxDECLARE_NO_COPY_CLASS(ibMetaDocManager);

	wxDECLARE_EVENT_TABLE();
};

#endif
