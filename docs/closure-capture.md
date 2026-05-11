# Lambda closure capture — design

> **Status**: design v2 — 2026-05-11 PM (per-frame heap promotion).
> Replaces v1 (per-slot boxing + capture vectors) which was rolled
> back after the FindOrAddCapture helper was flagged as wrong shape
> (search-then-append on a vector violates single-pass + no-search-
> collections invariants).
>
> Aligns with original lambda design intent — lambdas have always been
> built on top of `m_pppArrayList`'s multi-layer chain (see
> `project_lambda.md`, `session-2026-05-10.md` "Lambda eval-in-lambda"
> addendum). Capture is just "extend the chain past lambda boundary".

---

## Semantics (unchanged from v1)

```c
function makeAdder(n) {
    return Function(x) { return x + n; };   // captures n
}
var add5 = makeAdder(5);
Message(add5(10));   // → 15
```

* **Lexical scope** — capture sees defining environment, not call site.
* **Live reference** — outer mutations visible inside lambda; inner
  mutations visible outside (when outer frame still alive).
* **Full parent chain** — nested lambdas can reach any enclosing
  function's locals through chained shared_ptr frames.
* **Compile-time resolved** — symbol resolution at compile decides
  depth; runtime is dumb dereference.

Counter / live-link example:
```c
function counter() {
    var i = 0;
    var bump = Function() { i = i + 1; return i; };
    return bump;
}
var c = counter();
Message(c());   // → 1
Message(c());   // → 2  (i lives between calls)
```

