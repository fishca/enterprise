#ifndef _VALUE_PTR_H__
#define _VALUE_PTR_H__

#include "backend_core.h"

// ----------------------------------------------------------------------------
// CValuePtr: helper class to avoid memleaks because of missing calls
//                  to CValuePtr::DecrRef
// ----------------------------------------------------------------------------

template <class T>
class CValuePtr : public CValue {
public:

	constexpr CValuePtr() : CValue() {}
	constexpr CValuePtr(nullptr_t) : CValue() {} // construct empty CValuePtr

	explicit CValuePtr(T* ptr) : CValue(ptr) {}

	// copy ctor
	CValuePtr(const CValuePtr<T>& to_copy) : CValue(to_copy) {}

	// generalized copy ctor: U must be convertible to T	
	template <typename U>
	CValuePtr(const CValuePtr<U>& to_copy) : CValue(to_copy) {}

	inline T& operator*() const {
		T* const ptr = Ptr();
		wxASSERT(ptr != nullptr);
		return *(ptr);
	}

	inline T* operator->() const { return Ptr(); }
	inline operator T* () const { return Ptr(); }
	inline operator bool() const noexcept { return m_pRef != nullptr; }

	void operator = (const CValuePtr& other) {
		CValue::operator=(other);
	}

	void operator = (T* ptr) {
		CValue::operator=(ptr);
	}

private:

	inline T* Ptr() const {
		return static_cast<T*>(m_pRef);
	}
};

template <class _Ty1, class _Ty2>
inline bool operator==(const CValuePtr<_Ty1>& _Left, const CValuePtr<_Ty2>& _Right) noexcept {
	return _Left.GetRef() == _Right.GetRef();
}

template <class _Ty1, class _Ty2>
inline bool operator!=(const CValuePtr<_Ty1>& _Left, const CValuePtr<_Ty2>& _Right) noexcept {
	return _Left.GetRef() != _Right.GetRef();
}

template <class _Ty>
inline bool operator==(nullptr_t, const CValuePtr<_Ty>& _Right) noexcept {
	return nullptr == _Right.GetRef();
}

template <class _Ty>
inline bool operator!=(nullptr_t, const CValuePtr<_Ty>& _Right) noexcept {
	return nullptr != _Right.GetRef();
}

template <class _Ty>
inline bool operator==(const CValuePtr<_Ty>& _Left, nullptr_t) noexcept {
	return _Left.GetRef() != nullptr;
}

template <class _Ty>
inline bool operator!=(const CValuePtr<_Ty>& _Left, nullptr_t) noexcept {
	return _Left.GetRef() != nullptr;
}

template <class _Ty1, class _Ty2>
inline bool operator==(const CValuePtr<_Ty1>& _Left, _Ty2* _Right) noexcept {
	return _Left.GetRef() == _Right;
}

template <class _Ty1, class _Ty2>
inline bool operator==(_Ty1* _Left, const CValuePtr<_Ty2>& _Right) noexcept {
	return _Left == _Right.GetRef();
}

template <class _Ty1, class _Ty2>
inline bool operator!=(_Ty1* _Left, const CValuePtr<_Ty2>& _Right) noexcept {
	return _Left != _Right.GetRef();
}

template <class _Ty1, class _Ty2>
inline bool operator!=(const CValuePtr<_Ty1>& _Left, _Ty2* _Right) noexcept {
	return _Left.GetRef() != _Right;
}

#endif 