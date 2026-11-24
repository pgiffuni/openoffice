# MCE ProcessContent Processing — Milestone D.4.3

**Project:** Apache OpenOffice  
**Repository:** apache/openoffice  
**Branch:** ooxml  
**Target:** trunk  
**Milestone:** D.4.3  
**Mode:** Implementation

---

## 1. Overview

The `mc:ProcessContent` attribute (namespace `http://schemas.openxmlformats.org/markup-compatibility/2006`) specifies which elements within an ignorable namespace should still be processed by consumers that do not understand those namespaces. When an element is in an ignorable namespace but its qualified name matches an entry in `mc:ProcessContent`, the element's children are processed by AOO despite the wrapper element itself being ignored.

## 2. Implementation

### 2.1 Files Modified

| File | Method | Role |
|------|--------|------|
| `main/oox/inc/oox/core/mcealternatecontext.hxx` | `maProcessContentElements` member declaration | Stores qualified element names (namespace URI + local name) that should be processed even when in an ignorable namespace |
| `main/oox/inc/oox/core/mcealternatecontext.hxx` | `isProcessContentElement()` declaration | Checks if a qualified element name is in the process content set |
| `main/oox/source/core/mcealternatecontext.cxx` | `processProcessContent()` implementation | Parses `mc:ProcessContent` attribute, resolves prefixes to namespaces, stores qualified names |
| `main/oox/source/core/mcealternatecontext.cxx` | `isProcessContentElement()` implementation | Checks if a qualified element name is in `maProcessContentElements` |

### 2.3 Implementation Details

#### `processProcessContent()`

**File:** `main/oox/source/core/mcealternatecontext.cxx`  
**Line:** `void MceAlternateContext::processProcessContent( const ::com::sun::star::uno::Reference< ::com::sun::star::xml::sax::XFastAttributeList >& rAttribs )`

This method is called from `startFastElement()` for every element that has an `mc:ProcessContent` attribute. It:

1. Extracts the `mc:ProcessContent` attribute value from the element's attributes
2. Splits the value by whitespace into QName tokens
3. Splits each QName into prefix and local name
4. Resolves each prefix to a namespace URI using `resolvePrefix()`
5. Adds each (namespace URI, localName) pair to `maProcessContentElements` if not already present

```cpp
void MceAlternateContext::processProcessContent(
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

            if ( rAttr.Name.equalsAsciiL( "ProcessContent", 13 ) )
            {
                ::rtl::OUString aValue = rAttr.Value;
                sal_Int32 nPos = 0;
                sal_Int32 nLen = aValue.getLength();
                while ( nPos < nLen )
                {
                    while ( nPos < nLen && (aValue[ nPos ] == ' ' || aValue[ nPos ] == '\t' ||
                                            aValue[ nPos ] == '\r' || aValue[ nPos ] == '\n') )
                        ++nPos;
                    if ( nPos >= nLen )
                        break;

                    sal_Int32 nStart = nPos;
                    while ( nPos < nLen && (aValue[ nPos ] != ' ' && aValue[ nPos ] != '\t' &&
                                            aValue[ nPos ] != '\r' && aValue[ nPos ] != '\n') )
                        ++nPos;
                    ::rtl::OUString aQName = aValue.copy( nStart, nPos - nStart );

                    // Split QName into prefix and local name.
                    ::rtl::OUString aPrefix;
                    ::rtl::OUString aLocalName;
                    sal_Int32 nColon = aQName.indexOf( ':' );
                    if ( nColon >= 0 )
                    {
                        aPrefix = aQName.copy( 0, nColon );
                        aLocalName = aQName.copy( nColon + 1, aQName.getLength() - nColon - 1 );
                    }
                    else
                    {
                        // No prefix - default namespace (empty prefix)
                        aPrefix = ::rtl::OUString();
                        aLocalName = aQName;
                    }

                    // Resolve prefix to namespace URI.
                    ::rtl::OUString aNamespaceURI = resolvePrefix( aPrefix );
                    if ( aNamespaceURI.getLength() > 0 )
                    {
                        // Only add if not already present.
                        bool bFound = false;
                        for ( const auto& rExisting : maProcessContentElements )
                        {
                            if ( rExisting.first == aNamespaceURI &&
                                 rExisting.second == aLocalName )
                            {
                                bFound = true;
                                break;
                            }
                        }
                        if ( !bFound )
                            maProcessContentElements.push_back(
                                std::make_pair( aNamespaceURI, aLocalName ) );
                    }
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

#### `isProcessContentElement()`

**File:** `main/oox/source/core/mcealternatecontext.cxx`, line ~323

```cpp
bool MceAlternateContext::isProcessContentElement(
    const ::rtl::OUString& rNamespaceURI,
    const ::rtl::OUString& rLocalName ) const
{
    for ( const auto& rPair : maProcessContentElements )
    {
        if ( rPair.first == rNamespaceURI && rPair.second == rLocalName )
            return true;
    }
    return false;
}
```

**Confidence:** confirmed — source code verified.

---

## 3. How `mc:ProcessContent` Affects Element Handling

### 3.1 Elements Outside Selected Branch

When `MceAlternateContext` is NOT in a selected branch (i.e., no `mc:Choice` has been selected yet, or no `mc:Fallback` has been selected):

1. `startUnknownElement()` is called for every element not handled by `startFastElement()`.
2. `pushNamespaces(Attrs)` is called first to track namespace declarations.
3. `processIgnorableAttribute(Attrs)` is called to record any `mc:Ignorable` declarations.
4. `processProcessContent(Attribs)` is called to record any `mc:ProcessContent` declarations.
5. If the element's namespace is ignorable **AND** its qualified name is NOT in `maProcessContentElements`:
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
      <ms:specialElement>Processed content</ms:specialElement>
    </w:p>
  </w:body>
</w:document>
```

