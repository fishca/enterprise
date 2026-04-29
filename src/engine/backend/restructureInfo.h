#ifndef __RESTRUCT_INFO_H__
#define __RESTRUCT_INFO_H__

//*******************************************************************************
//*                             Structure changes                               *
//*******************************************************************************
//
// Ledger of DDL operations applied during a metadata save (CREATE / ALTER /
// DROP TABLE). One process-wide instance (`s_restructureInfo`, defined in
// metaObject.cpp) is filled while metadata is being persisted; the dialog
// `ibFrameApplyChange` reads it at end of save to show what changed.
//
// Thread safety: callers must serialise — single metadata-save flow at a time
// (which is enforced separately by exclusive mode + the registry). Helpers
// here are not synchronised.

#include "backend.h"

#include <vector>

enum class ibRestructure : uint8_t {
	info,
	warning,
	error,
};

class BACKEND_API ibRestructureInfo {
public:
	struct Entry {
		ibRestructure type;
		wxString      descr;
	};

	// ---- mutators ------------------------------------------------------------
	void AppendInfo   (const wxString& s) { m_entries.push_back({ ibRestructure::info,    s }); }
	void AppendWarning(const wxString& s) { m_entries.push_back({ ibRestructure::warning, s }); ++m_warnings; }
	void AppendError  (const wxString& s) { m_entries.push_back({ ibRestructure::error,   s }); ++m_errors;   }

	void Clear() noexcept { m_entries.clear(); m_warnings = 0; m_errors = 0; }

	// ---- queries -------------------------------------------------------------
	bool   IsEmpty()      const noexcept { return m_entries.empty(); }
	bool   HasWarnings()  const noexcept { return m_warnings > 0; }
	bool   HasErrors()    const noexcept { return m_errors > 0; }
	size_t Count()        const noexcept { return m_entries.size(); }

	const Entry& At(size_t i) const { return m_entries.at(i); }

	// Range-based for support — read-only iteration.
	auto begin() const noexcept { return m_entries.cbegin(); }
	auto end()   const noexcept { return m_entries.cend();   }

	// ---- exclusive-mode gate -------------------------------------------------
	// Acquires exclusive (monopoly) mode for the current session if not
	// already held — blocks new connections for the remainder of the apply.
	// Throws ibBackendCoreException if other sessions are connected and
	// acquisition fails. If this call auto-acquired exclusive,
	// ReleaseAutoExclusive() will drop it later; if the caller already held
	// exclusive explicitly, it stays held.
	static void RequireExclusiveForDDL();

	// Release exclusive mode if it was auto-acquired by RequireExclusiveForDDL
	// on this thread. No-op if exclusive was already held before the gate
	// ran. Pair with RequireExclusiveForDDL inside OnAfterSaveDatabase.
	static void ReleaseAutoExclusive();

	// ---- legacy aliases (deprecated, prefer the new names) -------------------
	bool HasRestructureInfo() const noexcept { return !IsEmpty(); }
	void ResetRestructureInfo()       noexcept { Clear(); }
	wxString      GetDescription(unsigned int i) const { return m_entries.at(i).descr; }
	ibRestructure GetType       (unsigned int i) const { return m_entries.at(i).type;  }
	unsigned int  GetCount()                     const noexcept
		{ return static_cast<unsigned int>(m_entries.size()); }

private:
	std::vector<Entry> m_entries;
	size_t             m_warnings = 0;
	size_t             m_errors   = 0;
};

#endif // !__RESTRUCT_INFO_H__
