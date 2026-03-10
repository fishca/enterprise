#ifndef __TABLE_VIEW_H__
#define __TABLE_VIEW_H__

#include <wx/variant.h>
#include <wx/dnd.h>             // For wxDragResult declaration only.
#include <wx/dynarray.h>
#include <wx/itemid.h>
#include <wx/font.h>

#include "backend/backend.h"

// ----------------------------------------------------------------------------
// wxDataViewExtCtrl globals
// ----------------------------------------------------------------------------

class BACKEND_API wxDataViewExtModel;

// ----------------------------------------------------------------------------
// wxDataViewExtCtrl flags
// ----------------------------------------------------------------------------

// size of a wxDataViewExtRenderer without contents:
#define wxDVC_DEFAULT_RENDERER_SIZE     20

// the default width of new (text) columns:
#define wxDVC_DEFAULT_WIDTH             80

// the default width of new toggle columns:
#define wxDVC_TOGGLE_DEFAULT_WIDTH      30

// the default minimal width of the columns:
#define wxDVC_DEFAULT_MINWIDTH          30

// The default alignment of wxDataViewExtRenderers is to take
// the alignment from the column it owns.
#define wxDVR_DEFAULT_ALIGNMENT         -1

// ---------------------------------------------------------
// wxDataViewExtItem
// ---------------------------------------------------------

// Make it a class and not a typedef to allow forward declaring it.
class wxDataViewExtItem : public wxItemId<void*>
{
public:
	wxDataViewExtItem() : wxItemId<void*>() {}
	explicit wxDataViewExtItem(void* pItem) : wxItemId<void*>(pItem) {}
};

WX_DEFINE_USER_EXPORTED_ARRAY(wxDataViewExtItem, wxDataViewExtItemArray, BACKEND_API);

// ---------------------------------------------------------
// wxDataViewExtModelNotifier
// ---------------------------------------------------------

class BACKEND_API wxDataViewExtModelNotifier
{
public:
	wxDataViewExtModelNotifier() { m_owner = NULL; }
	virtual ~wxDataViewExtModelNotifier() { m_owner = NULL; }

	virtual bool ItemAdded(const wxDataViewExtItem& parent, const wxDataViewExtItem& item) = 0;
	virtual bool ItemDeleted(const wxDataViewExtItem& parent, const wxDataViewExtItem& item) = 0;
	virtual bool ItemChanged(const wxDataViewExtItem& item) = 0;
	virtual bool ItemsAdded(const wxDataViewExtItem& parent, const wxDataViewExtItemArray& items);
	virtual bool ItemsDeleted(const wxDataViewExtItem& parent, const wxDataViewExtItemArray& items);
	virtual bool ItemsChanged(const wxDataViewExtItemArray& items);
	virtual bool ValueChanged(const wxDataViewExtItem& item, unsigned int col) = 0;
	virtual bool Cleared() = 0;

#pragma region __table_notifier__h__

	virtual unsigned int GetCurrentModelColumn() const = 0;
	virtual void StartEditing(const wxDataViewExtItem& item, unsigned int col) const = 0;
	virtual bool ShowFilter(struct CFilterRow& filter) = 0;

	virtual void Select(const wxDataViewExtItem& item) const = 0;
	virtual int GetCountPerPage() const = 0;

	virtual wxDataViewExtItem GetSelection() const = 0;
	virtual int GetSelections(wxDataViewExtItemArray& sel) const = 0;

#pragma endregion 

	// some platforms, such as GTK+, may need a two step procedure for ::Reset()
	virtual bool BeforeReset() { return true; }
	virtual bool AfterReset() { return Cleared(); }

	virtual void Resort() = 0;

	void SetOwner(wxDataViewExtModel* owner) { m_owner = owner; }
	wxDataViewExtModel* GetOwner() const { return m_owner; }

private:
	wxDataViewExtModel* m_owner;
};

// ----------------------------------------------------------------------------
// wxDataViewExtItemAttr: a structure containing the visual attributes of an item
// ----------------------------------------------------------------------------

// TODO: Merge with wxItemAttr somehow.

class BACKEND_API wxDataViewExtItemAttr
{
public:
	// ctors
	wxDataViewExtItemAttr()
	{
		m_bold = false;
		m_italic = false;
		m_strikethrough = false;
	}

	// setters
	void SetColour(const wxColour& colour) { m_colour = colour; }
	void SetBold(bool set) { m_bold = set; }
	void SetItalic(bool set) { m_italic = set; }
	void SetStrikethrough(bool set) { m_strikethrough = set; }
	void SetBackgroundColour(const wxColour& colour) { m_bgColour = colour; }

	// accessors
	bool HasColour() const { return m_colour.IsOk(); }
	const wxColour& GetColour() const { return m_colour; }

	bool HasFont() const { return m_bold || m_italic || m_strikethrough; }
	bool GetBold() const { return m_bold; }
	bool GetItalic() const { return m_italic; }
	bool GetStrikethrough() const { return m_strikethrough; }

	bool HasBackgroundColour() const { return m_bgColour.IsOk(); }
	const wxColour& GetBackgroundColour() const { return m_bgColour; }

