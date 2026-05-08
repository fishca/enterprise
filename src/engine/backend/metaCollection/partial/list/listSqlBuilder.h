#ifndef __LIST_SQL_BUILDER_H__
#define __LIST_SQL_BUILDER_H__

#include "backend/tableInfo.h"

class ibDatabaseLayer;
class ibPreparedStatement;
class ibValueMetaObjectRecordDataRef;
class ibValueTableRow;

// Helpers shared by the paged Fetch impls.  Encapsulates the SQL
// fragments derived from ibFilterRow / ibSortOrder /
// ibValueMetaObjectRecordDataRef so we don't keep copying ~60 LOC of
// WHERE / ORDER-BY building into every concrete model.
//
// Static-only by design; this is a prototype for what will become a
// global query builder once the paging refactor stabilises.
class BACKEND_API ibListSqlBuilder {
public:
	// "SELECT * FROM <t> " on non-FB; "SELECT FIRST <firstN> * FROM <t> "
	// on Firebird (FB has no LIMIT — uses the FIRST clause inline).
	static wxString BuildSelectHead(ibDatabaseLayer* db,
	                                const wxString& tableName,
	                                int firstN);

	// " LIMIT <n>" on non-FB; empty on FB (covered in BuildSelectHead).
	static wxString AppendLimit(ibDatabaseLayer* db, int n);

	// " WHERE <chained predicates>" or empty if no active filters.
	// Reference attributes (ibValueMetaObjectAttributePredefined holding
	// the row's PK) compare against guidName; other attributes use
	// ibValueMetaObjectAttributeBase::GetCompositeSQLFieldName.
	//
	// `whereAlreadyOpen=true` means the caller already emitted " WHERE "
	// (e.g. FolderRef's parent-ref filter) and new clauses must be
	// AND'd on.  Default false matches the Ref / Register paths where
	// filter is the first WHERE source.
	static wxString BuildFilterWhere(const ibValueMetaObjectRecordDataRef* meta,
	                                 const ibFilterRow& filter,
	                                 bool whereAlreadyOpen = false);

	// " ORDER BY <fields ASC/DESC, ..., <guidName ASC/DESC>>" or empty
	// if no enabled sorts.  `reverse=true` flips ASC<->DESC; useful for
	// the backward-walk path when the SQL has to scan in inverse order
	// and then the buffer reverses the result.
	static wxString BuildOrderBy(const ibValueMetaObjectRecordDataRef* meta,
	                             const ibSortOrder& sort,
	                             bool reverse);

	// Cursor-anchor predicate appended to the WHERE chain. Builds the
	// "rows after this last row" condition matching the configured sort:
	//   non-reference sort cols    : composite >= ?  (or <= for backward)
	//   tail tiebreak on guidName  : case-when sortText then guid > 'last' else true end
	//
	// `whereAlreadyOpen=true` means caller already emitted " WHERE " and
	// new clauses are AND'd.  Returns either " AND ..." or " WHERE ..."
	// extension to be concatenated to the existing SQL fragment.
	static wxString BuildAnchorPredicate(const ibValueMetaObjectRecordDataRef* meta,
	                                     const ibSortOrder& sort,
	                                     bool whereAlreadyOpen,
	                                     ibFetchDirection direction,
	                                     const wxString& lastGuidStr);

	// Bind the parameters emitted by BuildFilterWhere. `position` is
	// in/out and is advanced past the bound params.
	static void BindFilterParams(ibPreparedStatement* statement,
	                             int& position,
	                             const ibValueMetaObjectRecordDataRef* meta,
	                             const ibFilterRow& filter);

	// Bind the parameters emitted by BuildAnchorPredicate's non-reference
	// sort columns. `sortValues` carries one ibValue per enabled
	// non-reference sort (in declaration order).  Reference (PK) sort is
	// inlined as a literal in the SQL — no parameter binding needed.
	static void BindAnchorSortParams(ibPreparedStatement* statement,
	                                 int& position,
	                                 const ibValueMetaObjectRecordDataRef* meta,
	                                 const ibSortOrder& sort,
	                                 const std::vector<ibValue>& sortValues);

private:
	ibListSqlBuilder() = delete;
};

#endif
