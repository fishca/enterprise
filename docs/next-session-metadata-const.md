# Next session: ibMetaData const-protection

## Goal

Plug the only remaining backdoor in the const-meta refactor: runtime
path through `m_metaObject->GetMetaData()->FindXxx<X>(id)` returns
non-const `X*` and lets runtime mutate metadata via `ProcessCommand`,
property setters, etc.

After this session, `runtime → const meta → const metaData → const find* → const T*`
becomes a complete compile-time barrier.

## Surface to convert

All to overload pair (`const`-this returns `const T*`, non-const-this
returns `T*`):

**ibMetaData (in metaData.h):**
- `FindAnyObjectByFilter<T,T2>(...) const` × 3 overloads
- `FindObjectByFilter<T1,T2,T3>(...) const` × 2 overloads (helper)
- `FillArrayObjectByFilter<T,T2>(...) const` × 2 overloads (helper)
- `GetAnyArrayObject<T>(...) const` × 3 overloads
- `GetCommonMetaObject() const`

**ibValueMetaObject (in metaObject.h):**
- `FindAnyObjectByFilter<T,T2>(...) const` × 1 overload
- `FindObjectByFilter<T>(...) const` × 3 overloads (by name / ibMetaID / ibGuid)
- `FillArrayObjectByFilter<T>(...) const` × 1 overload
- `GetAnyArrayObject<T>(...) const` × 1 overload
- `GetMetaData() const` (root virtual; pure in `backend_type.h`,
  overridden in `metaObject.h`, `attribute/metaAttributeObject.h`,
  `partial/chartOfCharacteristicTypes.h`, `propertyManager/propertyObject.h`)

**Sibling collection helpers** (call `FillArrayObjectByFilter` from `const`
context — must be paired too, otherwise the cascade in callers doesn't
terminate):

- `metaInterfaceObject.h::GetInterfaceArrayObject() const → vector<X*>` (×2)
- Many `commonObject.h` `GetXxxArrayObject() const → vector<X*>` (line 184,
  191, 1001, 1019, 1026, 1033, 1040, 1156 from compile attempt)
- `partial/*.h` per-type collection helpers

**Derived overrides of `GetCommonMetaObject` and `GetMetaData`:**
- `metadataConfiguration.h` (`ibMetaDataConfiguration`)
- `metadataDataProcessor.h`
- `metadataReport.h`

## Estimated cascade

From the first attempt: ~4700 compile error instances, 68 unique. Dominant
errors were C2663 (call non-const on const) at every callsite that does
`for (auto* x : metaObject->GetXxxArrayObject()) x->NonConstMethod()`.

## Strategy — single big-bang batch

**Don't try incremental.** Every Find/Fill method is called from many
sibling helpers. Partial conversion creates a worse cascade than full
conversion. Plan one focused 2-4 hour session.

### Pass 1: add overload pairs to ALL tree-walker methods

Use the DRY pattern (Effective C++ Item 3) where the const overload
holds the real implementation and the non-const overload delegates via
`std::as_const` + `const_cast`:

```cpp
// non-const: delegate
template <typename T>
T* FindObjectByFilter(const ibMetaID& id, ...) {
    return const_cast<T*>(
        std::as_const(*this).template FindObjectByFilter<T>(id, ...));
}

// const: real implementation, returns const T*
template <typename T>
const T* FindObjectByFilter(const ibMetaID& id, ...) const {
    // walk m_children with `const auto& child` → child is `T* const&`
    // dynamic_cast<const T*>(child)  -- adds const, downcasts
    // dynamic_cast<const T*>(this)   -- self-match, no const_cast
}
```

Apply to every method in the surface list above.

### Pass 2: build, fix call sites that store `T*`

Cascading errors at sites like:

```cpp
ibValueMetaObjectModule* m = metaData->FindAnyObjectByFilter<ibValueMetaObjectModule>(id);
//                       ^^^ from const this → returns const* → assignment fails
```

Fix: `const ibValueMetaObjectModule* m = ...` (most cases — the call site
doesn't actually mutate).

### Pass 3: audit non-const callers

Sites that genuinely need to mutate the result (rare — designer paths
mostly). Either:
- move the calling method out of `const` context, or
- explicit `const_cast` at the boundary (and add memory note documenting why)

## Pre-work to do BEFORE this session

1. List all `Get*ArrayObject() const → vector<X*>` methods across
   `metaCollection/`. Grep:
   ```
   grep -rn "std::vector<\w*\*>\s*Get\w*ArrayObject\b" --include="*.h"
   ```
   Each must be paired (or made non-const if called from non-const-only context).
2. Verify `m_children` access through const method gives
   `ibValueMetaObject* const&` (mutable pointee). If yes, in const overloads
   `dynamic_cast<const _T1*>(child)` works without further casting.
3. Check that `static_cast<const _T1*>(child)` compiles where today's body
   uses `static_cast<_T1*>(child)` (it should — adding const is implicit).
4. Plan to delete the partial work from this session's attempt
   (the snippets are recoverable from git stash if useful).

## Skip if

Documentation-grade const protection is sufficient. Current state catches
accidental mutations through direct meta access (~80% of typical paths).
Only the `Find*`-indirected mutation remains. That's an ergonomic backdoor,
not a malicious one.

## Estimated time

2-4 hours focused. Mechanical pattern; cascade resolves in a few build
iterations once all sibling helpers are paired in pass 1.
