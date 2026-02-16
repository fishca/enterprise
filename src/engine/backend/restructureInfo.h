#ifndef __RESTRUCT_INFO_H__
#define __RESTRUCT_INFO_H__

//*******************************************************************************
//*                             Structure changes                               *
//*******************************************************************************

enum ERestructure {

	restructure_info,
	restructure_warning,
	restructure_error,
};

#include "backend.h"

class CRestructureInfo {

	struct CRestructureData {

		CRestructureData(ERestructure t, const wxString& str) :
			m_type(t), m_strDescr(str)
		{
		}

		ERestructure m_type;
		wxString m_strDescr;
	};

public:

	bool HasRestructureInfo() const { return m_listRestructure.size() > 0; }

	void AppendInfo(const wxString& str) {
		m_listRestructure.emplace_back(
			ERestructure::restructure_info, str);
	}

	void AppendWarning(const wxString& str) {
		m_listRestructure.emplace_back(
			ERestructure::restructure_warning, str);
	}

	void AppendError(const wxString& str) {
		m_listRestructure.emplace_back(
			ERestructure::restructure_error, str);
	}

	void ResetRestructureInfo() { return m_listRestructure.clear(); }

	wxString GetDescription(unsigned int idx) const { return m_listRestructure.at(idx).m_strDescr; }
	ERestructure GetType(unsigned int idx) const { return m_listRestructure.at(idx).m_type; }

	unsigned int GetCount() const { return m_listRestructure.size(); }

private:

	std::vector<CRestructureData> m_listRestructure;
};


#endif // !__RESTRUCT_INFO_H__
