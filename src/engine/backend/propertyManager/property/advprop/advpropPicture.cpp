#include "advpropPicture.h"

#include <wx/log.h>
#include <wx/regex.h>

// -----------------------------------------------------------------------
// wxPGPictureProperty
// -----------------------------------------------------------------------

// static long gs_imageFilterIndex = -1; TODO: new wxPropertyGrid misses the
//                                             wxPG_FILE_FILTER_INDEX attribute ID
static wxString gs_imageInitialPath = wxEmptyString;

#if wxCHECK_VERSION(3, 1, 0)
wxPG_IMPLEMENT_PROPERTY_CLASS(wxPGPictureProperty, wxPGProperty, TextCtrl)
#else
WX_PG_IMPLEMENT_PROPERTY_CLASS(wxPGPictureProperty, wxPGProperty, wxString, const wxString&, TextCtrl)
#endif

void wxPGPictureProperty::GetChildValues(const wxString& parentValue, wxArrayString& childValues) const
{
	// some properties can contain value like "[-1;-1]" which must be modified due to use of ";" as a
	// string separator
	wxString values = parentValue;

	wxRegEx regex(wxT("\\[.+;.+\\]"));
	if (regex.IsValid())
	{
		if (regex.Matches(values))
		{
			wxString sizeVal = regex.GetMatch(values);
			sizeVal.Replace(wxT(";"), wxT("<semicolon>"));
			sizeVal.Replace(wxT("["), wxT(""));
			sizeVal.Replace(wxT("]"), wxT(""));
			regex.Replace(&values, sizeVal);
		}
	}

	childValues = wxStringTokenize(values, wxT(';'), wxTOKEN_RET_EMPTY_ALL);
	for (wxArrayString::iterator value = childValues.begin(); value != childValues.end(); ++value)
	{
		value->Trim(false);
		value->Replace(wxT("<semicolon>"), wxT(";"));
	}
}

wxPGPictureProperty::wxPGPictureProperty(const wxString& label,
	const wxString& strName,
	const wxString& value) : wxPGProperty(label, strName)
{
	SetValue(WXVARIANT(value));
	CreateChildren();
	UpdateChildValues(value);
}

void wxPGPictureProperty::CreateChildren()
{
	wxString  propValue = m_value.GetString();
	wxVariant thisValue = WXVARIANT(propValue);
	wxVariant childValue;
	int       childIndex = 0;
	wxArrayString childVals;
	GetChildValues(propValue, childVals);
	wxString  source;
	if (childVals.Count() > 0)
	{
		source = childVals.Item(0);
	}
	else
	{
		source = _("Load From File");
	}
	m_prevSrc = -1;
	if (source == wxString(_("Load From Art Provider")))
	{
		childIndex = 1;
	}

	childValue = WXVARIANT(childIndex);

	CreatePropertySource(childIndex);

	ChildChanged(thisValue, 0, childValue);
}

wxPGProperty* wxPGPictureProperty::CreatePropertySource(int sourceIndex)
{
	wxPGChoices sourceChoices;

	// Add 'source' property (common for all other children)
	sourceChoices.Add(_("Load From Art Provider"));

	wxPGProperty* srcProp = new wxEnumProperty(_("Source"), wxT("source"), sourceChoices, sourceIndex);
	srcProp->SetHelpString(
		wxString(_("Load From Art Provider:\n")) +
		wxString(_("Query registered providers for bitmap with given ID.\n\n")));
	AddPrivateChild(srcProp);
	return srcProp;
}

