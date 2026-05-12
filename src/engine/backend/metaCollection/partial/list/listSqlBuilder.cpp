////////////////////////////////////////////////////////////////////////////
//	Author      : (paging refactor)
//	Description : SQL builder helpers shared by the paged Fetch impls.
//	              See docs/paging-design.md.
////////////////////////////////////////////////////////////////////////////

#include "listSqlBuilder.h"

#include "objectList.h"  // pulls commonObject.h → meta + reference types
#include "backend/databaseLayer/databaseLayer.h"
#include "backend/databaseLayer/preparedStatement.h"
#include "backend/metaCollection/attribute/metaAttributeObject.h"

wxString ibListSqlBuilder::BuildSelectHead(ibDatabaseLayer* db,
                                           const wxString& tableName,
                                           int firstN)
{
	if (db != nullptr && db->GetDatabaseLayerType() == DATABASELAYER_FIREBIRD) {
		return wxString::Format("SELECT FIRST %d * FROM %s ", firstN, tableName);
	}
	return wxString::Format("SELECT * FROM %s ", tableName);
}

wxString ibListSqlBuilder::AppendLimit(ibDatabaseLayer* db, int n)
{
	if (db != nullptr && db->GetDatabaseLayerType() != DATABASELAYER_FIREBIRD) {
		return wxString::Format(" LIMIT %d", n);
	}
	return wxEmptyString;
}

/////////////////////////////////////////////////////////////////////////////

wxString ibListSqlBuilder::BuildFilterWhere(const ibValueMetaObjectRecordDataRef* meta,
                                            const ibFilterRow& filter,
                                            bool whereAlreadyOpen)
{
	wxString whereText;
	bool whereOpen = whereAlreadyOpen;

	for (const auto& f : filter.m_filters) {
		if (!f.m_filterUse) continue;

		const ibValueMetaObjectAttributeBase* attr =
			meta->FindAnyAttributeObjectByFilter(f.m_filterModel);
		wxASSERT(attr);

		const wxString op =
			f.m_filterComparison == ibComparisonType::ibComparisonType_Equal ? "=" : "<>";

		whereText += whereOpen ? " AND " : " WHERE ";
		whereOpen = true;

		if (!meta->IsDataReference(f.m_filterModel)) {
			whereText += ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName(attr, op);
		}
		else {
			whereText += wxString::Format("%s %s ?", guidName, op);
		}
	}
	return whereText;
}

/////////////////////////////////////////////////////////////////////////////

wxString ibListSqlBuilder::BuildOrderBy(const ibValueMetaObjectRecordDataRef* meta,
                                        const ibSortOrder& sort,
                                        bool reverse)
{
	wxString orderText;
	bool firstOrder = true;

	auto appendField = [&](const wxString& field, const wxString& op) {
		if (firstOrder) {
			orderText += " ORDER BY ";
			firstOrder = false;
		}
		else {
			orderText += ", ";
		}
		orderText += wxString::Format("%s %s", field, op);
	};

	// Pass 1 — non-reference (regular attribute) sorts. Each attribute may
	// expand to multiple SQL fields (composite key components), each
	// emitted with its own ASC/DESC.
	for (const auto& s : sort.m_sorts) {
		if (!s.m_sortEnable) continue;
		if (meta->IsDataReference(s.m_sortModel)) continue;

		const bool asc = reverse ? !s.m_sortAscending : s.m_sortAscending;
		const wxString op = asc ? "ASC" : "DESC";
		const ibValueMetaObjectAttributeBase* attr =
			meta->FindAnyAttributeObjectByFilter(s.m_sortModel);
		wxASSERT(attr);

		for (auto& field : ibValueMetaObjectAttributeBase::GetSQLFieldData(attr)) {
			const wxString fieldName =
				(field.m_type == ibValueMetaObjectAttributeBase::ibFieldTypes_Reference)
				? field.m_field.m_fieldRefName.m_fieldRefName
				: field.m_field.m_fieldName;
			appendField(fieldName, op);
		}
	}

	// Pass 2 — reference (PK) sorts append the row identity column.
	for (const auto& s : sort.m_sorts) {
		if (!s.m_sortEnable) continue;
		if (!meta->IsDataReference(s.m_sortModel)) continue;

		const bool asc = reverse ? !s.m_sortAscending : s.m_sortAscending;
		appendField(guidName, asc ? "ASC" : "DESC");
	}

	return orderText;
}

