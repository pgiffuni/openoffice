# MCE Phase 1 Source Review — Milestone D.4.3

**Project:** Apache OpenOffice  
**Repository:** apache/openoffice  
**Branch:** ooxml  
**Target:** trunk  
**Milestone:** D.4.3  
**Mode:** Source review — no build or runtime testing performed

---

## 1. Scope of Review

This review covers all MCE implementation files:

| File | Type | Status | Purpose |
|------|------|--------|---------|
| `main/oox/inc/oox/core/mceprocessor.hxx` | Header | Complete | MceProcessor class declaration |
| `main/oox/source/core/mceprocessor.cxx` | Implementation | Complete | MceProcessor: namespace resolution, eligibility, selection |
| `main/oox/inc/oox/core/mcealternatecontext.hxx` | Header | Complete | MceAlternateContext class declaration |
| `main/oox/source/core/mcealternatecontext.cxx` | Implementation | Complete | MceAlternateContext: branch selection, event forwarding, namespace stack, ignorable namespaces, process content |
| `main/oox/source/core/fragmenthandler.cxx` | Modified | Complete | `startFastElement()` calls `pushNamespaces()` for ALL elements |
| `main/oox/inc/oox/core/fragmenthandler.hxx` | Modified | Complete | `getMarkupCompatNamespace()`, `maNamespaceStack`, `pushNamespaces`, `popNamespaces`, `resolvePrefix`, friend declaration |
| `main/writerfilter/source/ooxml/OOXMLFastDocumentHandler.cxx` | Modified | Complete | `createFastChildContext` intercepts `XML_AlternateContent`, calls `pushNamespaces(rAttribs)` |

---

## 2. MceProcessor — Serialization Logic Review

### 2.1 Method Inventory

| Method | File:Line | Status | Assessment |
|--------|-----------|--------|------------|
| `MceProcessor()` | mceprocessor.cxx:16 | Implemented | Default constructor, no-op |
| `~MceProcessor()` | mceprocessor.cxx:20 | Implemented | Destructor, no-op |
| `getMarkupCompatNamespace()` | mceprocessor.cxx:24 | Implemented | Returns `http://schemas.openxmlformats.org/markup-compatibility/2006` |
| `getSupportedNamespaceSet()` | mceprocessor.cxx:30 | Implemented | Returns `http://schemas.openxmlformats.org/drawingml/2006/main` |
| `isNamespaceSupported()` | mceprocessor.cxx:47 | Implemented | Linear search in supported namespaces |
| `extractNamespaceDeclarations()` | mceprocessor.cxx:53 | Implemented | Extracts `xmlns:*` declarations and `mc:Requires` from attributes |
| `isChoiceEligible()` | mceprocessor.cxx:93 | Implemented | Resolves prefixes, checks all are supported |
| `selectBranch()` | mceprocessor.cxx:133 | Implemented | First eligible Choice, else Fallback, else NONE |
| `resolvePrefix()` | mceprocessor.cxx:162 | Implemented | Linear search in prefix map |

### 2.2 Namespace Resolution

The `extractNamespaceDeclarations()` method correctly extracts `xmlns:*` declarations from `XFastAttributeList::getUnknownAttributes()`. The `resolvePrefix()` method searches the prefix map for a matching prefix.

**Assessment:** Correctly implemented. Matches the pattern used in other AOO code.

### 2.3 Choice Eligibility

`isChoiceEligible()` correctly:
1. Returns `false` for empty `Requires` (conservative)
2. Splits by whitespace
3. Resolves each prefix to a namespace URI
4. Requires ALL prefixes to resolve to supported namespaces

**Assessment:** Correctly implemented.

### 2.4 Branch Selection

`selectBranch()` correctly:
- Evaluates Choice branches in document order
- Returns first eligible Choice
- Falls back to `Fallback` if no Choice eligible
- Returns `BRANCH_NONE` if neither Choice nor Fallback available

**Assessment:** Correctly implemented.

---

## 3. MceAlternateContext — Context Handler Review

### 3.1 Method Inventory

