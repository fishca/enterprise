#include "gridEditor.h"

#include <wx/clipbrd.h>

void CGridEditor::Copy()
{
	wxString copy_data;
	bool something_in_this_line;

	for (int i = 0; i < m_numRows; i++) {
		something_in_this_line = false;
		for (int j = 0; j < m_numCols; j++) {
			if (IsInSelection(i, j)) {
				if (something_in_this_line == false) {
					if (copy_data.IsEmpty() == false) {
						copy_data.Append(wxT("\n"));
					}
					something_in_this_line = true;
				}
				else {
					copy_data.Append(wxT("\t"));
				}
				copy_data = copy_data + GetCellValue(i, j);
			}
		}
	}

	if (wxTheClipboard->Open()) {

		wxDataObjectComposite *composite = new wxDataObjectComposite();
		composite->Add(new wxTextDataObject(copy_data));

		wxTheClipboard->SetData(composite);
		wxTheClipboard->Close();
	}
}

void CGridEditor::Paste()
{
	wxString copy_data, cur_field, cur_line;

	if (wxTheClipboard->Open()
		&& wxTheClipboard->IsSupported(wxDF_TEXT)) {
		wxTextDataObject textObj;
		if (wxTheClipboard->GetData(textObj)) {
			copy_data = textObj.GetText();
		}
		wxTheClipboard->Close();
	}

	int i = GetGridCursorRow(), j = GetGridCursorCol();
	int k = j;

	while (!copy_data.IsEmpty()) {
		cur_line = copy_data.BeforeFirst('\n');
		while (!cur_line.IsEmpty()) {
			cur_field = cur_line.BeforeFirst('\t');
			SetCellValue(i, j, cur_field);
			j++;
			cur_line = cur_line.AfterFirst('\t');
		}
		i++;
		j = k;
		copy_data = copy_data.AfterFirst('\n');
	}

	wxGridExt::ForceRefresh();
}
