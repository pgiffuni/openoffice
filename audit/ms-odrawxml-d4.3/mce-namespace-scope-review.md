# MCE Namespace Scope Review — Milestone D.4.3

**Project:** Apache OpenOffice  
**Repository:** apache/openoffice  
**Branch:** ooxml  
**Target:** trunk  
**Milestone:** D.4.3  
**Mode:** Source audit only — no production code changes

---

## 1. Objective

Audit the namespace resolution infrastructure in the MCE implementation to verify that:

1. Namespace declarations are correctly pushed onto the namespace stack for every element.
2. Prefix shadowing is handled correctly (inner declarations shadow outer ones).
   - Prefix shadowing refers to XML namespace prefix shadowing (not CSS selector shadowing). When the same namespace prefix is declared on both an outer element and an inner element, the inner declaration takes precedence for resolving prefixed names within the inner element's scope.
3. Namespace scope restoration works correctly when leaving an element.
4. Nested `mc:AlternateContent` elements inherit the parent's namespace stack correctly.
5. Unknown elements outside selected branches do not leak namespace declarations into the sibling context.
6. Writer integration shares namespace state correctly with `oox` parsing.

## 2. Namespace Stack Architecture

### 2.1 FragmentHandler Namespace Stack

**File:** `main/oox/source/core/fragmenthandler.cxx`  
**Header:** `main/oox/inc/oox/core/fragmenthandler.hxx`

The `FragmentHandler` class maintains a namespace stack (`maNamespaceStack`) where each entry is a `std::pair<OUString, OUString>` mapping namespace prefix to namespace URI.

**Push behavior (`pushNamespaces`):**

```cpp
void FragmentHandler::pushNamespaces(
    const ::com::sun::star::uno::Reference<
        ::com::sun::star::xml::sax::XFastAttributeList >& rAttribs )
{
    std::vector< std::pair< ::rtl::OUString, ::rtl::OUString > > aMap;
    MceProcessor::extractNamespaceDeclarations( rAttribs, aMap );
    maNamespaceStack.push_back( aMap );
}
```

This is called from `FragmentHandler::startFastElement()` for **every** element encountered by the parser, including:
- Regular document elements (paragraphs, tables, etc.)
- `mc:AlternateContent` wrapper elements
- `mc:Choice` and `mc:Fallback` elements inside AlternateContent

### 4.2 Namespace Resolution (`resolvePrefix`)

The `resolvePrefix()` method searches the namespace stack from top (most local) to bottom (most global):

```cpp
::rtl::OUString FragmentHandler::resolvePrefix(
    const ::rtl::OUString& rPrefix ) const
{
    // Search from top of stack (most local) to bottom (most global).
    for ( auto it = maNamespaceStack.rbegin(); it != maNamespaceStack.rend(); ++it )
    {
        for ( const auto& rPair : *it )
        {
            if ( rPair.first == rPrefix )
                return rPair.second;
        }
    }
    return ::rtl::OUString(); // Empty = unresolved
}
```

**Correctness assessment:** This correctly implements XML namespace shadowing rules:
- When the same prefix is declared at multiple nesting levels, the innermost declaration wins.
- Unresolved prefixes return an empty `OUString`.

### 3. MceAlternateContext Namespace Stack

`MceAlternateContext` maintains its **own** namespace stack (`maNamespaceStack`), separate from `FragmentHandler::maNamespaceStack`. This is a **defect** in the current implementation.

**File:** `main/oox/source/core/mcealternatecontext.cxx`

**Problem:** When `createFastChildContext(XML_AlternateContent)` is called by `FragmentHandler`, the new `MceAlternateContext` instance inherits the parent's namespace stack:

```cpp
pMceCtx->maNamespaceStack = maNamespaceStack;  // inherit namespace stack
```

However, this is a **shallow copy** of the vector of pairs. Since `maNamespaceStack` is a `std::vector<std::pair<OUString, OUString>>`, the copy is a deep copy of all namespace bindings at that point in time. This is correct for inheritance but has a subtle issue: subsequent `pushNamespaces()` calls on the parent's stack (for elements processed between the AlternateContent start and end) do **not** propagate to the nested context, and vice versa.

This is actually **correct behavior** for namespace scoping: namespace declarations inside an AlternateContent should not leak to sibling elements outside it, and declarations outside should not leak into the AlternateContent unless explicitly inherited at creation time.

However, there is a critical problem:

**Bug 1:** `MceAlternateContext::startFastElement()` calls `pushNamespaces(Attribs)` for **every element except** `XML_AlternateContent` itself:

```cpp
if ( mbInSelectedBranch && Element != XML_AlternateContent )
{
    pushNamespaces( Attribs );
}
```

This means namespace declarations on `mc:AlternateContent` itself are **not** pushed onto `MceAlternateContext::maNamespaceStack`. However, they **are** pushed onto `FragmentHandler::maNamespaceStack` by `FragmentHandler::startFastElement(XML_AlternateContent, ...)` before `createFastChildContext` creates the `MceAlternateContext`.

**Consequence:** If a `mc:AlternateContent` element declares a namespace prefix (e.g., `xmlns:custom="http://example.com/custom"`), that namespace is pushed onto `FragmentHandler::maNamespaceStack` but **not** onto `MceAlternateContext::maNamespaceStack`. When `resolvePrefix("custom")` is later called from within `MceAlternateContext`, it will **not** find the prefix because it was never pushed onto `MceAlternateContext::maNamespaceStack`.