	bool IsDefault() const { return !(HasColour() || HasFont() || HasBackgroundColour()); }

	// Return the font based on the given one with this attribute applied to it.
	wxFont GetEffectiveFont(const wxFont& font) const;

private:
	wxColour m_colour;
	bool     m_bold;
	bool     m_italic;
	bool     m_strikethrough;
	wxColour m_bgColour;
};

// ---------------------------------------------------------
// wxDataViewExtModel
// ---------------------------------------------------------

typedef wxVector<wxDataViewExtModelNotifier*> wxDataViewExtModelNotifiers;

class BACKEND_API wxDataViewExtModel : public wxRefCounter
{
public:
	wxDataViewExtModel();

	// get value into a wxVariant
	virtual void GetValue(wxVariant& variant,
		const wxDataViewExtItem& item, unsigned int col) const = 0;

	// return true if the given item has a value to display in the given
	// column: this is always true except for container items which by default
	// only show their label in the first column (but see HasContainerColumns())
	virtual bool HasValue(const wxDataViewExtItem& item, unsigned col) const
	{
		return col == 0 || !IsContainer(item) || HasContainerColumns(item);
	}

	// usually ValueChanged() should be called after changing the value in the
	// model to update the control, ChangeValue() does it on its own while
	// SetValue() does not -- so while you will override SetValue(), you should
	// be usually calling ChangeValue()
	virtual bool SetValue(const wxVariant& variant,
		const wxDataViewExtItem& item,
		unsigned int col) = 0;

	bool ChangeValue(const wxVariant& variant,
		const wxDataViewExtItem& item,
		unsigned int col)
	{
		return SetValue(variant, item, col) && ValueChanged(item, col);
	}

	// Get text attribute, return false of default attributes should be used
	virtual bool GetAttr(const wxDataViewExtItem& WXUNUSED(item),
		unsigned int WXUNUSED(col),
		wxDataViewExtItemAttr& WXUNUSED(attr)) const
	{
		return false;
	}

	// Override this if you want to disable specific items
	virtual bool IsEnabled(const wxDataViewExtItem& WXUNUSED(item),
		unsigned int WXUNUSED(col)) const
	{
		return true;
	}

	// define hierarchy
	virtual wxDataViewExtItem GetParent(const wxDataViewExtItem& item) const = 0;
	virtual bool IsContainer(const wxDataViewExtItem& item) const = 0;
	// Is the container just a header or an item with all columns
	virtual bool HasContainerColumns(const wxDataViewExtItem& WXUNUSED(item)) const
	{
		return false;
	}
	virtual unsigned int GetChildren(const wxDataViewExtItem& item, wxDataViewExtItemArray& children) const = 0;

	// delegated notifiers
	bool ItemAdded(const wxDataViewExtItem& parent, const wxDataViewExtItem& item);
	bool ItemsAdded(const wxDataViewExtItem& parent, const wxDataViewExtItemArray& items);
	bool ItemDeleted(const wxDataViewExtItem& parent, const wxDataViewExtItem& item);
	bool ItemsDeleted(const wxDataViewExtItem& parent, const wxDataViewExtItemArray& items);
	bool ItemChanged(const wxDataViewExtItem& item);
	bool ItemsChanged(const wxDataViewExtItemArray& items);
	bool ValueChanged(const wxDataViewExtItem& item, unsigned int col);
	bool Cleared();

#pragma region __table_notifier__h__

	unsigned int GetCurrentModelColumn(int view_id = 0) const;
	void StartEditing(const wxDataViewExtItem& item, unsigned int col, int view_id = 0) const;
	bool ShowFilter(struct CFilterRow& filterm, int view_id = 0);

	void Select(const wxDataViewExtItem& item, int view_id = 0) const;
	int GetCountPerPage(int view_id = 0) const;

	wxDataViewExtItem GetSelection(int view_id = 0) const;
	int GetSelections(wxDataViewExtItemArray& sel, int view_id = 0) const;

#pragma endregion 

	// some platforms, such as GTK+, may need a two step procedure for ::Reset()
	bool BeforeReset();
	bool AfterReset();

	// delegated action
	virtual void Resort();

	void AddNotifier(wxDataViewExtModelNotifier* notifier);
	void RemoveNotifier(wxDataViewExtModelNotifier* notifier);

	int GetViewCount() const { return m_notifiers.size(); }

	// default compare function
	virtual int Compare(const wxDataViewExtItem& item1, const wxDataViewExtItem& item2,
		unsigned int column, bool ascending) const;
	virtual bool HasDefaultCompare() const { return false; }

	// internal
	virtual bool IsListModel() const { return false; }
	virtual bool IsVirtualListModel() const { return false; }

protected:
	// Dtor is protected because the objects of this class must not be deleted,
	// DecRef() must be used instead.
	virtual ~wxDataViewExtModel();

	// Helper function used by the default Compare() implementation to compare
	// values of types it is not aware about. Can be overridden in the derived
	// classes that use columns of custom types.
	virtual int DoCompareValues(const wxVariant& WXUNUSED(value1),
		const wxVariant& WXUNUSED(value2)) const
	{
		return 0;
	}

private:
	wxDataViewExtModelNotifiers  m_notifiers;
};

#endif