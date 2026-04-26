#include "wfrontend.h"

// Single-page client served by every web entry-point (wenterprise-server,
// future Apache/IIS/CGI modules). The blob lives here, not in the host, so
// adding a new entry-point doesn't require copy-pasting ~400 lines of HTML.
//
// MSVC has a ~16KB per-raw-string-literal cap. The two R"HTML(...)" pieces
// below are adjacent string literals — the C++ preprocessor concatenates
// them into one C string at compile time. Split point is after </style></head>.

static const char kClientHTML[] = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><title>OES Web</title>
<style>
body{margin:0;font:14px/1.4 system-ui,sans-serif;display:flex;height:100vh}
#sidebar{width:260px;border-right:1px solid #ccc;padding:10px;overflow:auto;background:#f7f7f7}
#main{flex:1;padding:12px;overflow:auto}
/* Browser-style tab strip: flat bottom border on the active tab merges
   with the content area so it reads as one surface. Inactive tabs sit
   slightly lower and darker — same visual language as Chrome/Firefox/
   VS Code, which most users will recognise. */
/* tabs-row: flex container holding the scroll buttons + the scrollable
   tab strip. Buttons collapse to 0 width when not needed (scroll fits
   entirely) so narrow windows don't waste horizontal space. */
#tabs-row{display:flex;align-items:stretch;background:#ececec;border-bottom:1px solid #bbb;padding-top:6px}
.tabs-scroll{flex:0 0 auto;border:none;background:transparent;color:#555;
  cursor:pointer;padding:0 6px;font:700 14px/1 Arial,sans-serif;
  align-self:stretch;display:none}
.tabs-scroll:hover{color:#000;background:#dcdcdc}
.tabs-scroll.visible{display:inline-flex;align-items:center}
#tabs{display:flex;gap:1px;margin:0;padding:0 8px 0;flex:1 1 auto;
  overflow-x:auto;scroll-behavior:smooth;scrollbar-width:none}
#tabs::-webkit-scrollbar{display:none}
.tab{padding:4px 10px 5px;border:1px solid #bbb;border-bottom:none;
  border-radius:6px 6px 0 0;background:#dcdcdc;font-size:.9em;
  cursor:pointer;display:inline-flex;align-items:center;gap:6px;
  color:#444;position:relative;top:1px;max-width:240px;
  flex:0 0 auto; /* don't shrink in the overflow-x flex container */
  transition:background .1s,color .1s}
