# Next session — Closure capture Phase A handoff

## Read first (5 min)

1. `docs/closure-capture.md` — full v2 design (per-frame heap, NOT per-slot boxing). Pay attention to "Mechanism" + "Phase A" sections.
2. `docs/session-2026-05-11.md` — context for today's LINQ arc + the v1 rollback diagnosis ("First-try rollback") that motivated v2.
3. Memory: `project_closure_capture_design.md` (v2 summary), `feedback_no_search_collections.md` (the rule that killed v1).

## Why this matters

Chain-syntax LINQ is crippled without closures:
```c
var threshold = 5;
arr.Where(Function(x) { return x > threshold; })   // <-- threshold not visible inside today
```
Closure capture closes LINQ pivot risks #1 and #5 (chain syntax becomes self-sufficient; block syntax becomes a stylistic choice rather than a forced one).

## Key design constraint (user-flagged)

**No search-collections in single-pass compile**. v1 had `FindOrAddCapture(name, outerSlot, captureLevel)` linear-walking a per-lambda vector — rolled back. v2 sidesteps by using **per-frame** capture (one `shared_ptr<HeapFrame>` per enclosing function level), not per-slot.

## Phase A scope

Compile-side only. Runtime stays broken (m_pppArrayList[1] not wired yet). The goal of Phase A is to **emit correct OPER_GET / OPER_SET opcodes with `depth ≥ 1`** in lambda body bytecode + mark functions that need heap-allocated frames. Bytecode dump verifies emit; running closure code would crash (Phase B job).

### Step A.1 — fields

Add single boolean fields. **No vectors, no FindOrAdd, no Get/Set helpers** — just plain field access.

**`ibCompileContext`** (compileContext.h):
```cpp
struct ibCompileContext {
    // ... existing ...
    bool m_needsHeapFrame = false;   // any inner lambda → set during lambda parse
};
```

**`ibByteFunction`** (byteCode.h, around line 254-330):
```cpp
struct ibByteFunction {
    // ... existing ...
    bool m_needsHeapFrame = false;
};
```

The templated ctor `ibByteFunction(long lAddress, const CompileFn& src)` copies fields from compile-side. Add `m_needsHeapFrame(src.m_needsHeapFrame)` to the init list.

### Step A.2 — mark on lambda encounter

In `CompileLambdaExpression` (compileCode.cpp). Find where lambda body parsing begins. Right at entry (before any body parse), set the **enclosing function context's** `m_needsHeapFrame = true`.

"Enclosing function context" = walk `m_parentContext` up until you hit a context with `IsReturnFunction(m_numReturn)` (RETURN_FUNCTION or RETURN_PROCEDURE — NOT RETURN_LAMBDA_*, NOT RETURN_BLOCK, NOT RETURN_NONE). That's the immediate enclosing function. Set `m_needsHeapFrame = true` on its ibFunction. If nested deeper, the outer-outer function gets marked when a doubly-nested lambda is parsed inside another lambda.

Implementation hint: walk `m_parentContext` chain, set `m_needsHeapFrame` on EVERY function-level ctx between the lambda and root. Each level whose `m_needsHeapFrame` was already true short-circuits the rest of the walk (single-pass discipline — no repeated work).

### Step A.3 — GetVariable extends past lambda boundary

In `ibCompileContext::GetVariable` (compileContext.cpp). The current lambda discipline (line 126: `bool isLambdaFunction = IsReturnLambda(m_numReturn)`) restricts parent walk to root context only. Replace with **counting** logic:

```cpp
int captureDepth = 0;
for (ibCompileContext* pCur = m_parentContext; pCur; pCur = pCur->m_parentContext) {
    if (pCur->FindVariable(strVarName, currentVariable)) {
        ibParamUnit variable;
        variable.m_numArray = captureDepth;          // depth in lambda's m_pppArrayList
        variable.m_numIndex = currentVariable->m_numVariable;
        variable.m_strType  = currentVariable->m_strType;
        return variable;
    }
    if (IsReturnFunction(pCur->m_numReturn) || IsReturnLambda(pCur->m_numReturn)) {
        // crossed a function/lambda boundary — next level is one more
        // frame up in lambda's captured chain.
        captureDepth++;
    }
}
// Not found — fall through to bytecode chain walk + rootCtx fallback
// (existing logic, unchanged).
```

NB: `captureDepth` starts at 0 because m_parentContext is one step removed. For lambdas, captureDepth=1 means "first enclosing function frame", which lands in m_pppArrayList[1] at runtime.

### Step A.4 — verify via bytecode dump

Write a test:
```c
function makeAdder(n) {
    return Function(x) { return x + n; };
}
var add5 = makeAdder(5);
// (don't call add5 — runtime would crash, Phase B job)
```

Compile, dump bytecode for the lambda. Expect to see:
* `OPER_ADD` reading from `x` (depth=0, slot=lambda's x) and `n` (depth=1, slot=makeAdder's n).
* `makeAdder`'s ibByteFunction has `m_needsHeapFrame = true`.

Validation method: log emit positions or use existing `LinqCompileLog` infrastructure if helpful (or just inspect via debugger).

## Pitfalls

1. **Don't add Find/lookup helpers**. The whole point of v2 is no search-collections. If you find yourself writing `auto it = std::find_if(...)` in compile-side closure logic, STOP — wrong approach.
2. **Don't introduce new opcodes**. OPER_GET / OPER_SET with `depth ≥ 1` already work via m_pppArrayList chain. Closure just extends the chain at Phase B time.
3. **Don't touch block-scope (RETURN_BLOCK) contexts**. They share frames with enclosing function — captureDepth shouldn't increment crossing them.
4. **Lambda inside lambda**: captureDepth increments crossing EACH lambda boundary. So an inner-inner lambda referencing the outermost function's local goes through depth=2 (two function/lambda boundaries crossed).

## Validation criteria for "Phase A done"

* Build clean.
* Bytecode of the makeAdder example emits `OPER_GET depth=1` for `n` inside lambda body.
* `m_needsHeapFrame = true` on makeAdder's ibByteFunction in m_listFunc.
* `m_needsHeapFrame = false` on functions with no inner lambda (regression check — sample a few existing functions).
* `add5()` invocation crashes (Phase B job) — that's expected; Phase A is compile-only.
* All existing tests still pass (no regressions in non-closure lambda paths).

## After Phase A

Phase B writes runtime: HeapFrame struct, OPER_FUNC entry heap-allocation, OPER_LFUNC capture-frame copy, OPER_CALL_VAL m_pppArrayList wiring. After Phase B the makeAdder example actually runs and prints 15.

Then Phases C-F per `docs/closure-capture.md`.

## What NOT to do

* Don't try to do Phase B in the same pass. Compile-only first, validate emit, then runtime.
* Don't start with AOT (Phase E) — bytecode shape stabilizes through Phase A-D first.
* Don't add lambda escape analysis — that's an optimization for after Phase D works.
* Don't make `m_needsHeapFrame` per-slot or a vector. ONE boolean per function.

## Test fixture

Drop into `enterprise/test_closure_basic.txt` (uncommitted artifact, working-copy convention):
```c
function makeAdder(n) {
    return Function(x) { return x + n; };
}
var add5 = makeAdder(5);
// Phase A compile-only test — runtime not wired yet, don't call add5.
// Once Phase B lands, uncomment:
// Message(add5(10));   // expect 15
```

Phase B test:
```c
var i = 0;
function counter() {
    return Function() { i = i + 1; return i; };
}
var c = counter();
Message(c());   // 1
Message(c());   // 2
```
