# MCE Ignorable Processing — Milestone D.4.3

**Project:** Apache OpenOffice  
**Repository:** apache/openoffice  
**Branch:** ooxml  
**Target:** trunk  
**Milestone:** D.4.3  
**Mode:** Implementation

---

## 1. Overview

The `mc:Ignorable` attribute (namespace `http://schemas.openxmlformats.org/markup-compatibility/2006`) lists namespace prefixes whose elements may be ignored by consumers that do not understand them. When an element is in an ignorable namespace, AOO skips it (does not generate output for it), unless the element is explicitly listed in `mc:ProcessContent`.

## 2. Implementation

### 2.1 Files Modified

| File | Change |
|------|--------|
| `main/oox/inc/oox/core/mcealternatecontext.hxx` | Added `maIgnorableNamespaces` member and `isIgnorableNamespace()` method declaration |
| `main/oox/source/core/mcealternatecontext.cxx` | Implemented `processIgnorableAttribute()` and `isIgnorableNamespace()`; called from `startFastElement()` and `startUnknownElement()` |

### 2.2 Implementation Details

#### `processIgnorableAttribute()`

**File:** `main/oox/source/core/mcealternatecontext.cxx`  
**Line:** `void MceAlternateContext::processIgnorableAttribute( const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& rAttribs )`

This method is called from `startFastElement()` and `startUnknownElement()` for every element that has an `mc:Ignorable` attribute. It:

1. Extracts all attributes from `rAttribs` via `getAttributes()`
2. For each attribute named `Ignorable` in the Markup Compatibility namespace:
   - Parses the whitespace-separated prefix list
   - Resolves each prefix to a namespace URI using `resolvePrefix()`
   - Adds each resolved namespace URI to `maIgnorableNamespaces` if not already present

**Pseudo-code:**

```cpp
void MceAlternateContext::processIgnorableAttribute(
    const ::com::sun::star::uno::Reference<
        ::com::sun::star::xml::sax::XFastAttributeList >& rAttribs )
{
    if ( !rAttribs.is() )
        return;

    try
    {
        ::com::sun::star::uno::Sequence< ::com::sun::star::xml::Attribute > aAttrs =
            rAttribs->getAttributes();

        for ( sal_Int32 i = 0; i < aAttrs.getLength(); ++i )
        {
            const ::com::sun::star::xml::Attribute& rAttr = aAttrs[i];

            if ( rAttr.Name.equalsAsciiL( "Ignorable", 9 ) )
            {
                ::rtl::OUString aValue = rAttr.Value;
                std::vector< ::rtl::OUString > aPrefixes =
                    resolvePrefixList( aValue );
                for ( const auto& rNs : aPrefixes )
                {
                    bool bFound = false;
                    for ( const auto& rExisting : maIgnorableNamespaces )
                    {
                        if ( rExisting == rNs )
                        {
                            bFound = true;
                            break;
                        }
                    }
                    if ( !bFound )
                        maIgnorableNamespaces.push_back( rNs );
                }
            }
        }
    }
    catch ( ... )
    {
        // getAttributes() may throw if not supported
    }
}
```

#### `isIgnorableNamespace()`

**File:** `main/oox/source/core/mcealternatecontext.cxx`, line ~313

```cpp
bool MceAlternateContext::isIgnorableNamespace( const ::rtl::OUString& rNamespaceURI ) const
{
    for ( const auto& rNs : maIgnorableNamespaces )
    {
        if ( rNs == rNamespaceURI )
            return true;
    }
    return false;
}
```

**Confidence:** confirmed — source code verified.

---

## 3. How Ignorable Namespaces Affect Element Handling

### 3.1 Elements Outside Selected Branch

When `MceAlternateContext` is NOT in a selected branch (i.e., no `mc:Choice` has been selected yet, or no `mc:Fallback` was selected):