| Method | File:Line | Status | Assessment |
|--------|-----------|--------|------------|
| `MceAlternateContext()` | mcealternatecontext.cxx:12 | Implemented | Constructor |
| `~MceAlternateContext()` | mcealternatecontext.cxx:25 | Implemented | Destructor |
| `startFastElement()` | mcealternatecontext.cxx:31 | Implemented | Handles `XML_Choice`, `XML_Fallback`, `XML_AlternateContent` |
| `startUnknownElement()` | mcealternatecontext.cxx:104 | Implemented | Forwards to delegate if in selected branch |
| `endFastElement()` | mcealternatecontext.cxx:139 | Implemented | Handles `XML_AlternateContent`, `XML_Choice`, `XML_Fallback` |
| `endUnknownElement()` | mcealternatecontext.cxx:157 | Implemented | Forwards to delegate if in selected branch |
| `createFastChildContext()` | mcealternatecontext.cxx:168 | Implemented | Handles nested `MCAlternateContent` |
| `createUnknownChildContext()` | mcealternatecontext.cxx:203 | Implemented | Forwards to delegate if in selected branch |
| `characters()` | mcealternatecontext.cxx:215 | Implemented | Forwards to delegate if in selected branch |

### 3.2 State Machine

| State | Transitions | Assessment |
|-------|-------------|------------|
| `STATE_INITIAL` | → `STATE_CHOICE_SCAN` on first Choice | Correct |
| `STATE_CHOICE_SCAN` | → `STATE_CHOICE_SELECTED` on eligible Choice | Correct |
| `STATE_CHOICE_SCAN` | → `STATE_FALLBACK_SELECTED` on Fallback with no eligible Choice | Correct |
| `STATE_CHOICE_SELECTED` | → `STATE_DONE` on `XML_AlternateContent` end | Correct |
| `STATE_FALLBACK_SELECTED` | → `STATE_DONE` on `AlternateContent` end | Correct |
| `STATE_DONE` | Terminal | Correct |

### 3.3 Namespace Stack

The `maNamespaceStack` is a stack of prefix-to-URI maps. Each element in the stack represents the namespace declarations in scope at a particular XML element level.

**Status:** Correctly implemented. `pushNamespaces()` extracts namespace declarations from element attributes and pushes them onto the stack. `popNamespaces()` removes the top entry. `resolvePrefix()` searches from top (most local) to bottom (most global), correctly implementing XML namespace shadowing rules.

### 3.4 Ignorable Namespaces

The `maIgnorableNamespaces` vector stores namespace URIs declared as ignorable via `mc:Ignorable`. `isIgnorableNamespace()` checks if a namespace URI is in this list.

**Status:** Correctly implemented. Populated by `processIgnorableAttribute()`.

### 3.5 Process Content Elements

The `maProcessContentElements` vector stores qualified element names (namespace URI + local name) that should be processed even in ignorable namespaces. `isProcessContentElement()` checks if a qualified name is in this list.

**Status:** Correctly implemented. Populated by `processProcessContent()`.

---

## 4. FragmentHandler Integration

### 4.1 `fragmenthandler.hxx`

| Change | File:Line | Assessment |
|--------|-----------|------------|
| `getMarkupCompatNamespace()` | fragmenthandler.hxx:43-49 | Added — returns the markup compatibility namespace URI |
| `maNamespaceStack` | fragmenthandler.hxx:100 | Added — namespace stack for all elements |
| `resolvePrefix()` | fragmenthandler.hxx:104-107 | Added — resolves prefix using namespace stack |
| `pushNamespaces()` | fragmenthandler.hxx:109-111 | Added — pushes namespace declarations onto stack |
| `popNamespaces()` | fragmenthandler.hxx:114-115 | Added — pops namespace declaration set |

**Assessment:** Correctly integrates `MceAlternateContext` with `FragmentHandler`.

### 4.2 `fragmenthandler.cxx`

| Change | File:Line | Assessment |
|--------|-----------|------------|
| `#include "oox/core/mcealternatecontext.hxx"` | fragmenthandler.cxx:27 | Added — includes MceAlternateContext header |
| `getMarkupCompatNamespace()` | fragmenthandler.cxx:43-49 | Implemented — returns the markup compatibility namespace URI |
| `startFastElement()` | fragmenthandler.cxx:123-125 | Implemented — calls `pushNamespaces(rAttribs)` for ALL elements |
| `createFastChildContext()` | fragmenthandler.cxx:141-155 | Modified — intercepts `XML_AlternateContent`, creates `MceAlternateContext` |

**Assessment:** Correctly integrates `MceAlternateContext` with `FragmentHandler`. The `pushNamespaces( rAttribs )` call in `startFastElement` ensures namespace declarations from ALL elements are pushed onto `FragmentHandler::maNamespaceStack`.

---

## 5. WriterFilter Integration

### 5.1 `OOXMLFastDocumentHandler.cxx`

| Change | File:Line | Assessment |
|--------|-----------|------------|
| `#include <oox/core/mcealternatecontext.hxx>` | OOXMLFastDocumentHandler.cxx:36 | Added — includes MceAlternateContext header |
| `#include <oox/token/tokens.hxx>` | OOXMLFastDocumentHandler.cxx:37 | Added — includes token definitions for `XML_AlternateContent` |
| `createFastChildContext()` | OOXMLFastDocumentHandler.cxx:103-133 | Modified — intercepts `XML_AlternateContent`, creates `MceAlternateContext`, calls `pushNamespaces( rAttribs )` |