/////////////////////////////////////////////////////////////////////////////

wxString ibListSqlBuilder::BuildAnchorPredicate(const ibValueMetaObjectRecordDataRef* meta,
                                                const ibSortOrder& sort,
                                                bool whereAlreadyOpen,
                                                ibFetchDirection direction,
                                                const wxString& lastGuidStr)
{
	// Forward and Reset both walk in sort order; Reset additionally
	// INCLUDES the anchor row (used by GetFirstFetch when restoring
	// the user's previous selection — items[0] should be the anchor
	// itself, not its successor).  Backward walks the other direction.
	const bool forward = (direction == ibFetchDirection::Forward) ||
	                     (direction == ibFetchDirection::Reset);
	const bool inclusiveTail = (direction == ibFetchDirection::Reset);

	// Earlier implementation built a flat AND-chain
	// (`c1 cmp ? AND c2 cmp ? AND case-when uuid …`).  That was correct
	// for a single sort col but wrong for K>=2 with mixed ASC/DESC: a
	// row with `c1 better than anchor's c1` but `c2 worse than anchor's
	// c2` was excluded by the AND, even though it sits "before/after"
	// anchor in display order.  In Tree views with `isFolder DESC + name
	// ASC` this dropped folder rows whose name happens to sort after
	// the anchor non-folder name, so backward fetch returned 0 and the
	// head of the tree disappeared from the loaded buffer.
	//
	// Proper composite cursor predicate is the lexicographic
	// "OR-of-AND" form:
	//   (c_1 strict_op_1 ?)
	//   OR (c_1 = ? AND c_2 strict_op_2 ?)
	//   OR …
	//   OR (c_1 = ? AND … AND c_K = ? AND uuid tail_op ?)
	// where strict_op_i flips ASC/DESC by direction, and tail_op honors
	// inclusiveTail (Reset includes anchor row).
	//
	// Bind order in BindAnchorSortParams must match: clause i needs
	// (c_1, …, c_{i-1}) equality + c_i strict; tail needs (c_1, …, c_K)
	// equality (uuid is hard-coded into the SQL string as `lastGuidStr`).

	struct SortCol {
		const ibValueMetaObjectAttributeBase* attr;
		bool asc;
	};
	std::vector<SortCol> cols;
	bool refAsc = true;
	for (const auto& s : sort.m_sorts) {
		if (!s.m_sortEnable) continue;
		if (meta->IsDataReference(s.m_sortModel)) {
			refAsc = s.m_sortAscending;
			continue;
		}
		const ibValueMetaObjectAttributeBase* attr =
			meta->FindAnyAttributeObjectByFilter(s.m_sortModel);
		wxASSERT(attr);
		cols.push_back({ attr, s.m_sortAscending });
	}

	auto strictOpFor = [&](bool asc) -> wxString {
		return (forward == asc) ? ">" : "<";
	};
	auto inclusiveOpFor = [&](bool asc) -> wxString {
		return (forward == asc) ? ">=" : "<=";
	};

	auto eqClauseUpTo = [&](size_t kExclusive) -> wxString {
		wxString eq;
		for (size_t j = 0; j < kExclusive; ++j) {
			if (!eq.IsEmpty()) eq += " AND ";
			eq += ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName(
				cols[j].attr, "=");
		}
		return eq;
	};

	// Build OR-of-AND.
	wxString predicate;
	auto addOr = [&](const wxString& clause) {
		if (!predicate.IsEmpty()) predicate += " OR ";
		predicate += clause;
	};

	for (size_t i = 0; i < cols.size(); ++i) {
		wxString clause;
		const wxString eqPart = eqClauseUpTo(i);
		if (!eqPart.IsEmpty()) clause += eqPart + " AND ";
		clause += ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName(
			cols[i].attr, strictOpFor(cols[i].asc));
		addOr("(" + clause + ")");
	}

	// Tail row: all equalities + uuid tiebreak.
	{
		const wxString tailOp = inclusiveTail
			? inclusiveOpFor(refAsc) : strictOpFor(refAsc);
		wxString clause;
		const wxString eqPart = eqClauseUpTo(cols.size());
		if (!eqPart.IsEmpty()) clause += eqPart + " AND ";
		clause += wxString::Format("%s %s '%s'", guidName, tailOp, lastGuidStr);
		addOr("(" + clause + ")");
	}

	const wxString header = whereAlreadyOpen ? " AND " : " WHERE ";
	return header + "(" + predicate + ")";
}

