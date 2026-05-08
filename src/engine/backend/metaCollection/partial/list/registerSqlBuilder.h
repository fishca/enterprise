#ifndef __REGISTER_SQL_BUILDER_H__
#define __REGISTER_SQL_BUILDER_H__

#include "backend/tableInfo.h"

class ibDatabaseLayer;
class ibPreparedStatement;
class ibValueMetaObjectRegisterData;
class ibValueMetaObjectAttributeBase;

// Cursor-paginated SQL helpers for register lists
// (ibValueListRegisterObject).  Mirrors ibListSqlBuilder for catalog
// references but built around register identity:
//   HasRecorder         → (recorder, line_number) is the row identity
//   HasPeriod (no rec)  → (period, dim1, dim2, ...) is the row identity
//   else                → (dim1, dim2, ...) is the row identity
//
// The builder appends the identity tuple to the ORDER BY tail
// independently of m_sortOrder, so the cursor predicate has a total
// order even when the user reorders or disables user sort columns.
//
// The effective ORDER BY is `[user_sort cols] ++ [identity tail]`,
// deduped by metaID.  The anchor's m_sortValues holds one ibValue per
// effective col in iteration order.  Predicate uses composite >= on
// every col except the last, plus case-when strict tiebreak on the
// last col — same Ref-style scheme that avoids row-constructor SQL
// (which Firebird does not support).
class BACKEND_API ibRegisterSqlBuilder {
public:

	// One column in the effective ORDER BY.  `ascending` is the col's
	// own direction, NOT pre-flipped for backward-walk; the builder
	// flips inside BuildOrderBy when reverse=true.
	struct OrderCol {
		const ibValueMetaObjectAttributeBase* m_attr = nullptr;
		bool                                  m_ascending = true;
	};

	// Identity attribute list for the register, in canonical order.
	// HasRecorder: {recorder, line_number}
	// else:        {period?, dim1, dim2, ...}
	static std::vector<const ibValueMetaObjectAttributeBase*>
		IdentityAttrs(const ibValueMetaObjectRegisterData* meta);

	// Effective ORDER BY columns: enabled user sorts first, then
	// identity-tail cols not already covered by user sort.  Identity
	// tail defaults to ASC when added; if a user already sorted by
	// the same metaID, that direction wins (no duplicate emitted).
	static std::vector<OrderCol> EffectiveOrder(
		const ibValueMetaObjectRegisterData* meta,
		const ibSortOrder& sort);

	// "SELECT * FROM <t> " on non-FB; "SELECT FIRST <firstN> * FROM <t> "
	// on Firebird.  Same as ibListSqlBuilder::BuildSelectHead — kept
	// here as a thin pass-through so register Fetch has one include.
	static wxString BuildSelectHead(ibDatabaseLayer* db,
	                                const wxString& tableName,
	                                int firstN);

	// " LIMIT <n>" on non-FB; empty on FB.
	static wxString AppendLimit(ibDatabaseLayer* db, int n);

	// " WHERE <chained predicates>" or empty if no active filters.
	// Register's m_filterRow uses regular attribute fields only — no
	// special PK/reference handling like the catalog Ref builder.
	static wxString BuildFilterWhere(const ibValueMetaObjectRegisterData* meta,
	                                 const ibFilterRow& filter);

	// " ORDER BY <effective cols ASC/DESC>" — never empty (identity
	// tail guarantees at least one col).  reverse=true flips ASC<->DESC
	// across all cols for backward-walk.
	static wxString BuildOrderBy(const std::vector<OrderCol>& effective,
	                             bool reverse);

	// Cursor-anchor predicate appended to the WHERE chain.  Layout:
	//   col1 cmp_op ? AND col2 cmp_op ? AND ... AND colN cmp_op ?
	//     AND case when col1 = ? AND ... AND colN-1 = ?
	//              then colN strict_op ?
	//              else true
	//         end
	// where cmp_op = ">=" forward / "<=" backward, strict_op = ">" / "<".
	// Reset direction is forward + inclusive tail (strict_op = ">=").
	//
	// Returns either " WHERE ..." or " AND ..." depending on whereOpen.
	static wxString BuildAnchorPredicate(const std::vector<OrderCol>& effective,
	                                     bool whereAlreadyOpen,
	                                     ibFetchDirection direction);

	// Bind register filter params from m_filterRow.  Position is in/out.
	static void BindFilterParams(ibPreparedStatement* statement,
	                             int& position,
	                             const ibValueMetaObjectRegisterData* meta,
	                             const ibFilterRow& filter);

	// Bind anchor params: composite cmp_op pass (one per effective col)
	// then case-when tiebreak pass (eq on cols 0..N-2, strict on col N-1).
	// `sortValues` size must match `effective` size; values supplied by
	// the row at anchor time.
	static void BindAnchorParams(ibPreparedStatement* statement,
	                             int& position,
	                             const std::vector<OrderCol>& effective,
	                             const std::vector<ibValue>& sortValues);

private:
	ibRegisterSqlBuilder() = delete;
};

#endif