.tab:hover{background:#e8e8e8;color:#222}
.tab.active{background:#fff;color:#000;border-color:#bbb;top:0;
  box-shadow:0 -1px 0 #fff inset;z-index:1}
/* Icon sits on the LEFT of the title (horizontal flow) — matches the
   way Chrome / VS Code / desktop wxAUI render their MDI tabs. No icon
   → the row collapses to text only without a layout shift. */
.tab-body{display:flex;flex-direction:row;align-items:center;
  gap:6px;min-width:0;cursor:pointer}
.tab-icon{width:16px;height:16px;object-fit:contain;flex:0 0 auto;
  image-rendering:-webkit-optimize-contrast}
.tab .tab-title{overflow:hidden;text-overflow:ellipsis;white-space:nowrap;
  max-width:200px;cursor:pointer;line-height:1.2}
/* Active tab title: bold so the current pane is obvious even at a
   glance. Modified gains a leading '*' on top of that (same sentinel
   desktop MDI uses to flag unsaved state). */
.tab.active .tab-title{font-weight:700}
.tab.modified .tab-title{font-weight:700}
.tab.modified .tab-title::before{content:'* '}
/* Close button: square so width==height at rest, flex-centered glyph so
   the × never drifts off the centreline when the tab padding changes.
   Font-size kept sub-em so ×'s visual weight matches the title text
   above, and box-sizing:border-box means hover's border/background
   don't jump the geometry. */
.tab-close{box-sizing:border-box;width:16px;height:16px;flex:0 0 16px;
  display:inline-flex;align-items:center;justify-content:center;
  border-radius:50%;cursor:pointer;color:#888;font:700 14px/1 Arial,sans-serif;
  padding:0;margin-left:2px;transition:background .1s,color .1s;
  user-select:none}
.tab-close:hover{background:#d73a3a;color:#fff}
h3{margin:.6em 0 .3em;font-size:.9em;color:#555;text-transform:uppercase;letter-spacing:.04em}
ul{list-style:none;padding:0;margin:0}
li{padding:4px 6px;cursor:pointer;border-radius:3px}
li:hover{background:#e3e9f0}
/* Host is flex column + full height so proportion-based growth on
   inner controls actually has room to expand into. Without this, the
   host was block-level with natural height and `flex-grow:1` on a
   child textctrl couldn't stretch because the parent chain had no
   defined height to share. */
.form-host{border:1px solid #ddd;border-radius:4px;padding:10px;
  display:flex;flex-direction:column;min-height:0;height:calc(100vh - 90px)}
.form-host > .boxsizer{flex:1;min-height:0}
/* align-items:flex-start so children stay at their natural cross-axis
   size by default. wxSizer semantics: a vertical sizer does NOT stretch
   children across the panel unless wxEXPAND is set — the default flex
   `align-items:stretch` would give every child full width, which is
   not what desktop does. applyLayout overrides per-child with
   align-self:stretch when wxEXPAND is present. */
.boxsizer{display:flex;gap:6px;margin:4px 0;align-items:flex-start}
.boxsizer.vert{flex-direction:column}
.boxsizer.horz{flex-direction:row}
/* Label-column alignment: vertical box-sizers whose children all expose
   a label (data-labelrow) render as a 2-column CSS grid; each child uses
   `display:contents` so its label and input become direct grid items
   (label in column 1, rest in column 2). Browser's max-content sizing
   lines up labels without server-side measurement — equivalent in effect
   to desktop's ibVisualHost::CalculateLabelSize. */
.boxsizer.vert.labelcol{display:grid;grid-template-columns:max-content 1fr;
  row-gap:4px;column-gap:8px;align-items:center}
.boxsizer.vert.labelcol > [data-labelrow]{display:contents}
.boxsizer.vert.labelcol > [data-labelrow] > .statictext{grid-column:1;align-self:center}
.boxsizer.vert.labelcol > [data-labelrow] > .texted{grid-column:2}
.gridsizer{display:grid;gap:6px;margin:4px 0}
.staticboxsizer{border:1px solid #bbb;border-radius:4px;padding:8px;margin:4px 0}
.staticboxsizer>legend{padding:0 6px;color:#555;font-weight:600}
/* pre-line preserves explicit \n in the caption while still wrapping
   at whitespace. Desktop's static text control draws multi-line
   captions natively; the default <span> textContent collapses
   newlines into a single space, so without this a two-line label
   shows as one run. */
.statictext{display:inline-block;padding:2px 0;white-space:pre-line}
/* Standalone buttons: platform-native-ish look with subtle gradient
   + hover/active shades. Matches the toolbar tool styling closely so
   the whole form reads as one control surface. */
button.ctrl{padding:5px 14px;border:1px solid #b5bfcc;border-radius:3px;
  background:linear-gradient(#fafbfc,#eef1f5);font:inherit;color:#222;
  cursor:pointer;min-width:72px}
button.ctrl:hover:not(:disabled){background:linear-gradient(#f2f6fc,#dde6f0);
  border-color:#8fa7c8}
button.ctrl:active:not(:disabled){background:#c6d5e8;border-color:#6b88ae}
button.ctrl:disabled{opacity:.55;cursor:default}
input.ctrl{padding:3px 6px;border:1px solid #aaa;border-radius:3px}
/* Text editor group: input + inline side-buttons share one border,
   mirroring desktop ibControlTextEditor where the Select/Open/Clear
   glyphs sit INSIDE the control's frame. Inner input has no own
   border; buttons are borderless icon-style buttons hugging the
   right edge. Hover highlights each button individually. */
.texted{display:flex;align-items:stretch;border:1px solid #aaa;
  border-radius:3px;background:#fff;overflow:hidden;min-height:0}
/* input/textarea and side-buttons inherit colour from the .texted
   group, so the fg/bg pushed inline by TextCtrl.render reaches them.
   background:transparent lets the group's bg show through uniformly
   across input + button strip. */
.texted > input,.texted > textarea{border:0;padding:3px 6px;outline:none;
  font:inherit;color:inherit;background:transparent;
  flex:1;min-width:0;min-height:0;resize:none}
.texted > button{border:0;border-left:1px solid rgba(0,0,0,.08);
  padding:0 8px;cursor:pointer;font:inherit;color:inherit;
  background:transparent;flex-shrink:0}
.texted > button:hover:not(:disabled){background:rgba(0,0,0,.06)}
.texted > button:active:not(:disabled){background:rgba(0,0,0,.12)}
.toolbar{display:flex;align-items:center;gap:2px;background:#eef1f4;border:1px solid #c9ced3;border-radius:3px;padding:3px 4px;margin:2px 0}
.tool{display:inline-flex;align-items:center;gap:4px;padding:3px 10px;border:1px solid transparent;border-radius:3px;background:transparent;font:inherit;color:inherit;cursor:pointer;line-height:1.3}
.tool:hover:not(:disabled){background:#dde6f0;border-color:#b5c4d6}
.tool:active:not(:disabled){background:#c6d5e8;border-color:#90a9c7}
.tool:disabled{opacity:.5;cursor:default}
.tool .tool-icon{display:inline-block;width:16px;height:16px;flex-shrink:0;vertical-align:middle}
.tool span.tool-icon{background:#9aa5b1;border-radius:2px}
.tool img.tool-icon{object-fit:contain}
.tool.tool-picture-only{padding:3px 6px}
.toolseparator{display:inline-block;width:1px;height:18px;background:#c9ced3;margin:0 3px}
#status{font-size:.85em;color:#888;margin-top:8px}
/* Top-right network indicator: shown after 500ms of in-flight work
   (loading spinner) or on fetch failure (connection-lost banner).
   Fixed position so the form below never jitters as it appears. */
#netStatus{position:fixed;top:8px;right:12px;padding:5px 12px;border-radius:4px;
  font-size:.85em;display:none;z-index:1000;box-shadow:0 1px 3px rgba(0,0,0,.1);
  transition:opacity .15s}
#netStatus.loading{background:#eef2ff;color:#3b50a0;border:1px solid #c4cfed}
#netStatus.error{background:#fce5e5;color:#8a2e2e;border:1px solid #e6aaaa}
#netStatus.debug-paused{background:#fff4e0;color:#a04020;border:1px solid #f0c0a0}
#netStatus .dot{display:inline-block;width:8px;height:8px;border-radius:50%;
  margin-right:6px;vertical-align:middle}
#netStatus.loading .dot{background:#6a84e0;animation:pulse 1s infinite}
#netStatus.error .dot{background:#c43b3b}
#netStatus.debug-paused .dot{background:#d06030;animation:pulse 1.2s infinite}
@keyframes pulse{0%,100%{opacity:1}50%{opacity:.3}}
/* Full-page overlay that hides the initial "Pick a form" placeholder
   and the tab-bar building up in parallel fetches. Removed in one go
   after login + loadForms + refreshTabs + paintActiveTab all resolve,
   so the user sees either nothing or the finished layout — no
   mid-construction flicker. */
#bootOverlay{position:fixed;inset:0;background:#fff;z-index:2000;
  display:flex;align-items:center;justify-content:center;
  transition:opacity .15s}
#bootOverlay.hide{opacity:0;pointer-events:none}
.bootSpinner{width:36px;height:36px;border:3px solid #c8c8c8;
  border-top-color:#3b50a0;border-radius:50%;
  animation:bootSpin 1s linear infinite}
@keyframes bootSpin{to{transform:rotate(360deg)}}
/* Debug indicator inside the sidebar status line. Reuses #status's
   existing real estate ("connecting..." / "connected") rather than
   adding a separate overlay or badge. */
#status .dbg{display:inline-block;margin-left:6px;padding:1px 6px;
  border-radius:4px;font-size:.78em;letter-spacing:.04em;font-weight:600}
#status .dbg.mode{background:#fdecea;color:#a04020;border:1px solid #f0c0b0}
#status .dbg.paused{background:#a04020;color:#fff;border:1px solid #802010}
/* Credentials prompt — shown when GET /auth-info says hasUsers=true.
   Stacked on top of bootOverlay; both disappear together once login
   succeeds and the initial fetches finish. */
#authOverlay{position:fixed;inset:0;background:#f3f5f9;z-index:2100;
  display:flex;align-items:center;justify-content:center}
#authOverlay.hide{display:none}
#authForm{background:#fff;padding:24px 28px;border-radius:8px;
  box-shadow:0 8px 24px rgba(0,0,0,.14);min-width:280px;
  display:flex;flex-direction:column;gap:10px;font-size:.92em}
#authForm h3{margin:0 0 4px;font-weight:600}
#authForm label{display:flex;flex-direction:column;gap:4px;color:#444}
#authForm input{padding:6px 8px;border:1px solid #c3c9d1;border-radius:4px;
  font-size:1em}
#authForm input:focus{outline:2px solid #3b50a0;outline-offset:-1px;border-color:#3b50a0}
#authError{color:#a22;font-size:.9em;min-height:1.1em}
#authError.hide{visibility:hidden}
#authForm button{padding:8px;background:#3b50a0;color:#fff;border:0;
  border-radius:4px;font-size:1em;cursor:pointer;margin-top:4px}
#authForm button:hover{background:#2f4189}
/* Big rotating-circle loader shown centred on the content area while
   a request is in flight > 500ms OR the host hasn't painted its tree
   yet. Fixed-position + offset by sidebar width so it sits over the
   main area (body is a flex row); semi-transparent so the previous
   tree stays faintly visible underneath, pointer-events:none so
   clicks still reach the DOM below once they become interactive. */
#spinner{position:fixed;top:0;left:260px;right:0;bottom:0;display:none;
  align-items:center;justify-content:center;background:rgba(255,255,255,.6);
  z-index:100;pointer-events:none}
#spinner.show{display:flex}
#spinner .ring{width:42px;height:42px;border:4px solid #dbe3f0;
  border-top-color:#3b50a0;border-radius:50%;animation:spin 1s linear infinite}
@keyframes spin{to{transform:rotate(360deg)}}
</style></head>)HTML"
// Split here: MSVC's ~16KB per-string-literal cap was blown by the added
// JS + CSS for toolbars/polling; adjacent string concatenation (C++
// standard) stitches the two R-strings back into one blob at compile time.
R"HTML(<body>
<div id="sidebar">
  <h3>Session</h3>
  <div id="status">connecting&hellip;</div>
  <h3>Forms</h3>
  <ul id="forms"></ul>
</div>
<div id="main">
  <div id="tabs-row">
    <button id="tabs-prev" class="tabs-scroll" type="button" aria-label="Scroll tabs left">&#8249;</button>
    <div id="tabs"></div>
    <button id="tabs-next" class="tabs-scroll" type="button" aria-label="Scroll tabs right">&#8250;</button>
  </div>
  <div id="tab-body"><p>Pick a form on the left.</p></div>
</div>
<div id="bootOverlay"><div class="bootSpinner"></div></div>
<div id="authOverlay" class="hide">
  <form id="authForm" autocomplete="on">
    <h3>Login</h3>
    <label>User <input id="authUser" name="user" type="text" autocomplete="username" autofocus></label>
    <label>Password <input id="authPwd" name="password" type="password" autocomplete="current-password"></label>
    <div id="authError" class="hide">Login failed</div>
    <button type="submit">Sign in</button>
  </form>
</div>
<script>
const API=location.pathname.replace(/\/$/,'');

// Tab-close cleanup: on pagehide fire a sendBeacon to /logout so the
// server DELETEs the session row immediately instead of waiting for the
// 2-minute idle-timeout sweep. The historic race with reload-GET / is
// no longer a concern — the server's GET / handler re-mints the
// session for an unknown id (cookie or header), so a beacon-destroyed
// session followed by a reload-GET lands on a fresh same-id session
// instead of the "half-destroyed" one from before. The 2-minute
// idle-timeout stays as belt-and-suspenders for browsers that drop
// beacons (Safari edge cases, Task Manager kill, tab crash).
const status=document.getElementById('status');
const formsList=document.getElementById('forms');
const main=document.getElementById('tab-body');
const tabsBar=document.getElementById('tabs');

// Top-right network-status indicator. Two states:
//   loading — at least one request has been in flight > 500ms (hides
//             the momentary sub-second fetches that would otherwise
//             flicker the banner on every click).
//   error   — last completed fetch failed (network down, !r.ok, or
//             fetch itself threw). Sticky until a subsequent success
//             clears it, so an intermittent poll failure is visible
//             even after it resolves.
const netStatus=document.createElement('div');
netStatus.id='netStatus';
netStatus.innerHTML='<span class="dot"></span><span class="txt"></span>';
document.body.appendChild(netStatus);
const netStatusTxt=netStatus.querySelector('.txt');
function showNetStatus(cls,text){
  netStatus.className=cls;
  netStatusTxt.textContent=text;
  netStatus.style.display='block';
}
function hideNetStatus(){ netStatus.style.display='none'; }

// Big centred spinner over the main content area — shows when the
// browser is waiting for a server response (host still painting,
// command still running). Raised after the same 500ms threshold so
// sub-second clicks stay flicker-free.
const spinner=document.createElement('div');
spinner.id='spinner';
spinner.innerHTML='<div class="ring"></div>';
// Fixed-position overlay — append to body so main.innerHTML=''
// during tree rebuilds doesn't wipe the spinner out.
document.body.appendChild(spinner);
function showSpinner(){ spinner.classList.add('show'); }
function hideSpinner(){ spinner.classList.remove('show'); }

let inflight=0, loadingTimer=null, lastError=false;
// Set by pollDebugStatus from /debug-status. dbgMode reflects wes
// process-wide --debug; dbgPaused reflects this tab's session being
// parked at a breakpoint. Used by beginRequest below to relabel the
// loading banner — when scripts may stop on a breakpoint, "Загрузка…"
// is misleading; "В режиме отладки" / "Отладчик: пауза" tells the user
// the real reason the UI is waiting.
let dbgMode   = false;
let dbgPaused = false;
// Threshold chosen so the common triad (poll + user click fetching
// back-to-back) completes under it on a healthy connection. Anything
// slower than this is genuinely worth a spinner. Polls are ~5ms,
// /tab close ~230ms, /action ~200ms — so 800ms only fires on real lag.
const LOADING_THRESHOLD_MS = 800;
function beginRequest(){
  // Fresh in-flight session → reset the timer to measure from now,
  // not from a poll that finished long ago.
  if(inflight===0 && loadingTimer){ clearTimeout(loadingTimer); loadingTimer=null; }
  inflight++;
  if(loadingTimer) return;
  loadingTimer=setTimeout(()=>{
    loadingTimer=null;
    if(inflight>0 && !lastError && !dbgPaused){
      // Relabel as "В режиме отладки" when wes was started with
      // --debug — any stall in this state likely means a breakpoint
      // is about to fire (or just fired) on the script thread.
      showNetStatus(dbgMode ? 'debug-paused' : 'loading',
                    dbgMode ? 'В режиме отладки' : 'Загрузка…');
      showSpinner();
    }
  }, LOADING_THRESHOLD_MS);
}
function endRequest(ok){
  inflight=Math.max(0,inflight-1);
  if(inflight===0 && loadingTimer){ clearTimeout(loadingTimer); loadingTimer=null; }
  if(ok){
    lastError=false;
    if(inflight===0 && !dbgPaused){ hideNetStatus(); hideSpinner(); }
  } else {
    lastError=true;
    if(!dbgPaused) showNetStatus('error','Нет связи с сервером');
    if(inflight===0) hideSpinner();
  }
}

// Per-tab session id. Lives in sessionStorage, which is scoped to a
// single browser tab — every new tab (or window) starts with its own
// fresh id, so the server can back each tab with a distinct ibWebSession.
// Cookie-only clients still work via the legacy fallback on the server,
// but all fetches we make here attach X-OES-Session so the server routes
// us correctly.
function genTabSid(){
  // Dashed UUID form — identical to ibGuid::str() on the server so
  // this one string serves as X-OES-Session header, oes_session
  // cookie, SessionManager map key, and sys_session.session PK.
  // crypto.randomUUID is the modern path; the fallback hand-rolls
  // a v4-ish layout so ancient browsers still produce a parsable
  // UUID shape.
  if (crypto && crypto.randomUUID) return crypto.randomUUID();
  const hex = Array.from({length:32},()=>((Math.random()*16)|0).toString(16)).join('');
  return hex.slice(0,8) + '-' + hex.slice(8,12) + '-4' + hex.slice(13,16)
    + '-' + ((8 + ((Math.random()*4)|0)).toString(16)) + hex.slice(17,20)
    + '-' + hex.slice(20,32);
}
let tabSid = sessionStorage.getItem('oes_tab_sid') || genTabSid();
sessionStorage.setItem('oes_tab_sid', tabSid);

// "Duplicate tab" copies sessionStorage verbatim — both tabs boot
// with the same oes_tab_sid, slam the same server-side ibWebSession,
// and the user sees cross-talk (one tab's tabs strip reflecting the
// other's clicks). Detect collisions via a BroadcastChannel handshake:
// on startup each tab pings its sid, any other tab with the same sid
// claims ownership, and the later arrival rotates to a fresh sid and
// hard-reloads so the rest of the page re-inits cleanly.
if (typeof BroadcastChannel !== 'undefined') {
  const bc = new BroadcastChannel('oes-tab-sid');
  let conflicted = false;
  bc.onmessage = (ev) => {
    if (!ev.data || ev.data.sid !== tabSid) return;
    if (ev.data.type === 'ping') {
      bc.postMessage({ type: 'claim', sid: tabSid });
    } else if (ev.data.type === 'claim') {
      conflicted = true;
    }
  };
  bc.postMessage({ type: 'ping', sid: tabSid });
  setTimeout(() => {
    if (conflicted) {
      tabSid = genTabSid();
      sessionStorage.setItem('oes_tab_sid', tabSid);
      // Clean reload — server mints a fresh session on the first hit
      // with the new tabSid. The original tab keeps its session intact.
      location.reload();
    }
  }, 150);
}

// Thin fetch wrapper — tracks in-flight count + last-success state so
// the banner can reflect them. Injects X-OES-Session on every request
// so the server picks the tab-local session over the shared cookie.
async function netFetch(url, opts){
  beginRequest();
  const o = Object.assign({}, opts || {});
  o.headers = Object.assign({}, o.headers || {}, { 'X-OES-Session': tabSid });
  try {
    const r=await fetch(url, o);
    endRequest(r.ok);
    return r;
  } catch (e) {
    endRequest(false);
    throw e;
  }
}

// Best-effort tab-close cleanup. pagehide (and the legacy beforeunload
// fallback) fire both on close and on reload/navigation, but the
// server's GET / mints a fresh session for an unknown id, so a racing
// reload just lands on a clean slate with the same tabSid — no broken
// state. sendBeacon is queued by the browser and delivered even after
// the page unloads; we pass ?sid because beacon can't attach headers.
window.addEventListener('pagehide', () => {
  try {
    navigator.sendBeacon(
      API + '/logout?sid=' + encodeURIComponent(tabSid));
  } catch (e) { /* best-effort */ }
});

// In-flight guard so overlapping refresh calls don't race on the DOM.
// The server-side /session is authoritative, so dropping a concurrent
// call is safe — the first one's result already reflects current state.
let refreshingTabs=false;
// metaGeneration captured on first successful /session read. Set to
// null until the first /session response arrives. See showSessionLost
// for how it distinguishes "config changed" from "session lost".
let initialMetaGen = null;
async function refreshTabs(){
  if(refreshingTabs) return;
  refreshingTabs=true;
  try{
  const info=await netFetch(API+'/session',{credentials:'same-origin'}).then(r=>r.json());
  // Capture the initial metaGeneration the first time we see it so
  // later 401s can report "configuration changed" instead of a generic
  // "session lost" when the server bumped it.
  if (initialMetaGen === null && info && typeof info.metaGeneration === 'number')
    initialMetaGen = info.metaGeneration;
  tabsBar.innerHTML='';
  // Dedup by index in case the server ever returns duplicates — cheap
  // defence that keeps the strip in one-to-one correspondence with
  // distinct backend tabs.
  const seen=new Set();
  (info.tabs||[]).filter(t=>{ const k=(t.index|0); if(seen.has(k)) return false; seen.add(k); return true; }).forEach((t,i)=>{
    const el=document.createElement('div');
    el.className='tab'+(i===info.activeTab?' active':'')+(t.modified?' modified':'');
    // Column-wrap: icon on top, title below. Matches MDI convention
    // (wxAuiMDIChildFrame paints icon then caption). No icon → column
    // collapses to text-only, no layout jump.
    const body=document.createElement('span');
    body.className='tab-body';
    if(t.hasIcon){
      const ico=document.createElement('img');
      ico.className='tab-icon';
      // <img src> can't attach X-OES-Session, so pass the per-tab
      // session id via query string — server prefers it over cookie.
      ico.src=API+'/tab/'+i+'/icon?sid='+encodeURIComponent(tabSid);
      ico.alt='';
      body.appendChild(ico);
    }
    const label=document.createElement('span');
    label.className='tab-title';
    const title=(t.formName||t.title||'#'+i);
    label.textContent=title;
    body.title=title;  // full name on hover when truncated
    body.appendChild(label);
    body.onclick=async()=>{
      // Optimistic active highlight: repaint the bar locally so the
      // click feels instant. No refreshTabs — switching tabs doesn't
      // change the tabs list, so another GET /session would be wasted;
      // polling reconciles server-side changes within 2s anyway.
      document.querySelectorAll('.tab').forEach(t=>t.classList.remove('active'));
      el.classList.add('active');
      const tree=await netFetch(API+'/tab/'+i,{method:'POST',credentials:'same-origin'}).then(r=>r.json());
      if(tree&&!tree.error){
        main.innerHTML='';
        main.appendChild(render(tree));
      }
    };
    el.appendChild(body);
    const close=document.createElement('span');
    close.className='tab-close';
    close.textContent='\u00d7';
    close.title='Close tab';
    close.onclick=async(e)=>{
      e.stopPropagation();
      const t0=performance.now();
      const wasActive=el.classList.contains('active');
      // Re-resolve the tab's index at click time: closing an earlier
      // tab shifts every later tab's index down by one on the server
      // (m_tabs.erase), but the captured `i` from forEach is frozen.
      // Using the stale `i` can target a non-existent tab → server
      // returns "{}" and the real tab silently survives, so next F5
      // shows the "ghost" back (user reports "Pick a form on the left"
      // alternating). Derive the current index from the DOM position
      // instead — refreshTabs rebuilt the strip in server order, so
      // the DOM index == server index.
      const realIndex=Array.from(tabsBar.children).indexOf(el);
      el.remove();
      if(wasActive) main.innerHTML='';
      const tFetchStart=performance.now();
      const r=await netFetch(API+'/tab/'+realIndex,{method:'DELETE',credentials:'same-origin'});
      const tFetchEnd=performance.now();
      const tree=await r.json();
      const tJsonEnd=performance.now();
      if(tree&&!tree.error&&Object.keys(tree).length>0){
        main.innerHTML='';
        main.appendChild(render(tree));
      }
      const tRenderEnd=performance.now();
      if (wasActive) {
        const siblings=tabsBar.querySelectorAll('.tab');
        if (siblings.length>0) siblings[siblings.length-1].classList.add('active');
      }
      // Re-fetch the tab strip so the remaining tabs get their
      // indices refreshed to match the server's new numbering. Without
      // this, the next click still carries stale `i` values.
      refreshTabs();
      console.log(`close tab ${realIndex}: total=${(tRenderEnd-t0).toFixed(1)}ms`
        + ` fetch=${(tFetchEnd-tFetchStart).toFixed(1)}ms`
        + ` json=${(tJsonEnd-tFetchEnd).toFixed(1)}ms`
        + ` render=${(tRenderEnd-tJsonEnd).toFixed(1)}ms`
        + ` inflight_now=${inflight}`);
    };
    el.appendChild(close);
    tabsBar.appendChild(el);
  });
  } finally { refreshingTabs=false; }
}

)HTML"
// MSVC C2026: a single raw string literal can't exceed ~16KB. The
// full kClientHTML is stitched from adjacent literals (the C++ compiler
// concatenates them at compile time); each break point sits in inert
// whitespace between functions so no token is split across literals.
R"HTML(
async function login(user, password){
  const body = new URLSearchParams({user:user||'',password:password||''});
  const r = await netFetch(API+'/login',
    {method:'POST',body,credentials:'same-origin'});
  return r.ok;
}
// GET /auth-info. Decides between open-access auto-login (empty sys_user
// → server's /login passes through empty creds) and a credentials form
// (sys_user populated — we must ask the user). Failure to reach the
// endpoint is treated as "no auth required" so a stale fetch doesn't
// block a working configuration.
async function authInfo(){
  try {
    const r = await fetch(API+'/auth-info',{credentials:'same-origin'});
    if (!r.ok) return { hasUsers: false };
    return await r.json();
  } catch (e) { return { hasUsers: false }; }
}
// Show #authOverlay, resolve the returned promise on successful login.
// Repeated wrong creds clear the password input and flash the error.
function promptLogin(){
  return new Promise(resolve => {
    const overlay = document.getElementById('authOverlay');
    const form    = document.getElementById('authForm');
    const userEl  = document.getElementById('authUser');
    const pwdEl   = document.getElementById('authPwd');
    const errEl   = document.getElementById('authError');
    overlay.classList.remove('hide');
    userEl.focus();
    form.addEventListener('submit', async (ev) => {
      ev.preventDefault();
      errEl.classList.add('hide');
      const ok = await login(userEl.value, pwdEl.value);
      if (ok) {
        overlay.classList.add('hide');
        resolve(true);
        return;
      }
      pwdEl.value = '';
      errEl.classList.remove('hide');
      pwdEl.focus();
    });
  });
}
async function loadForms(){
  const forms=await netFetch(API+'/forms',{credentials:'same-origin'}).then(r=>r.json());
  formsList.innerHTML='';
  for(const f of forms){
    const li=document.createElement('li');
    li.textContent=f.synonym||f.name||('#'+f.id);
    li.onclick=()=>openForm(f.id);
    formsList.appendChild(li);
  }
}
async function openForm(id){
  const tree=await netFetch(API+'/form/'+id,{credentials:'same-origin'}).then(r=>r.json());
  main.innerHTML='';
  main.appendChild(render(tree));
  refreshTabs();
}
// Per-type control classes. Each owns its own render(node)->HTMLElement
// — new control = new class here, no need to touch the central switch.
// The registry at the bottom maps ibWebWindow::GetControlType() strings
// to the class that knows how to draw them.

class BaseControl{
  render(node){
    const el=document.createElement('div');
    el.style.cssText='border:1px dashed #ccc;padding:4px;margin:2px 0;font-size:.85em;color:#888';
    el.textContent='['+node.type+']';
    return el;
  }
  // Shared post-pass applied by the dispatcher to every node.
  // Applies shown/enabled plus the visual-style fields pushed from
  // ibValueWindow::UpdateWindow (fg/bg/font/tooltip) — one place so
  // every control gets them without per-class code.
  applyCommon(el,node){
    if(node.shown===false) el.style.display='none';
    if(node.enabled===false){ if('disabled' in el) el.disabled=true; else el.style.opacity='.5'; }
    if(node.fg) el.style.color=node.fg;
    if(node.bg) el.style.backgroundColor=node.bg;
    if(node.font){
      if(node.font.size)   el.style.fontSize=node.font.size+'pt';
      if(node.font.family) el.style.fontFamily=node.font.family;
      if(node.font.bold)   el.style.fontWeight='bold';
      if(node.font.italic) el.style.fontStyle='italic';
      if(node.font.underline) el.style.textDecoration='underline';
    }
    if(node.tooltip) el.title=node.tooltip;
    if(node.minSize){
      if(node.minSize.w) el.style.minWidth =node.minSize.w+'px';
      if(node.minSize.h) el.style.minHeight=node.minSize.h+'px';
    }
    if(node.maxSize){
      if(node.maxSize.w) el.style.maxWidth =node.maxSize.w+'px';
      if(node.maxSize.h) el.style.maxHeight=node.maxSize.h+'px';
    }
  }
}

class HostControl extends BaseControl{
  render(){ const el=document.createElement('div');el.className='form-host';return el; }
}
class BoxSizer extends BaseControl{
  // orient: wxHORIZONTAL=4, wxVERTICAL=8.
  render(node){
    const el=document.createElement('div');
    el.className='boxsizer '+(node.orient===8?'vert':'horz');
    return el;
  }
}
class WrapSizer extends BaseControl{
  render(){
    const el=document.createElement('div');
    el.className='boxsizer horz';el.style.flexWrap='wrap';
    return el;
  }
}
class StaticBoxSizer extends BaseControl{
  render(node){
    const el=document.createElement('fieldset');el.className='staticboxsizer';
    const lg=document.createElement('legend');lg.textContent=node.label||'';
    el.appendChild(lg);
    return el;
  }
}
class GridSizer extends BaseControl{
  render(node){
    const el=document.createElement('div');el.className='gridsizer';
    el.style.gridTemplateColumns=`repeat(${Math.max(1,node.cols||1)},auto)`;
    return el;
  }
}
class StaticText extends BaseControl{
  render(node){
    const el=document.createElement('span');el.className='statictext';
    el.textContent=node.label||'';
    return el;
  }
}
class Button extends BaseControl{
  render(node){
    const el=document.createElement('button');el.className='ctrl';
    // Representation (0=Auto, 1=Text, 2=Picture, 3=PictureAndText) —
    // server resolves Auto before pushing, so 0 shouldn't arrive. Picture
    // is a data:image/png;base64 URI straight from ibBackendPicture.
    const rep=node.representation;
    const wantPic=node.hasPicture && node.picture && rep!==1;
    const wantText=rep!==2;
    if(wantPic){
      const img=document.createElement('img');
      img.src=node.picture;img.alt='';
      img.style.cssText='height:1em;vertical-align:middle;'+(wantText?'margin-right:4px;':'');
      el.appendChild(img);
    }
    if(wantText) el.appendChild(document.createTextNode(node.label||''));
    if(node.id){
      el.onclick=async()=>{
        const r=await netFetch(API+'/action/'+node.id,{method:'POST',credentials:'same-origin'});
        if(r.ok){
          const tree=await r.json();
          if(tree&&!tree.error){ main.innerHTML=''; main.appendChild(render(tree)); }
        }
      };
    }
    return el;
  }
}
)HTML"
// Third split: TextCtrl + Toolbar + Tool + CheckBox go in their own
// R-string literal. Same MSVC 16KB per-literal cap kept biting as the
// textctrl renderer grew with label prefix + side-buttons.
R"HTML(
class TextCtrl extends BaseControl{
  render(node){
    const multi=!!node.multilineMode;
    // Outer row: label (prefix) + input-group (input + side buttons
    // sharing one border). Matches desktop ibControlTextEditor layout
    // where the caption sits left-of-frame and the Select/Open/Clear
    // glyphs sit INSIDE the frame flush to the right edge.
    const wrap=document.createElement('div');
    wrap.style.cssText='display:flex;align-items:stretch;gap:6px;min-height:0';
    // Signal for the containing vertical box-sizer: this child has a
    // label+control pair. If >=2 siblings carry this attr, the sizer
    // switches to CSS-grid layout (see .boxsizer.vert.labelcol).
    if(node.label) wrap.dataset.labelrow='1';
    if(node.label){
      const lbl=document.createElement('span');
      lbl.className='statictext';
      lbl.textContent=node.label;
      lbl.style.flexShrink='0';
      lbl.style.alignSelf='center';
      wrap.appendChild(lbl);
    }
    // Input group — shared border wraps input + inline side-buttons.
    const group=document.createElement('div');
    group.className='texted';
    group.style.flex='1';
    // data-ctrl-id lets post-render refocus logic (e.g. after Clear)
    // find the specific input even though main.innerHTML was replaced.
    if(node.id) group.dataset.ctrlId=node.id;
    // .texted has a hardcoded #fff background that would otherwise mask
    // the fg/bg that applyCommon just set on the outer wrap. Push node
    // colours onto the group directly so the input (background:transparent)
    // inherits through.
    if(node.bg) group.style.backgroundColor=node.bg;
    if(node.fg) group.style.color=node.fg;
    const el=document.createElement(multi?'textarea':'input');
    // No .ctrl class — border/padding is on .texted now.
    if(!multi) el.type=node.passwordMode?'password':'text';
    el.value=node.value||'';
    // commit declared at render scope so side-button handlers below
    // can removeEventListener it before teardown — prevents a racing
    // synthetic 'change' on the about-to-be-removed input.
    let commit=null;
    if(node.id){
      // Commit on blur + Enter. wxEVT_CONTROL_TEXT_INPUT per-keystroke
      // would be noisy for HTTP; send once when the user is done.
      commit=async()=>{
        const body=new URLSearchParams({value:el.value});
        const r=await netFetch(API+'/change/'+node.id,{
          method:'POST',credentials:'same-origin',
          headers:{'Content-Type':'application/x-www-form-urlencoded'},
          body:body.toString()
        });
        if(r.ok){
          const tree=await r.json();
          if(tree&&!tree.error){ main.innerHTML=''; main.appendChild(render(tree)); }
        }
      };
      el.addEventListener('change',commit);
      if(!multi){
        el.addEventListener('keydown',ev=>{ if(ev.key==='Enter'){ ev.preventDefault(); commit(); } });
      }
    }
    group.appendChild(el);
    // Desktop ibControlTextEditor side buttons: Select (…), Open (▷),
    // Clear (×). Rendered INSIDE the input group's border, flush to
    // the right, same shared-frame look as desktop. Each click fires
    // /fire/<id>/<kind> → ibWebTextCtrl::HandleRequest → FireCommand
    // → ibValueTextCtrl's OnSelect/Open/ClearButtonPressed handler.
    const makeSideBtn=(label,kind,title)=>{
      const btn=document.createElement('button');
      btn.type='button';
      btn.textContent=label;
      btn.title=title;
      // Prevent focus theft: without this the input blurs on mousedown,
      // which fires its 'change' listener and POSTs /change concurrently
      // with our /fire. The two requests race — user saw Clear needing
      // two clicks because /change arrived AFTER /fire and re-set the
      // just-cleared value. preventDefault on mousedown keeps focus on
      // the input, no blur, no change event, no race.
      btn.addEventListener('mousedown', e => e.preventDefault());
      btn.onclick=async()=>{
        // Detach the input's commit listener BEFORE the fetch. When the
        // response's innerHTML='' removes the input, some browsers fire a
        // synthetic 'change' on the dirty node — that would POST /change
        // with uncommitted text right after /fire cleared it. User saw
        // Clear leaving a partial prefix of a long typed string. Removing
        // the listener makes the teardown silent.
        if(commit) el.removeEventListener('change', commit);
        const r=await netFetch(API+'/fire/'+node.id+'/'+kind,
          {method:'POST',credentials:'same-origin'});
        if(r.ok){
          const tree=await r.json();
          if(tree&&!tree.error&&Object.keys(tree).length>0){
            main.innerHTML=''; main.appendChild(render(tree));
            // Clear refocuses the (now empty) input with cursor at 0 —
            // matches desktop ibControlTextEditor::SetInsertionPoint(wxNOT_FOUND).
            if(kind==='buttonClear'){
              const ed=main.querySelector(`[data-ctrl-id="${node.id}"] input, [data-ctrl-id="${node.id}"] textarea`);
              if(ed){ ed.focus(); try{ ed.setSelectionRange(0,0); }catch(_){ } }
            }
          }
        }
      };
      group.appendChild(btn);
    };
    if(node.id && node.showSelectButton) makeSideBtn('…','buttonSelect','Select');
    if(node.id && node.showOpenButton)   makeSideBtn('▷','buttonOpen','Open');
    if(node.id && node.showClearButton)  makeSideBtn('×','buttonClear','Clear');
    wrap.appendChild(group);
    return wrap;
  }
}

class Toolbar extends BaseControl{
  // Container. The children render themselves (Tool / ToolSeparator);
  // the toolbar's job is just to lay them out horizontally on a
  // panel-style strip background.
  render(){ const el=document.createElement('div');el.className='toolbar';return el; }
}

class Tool extends BaseControl{
  // Representation modes from the server (ibRepresentation enum):
  //   0 Auto  — server resolves before sending; browser never sees 0
  //   1 Text            — label only, no icon
  //   2 Picture         — icon only, no label
  //   3 PictureAndText  — icon + label
  //
  // node.picture, when present, is a data:image/png;base64,... URI
  // produced server-side via ibBackendPicture::CreateBase64Image.
  // hasPicture=true with empty node.picture falls back to a CSS block.
  render(node){
    const el=document.createElement('button');el.className='tool';
    el.type='button';

    const rep = node.representation|0;
    const showIcon = (rep === 2 || rep === 3) && !!node.hasPicture;
    // Picture mode = icon only, Text = label only, PictureAndText =
    // both, Auto (0) resolved server-side.
    const showText = !!node.label && (rep === 1 || rep === 3);
    if (rep === 2 && node.hasPicture) el.classList.add('tool-picture-only');

    if (showIcon) {
      if (node.picture) {
        // Server-encoded PNG data URI — render directly.
        const ic = document.createElement('img');
        ic.className = 'tool-icon';
        ic.src = node.picture;
        ic.alt = node.label || '';
        el.appendChild(ic);
      } else {
        // Placeholder when hasPicture is true but no data — e.g. a
        // picture ref that didn't resolve to a bitmap.
        const ic = document.createElement('span');
        ic.className = 'tool-icon';
        el.appendChild(ic);
      }
    }
    if (showText) {
      const lbl = document.createElement('span');
      lbl.textContent = node.label;
      el.appendChild(lbl);
    }
    if (node.enabled===false) el.disabled=true;
    if(node.id){
      el.onclick=async()=>{
        const r=await netFetch(API+'/action/'+node.id,{method:'POST',credentials:'same-origin'});
        if(r.ok){
          const tree=await r.json();
          if(tree&&!tree.error){
            main.innerHTML='';
            if(Object.keys(tree).length>0) main.appendChild(render(tree));
          }
          // Tool actions can close / open / switch forms (ActionClose,
          // custom scripts that call OpenForm). Refresh the tab strip
          // so the dying tab disappears immediately instead of waiting
          // for the next 2s poll tick — fast now that localhost
          // requests redirect to 127.0.0.1 (no IPv6 fallback tax).
          refreshTabs();
        }
      };
    }
    return el;
  }
}

class ToolSeparator extends BaseControl{
  render(){ const el=document.createElement('span');el.className='toolseparator';return el; }
}

class CheckBox extends BaseControl{
  render(node){
    const wrap=document.createElement('label');wrap.className='ctrl checkbox';
    const el=document.createElement('input');el.type='checkbox';
    el.checked=!!node.value;
    if(node.id){
      el.addEventListener('change',async()=>{
        const body=new URLSearchParams({checked:el.checked?'1':'0'});
        const r=await netFetch(API+'/toggle/'+node.id,{
          method:'POST',credentials:'same-origin',
          headers:{'Content-Type':'application/x-www-form-urlencoded'},
          body:body.toString()
        });
        if(r.ok){
          const tree=await r.json();
          if(tree&&!tree.error){ main.innerHTML=''; main.appendChild(render(tree)); }
        }
      });
    }
    wrap.appendChild(el);
    if(node.label){ const s=document.createElement('span');s.textContent=' '+node.label;wrap.appendChild(s); }
    return wrap;
  }
}

)HTML"
// Second split in the JS chunk — renderer classes (above) live in one
// R-string; the dispatch map, layout flags, render() and polling loop
// (below) live in another. Keeps each literal comfortably under MSVC's
// ~16KB per-raw-string cap as the client keeps growing.
R"HTML(
const renderers={
  'visualHost':    new HostControl(),
  'mdiChild':      new HostControl(),
  'boxsizer':      new BoxSizer(),
  'wrapsizer':     new WrapSizer(),
  'staticboxsizer':new StaticBoxSizer(),
  'gridsizer':     new GridSizer(),
  'statictext':    new StaticText(),
  'button':        new Button(),
  'textctrl':      new TextCtrl(),
  'checkbox':      new CheckBox(),
  'toolbar':       new Toolbar(),
  'tool':          new Tool(),
  'toolseparator': new ToolSeparator(),
};
const fallback=new BaseControl();

// wx layout flag constants (match wx/defs.h) — used to turn each
// child's "layout": {proportion, flag, border} hint into CSS on its
// flex container. proportion -> flex-grow, wxEXPAND -> flex-grow=1,
// wxLEFT/RIGHT/UP/DOWN + border -> per-side margin, wxALIGN_* ->
// align-self on the main/cross axis.
const WX_LEFT=0x0010, WX_RIGHT=0x0020, WX_TOP=0x0040, WX_BOTTOM=0x0080;
const WX_ALIGN_RIGHT=0x0200, WX_ALIGN_CENTER_H=0x0100;
const WX_ALIGN_BOTTOM=0x0400, WX_ALIGN_CENTER_V=0x0800;
const WX_EXPAND=0x2000;

function applyLayout(el, layout){
  if(!layout) return;
  const p=layout.proportion|0, f=layout.flag|0, b=layout.border|0;
  // Main axis: proportion > 0 → share the remaining space.
  // Cross axis: only wxEXPAND stretches. proportion alone does NOT
  // imply cross-axis growth — desktop wxSizer treats them as
  // independent axes (form1 textctrl: prop=1, no EXPAND → grows
  // vertically, width stays natural).
  if(p>0) el.style.flexGrow=String(p);
  if(f & WX_EXPAND) el.style.alignSelf='stretch';
  if(b>0){
    if(f & WX_LEFT)   el.style.marginLeft  =b+'px';
    if(f & WX_RIGHT)  el.style.marginRight =b+'px';
    if(f & WX_TOP)    el.style.marginTop   =b+'px';
    if(f & WX_BOTTOM) el.style.marginBottom=b+'px';
  }
  // Explicit align flags override the default flex-start.
  if(f & WX_ALIGN_CENTER_V || f & WX_ALIGN_CENTER_H) el.style.alignSelf='center';
  else if(f & WX_ALIGN_RIGHT || f & WX_ALIGN_BOTTOM) el.style.alignSelf='flex-end';
}

function render(node){
  if(!node||typeof node!=='object') return document.createTextNode('');
  const ctrl=renderers[node.type]||fallback;
  const el=ctrl.render(node);
  ctrl.applyCommon(el,node);
  for(const kid of (node.children||[])){
    const childEl=render(kid);
    applyLayout(childEl, kid.layout);
    el.appendChild(childEl);
  }
  // Auto-detect label-column: vertical box-sizer with >=2 direct children
  // carrying data-labelrow → switch to CSS-grid alignment. One heuristic
  // lives here, not in Boxsizer.render, because detection needs children
  // already attached.
  if(node.type==='boxsizer' && node.orient===8){
    let labelRows=0;
    for(const c of el.children) if(c.dataset && c.dataset.labelrow) labelRows++;
    if(labelRows>=2) el.classList.add('labelcol');
  }
  return el;
}

// Poll the active host every few seconds so timer-driven updates
// surface between user actions. Diff by JSON string to avoid pointless
// DOM rebuilds. Pauses when the tab is hidden (visibility API) — no
// point fetching when the user isn't looking. Replace-on-change keeps
// the server's state authoritative; pending local edits on form
// inputs are preserved by the browser between polls only up to the
// next mismatch, which is the expected desktop-like behaviour.
let lastActiveJson = null;
async function pollActive(){
  if (inflight > 0) return;
  const t0 = performance.now();
  try {
    const r = await netFetch(API+'/active', {credentials:'same-origin'});
    const tFetch = performance.now();
    // 401 = session expired / server restarted / idle-timeout evicted.
    // Show a sticky banner with a reload button and stop polling — the
    // user has to reload to mint a fresh session + re-run OnStart.
    if (r.status === 401) { showSessionLost(); return; }
    if (!r.ok) { console.log(`poll /active: fetch=${(tFetch-t0).toFixed(1)}ms NOT OK`); return; }
    const text = await r.text();
    const tText = performance.now();
    if (text === lastActiveJson || text === '{}') {
      if (tText - t0 > 300)
        console.log(`poll /active: fetch=${(tFetch-t0).toFixed(1)}ms read=${(tText-tFetch).toFixed(1)}ms (no change)`);
      return;
    }
    lastActiveJson = text;
    const tree = JSON.parse(text);
    const tParse = performance.now();
    if (tree && !tree.error) {
      main.innerHTML = '';
      main.appendChild(render(tree));
    }
    const tRender = performance.now();
    if (tRender - t0 > 300)
      console.log(`poll /active: fetch=${(tFetch-t0).toFixed(1)}ms read=${(tText-tFetch).toFixed(1)}ms parse=${(tParse-tText).toFixed(1)}ms render=${(tRender-tParse).toFixed(1)}ms`);
  } catch (e) {
    console.log(`poll /active failed after ${(performance.now()-t0).toFixed(1)}ms`, e);
  }
}
let pollTimer = null;
function startPolling(){
  if (pollTimer) return;
  pollTimer = setInterval(pollActive, 2000);
}
function stopPolling(){
  if (pollTimer) { clearInterval(pollTimer); pollTimer = null; }
}

// Live updates via Server-Sent Events.
// Server pushes "data: <json>" frames when state changes (button click,
// timer tick, tab switch, script refresh). Falls back to 2s polling when
// EventSource is missing, connection errors, or the tab becomes hidden
// (SSE costs one open socket per tab — no point keeping it alive while
// the user isn't looking).
let liveSource = null;
function applyActiveJsonText(text){
  if (text === lastActiveJson || text === '{}' || !text) return;
  lastActiveJson = text;
  try {
    const tree = JSON.parse(text);
    if (tree && !tree.error) {
      main.innerHTML = '';
      main.appendChild(render(tree));
    }
  } catch (e) {
    console.log('sse parse failed', e);
  }
}
function startLive(){
  if (liveSource) return;
  if (typeof EventSource === 'undefined') { startPolling(); return; }
  try {
    liveSource = new EventSource(API + '/stream', { withCredentials: true });
  } catch (e) {
    console.log('SSE unavailable, falling back to polling', e);
    startPolling();
    return;
  }
  liveSource.onmessage = ev => applyActiveJsonText(ev.data);
  liveSource.onerror = () => {
    // Either the server dropped us or the network flapped. Tear down
    // the EventSource and fall back to polling — polling renews the
    // error banner via netFetch on each failed cycle, so the user sees
    // the status. When the network recovers, the page reload (or next
    // startLive call on visibilitychange) re-upgrades to SSE.
    if (liveSource) { liveSource.close(); liveSource = null; }
    startPolling();
  };
  // If we upgrade to SSE mid-session, make sure the polling loop isn't
  // also firing — doubles up on server work and fights with lastActiveJson.
  stopPolling();
}
function stopLive(){
  if (liveSource) { liveSource.close(); liveSource = null; }
}

// Session-lost banner — fires when /active returns 401 (server restart,
// idle-timeout eviction, or explicit destroy). Blocks the page so the
// user doesn't keep typing into inputs whose commits will silently fail,
// and offers a one-click reload to mint a fresh session.
let sessionLostShown = false;
async function checkMetaChanged(){
  // Best-effort — the /session endpoint carries metaGeneration even
  // when the session is gone (auth=false stub still includes it), so
  // this works both for live-session drift and post-eviction.
  try {
    const info = await netFetch(API+'/session', {credentials:'same-origin'}).then(r=>r.json());
    if (info && typeof info.metaGeneration === 'number') {
      if (initialMetaGen === null) initialMetaGen = info.metaGeneration;
      return info.metaGeneration !== initialMetaGen;
    }
  } catch (_) {}
  return false;
}
async function showSessionLost(){
  if (sessionLostShown) return;
  sessionLostShown = true;
  stopPolling(); stopLive();
  const configChanged = await checkMetaChanged();
  const overlay = document.createElement('div');
  overlay.style.cssText='position:fixed;inset:0;background:rgba(0,0,0,.4);'
    +'display:flex;align-items:center;justify-content:center;z-index:9999';
  const box = document.createElement('div');
  box.style.cssText='background:#fff;padding:20px 24px;border-radius:6px;'
    +'box-shadow:0 6px 30px rgba(0,0,0,.3);max-width:400px;text-align:center';
  const msg = document.createElement('p');
  msg.textContent = configChanged
    ? 'Конфигурация обновлена. Обновите страницу, чтобы продолжить работу с новой версией.'
    : 'Сессия потеряна. Сервер перезапущен или сессия истекла.';
  msg.style.cssText='margin:0 0 16px 0;font-size:14px;color:#333';
  const btn = document.createElement('button');
  btn.textContent='Обновить страницу';
  btn.className='ctrl';
  btn.onclick=()=>location.reload();
  box.appendChild(msg); box.appendChild(btn);
  overlay.appendChild(box);
  document.body.appendChild(overlay);
}
document.addEventListener('visibilitychange', ()=>{
  if (document.hidden) { stopLive(); stopPolling(); }
  else { startLive(); }
});

// On session start, OnStart may have opened forms (auto-populated tabs)
// before the walker ever ran — /active alone returns a bare visualHost
// with no children. POST /tab/<active> forces CreateAndUpdateVisualHost
// on the active tab's host and returns its tree, so the body paints on
// first load without waiting for a user click. No-op when the session
// has no tabs.
async function paintActiveTab(){
  const info=await netFetch(API+'/session',{credentials:'same-origin'}).then(r=>r.json());
  if(!info||!info.tabs||info.tabs.length===0) return;
  const active=info.activeTab|0;
  const tree=await netFetch(API+'/tab/'+active,{method:'POST',credentials:'same-origin'}).then(r=>r.json());
  if(tree&&!tree.error&&Object.keys(tree).length>0){
    main.innerHTML='';
    main.appendChild(render(tree));
  }
}

// Tab strip overflow controls. The strip is overflow-x:auto; when the
// combined width of all tabs exceeds the container, reveal ‹ › buttons
// that scrollBy half a strip-width per click. ResizeObserver catches
// both window resizes and tab-count changes.
(function(){
  const prev = document.getElementById('tabs-prev');
  const next = document.getElementById('tabs-next');
  if (!prev || !next || !tabsBar) return;
  const step = () => Math.max(80, tabsBar.clientWidth * 0.6);
  prev.addEventListener('click', () => tabsBar.scrollBy({ left: -step(), behavior: 'smooth' }));
  next.addEventListener('click', () => tabsBar.scrollBy({ left:  step(), behavior: 'smooth' }));
  const sync = () => {
    const overflowing = tabsBar.scrollWidth > tabsBar.clientWidth + 1;
    prev.classList.toggle('visible', overflowing);
    next.classList.toggle('visible', overflowing);
  };
  new ResizeObserver(sync).observe(tabsBar);
  new MutationObserver(sync).observe(tabsBar, { childList: true });
  tabsBar.addEventListener('scroll', sync, { passive: true });
  sync();
})();

(async()=>{
  const bootOverlay = document.getElementById('bootOverlay');
  // Step 1: find out whether we need to prompt for credentials.
  const info = await authInfo();
  // Step 2: authenticate.
  // Open-access (no sys_user rows): loop auto-login until it succeeds.
  // The server's timed_mutex may reject under rapid-F5 contention;
  // retrying rather than falling to the form is correct because an
  // empty-creds form can't collect anything useful.
  // Populated sys_user: show the credentials form; promptLogin loops
  // internally until /login accepts the creds.
  let authenticated = false;
  if (!info.hasUsers) {
    authenticated = await login('', '');
  }
  if (!authenticated) {
    await promptLogin();   // resolves only on successful /login
    authenticated = true;
  }
  status.textContent = 'connected';
  // Step 3: initial fetches in parallel so the boot overlay hides only
  // after sidebar, tab strip, and active host have all landed — user
  // sees a finished page rather than elements popping in one by one.
  await Promise.all([
    loadForms().catch(() => {}),
    refreshTabs().catch(() => {}),
    paintActiveTab().catch(() => {}),
  ]);
  startLive();
  if (bootOverlay) {
    bootOverlay.classList.add('hide');
    setTimeout(() => bootOverlay.remove(), 200);
  }
  // Debug indicator: a "DEBUG" pill in the sidebar status line while
  // the wes process is in --debug mode. When this tab's session parks
  // at a breakpoint, also takes over the top netStatus banner with
  // "Отладчик: пауза" so the generic "Загрузка…" doesn't mask the real
  // reason. Polls /debug-status; cheap (in-memory flag check on wes).
  const dbgPill = document.createElement('span');
  dbgPill.className = 'dbg';
  dbgPill.style.display = 'none';
  status.appendChild(dbgPill);
  let dbgState = { debugMode: false, paused: false };
  async function pollDebugStatus(){
    try {
      const r = await fetch(API+'/debug-status',{credentials:'same-origin'});
      if (!r.ok) return;
      const next = await r.json();
      if (next.debugMode === dbgState.debugMode && next.paused === dbgState.paused)
        return;
      dbgState = next;
      dbgPaused = next.paused;
      dbgMode   = next.debugMode;
      if (next.paused) {
        dbgPill.textContent = 'PAUSED';
        dbgPill.className = 'dbg paused';
        dbgPill.style.display = '';
        showNetStatus('debug-paused','Отладчик: пауза');
        hideSpinner();
      } else {
        if (next.debugMode) {
          dbgPill.textContent = 'DEBUG';
          dbgPill.className = 'dbg mode';
          dbgPill.style.display = '';
        } else {
          dbgPill.style.display = 'none';
        }
        // Resume — clear the paused banner. inflight keeps its own
        // loading state, so it'll re-flash if a request is still slow.
        if (inflight === 0) hideNetStatus();
      }
    } catch (e) { /* network blip — try again next tick */ }
  }
  setInterval(pollDebugStatus, 500);
  pollDebugStatus();
})();
</script></body></html>
)HTML";

extern "C" WFRONTEND_API const char* wfrontendClientHTML()
{
	return kClientHTML;
}
