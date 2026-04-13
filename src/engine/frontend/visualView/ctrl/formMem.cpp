#include "form.h"
#include "backend/metaCollection/metaFormObject.h"

bool CValueForm::LoadForm(const wxMemoryBuffer& formData)
{
	CMemoryReader readerData(formData);
	u64 clsid = 0;
	std::shared_ptr<CMemoryReader>readerMemory(readerData.open_chunk_iterator(clsid));
	if (!readerMemory)
		return false;
	u64 form_id = 0;
	std::shared_ptr<CMemoryReader>readerMetaMemory(readerMemory->open_chunk_iterator(form_id));
	if (!readerMetaMemory)
		return false;
	for (unsigned int idx = GetChildCount(); idx > 0; idx--) {
		IValueFrame* controlChild = GetChild(idx - 1);
		if (controlChild != nullptr) {
			RemoveControl(controlChild);
		}
	}
	std::shared_ptr <CMemoryReader>readerDataMemory(readerMetaMemory->open_chunk(eDataBlock));
	//SetReadOnly(m_metaFormObject->IsEditable());
	if (!LoadControl(m_metaFormObject, *readerDataMemory)) {
		return false;
	}
	std::shared_ptr <CMemoryReader> readerChildMemory(readerMetaMemory->open_chunk(eChildBlock));
	if (readerChildMemory) {
		if (!LoadChildForm(*readerChildMemory, this)) {
			return false;
		}
	}

	CValueForm::PrepareNames();
	return true;
}

bool CValueForm::LoadChildForm(CMemoryReader& readerData, IValueFrame* controlParent)
{
	class_identifier_t clsid = 0;
	CMemoryReader* prevReaderMemory = nullptr;
	while (!readerData.eof()) {
		CMemoryReader* readerMemory = readerData.open_chunk_iterator(clsid, &*prevReaderMemory);
		if (!readerMemory)
			break;
		u64 form_id = 0;
		CMemoryReader* prevReaderMetaMemory = nullptr;
		while (!readerData.eof()) {
			CMemoryReader* readerMetaMemory = readerMemory->open_chunk_iterator(form_id, &*prevReaderMetaMemory);
			if (!readerMetaMemory)
				break;
			wxASSERT(clsid != 0);
			IValueFrame* newControl = CValueForm::NewObject(clsid, controlParent, false);
			//newControl->SetReadOnly(m_propEnabled);
			std::shared_ptr <CMemoryReader>readerDataMemory(readerMetaMemory->open_chunk(eDataBlock));
			if (!newControl->LoadControl(m_metaFormObject, *readerDataMemory))
				return false;
			std::shared_ptr <CMemoryReader> readerChildMemory(readerMetaMemory->open_chunk(eChildBlock));
			if (readerChildMemory) {
				if (!LoadChildForm(*readerChildMemory, newControl))
					return false;
			}
			prevReaderMetaMemory = readerMetaMemory;
		}
		prevReaderMemory = readerMemory;
	}
	return true;
}

wxMemoryBuffer CValueForm::SaveForm()
{
	CMemoryWriter writerData;

	//Save common object
	CMemoryWriter writerMemory;
	CMemoryWriter writerMetaMemory;
	CMemoryWriter writerDataMemory;
	if (!SaveControl(m_metaFormObject, writerDataMemory)) {
		return wxMemoryBuffer();
	}
	writerMetaMemory.w_chunk(eDataBlock, writerDataMemory.pointer(), writerDataMemory.size());
	CMemoryWriter writerChildMemory;
	if (!SaveChildForm(writerChildMemory, this))
		return wxMemoryBuffer();
	writerMetaMemory.w_chunk(eChildBlock, writerChildMemory.pointer(), writerChildMemory.size());
	writerMemory.w_chunk(GetControlID(), writerMetaMemory.pointer(), writerMetaMemory.size());
	writerData.w_chunk(GetClassType(), writerMemory.pointer(), writerMemory.size());

	//CValueForm::PrepareNames();
	return writerData.buffer();
}

bool CValueForm::SaveChildForm(CMemoryWriter& writerData, IValueFrame* controlParent)
{
	for (unsigned int idx = 0; idx < controlParent->GetChildCount(); idx++) {
		CMemoryWriter writerMemory;
		CMemoryWriter writerMetaMemory;
		CMemoryWriter writerDataMemory;
		IValueFrame* child = controlParent->GetChild(idx);
		wxASSERT(child);
		if (!child->SaveControl(m_metaFormObject, writerDataMemory)) {
			return false;
		}
		writerMetaMemory.w_chunk(eDataBlock, writerDataMemory.pointer(), writerDataMemory.size());
		CMemoryWriter writerChildMemory;
		if (!SaveChildForm(writerChildMemory, child)) {
			return false;
		}
		writerMetaMemory.w_chunk(eChildBlock, writerChildMemory.pointer(), writerChildMemory.size());
		writerMemory.w_chunk(child->GetControlID(), writerMetaMemory.pointer(), writerMetaMemory.size());
		writerData.w_chunk(child->GetClassType(), writerMemory.pointer(), writerMemory.size());
	}

	return true;
}