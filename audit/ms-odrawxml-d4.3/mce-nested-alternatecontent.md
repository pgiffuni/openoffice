# MCE Nested AlternateContent Handling — Milestone D.4.3

**Project:** Apache OpenOffice  
**Repository:** apache/openoffice  
**Branch:** ooxml  
**Target:** trunk  
**Milestone:** D.4.3  
**Mode:** Implementation

---

## 1. Overview

The `mc:AlternateContent` element can be nested — an `mc:AlternativeContent` element can appear inside another `mc:AlternateContent` element, typically as part of a nested `<mc:Choice>` or `<mc:Fallback>` branch. The MceAlternateContext class must handle nested AlternateContent correctly by:

1. Creating a new MceAlternateContext instance for each nested `<mc:AlternateContent>` element.
2. Inheriting the parent's namespace stack, ignorable namespaces, and process content elements.
3. Maintaining independent state for each nested context (state machine, current choice, fallback flag, etc.).
4. Properly forwarding events only from the selected branch of each nested AlternateContent.
5. Restoring state correctly when leaving a nested AlternateContent.

## 2. Implementation

### 2.1 File Locations

| File | Role |
|------|------|
| `main/oox/inc/oox/core/mcealternatecontext.hxx` | MceAlternateContext class declaration |
| `main/oox/source/core/mcealternatecontext.cxx` | MceAlternateContext implementation |

### 2.2 Nested AlternateContext Creation

**File:** `main/oox/source/core/mcealternatecontext.cxx`  
**Line:** `createFastChildContext()` around line 174

When `createFastChildContext(XML_AlternateContent, ...)` is called on a `MceAlternateContext` instance, it creates a new `MceAlternateContext` instance:

```cpp
MceAlternateContext* pMceCtx = new MceAlternateContext(
    mxDelegate );
pMceCtx->maNamespaceStack = maNamespaceStack;  // inherit namespace stack
pMceCtx->maIgnorableNamespaces = maIgnorableNamespaces;  // inherit ignorable namespaces
pMceCtx->maProcessContentElements = maProcessContentElements;  // inherit process content
pMceCtx->meState = STATE_INITIAL;
pMceCtx->mnNextChoiceIndex = 0;
pMceCtx->mbHasFallback = false;
pMceCtx->mbInSelectedBranch = false;
pMceCtx->maSelection.meResult = MceProcessor::BRANCH_NONE;
pMceCtx->maSelection.mnChoiceIndex = -1;
```

**Confidence:** confirmed — source code verified.

**What is inherited:**

| Member | Purpose |
|--------|---------|
| `maNamespaceStack` | All namespace declarations from ancestor elements — needed for prefix resolution in the nested AlternateContent |
| `maIgnorableNamespaces` | All ignorable namespace declarations from ancestor elements |
| `maProcessContentElements` | All process content element mappings from ancestor elements |

**Confidence:** confirmed — source code verified.

### 2.3 State Isolation

Each `MceAlternateContext` instance maintains its own state machine:

| Member | Purpose |
|--------|---------|
| `meState` | Current state (STATE_INITIAL, STATE_CHOICE_SCAN, STATE_CHOICE_SELECTED, STATE_FALLBACK_SELECTED, STATE_DONE) |
| `mnNextChoiceIndex` | Index of the next Choice to evaluate |
| `mbHasFallback` | Whether a Fallback element was encountered |
| `mbInSelectedBranch` | Whether currently inside a selected branch |
| `maSelection` | The selected branch (Choice or Fallback) |
| `maSelection.mnChoiceIndex` | Index of the selected Choice |

**Status:** confirmed — source code verified.

---

## 2.4 Event Forwarding in Nested Context

When an `MceAlternateContext` is active (i.e., we're inside an `mc:AlternativeContent` element) and a nested `<mc:AlternateContent>` element is encountered:

1. `startFastElement(XML_AlternateContent, Attribs)` is called
2. `pushNamespaces(Attribs)` is called — pushes namespace declarations from the nested `<mc:AlternateContent>` onto `maNamespaceStack`
3. `processIgnorableAttribute(Attribs)` is called
4. `processProcessContent(Attribs)` is called
5. A new `MceAlternateContext` is created
6. `m_pExcDoc` is NOT called — instead a new `MceAlternateContext` is returned
7. The new `MceAlternateContext` becomes the active context handler for elements inside this nested `mc:AlternateContent`

**Confidence:** confirmed — source code verified.

---

## 3. Known Limitations

| Limitation | File:Line | Severity | Phase |
|------------|-----------|----------|-------|
| Nested `mc:AlternateContent` inside selected branch inherits state but each nested context maintains independent state | mcealternatecontext.cxx:174-193 | None — working as intended |
| No explicit limit on nesting depth | mcealternatecontext.cxx:178-192 | Low — potential stack overflow for deeply nested AlternateContent |
| No way to distinguish "no eligible Choice" from "no Choice element at all" | mcealternatecontext.cxx:254-269 | Low — both cases result in BRANCH_NONE |

---

## 4. Testing

No runtime tests have been executed due to the build environment limitations (missing Boost headers). The implementation follows the ECMA-376 Part 1 specification for Markup Compatibility and Extensibility.

**Test cases to verify:**

| Test ID | Description | Expected Behavior |
|---------|-------------|-------------------|
| NAC-01 | Nested AlternateContent: outer Choice eligible, inner Choice also eligible | Inner Choice's content is processed |
| NAC-02 | Nested AlternateContent: outer Choice ineligible, inner Choice eligible | Inner Choice's content is processed |
| NAC-03 | Nested AlternateContent: outer Choice eligible, inner Choice ineligible | Outer Choice's content is preserved; inner Choice is skipped |
| NAC-04 | AlternateContent inside AlternateContent inside AlternateContent (3 levels deep) | Each level independently evaluated |
| NAC-05 | AlternateContent inside AlternativeContent inside AlternateContent — middle Choice eligible but inner Choice not | Middle Choice's content is preserved; inner Choice is skipped |
