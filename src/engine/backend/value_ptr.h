#ifndef _VALUE_PTR_H__
#define _VALUE_PTR_H__

#include "backend_core.h"

// ----------------------------------------------------------------------------
// CValuePtr: helper class to avoid memleaks because of missing calls
//                  to CValuePtr::DecrRef
// ----------------------------------------------------------------------------

template <class T>
class CValuePtr : public CValue {

	inline void OnSetValueT(CValue* ptr) {	
		if (this != ptr && !m_bReadOnly) {
			Reset();
			if (ptr != nullptr) {
				m_typeClass = eValueTypes::TYPE_REFFER;
				m_pRef = ptr;
				m_pRef->IncrRef();
			}
		}
	}

	inline void OnSetValueU(CValue* ptr) {
		if (ptr != nullptr)
			OnSetValueT(dynamic_cast<T*>(ptr));
	}

public:

	constexpr CValuePtr() : CValue() {}
	constexpr CValuePtr(nullptr_t) : CValue() {} // construct empty CValuePtr

	explicit CValuePtr(T* ptr) : CValue() { OnSetValueT(ptr); }

	// copy ctor
	CValuePtr(const CValue& to_copy) : CValue() { OnSetValueU(to_copy.m_pRef); }
	CValuePtr(const CValuePtr<T>& to_copy) : CValue() { OnSetValueT(to_copy.m_pRef); }

	// generalized copy ctor: U must be convertible to T	
	template <typename U>
	CValuePtr(const CValuePtr<U>& to_copy) : CValue() { OnSetValueU(to_copy.m_pRef); }

	inline T& operator*() const {
		T* const ptr = Ptr();
		wxASSERT(ptr != nullptr);
		return *(ptr);
	}

	inline T* operator->() const { return Ptr(); }

	inline operator T* () const { return Ptr(); }
	inline explicit operator bool() const noexcept { return m_pRef != nullptr; }

	void operator = (T* ptr) { OnSetValueT(ptr); }

	template <typename U>
	void operator = (const CValuePtr<U>& other) { OnSetValueU(other.m_pRef); }
	void operator = (const CValuePtr& other) { OnSetValueT(other.m_pRef); }

private:

	inline T* Ptr() const { return static_cast<T*>(m_pRef); }
};

template <class _Ty1, class _Ty2>
inline bool operator==(const CValuePtr<_Ty1>& _Left, const CValuePtr<_Ty2>& _Right) noexcept {
	return _Left.m_pRef == _Right.m_pRef;
}

template <class _Ty1, class _Ty2>
inline bool operator!=(const CValuePtr<_Ty1>& _Left, const CValuePtr<_Ty2>& _Right) noexcept {
	return _Left.m_pRef != _Right.m_pRef;
}

template <class _Ty>
inline bool operator==(nullptr_t, const CValuePtr<_Ty>& _Right) noexcept {
	return _Right.m_pRef == nullptr;
}

template <class _Ty>
inline bool operator!=(nullptr_t, const CValuePtr<_Ty>& _Right) noexcept {
	return _Right.m_pRef != nullptr;
}

template <class _Ty>
inline bool operator==(const CValuePtr<_Ty>& _Left, nullptr_t) noexcept {
	return _Left.m_pRef == nullptr;
}

template <class _Ty>
inline bool operator!=(const CValuePtr<_Ty>& _Left, nullptr_t) noexcept {
	return _Left.m_pRef != nullptr;
}

template <class _Ty1, class _Ty2>
inline bool operator==(const CValuePtr<_Ty1>& _Left, _Ty2* _Right) noexcept {
	return _Left.m_pRef == _Right;
}

template <class _Ty1, class _Ty2>
inline bool operator==(_Ty1* _Left, const CValuePtr<_Ty2>& _Right) noexcept {
	return _Left == _Right.m_pRef;
}

template <class _Ty1, class _Ty2>
inline bool operator!=(_Ty1* _Left, const CValuePtr<_Ty2>& _Right) noexcept {
	return _Left != _Right.m_pRef;
}

template <class _Ty1, class _Ty2>
inline bool operator!=(const CValuePtr<_Ty1>& _Left, _Ty2* _Right) noexcept {
	return _Left.m_pRef != _Right;
}

#endif 