LINQ chain capture (closes pivot risk #5):
```c
var threshold = 5;
var bigOnes = arr.Where(Function(x) { return x > threshold; }).ToArray();
```

---

## Mechanism — heap-allocated frame, NOT per-slot boxing

A function that has any inner lambda gets its **entire local frame
heap-allocated** (`shared_ptr<HeapFrame>`). The lambda holds a
shared_ptr to that frame in its `m_capturedFrames` vector. Frame stays
alive as long as some lambda holds a reference, surviving outer's
return.

```cpp
struct HeapFrame {
    std::vector<ibValue> locals;
};

class ibValueFunction : public ibValue {
    ibByteCode* m_parentBc;
    int         m_funcIndex;
    // Captured chain — direct enclosing fn at [0], outer-outer at [1],
    // etc. Per-reference append at materialise; no dedup needed
    // because same shared_ptr regardless of how many lambdas capture
    // it. Vector size = nesting depth (≤ 3-5 typical for real code),
    // not per-slot.
    std::vector<std::shared_ptr<HeapFrame>> m_capturedFrames;
};
```

**Why NOT per-slot boxing** (v1's path, rolled back):
* v1 wanted `m_lambdaCaptures: vector<ibLambdaCapture{outerSlotIdx, captureLevel}>` on each lambda. Required `FindOrAddCapture` to dedup per-reference appends. Linear search on every lambda-body identifier resolution — exactly the "search-collection in single-pass" anti-pattern user flagged.
* v2 sidesteps this entirely: capture chain is per-frame (not per-slot). Slot resolution at runtime is direct `m_pppArrayList[depth][slotIdx]` — existing OPER_GET/OPER_SET dispatch works unchanged.

**Why NOT new opcodes**:
* v1 reserved 4 new opcodes (`OPER_GET_CAPTURED / SET_CAPTURED / GET_BOXED / SET_BOXED`). v2 reuses existing `OPER_GET / OPER_SET` with `depth ≥ 1` in `m_param2.m_numArray`. Lambda's m_pppArrayList already supports the chain (currently used for root-context bindings). Adding outer-fn-locals is just chain extension.

---

## Compile-time

A function's `ibCompileContext` gains a single boolean:

```cpp
struct ibCompileContext {
    bool m_needsHeapFrame = false;   // set on first inner-lambda encounter
    // ... existing fields ...
};
```

**During lambda body parse** (`CompileLambdaExpression`):
1. Mark `enclosing_function_context->m_needsHeapFrame = true`. One flag, no search.
2. Inside lambda's body parsing, when resolving an identifier:
   * Found in own scope (lambda's locals) → emit `OPER_GET` with `depth=0`.
   * Found in 1st enclosing function's scope → emit `OPER_GET` with `depth=1`.
   * Found in N-th enclosing function's scope → emit `OPER_GET` with `depth=N`.
   * Found in root context (Catalogs etc) → existing depth (rootCtx fallback, unchanged).
3. Per-reference: simple `OPER_GET` emit. No capture-vector push, no `FindOrAdd`.

**Identifier resolution walk** in `ibCompileContext::GetVariable`:
Currently lambda discipline restricts to root context. Extend:
* Walk `m_parentContext` chain.
* Track `captureDepth` counter (incremented per lambda boundary crossed).
* On hit in some ancestor function frame F → emit OPER_GET with that depth.
* On hit in root context → unchanged (depth = root-shim layer).

Single pass over identifier reference. No collection-searches.

---

## Runtime

### Function entry — heap-allocate when needed

`OPER_FUNC` dispatch (function body entry):
* If function's `m_needsHeapFrame` (from bc-side `ibByteFunction`): `make_shared<HeapFrame>` sized to `m_lVarCount`.
* `m_pppArrayList[0]` points into `heapFrame->locals.data()`.
* Otherwise (no inner lambda): current stack-frame behavior unchanged.

### Lambda materialise — capture chain

`OPER_LFUNC` (instantiate ibValueFunction):
* Walk current run context's frame stack. For each enclosing function frame (those with HeapFrame), copy shared_ptr into lambda's `m_capturedFrames`.
* Vector size = number of enclosing fn frames currently live. Trivial single-pass walk, no search.

```cpp
// Sketch:
auto lambda = make_shared<ibValueFunction>(bc, funcIdx);
for (frame : enclosing_fn_frames_with_heap) {
    lambda->m_capturedFrames.push_back(frame.heapFramePtr);
}
```

### Lambda invoke — restore chain into m_pppArrayList

`OPER_CALL_LAMBDA`:
* `m_pppArrayList[0]` = lambda's own frame (its locals).
* `m_pppArrayList[1..N]` = pointers into `m_capturedFrames[0..N-1]->locals`.
* `m_pppArrayList[N+1..]` = existing shim's root-context layers (unchanged).

OPER_GET / OPER_SET in lambda body use these depths directly. No new dispatch logic.

### Frame lifetime

Outer function returns:
* Frame's shared_ptr refcount decrements.
* If lambda holds a copy (escaped lambda), frame stays alive.
* If lambda doesn't escape (called only within outer's body and discarded), refcount → 0, frame freed.

---

## What changes vs current state

| Layer | Change |
|-------|--------|
| `ibCompileContext` | `bool m_needsHeapFrame` |
| `ibByteFunction` | mirror `bool m_needsHeapFrame` |
| `GetVariable` (compile) | walk parent chain past lambda boundaries; mark `m_needsHeapFrame` on hit function; return depth |
| `ibValueFunction` | `vector<shared_ptr<HeapFrame>> m_capturedFrames` |
| Runtime — function entry | heap-alloc HeapFrame when `m_needsHeapFrame` |
| Runtime — OPER_LFUNC | populate lambda's `m_capturedFrames` from enclosing frames |
| Runtime — OPER_CALL_LAMBDA | wire `m_pppArrayList[1..N]` to captured frames' locals |
| OPER_GET / OPER_SET | unchanged — already dispatch on `depth` |
| AOT | `m_needsHeapFrame` per fn serialized (v10) |
| Debugger Locals | walk lambda's `m_capturedFrames`, render with origin fn name |

**Zero new opcodes**. Zero new compile-side `Find*` helpers. Single boolean per fn, single vector per lambda (small).

---

## Phases

### Phase A — compile-side resolve + emit (no runtime change)

1. Add `m_needsHeapFrame` field on `ibCompileContext` (compile-side) and `ibByteFunction` (bc-side; mirror via existing templated ctor).
2. In `CompileLambdaExpression`: set enclosing-fn-context's `m_needsHeapFrame = true`.
3. In `ibCompileContext::GetVariable`: when lambda boundary crossed during parent walk, increment `captureDepth` counter, continue walking. On hit at function frame, return `ibParamUnit{m_numArray=captureDepth, m_numIndex=outerSlot}`.
4. Compile-only test: write closure example, dump bytecode, verify OPER_GET depths are correct. Runtime stays broken (m_pppArrayList[1] not wired to outer's frame yet — script that runs lambda body would crash).

### Phase B — runtime heap frame + lambda capture

1. Add `HeapFrame` struct (somewhere shared between runtime + compile).
2. `OPER_FUNC` entry: when current function's `m_needsHeapFrame`, allocate HeapFrame and use its `locals` for m_pppArrayList[0].
3. `OPER_LFUNC` materialise: walk current call stack's frames, copy shared_ptrs into lambda's `m_capturedFrames`.
4. `OPER_CALL_LAMBDA` invoke: wire `m_pppArrayList[k+1] = capturedFrames[k]->locals.data()` for k = 0..N-1.

Run closure example end-to-end.

### Phase C — chain capture (nested lambdas)

When lambda L1 captures from outer fn F, and L1's body has another lambda L2 that captures from F too:
* L2's m_capturedFrames = L1's m_capturedFrames + L1's own frame (if L1 has m_needsHeapFrame too).
* No extra logic — the materialise walk over "enclosing frames" handles it.

### Phase D — LINQ chain syntax unlocked

With closure capture working, chain syntax becomes self-sufficient:
```c
var threshold = 5;
var q = arr.Where(Function(x) { return x > threshold; }).ToArray();
```
Lambda's body resolves `threshold` to enclosing fn frame at depth=1. m_pppArrayList[1] wired to the captured HeapFrame. Works.

### Phase E — AOT v10

* Bump `kAOTFormatVersion = 10`.
* Serialize `m_needsHeapFrame` per function (single bool).
* Serialize lambda's `m_captureDepth` (single int — how many enclosing frames it captures).

### Phase F — Debugger Locals

* Render captured frames with origin fn name as a label (`captured from outer makeAdder: n = 5`).
* `m_capturedFrames` walk in `SendLocalVariables`.

---

## Risks

1. **Heap-alloc per function call** for functions containing inner lambda. Mitigation deferred: small-object pool / arena. Initial: plain `make_shared`.
2. **Escape analysis** not done initially — every function-with-inner-lambda heap-allocates even if lambda doesn't escape (called within and discarded). Future optimization: detect non-escaping case, keep stack frame.
3. **Frame lifetime** — outer's locals live beyond outer's return when lambda holds reference. Memory growth on closure-heavy code. Matches JS / Python; users manage.
4. **AOT migration** — v9 → v10 invalidates caches. One-time recompile.

---

## Decisions to remember

* **Per-frame, not per-slot.** Entire outer frame heap-allocated, one shared_ptr per nested level. No per-slot promotion decisions.
* **Existing opcodes**. OPER_GET / OPER_SET with `depth ≥ 1` already work via m_pppArrayList chain — closure just extends the chain.
* **Single boolean per fn**. `m_needsHeapFrame` set during lambda body parse. No vector of captured-slot records.
* **Materialise-time capture**. Lambda copies enclosing frames' shared_ptrs into `m_capturedFrames`. Append-only, no dedup needed (same frame's shared_ptr = same target).
* **Identifier resolution = depth assignment**. Compile walks parent chain past lambda boundary, returns `OPER_GET depth=N` where N is the enclosing fn frame's position in lambda's m_pppArrayList.

---

## Files expected to touch

| File | Role |
|------|------|
| `compiler/compileContext.{h,cpp}` | `m_needsHeapFrame` + GetVariable lambda-boundary walk |
| `compiler/compileCode.cpp` | `CompileLambdaExpression` marks enclosing fn |
| `compiler/byteCode.h` | `m_needsHeapFrame` mirror on `ibByteFunction` |
| `compiler/procContext.h` | `HeapFrame` struct (or new header) |
| `compiler/procUnit.cpp` | OPER_FUNC heap-alloc, OPER_LFUNC capture, OPER_CALL_LAMBDA wire |
| `compiler/value.h` / `procUnit.cpp` | `m_capturedFrames` on `ibValueFunction` |
| `compiler/byteCodeAOT.cpp` | v10 — serialize `m_needsHeapFrame`, `m_captureDepth` |
| `backend/debugger/debugServer.cpp` | render captured frames in `SendLocalVariables` |
| `docs/closure-capture.md` | this file (v2 design) |

---

## v1 rollback summary

The v1 approach (per-slot boxing + capture vectors) was rolled back after the `FindOrAddCapture(name, outerSlot, captureLevel)` helper landed. Three user-flagged problems:

1. Linear walk through `m_lambdaCaptures` looking for existing entry before append — anti-pattern.
2. Vectors-where-I-search = code smell in OES design.
3. Single-pass compile: each reference resolves once; mechanisms that re-visit / dedup cut against the grain.

v2 sidesteps all three by NOT having a per-lambda capture vector indexed by slot at all. The capture chain is per-frame (one shared_ptr per nested enclosing function level), which is naturally append-only and small (depth ≤ nesting count, not per-reference).