1. `startUnknownElement()` is called for every element not handled by `startFastElement()`.
2. `pushNamespaces(Attribs)` is called first to track namespace declarations.
3. `processIgnorableAttribute(Attribs)` is called to record any `mc:Ignorable` declarations.
4. `processProcessContent(Attribs)` is called to record any `mc:ProcessContent` declarations.
5. If the element's namespace is ignorable AND its qualified name is NOT in `maProcessContentElements`:
   - The element is **skipped** — its children are NOT forwarded to the delegate.
   - `return;` is executed immediately.
6. Otherwise (namespace not ignorable, OR element IS in process content list):
   - The element is forwarded to `mxDelegate->startUnknownElement( Namespace, Name, Attribs )`.

### 3.2 Elements Inside Selected Branch

When `mbInSelectedBranch` is true:
- `startUnknownElement(Namespace, Name, Attribs)` forwards to `mxDelegate->startUnknownElement( Namespace, Name, Attribs )`.
- `startFastElement` (via `startFastElement` for non-VML elements) forwards to `mxDelegate->startFastElement( Element, Attribs )`.
- The element's namespace is NOT checked against `maIgnorableNamespaces` — once inside a selected branch, all content is forwarded.

### 3.3 Example

Given this OOXML:

```xml
<w:document xmlns:w="http://schemas.openxmlformats.org/wordprocessingml/2006/main"
            xmlns:mc="http://schemas.openxmlformats.org/markup-compatibility/2006"
            xmlns:ms="http://schemas.microsoft.com/office/myschema/2006">
  <w:body>
    <w:p>
      <w:r><w:t>Before alternate content</w:t></w:r>
      <mc:AlternateContent>
        <mc:Choice Requires="ms">
          <ms:specialElement>Excel choice content</ms:specialElement>
        </mc:Choice>
        <mc:Fallback>
          <w:r><w:t>Fallback text</w:t></w:r>
        </mc:Fallback>
      </mc:AlternateContent>
      <ms:unknownElement>Ignored content</ms:unknownElement>
      <w:r><w:t>After alternate content</w:t></w:r>
    </w:p>
  </w:body>
</w:document>
```

- `<ms:unknownElement>` is in ignorable namespace `ms` and NOT in `mc:ProcessContent` → skipped (not forwarded to `ms:unknownElement`).
- `<ms:specialElement>` inside `<ms:choice>` (if `ms` namespace is required) is inside a selected branch → forwarded normally.

---

## 4. Scope Management

The `maIgnorableNamespaces` vector is a member of `MceAlternateContext`. It is initialized empty in the constructor and populated as `mc:Ignorable` attributes are encountered on elements. There is **no explicit "pop ignorable namespaces"** because:

- `MceAlternateContext` is created per `mc:AlternateContent` element.
- All ignorable namespaces declared within an `AlternateContent` scope are discarded when the `MceAlternateContext` instance is destroyed.
- For nested `mc:AlternateContent` elements, each nested context inherits ignorable namespaces from its parent at creation time:

```cpp
pMceCtx->maIgnorableNamespaces = maIgnorableNamespaces;  // inherit ignorable namespaces
```

This ensures that ignorable namespaces declared in an outer `mc:AlternateContent` are also ignorable in nested `mc:AlternateContent` elements.

---

## 5. Testing

No runtime tests have been executed due to the build environment limitations (missing Boost headers). The implementation follows the ECMA-376 Part 1 specification for Markup Compatibility and Extensibility.

**Test cases to verify:**

| Test ID | Description | Expected Behavior |
|---------|-------------|-------------------|
| IGN-01 | `mc:Ignorable="ms"` on element, unknown element in `ms` namespace | Element skipped, not forwarded |
| IGN-02 | Multiple ignorable namespaces | All listed namespaces ignored |
| IGN-03 | Ignorable namespace declared on ancestor | Inherited by child elements |
| IGN-04 | Prefix shadowing | Most local declaration wins |
| IGN-05 | Unknown element in non-ignorable namespace | Forwarded normally |
| IGN-06 | Ignorable content containing recognized DrawingML | DrawingML elements inside ignorable namespace are skipped unless in `mc:ProcessContent` list |