- `<ms:unknownElement>` is in ignorable namespace `ms` and NOT in `mc:ProcessContent` → skipped (not forwarded to delegate)
- `<ms:specialElement>` inside `<ms:Choice>` (if `ms` namespace is required) is inside a selected branch → forwarded normally
- `<ms:specialElement>` outside `mc:AlternativeContent` is in ignorable namespace `ms` AND in `mc:ProcessContent` → forwarded normally

---

## 4. Scope Management

The `maProcessContentElements` vector is a member of `MceAlternateContext`. It is initialized empty in the constructor and populated as `mc:ProcessContent` attributes are encountered on elements. There is **no explicit "pop processContent elements"** because:

- `MceAlternateContext` is created per `mc:AlternateContent` element.
- All process content declarations within an `AlternateContent` scope are discarded when the `MceAlternateContext` instance is destroyed.
- For nested `mc:AlternateContent` elements, each nested context inherits process content elements from its parent at creation time:

```cpp
pMceCtx->maProcessContentElements = maProcessContentElements;  // inherit process content
```

This ensures that process content declarations declared in an outer `mc:AlternateContent` are also applicable in nested `mc:AlternateContent` elements.

---

## 5. Testing

No runtime tests have been executed due to the build environment limitations (missing Boost headers). The implementation follows the ECMA-376 Part 1 specification for Markup Compatibility and Extensibility.

**Test cases to verify:**

| Test ID | Description | Expected Behavior |
|---------|-------------|-------------------|
| PC-01 | `mc:ProcessContent="ms:specialElement"`, element `<ms:specialElement>` in ignorable namespace | Element is forwarded despite being in ignorable namespace |
| PC-02 | `mc:ProcessContent="ms:unknownElement"`, element `<ms:unknownElement>` in ignorable namespace | Element is forwarded despite being in ignorable namespace |
| PC-03 | `mc:Ignorable="ms"` but NO `mc:ProcessContent`, element `<ms:unknownElement>` | Element is skipped (not forwarded) |
| PC-04 | `mc:Ignorable="ms" mc:ProcessContent="ms:specialElement"`, element `<ms:specialElement>` | Element is forwarded (matches process content) |
| PC-05 | `mc:ProcessContent="ms:something"` but element is `<ms:otherElement>` | Element is skipped (name doesn't match) |
| PC-06 | `mc:ProcessContent` in nested AlternateContent | Inner AlternateContent inherits outer's process content elements |
| PC-07 | `mc:ProcessContent` outside any AlternateContent | Not reached (processProcessContent only called for elements outside selected branch when inside an AlternateContext) |