/////////////////////////////////////////////////////////////////////////////

void ibListSqlBuilder::BindFilterParams(ibPreparedStatement* statement,
                                        int& position,
                                        const ibValueMetaObjectRecordDataRef* meta,
                                        const ibFilterRow& filter)
{
	for (const auto& f : filter.m_filters) {
		if (!f.m_filterUse) continue;

		const ibValueMetaObjectAttributeBase* attr =
			meta->FindAnyAttributeObjectByFilter(f.m_filterModel);
		wxASSERT(attr);

		if (meta->IsDataReference(f.m_filterModel)) {
			ibValueReferenceDataObject* refData = nullptr;
			if (f.m_filterValue.ConvertToValue(refData)) {
				statement->SetParamString(position++, refData->GetGuid());
			}
			else {
				statement->SetParamString(position++, wxNullUniqueKey);
			}
		}
		else {
			ibValueMetaObjectAttributeBase::SetValueAttribute(
				attr, f.m_filterValue, statement, position);
		}
	}
}

void ibListSqlBuilder::BindAnchorSortParams(ibPreparedStatement* statement,
                                            int& position,
                                            const ibValueMetaObjectRecordDataRef* meta,
                                            const ibSortOrder& sort,
                                            const std::vector<ibValue>& sortValues)
{
	// Bind order matches BuildAnchorPredicate's OR-of-AND form:
	//   clause i (i = 0..K-1):  c_0 =, c_1 =, …, c_{i-1} =, c_i strict
	//                            → i+1 binds, all from sortValues[0..i].
	//   tail clause          :  c_0 =, c_1 =, …, c_{K-1} =
	//                            → K binds.  uuid is hard-coded into
	//                              the SQL string by BuildAnchorPredicate
	//                              (lastGuidStr), so no bind for it.

	// Collect non-reference sort cols in the same order
	// BuildAnchorPredicate uses, so sortValues[i] aligns with cols[i].
	struct SortCol {
		const ibValueMetaObjectAttributeBase* attr;
	};
	std::vector<SortCol> cols;
	for (const auto& s : sort.m_sorts) {
		if (!s.m_sortEnable) continue;
		if (meta->IsDataReference(s.m_sortModel)) continue;
		const ibValueMetaObjectAttributeBase* attr =
			meta->FindAnyAttributeObjectByFilter(s.m_sortModel);
		wxASSERT(attr);
		cols.push_back({ attr });
	}
	wxASSERT(sortValues.size() == cols.size());

	auto bindCol = [&](size_t k) {
		ibValueMetaObjectAttributeBase::SetValueAttribute(
			cols[k].attr, sortValues[k], statement, position);
	};

	// Per-col strict clauses.
	for (size_t i = 0; i < cols.size(); ++i) {
		for (size_t j = 0; j < i; ++j) bindCol(j);   // equalities
		bindCol(i);                                   // strict
	}
	// Tail: K equalities (no uuid bind).
	for (size_t j = 0; j < cols.size(); ++j) bindCol(j);
}