**Assessment:** Correctly integrates `MceAlternateContext` with Writer filter. The `pushNamespaces( rAttribs )` call ensures namespace declarations from `mc:AlternateContent` are pushed onto `MceAlternateContext::maNamespaceStack`.

---

## 6. Build File Changes

| File | Change | Assessment |
|------|--------|------------|
| `main/oox/Library_oox.mk` | Lines 76-77: Added `oox/source/core/mcealternatecontext` and `oox/source/core/mceprocessor` | Correct — adds MCE source files to build |
| `main/writerfilter/Library_writerfilter.mk` | Added `oox/inc` include path | Correct — adds OOXML include path for writerfilter |

---

## 7. Known Limitations

| Limitation | File:Line | Severity | Phase |
|------------|-----------|----------|-------|
| `resolvePrefix()` delegates to `mxDelegate->resolvePrefix(rPrefix)` | mcealternatecontext.cxx:298-311 | Low | Works correctly because `FragmentHandler::resolvePrefix` searches `maNamespaceStack` |
| Namespace declarations on elements inside `mc:AlternateContent` but outside `mc:Choice`/`mc:Fallback` are pushed onto `MceAlternateContext::maNamespaceStack` | mcealternatecontext.cxx:31 | Low | Works correctly — all elements inside `mc:AlternateContent` call `pushNamespaces()` via `startFastElement` |
| Namespace declarations on `mc:AlternateContent` itself are pushed onto `FragmentHandler::maNamespaceStack`, NOT `MceAlternateContext::maNamespaceStack` | fragmenthandler.cxx:123-125 | Low | `FillAsXmlStream::startFastElement(XML_AlternateContent, ...)` pushes onto FragmentHandler's stack, not MceAlternateContext's stack. But `MceAlternateContext::startFastElement(XML_AlternateContent)` does NOT call `pushNamespaces(Attribs)` because `Element == XML_AlternateContent`. This means namespace declarations on `mc:AlternateContent` itself are NOT in `MceAlternateContext::maNamespaceStack`. |

### 7.1 Critical Gap: Namespace Declarations on `mc:AlternateContent` Itself

**Severity:** Medium  
**File:** `main/oox/source/core/mcealternatecontext.cxx:45-48`  
**Description:** When `FillAsXmlStream::startFastElement(XML_AlternateContent, Attribs)` is called, `pushNamespaces(Attribs)` pushes namespaces onto `FragmentHandler::maNamespaceStack`. But `MceAlternateContext::startFastElement(XML_AlternateContent, ...)` does NOT call `pushNamespaces(Attribs)` because `Element == XML_AlternateContent`. This means namespace declarations on `mc:AlternateContent` itself are NOT in `MceAlternateContext::maNamespaceStack`.

**Impact:** If an `mc:AlternateContent` element has `xmlns:custom="http://example.com/custom"` attribute, and a `mc:Choice` inside it uses `Requires="custom"`, `resolvePrefix("custom")` will NOT find it because the namespace was pushed onto `FragmentHandler::maNamespaceStack`, not `MceAlternateContext::maNamespaceStack`.

**Recommendation:** Either:
1. Call `pushNamespaces(Attribs)` in `MceAlternateContext::startFastElement(XML_AlternateContent, ...)` as well (but this would push onto `MceAlternateContext::maNamespaceStack` twice if `FragmentHandler` already pushed them — need to avoid double-pushing), OR
2. Have `MceAlternateContext::resolvePrefix()` fall back to checking `XmlFilterBase::maNamespaceStack` (the FragmentHandler's stack) if not found in `MceAlternateContext::maNamespaceStack`.

**Recommended approach:** Option 2 — have `resolvePrefix()` first check `MceAlternateContext::maNamespaceStack`, then fall back to `mxDelegate->resolvePrefix(rPrefix)` if not found. This is more robust and avoids double-pushing namespaces.

---

## 7. Conclusion

The MceProcessor class is correctly implemented with proper namespace resolution, feature detection, and branch selection logic. The MceAlternateContext class correctly implements context handling, namespace stack management, and event forwarding. However, there is a gap in namespace resolution for namespaces declared on `mc:AlternateContent` itself — they are pushed onto `FragmentHandler::maNamespaceStack` but not `MceAlternateContext::maNamespaceStack`, so they won't be found by `MceAlternateContext::resolvePrefix()`.