wxPGProperty* wxPGPictureProperty::CreatePropertyArtId()
{
	wxPGChoices artIdChoices;

	// Create 'id' property ('Load From Art Provider' only)
	artIdChoices.Add(wxT("wxART_ADD_BOOKMARK"));
	artIdChoices.Add(wxT("wxART_DEL_BOOKMARK"));
	artIdChoices.Add(wxT("wxART_HELP_SIDE_PANEL"));
	artIdChoices.Add(wxT("wxART_HELP_SETTINGS"));
	artIdChoices.Add(wxT("wxART_HELP_BOOK"));
	artIdChoices.Add(wxT("wxART_HELP_FOLDER"));
	artIdChoices.Add(wxT("wxART_HELP_PAGE"));
	artIdChoices.Add(wxT("wxART_GO_BACK"));
	artIdChoices.Add(wxT("wxART_GO_FORWARD"));
	artIdChoices.Add(wxT("wxART_GO_UP"));
	artIdChoices.Add(wxT("wxART_GO_DOWN"));
	artIdChoices.Add(wxT("wxART_GO_TO_PARENT"));
	artIdChoices.Add(wxT("wxART_GO_HOME"));
	artIdChoices.Add(wxT("wxART_FILE_OPEN"));
	artIdChoices.Add(wxT("wxART_FILE_SAVE"));
	artIdChoices.Add(wxT("wxART_FILE_SAVE_AS"));
	artIdChoices.Add(wxT("wxART_GOTO_FIRST"));
	artIdChoices.Add(wxT("wxART_GOTO_LAST"));
	artIdChoices.Add(wxT("wxART_PRINT"));
	artIdChoices.Add(wxT("wxART_HELP"));
	artIdChoices.Add(wxT("wxART_TIP"));
	artIdChoices.Add(wxT("wxART_REPORT_VIEW"));
	artIdChoices.Add(wxT("wxART_LIST_VIEW"));
	artIdChoices.Add(wxT("wxART_NEW_DIR"));
	artIdChoices.Add(wxT("wxART_HARDDISK"));
	artIdChoices.Add(wxT("wxART_FLOPPY"));
	artIdChoices.Add(wxT("wxART_CDROM"));
	artIdChoices.Add(wxT("wxART_REMOVABLE"));
	artIdChoices.Add(wxT("wxART_FOLDER"));
	artIdChoices.Add(wxT("wxART_FOLDER_OPEN"));
	artIdChoices.Add(wxT("wxART_GO_DIR_UP"));
	artIdChoices.Add(wxT("wxART_EXECUTABLE_FILE"));
	artIdChoices.Add(wxT("wxART_NORMAL_FILE"));
	artIdChoices.Add(wxT("wxART_TICK_MARK"));
	artIdChoices.Add(wxT("wxART_CROSS_MARK"));
	artIdChoices.Add(wxT("wxART_ERROR"));
	artIdChoices.Add(wxT("wxART_QUESTION"));
	artIdChoices.Add(wxT("wxART_WARNING"));
	artIdChoices.Add(wxT("wxART_INFORMATION"));
	artIdChoices.Add(wxT("wxART_MISSING_IMAGE"));
	artIdChoices.Add(wxT("wxART_COPY"));
	artIdChoices.Add(wxT("wxART_CUT"));
	artIdChoices.Add(wxT("wxART_PASTE"));
	artIdChoices.Add(wxT("wxART_DELETE"));
	artIdChoices.Add(wxT("wxART_NEW"));
	artIdChoices.Add(wxT("wxART_UNDO"));
	artIdChoices.Add(wxT("wxART_REDO"));
	artIdChoices.Add(wxT("wxART_PLUS"));
	artIdChoices.Add(wxT("wxART_MINUS"));
	artIdChoices.Add(wxT("wxART_CLOSE"));
	artIdChoices.Add(wxT("wxART_QUIT"));
	artIdChoices.Add(wxT("wxART_FIND"));
	artIdChoices.Add(wxT("wxART_FIND_AND_REPLACE"));
	artIdChoices.Add(wxT("wxART_FULL_SCREEN"));
	artIdChoices.Add(wxT("wxART_EDIT"));

	wxPGProperty* propArtId = new wxEditEnumProperty(_("Id"), wxT("id"), artIdChoices);
	propArtId->SetHelpString(_("Choose a wxArtID unique identifier of the bitmap or enter a wxArtID for your custom wxArtProvider. IDs with prefix 'gtk-' are available under wxGTK only."));

	return propArtId;
}

wxPGProperty* wxPGPictureProperty::CreatePropertyArtClient()
{
	wxPGChoices artClientChoices;

	// Create 'client' property ('Load From Art Provider' only)
	artClientChoices.Add(wxT("wxART_TOOLBAR"));
	artClientChoices.Add(wxT("wxART_MENU"));
	artClientChoices.Add(wxT("wxART_BUTTON"));
	artClientChoices.Add(wxT("wxART_FRAME_ICON"));
	artClientChoices.Add(wxT("wxART_CMN_DIALOG"));
	artClientChoices.Add(wxT("wxART_HELP_BROWSER"));
	artClientChoices.Add(wxT("wxART_MESSAGE_BOX"));
	artClientChoices.Add(wxT("wxART_OTHER"));

	wxPGProperty* propArtClient = new wxEditEnumProperty(_("Client"), wxT("client"), artClientChoices);
	propArtClient->SetHelpString(_("Choose a wxArtClient identifier of the client (i.e. who is asking for the bitmap) or enter a wxArtClient for your custom wxArtProvider."));

	return propArtClient;
}