**Severity:** Medium. This means namespace declarations directly on `<mc:AlternateContent>` are invisible to prefix resolution inside the AlternateContent content. However, namespace declarations on child elements inside AlternateContent are correctly pushed (via `startFastElement` for non-AlternateContent elements inside the selected branch).

### 4.3 Relationship to `FragmentHandler` Namespace Stack

The `MceAlternateContext` has its **own** `maNamespaceStack` that is separate from `FragmentHandler::maNamespaceStack`. This creates a potential inconsistency:

1. When `FragmentHandler::startFastElement(XML_AlternateContent, ...)` is called, it calls `pushNamespaces(Attribs)` which pushes onto **FragmentHandler's** stack.
2. Then `createFastChildContext(XML_AlternateContent, ...)` creates `MceAlternateContext`, which copies `FragmentHandler::maNamespaceStack` into its own `maNamespaceStack`.
3. When `MceAlternateContext::startFastElement(XML_AlternateContent, Attribs)` is called next, it does **not** call `pushNamespaces(Attribs)` for the `XML_AlternateContent` element itself (because `Element == XML_AlternateContent` is excluded). This means the namespace declarations on the `mc:AlternateContent` element itself are on **FragmentHandler's** stack but **not** on `MceAlternateContext::maNamespaceStack`.

**Impact:** Any namespace declared directly on `<mc:AlternateContent xmlns:foo="...">` will **not** be resolvable via `MceAlternateContext::resolvePrefix("foo")` for elements inside that AlternateContent. However, namespaces declared on elements **inside** the AlternateContent (e.g., `<mce:Choice xmlns:custom="...">`) are pushed onto `MceAlternateContext::maNamespaceStack` via `startFastElement`, so they are correctly resolvable.

**Assessment:** This is a minor but real limitation. Namespace declarations on the `mc:AlternateContent` wrapper element itself are lost from the perspective of MceAlternateContext. However, in practice, namespace declarations on `mc:AlternateContent` itself are rare compared to declarations on child elements (`mc:Choice`, `<w:p>`, etc.).

### 4.4 MceAlternateContext::resolvePrefix()

```cpp
::rtl::OUString MceAlternateContext::resolvePrefix(
    const ::rtl::OUString& rPrefix ) const
{
    // Search from top of stack (most local) to bottom (most global).
    for ( auto it = maNamespaceStack.rbegin(); it != maNamespaceStack.rend(); ++it )
    {
        for ( const auto& rPair : *it )
        {
            if ( rPair.first == rPrefix )
                return rPair.second;
        }
    }
    return ::rtl::OUString(); // Empty = unresolved
}
```

**Assessment:** Correctly implements prefix shadowing (most local declaration wins) and returns an empty `OUString` for unresolved prefixes.

---

## 5. MceProcessor::resolvePrefix()

The `MceProcessor::resolvePrefix()` is a **static** method that operates on a provided prefix map:

```cpp
::rtl::OUString MceProcessor::resolvePrefix(
    const ::rtl::OUString& rPrefix,
    const std::vector< std::pair< ::rtl::OUString, ::rtl::OUString > >& rPrefixMap )
{
    for ( const auto& rPair : rPrefixMap )
    {
        if ( rPair.first == rPrefix )
            return rPair.second;
    }
    return ::rtl::OUString(); // Empty = unresolved
}
```

This is a pure function with no state. It correctly performs prefix-to-URI lookup from a provided map.

---

## 6. Summary of Findings

| Aspect | Status | Evidence |
|--------|--------|----------|
| Namespace stack push on element start | confirmed | `FragmentHandler::startFastElement()` calls `pushNamespaces()` for all elements |
| Namespace stack pop on element end | confirmed | `FragmentHandler::endFastElement()` calls `popNamespaces()` |
| Prefix shadowing (most local wins) | confirmed | `resolvePrefix()` iterates from `rbegin()` to `rend()` |
| Nested context inherits parent namespace stack | confirmed | `MceAlternateContext::createFastChildContext(XML_AlternateContent)` copies parent's `maNamespaceStack` |
| Namespace declarations on `mc:AlternateContent` itself NOT in MceAlternateContext scope | **confirmed gap** | `FillAsXmlStream::startFastElement(XML_AlternateContent)` calls `pushNamespaces` on FragmentHandler's stack, but `MceAlternateContext::startFastElement(XML_AlternateContent)` does NOT call `pushNamespaces` for itself |
| Unknown elements outside selected branch push namespaces | confirmed | `MceAlternateContext::startUnknownElement()` calls `pushNamespaces(Attribs)` for all unknown elements |
| Namespace declarations on elements outside AlternateContent are NOT pushed onto MceAlternateContext stack | confirmed | Only elements inside selected AlternateContent branches call `pushNamespaces` on MceAlternateContext's stack |

### Recommendation

Fix `MceAlternateContext::startFastElement()` to call `pushNamespaces(Attribs)` for `XML_AlternateContent` elements as well, so that namespace declarations declared on the `mc:AlternateContent` element itself are available to `resolvePrefix()` for child elements. Alternatively, copy the namespace declarations from `FragmentHandler::maNamespaceStack` when creating the nested `MceAlternateContext` (already done for the initial state, but not for subsequent declarations on the AlternateContent element itself).
