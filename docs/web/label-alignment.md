# Label-column alignment on web

Web counterpart of desktop's `ibVisualHost::CalculateAndApply`
(`visualView/visualHost.cpp`): when a form contains multiple
captioned controls in a vertical column, all their inputs line up
on the same x — the widest label sets the column width, every
other control reserves the same gap on its left.

The desktop pass measures via `wxDC::GetTextExtent` and writes the
width back into each `ibControlDynamicBorder::ApplyLabelSize`.
Server-side measurement is not viable on web (no font metrics,
no per-client DPI / accessibility scale), so the alignment runs
**client-side, after layout**.

---

## Mechanism

Three pieces working together inside `wfrontend/web/webClient.cpp`:

1. **`.labelrow` wrap with empty placeholder.** Every TextCtrl
   renders as a flex row `<div class="labelrow" data-labelrow="1">`
   containing `<span class="statictext">` (caption, possibly empty)
   and the input group `<div class="texted">`. The placeholder is
   emitted **even when `node.label` is empty** so the post-pass has
   a slot to set `min-width` on.

2. **`alignLabelsGlobal(rootEl)`** runs in `requestAnimationFrame`
   after the form root (`visualHost` / `mdiChild`) is in the DOM.
   It scans every `[data-labelrow] > .statictext` descendant,
   resets stale `min-width` / `display`, takes the global maximum
   of `offsetWidth` across **captioned** labels, then walks the
   tree to apply.

3. **`applyLabelColumnWalk(node, maxW, parentOrient)`** is the
   recursive applier. It mirrors the desktop's horizontal-sizer
   reset:

   - Vertical sizer (`orient===8`) — every `[data-labelrow]`
     direct child gets `min-width = maxW`.
   - Horizontal sizer (`orient===4`) — only the **first**
     `[data-labelrow]` direct child gets `min-width = maxW`; later
     siblings get `display:none` on their empty placeholder
     ("the column breaks", matching `Apply()`'s
     `parentSizer == wxHORIZONTAL → maxLabelWidth = wxNOT_FOUND`).
   - Non-sizer container (host root) — transparent recursion,
     keeps the parent orientation context.

When no captioned label exists anywhere on the form, all empty
placeholders are hidden so the flex gap doesn't leak a visible
indent before each input.

---

## Why post-render, not at render time

`offsetWidth` only reflects natural width once the element has
been measured by the browser. At `render()` time the form's root
is still detached from `document`, so widths read as `0`.
`requestAnimationFrame` runs after the next layout pass, so the
populated label's offsetWidth is correct. Re-running is
idempotent — every entry resets `min-width` / `display` before
measuring, so polling refreshes (`pollActive` swap) and explicit
control re-renders don't drift.

## Why min-width, not CSS Grid

An earlier attempt used `display:grid;grid-template-columns:
max-content 1fr` for column alignment. Two problems:

- Grid loses `wxSizer`'s per-child `proportion` / `flag` / `border`
  semantics: `applyLayout` translates these to `flex-grow` /
  `align-self:stretch` / `margin`, which apply only when the
  parent is a flex container. In grid mode `flex-grow` is ignored.
- Inline `display:flex` on the TextCtrl wrap, set during
  rendering, beat the external CSS rule that tried to switch the
  wrap to `display:contents` for grid-flatten. CSS specificity
  rules: inline always wins over external selectors.

Reverting to plain flex layout + post-render `min-width` keeps
the entire `applyLayout` flex translation working unchanged, and
matches desktop both visually and semantically (the column-break
on horizontal sizers is the same rule).

---

## Files

| File | Role |
|---|---|
| `src/engine/frontend/web/webClient.cpp` | CSS rule `.labelrow` (flex row); `TextCtrl.render` emits unconditional `data-labelrow` + empty `.statictext` placeholder; `render()` schedules `alignLabelsGlobal` on `visualHost` / `mdiChild`; `applyLabelColumnWalk` + `alignLabelsGlobal` definitions |
| `src/engine/frontend/visualView/visualHost.cpp::CalculateAndApply` | Desktop reference implementation — `Calculate` (recursive measurement) + `Apply` (recursive application with horizontal-reset). The `parentSizer == wxHORIZONTAL → maxLabelWidth = wxNOT_FOUND` line on apply is the column-break this web pass mirrors. |

---

## Limitations / future work

- **Inline-label inputs only.** The current scheme assumes the
  TextCtrl wrap carries its caption inline (`node.label` set
  by `ibValueTextCtrl`). A form that uses an **explicit sibling**
  `ibValueStaticText` + `ibValueTextCtrl` pair in a vertical sizer
  won't participate — they're two separate nodes, neither is a
  `[data-labelrow]`. Most current OES forms use the inline-label
  path, but the sibling-pair pattern would need either a
  compile-time wrap in `ibValueBoxSizer::ToJSON` or a JS
  detection heuristic in `render()`.
- **Only TextCtrl participates.** Future controls that have a
  column-aligning caption (ComboBox, DatePicker, SpinCtrl) — none
  yet implemented as `ibWeb<Control>` — would need to follow
  TextCtrl's pattern (set `.labelrow` class, `data-labelrow="1"`,
  emit `.statictext` placeholder).
- **No `BuildForm` on web.** `ibValueForm::BuildForm`
  (`visualView/ctrl/formObject.cpp`) is a no-op under
  `OES_USE_WEB`. This alignment kicks in only for forms with
  explicit metadata. When `BuildForm` is enabled on web, the
  generated UI will go through the same `data-labelrow` pipeline
  automatically.

---

## Quick test

`/w/<prefix>/form/<metaID>` for any form with a vertical sizer
containing at least one captioned TextCtrl plus other uncaptioned
TextCtrls. Inputs should line up on the same x; the longest label
sets the column width.

Nested horizontal sizer below the column: its first child should
keep the same x as the upper inputs (the column carries through);
its remaining children should sit at their natural width with no
left placeholder gap.
