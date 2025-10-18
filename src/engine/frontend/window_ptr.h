#ifndef __WINDOW_PTR_H__
#define __WINDOW_PTR_H__

#include <wx/wx.h>

template<typename T>
class CWindowPtr : public wxSharedPtr<T> {

	struct CWindowDeleter {
		void operator()(wxWindow* win) {}
	};

public:

	typedef T element_type;

	explicit CWindowPtr(element_type* win)
		: wxSharedPtr<T>(win, CWindowDeleter())
	{
		win->Bind(wxEVT_CLOSE_WINDOW,
			[=](wxCloseEvent& event) { win->Destroy(); }
		);
	}

	CWindowPtr() {}
	CWindowPtr(const CWindowPtr& tocopy) : wxSharedPtr<T>(tocopy) {}

	CWindowPtr& operator=(const CWindowPtr& tocopy) {
		wxSharedPtr<T>::operator=(tocopy);
		return *this;
	}

	CWindowPtr& operator=(element_type* win) {
		return operator=(CWindowPtr(win));
	}

	void reset(T* ptr = NULL) {
		wxSharedPtr<T>::reset(ptr, CWindowDeleter());
	}
};

#endif // __WINDOW_PTR_H__