wxVariant wxPGPictureProperty::ChildChanged(wxVariant& thisValue, const int childIndex,
	wxVariant& childValue) const {
	wxPGPictureProperty* bp = (wxPGPictureProperty*)this;

	const wxString& val = thisValue.GetString();
	wxArrayString childVals;
	GetChildValues(val, childVals);
	wxString newVal = val;

	// Find the appropriate new state
	switch (childIndex)
	{
		// source
		case 0:
		{
			unsigned int count = GetChildCount();

			if (m_prevSrc != 4)
			{
				for (unsigned int i = 1; i < count; i++)
				{
					wxPGProperty* p = Item(i);
					if (p)
					{
						wxLogDebug(wxT("wxOESBP::ChildChanged: Removing:%s"), p->GetLabel().c_str());
						GetGrid()->DeleteProperty(p);
					}
				}
				bp->AddPrivateChild(bp->CreatePropertyArtId());
				bp->AddPrivateChild(bp->CreatePropertyArtClient());
			}

			if (childVals.GetCount() == 3)
				newVal = childVals.Item(0) + wxT("; ") + childVals.Item(1) + wxT("; ") + childVals.Item(2);
			else if (childVals.GetCount() > 1)
				newVal = childVals.Item(0) + wxT("; ; ");
			break;
		}

		// file_path || id || resource_name
		case 1:
		{
			if (Item(0)->GetValueAsString() == _("Load From Art Provider")) {

				Item(1)->SetValue(childValue);

				if (childVals.GetCount() == 3)
					newVal = childVals.Item(0) + wxT("; ") + Item(1)->GetValueAsString() + wxT("; ") + Item(2)->GetValueAsString();
				else if (childVals.GetCount() > 1)
					newVal = childVals.Item(0) + wxT("; ; ");
			}
			break;
		}
		case 2:

			Item(2)->SetValue(childValue);

			if (childVals.GetCount() == 3)
				newVal = childVals.Item(0) + wxT("; ") + Item(1)->GetValueAsString() + wxT("; ") + Item(2)->GetValueAsString();
			else if (childVals.GetCount() > 1)
				newVal = childVals.Item(0) + wxT("; ; ");

			break;
	}

	bp->SetPrevSource(childValue.GetInteger());

	if (newVal != val) {
		wxVariant ret = WXVARIANT(newVal);
		bp->SetValue(ret);

		return ret;
	}
	return thisValue;
}

void wxPGPictureProperty::UpdateChildValues(const wxString& value)
{
	wxArrayString childVals;
	GetChildValues(value, childVals);

	if (childVals[0].Contains(_("Load From Art Provider")))
	{
		Item(0)->SetValue(childVals[0]);

		if (childVals.Count() > 1)
		{
			wxString img = childVals[1];
			Item(1)->SetValue(childVals[1]);
		}

		if (childVals.Count() > 2)
		{
			wxString img = childVals[2];
			Item(2)->SetValue(childVals[2]);
		}
	}
}

void wxPGPictureProperty::RefreshChildren()
{
	//const wxString &propVal = GetValueAsString().AfterFirst(':');

	//if (!propVal.IsEmpty()) {
	//	UpdateChildValues(propVal);
	//}
}

void wxPGPictureProperty::OnSetValue()
{
}

wxString wxPGPictureProperty::SetupImage(const wxString& imgPath)
{
	if (!imgPath.IsEmpty())
	{
		wxFileName imgName = wxFileName(imgPath);

		// Allow user to specify any file path he needs (even if it seemingly doesn't exist)
		if (!imgName.FileExists()) return imgPath;

		wxString   res = wxT("");
		wxImage    img = wxImage(imgPath);

		if (!img.IsOk())
			return res;
	}
	return imgPath;
}

wxString wxPGPictureProperty::SetupResource(const wxString& resName)
{
	wxString res = wxEmptyString;
	// Keep old value from an icon resource only
	if (resName.Contains(wxT(";")) && resName.Contains(wxT("[")))
	{
		return resName.BeforeFirst(wxT(';'));
	}
	else if (resName.Contains(wxT(";")))
	{
		return res;
	}
	return resName;
}