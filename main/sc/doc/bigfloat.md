# High-precision numeric values (BigFloat) in Calc — architecture and source audit

Status: **Phase 0 (source audit) complete. Phases 1–3 implemented, not built
end-to-end** — see §9.1 for exactly what was and was not verified.

This document is the working design note for adding an arbitrary-precision decimal
numeric type to Calc on top of the already bundled Boost.Multiprecision
(`boost::multiprecision::cpp_dec_float`). It answers the audit questions in a form
that can be checked against the source, records the decisions taken, and lists the
places that are *not* touched yet.

## 0. The change set of Phases 1–3

New files:

```
main/sc/inc/bigfloat.hxx                  the type and its conversions (only Boost include)
main/sc/inc/bigfloattoken.hxx             the formula::svBigFloat token
main/sc/source/core/tool/bigfloat.cxx     exact decimal parsing and text conversion
main/sc/source/core/tool/bigfloattoken.cxx
main/sc/test/bigfloattests.cxx            gtest
main/sc/doc/bigfloat.md                   this note
```

Modified:

```
main/formula/inc/formula/token.hxx                    + formula::svBigFloat
main/formula/inc/formula/compiler.hrc                 + SC_OPCODE_BIGFLOAT, STOP_1_PAR 162->163
main/formula/inc/formula/opcode.hxx                   + ocBigFloat
main/formula/source/core/resource/core_resource.src   + 3 name entries
main/sc/inc/formularesult.hxx                         IsValue(), GetDouble()
main/sc/inc/cell.hxx, main/sc/source/core/data/cell2.cxx   ScFormulaCell::GetResultToken()
main/sc/source/core/data/cell.cxx                     change detection, CalcAsShown, non-finite
main/sc/source/core/tool/interpr1.cxx                 ISVALUE() recognises the new type
main/sc/source/core/inc/interpre.hxx                  token level stack API, ScBigFloat()
main/sc/source/core/tool/interpr4.cxx                 Push/Pop/GetBigFloatToken, dispatch, result switch,
                                                       PushCellResultToken(), GetCellString()
main/sc/source/core/tool/interpr2.cxx                 ScInterpreter::ScBigFloat()
main/sc/source/core/tool/cellform.cxx                 display without going through double
main/sc/source/core/tool/parclass.cxx                 parameter classification
main/sc/source/filter/excel/xlformula.cxx,
main/oox/source/xls/formulabase.cxx                   name round trip for XLSX/BIFF
main/sc/inc/helpids.h, main/sc/util/hidother.src,
main/sc/source/ui/src/scfuncs.src,
main/helpcontent2/source/text/scalc/01/04060110.xhp   help and Function Wizard
main/sc/Library_sc.mk, main/sc/GoogleTest_sc.mk        build
```


## 1. Guiding constraints (non-negotiable)

1. The existing `double` path for ordinary cells is preserved bit for bit. No cell
   becomes a BigFloat unless a formula explicitly produces one.
2. A BigFloat is never routed through `double` during a BigFloat operation.
3. BigFloat is *not* stored in the `ScFormulaResult` union and not in every cell:
   it travels as a `formula::FormulaToken`, exactly like the existing string and
   matrix results.
4. Boost.Multiprecision is included from exactly one Calc header. No Boost type
   leaks into a widely included public header.
5. No second Boost copy, no GMP, no MPFR, no new external dependency.
6. Internal precision and displayed precision are separate concerns.
7. BigFloat is a Number (OpenFormula allows arbitrary-precision Number subtypes),
   never Text.

## 2. Environment facts established during the audit

| Fact | Evidence |
| --- | --- |
| Boost 1.84.0 is the bundled version, `boost/multiprecision` is already copied | `main/boost/makefile.mk:105` (`GNUCOPY … boost$/multiprecision`), `main/external_deps.lst:196-201` |
| Boost is header-only for this purpose; there is no `-lboost` anywhere | `grep -rn "lboost" main` → 0 hits; `main/boost/prj/build.lst` has no library target; `main/RepositoryExternal.mk` has no boost entry |
| `main/inc/` (where the headers land) is on every compile line | `main/set_soenv.in:1477-1480`, `main/solenv/gbuild/platform/linux.mk:266` |
| sc already depends on Boost headers | `main/sc/inc/pch/precompiled_sc.hxx:46` includes `<boost/bind.hpp>` |
| `--with-system-boost` is supported | `main/configure.ac:5408-5442` |
| Compiler is gcc/g++ with **`-std=gnu++11`** and **`-Werror`** | `main/solenv/gbuild/platform/linux.mk:92` and `:98-99` |
| Only one `-DBOOST_…` macro exists in the tree, and it is unrelated | `main/slideshow/StaticLibrary_sldshw_s.mk:33` |

### 2.1 The one blocking build issue, and its fix

Boost 1.84's `boost/multiprecision/cpp_dec_float.hpp` transitively includes
`boost/math/tools/config.hpp` and `boost/multiprecision/detail/standalone_config.hpp`.
Both contain an unconditional

```cpp
#  if __cplusplus < 201402L
#    warning "The minimum language standard to use Boost.Math will be C++14 starting in July 2023 (Boost 1.82 release)"
#  endif
```

Under `-std=gnu++11` + `-Werror` this is a hard error, not a warning. Verified with
the exact bundled tarball (md5 `9dcd632441e4da04a461082ebbafd337`):

```
g++-15 -std=gnu++11 -Werror -I<boost-1.84> probe.cxx
error: #warning "The minimum language standard to use Boost.Math will be C++14 …" [-Werror=cpp]
```

Fix: the Boost includes live inside diagnostic-suppression pragmas in
`main/sc/inc/bigfloat.hxx` and nowhere else. GCC, clang and MSVC need three
different spellings — clang rejects the GCC option spelling under `-Werror`, so the
branches cannot be merged.

## 3. Numeric-path map

Role codes: **must** = has to change for BigFloat to work end to end;
**narrow** = must change eventually but only to *report* precision loss;
**keep** = stays `double` on purpose.

| File | Function / type | Current type | Class | Role | Must change for BigFloat? | Can remain `double`? | Phase |
| --- | --- | --- | --- | --- | --- | --- | --- |
| `main/formula/inc/formula/token.hxx:41` | `StackVarEnum` | enum | A | category tag of every stack value | **yes** — add `svBigFloat` | n/a | 2 |
| `main/formula/inc/formula/token.hxx:232` | `FormulaDoubleToken` | `double` | A | result transport | no | yes | — |
| `main/formula/inc/formula/compiler.hrc:110-193` | opcode range | id | A | function id space | **yes** — `SC_OPCODE_BIGFLOAT` | n/a | 3 |
| `main/formula/inc/formula/opcode.hxx` | `OpCodeEnum` | enum | A | opcode names | **yes** — `ocBigFloat` | n/a | 3 |
| `main/formula/source/core/api/token.cxx:89-132` | `IsFunction` / `GetParamCount` | opcode ranges | A | arity by range | no (1-par range used) | yes | 3 |
| `main/formula/source/core/api/FormulaCompiler.cxx:1563-1615` | `GetStringFromToken` | token | A/F | token → formula text | no (BigFloat never in a compiled token array) | yes | — |
| `main/formula/source/core/resource/core_resource.src` | 3 name string lists | resource | A | name ↔ opcode | **yes** | n/a | 3 |
| `main/sc/inc/token.hxx:57` | `ScToken` | base class | A | Calc token base | no (parent for the new token) | n/a | 2 |
| `main/sc/inc/bigfloat.hxx` (new) | `ScBigFloat` | `cpp_dec_float<50>` | — | the type | n/a | n/a | 1 |
| `main/sc/inc/bigfloattoken.hxx` (new) | `ScBigFloatToken` | `ScToken` | A/B | carries a BigFloat result | n/a | n/a | 2 |
| `main/sc/inc/formularesult.hxx:39-43` | `union { double mfValue; const FormulaToken* mpToken; }` | double/token | B | result storage | **no layout change** — the default arm of `ResolveToken` already parks unknown tokens in `mpToken` | yes | 2 |
| `main/sc/inc/formularesult.hxx:413` | `IsValue()` | 3-type whitelist | B | "is a number" predicate | **yes** — add `svBigFloat` | n/a | 2 |
| `main/sc/inc/formularesult.hxx:474` | `GetDouble()` | `default:` → `0.0` | B | result → double | **yes** — explicit narrowing arm | yes (narrowing) | 2 |
| `main/sc/inc/formularesult.hxx:505` | `GetString()` | `default:` → `""` | B | result → text | no (BigFloat has its own decimal text) | yes | — |
| `main/sc/source/core/data/cell.hxx:289` | `ScFormulaCell::aResult` | `ScFormulaResult` | C | cell storage | no | yes | — |
| `main/sc/source/core/data/cell2.cxx:406-441` | `GetValue` / `GetValueAlways` / `GetString` | `double` | C | cell access | **yes** — add `GetResultToken()`; `GetValue()` narrows | yes (narrowing) | 3 |
| `main/sc/source/core/data/cell.cxx:1689` | CalcAsShown rounding | `double` | C/E | precision-as-shown | **yes** — must **exclude** `svBigFloat` or it replaces the token with a double | yes | 3 |
| `main/sc/source/core/data/cell.cxx:1717` | non-finite result check | `double` | C | NaN/Inf → error | **yes** — exclude `svBigFloat`; the interpreter maps non-finite BigFloat to a Calc error | yes | 3 |
| `main/sc/source/core/data/cell.cxx:1656,1669` | content-change detection | `svDouble`/`svString` pairs | C | repaint / modified flag | **yes** — add a `svBigFloat` comparison | yes | 3 |
| `main/sc/source/core/data/cell.cxx:1591` | iteration convergence | `svDouble`/`svString` | C | circular references | **yes** — add `svBigFloat` | yes | 3 |
| `main/sc/source/core/data/conditio.cxx:247,271,327,364` | constant folding | `svDouble` only | E | conditional formats | narrow | yes | 6 |
| `main/sc/inc/document.hxx:811` | `ScDocument::GetValue` | `double` | G | scripting / add-ins | no | yes (narrowing) | — |
| `main/sc/source/core/data/document.cxx:2768` | `GetStringForFormula` | `double` | E/G | input-line text | narrow (Phase 7) | yes | 7 |
| `main/sc/source/core/tool/cellform.cxx:131-153` | display of a formula cell | `SvNumberFormatter::GetOutputString(double)` | E | **the display path** | **yes** — new BigFloat branch, must not go through double | yes for other cells | 3 |
| `main/sc/source/core/tool/cellform.cxx:205-209` | `GetInputString` | `double` | E | input line | narrow (Phase 7) | yes | 7 |
| `main/svl/source/numbers/zforlist.cxx:1541` | `GetOutputString(String&…)` | text only | E | svl | no — **there is no svl API that formats a numeric string** | — | 7 |
| `main/sc/source/core/inc/interpre.hxx:851-863` | `Interpret()` result | `xResult` token | A | producer | no — type-agnostic | n/a | — |
| `main/sc/source/core/tool/interpr4.cxx:1043` | `PopDouble()` | `svDouble` | A | operand pop | **yes** — `svBigFloat` must be a *known* case | yes | 2 |
| `main/sc/source/core/tool/interpr4.cxx:1612` | `PushDouble()` | `double` | A | result push | no — parallel `PushBigFloat()` added | yes | 2 |
| `main/sc/source/core/tool/interpr4.cxx:1884` | `GetDouble()` | switch over stack type | A | operand get | **yes** — must not silently mis-handle a new type | yes | 2 |
| `main/sc/source/core/tool/interpr4.cxx:3900-4002` | final result switch | `default: SetError(errUnknownStackVariable)` | A | producer | **yes** — add `svBigFloat` | n/a | 3 |
| `main/sc/source/core/tool/token.cxx:327-381` | `ScRawToken::Clone()` | per-type size | A | compiler transport | no — a BigFloat is never a raw token | n/a | — |
| `main/sc/source/core/tool/compiler.cxx:4297-4330` | `HandleExternalReference` | `default:` treats unknown as `svDoubleRef` | A | reference update | no (no BigFloat token ever reaches it) | n/a | — |
| `main/sc/inc/scmatrix.hxx:47,60` | `ScMatrix::fVal` | `double` | D | arrays | no | yes | 8 |
| `main/sc/source/filter/excel/xeformula.cxx:1161-1188` | `Factor()` | `default:` → `ProcessFunction()` | F | BIFF/OOXML formula export | no for Phase 3; **must** be handled before any file-format claim | yes | 7 |
| `main/sc/source/filter/excel/xetable.cxx:794` | `XclExpFormulaCell` | `GetValue()` | F | numeric persistence | no, verified: narrows, formula carries the value (§7.3) | yes | 7 |
| `main/sc/source/filter/xml/xmlcelli.cxx:172` | ODF `office:value` import | `→ double` | F | import | no; the formula is recalculated, §7.3 | yes | 7 |
| `main/sc/source/filter/xml/xmlexprt.cxx:4179` | ODF `office:cache-cell` export | `svDouble` only | F | external-ref cache | no | yes | 7 |
| `main/sc/source/ui/unoobj/tokenuno.cxx:349-437` | `ConvertToTokenSequence` | `DBG_ERROR` on unknown | G | UNO token API | no | yes | 9 |
| `main/sc/source/core/tool/rangeseq.cxx` | range sequencing | `double` | D | array/sequence evaluation | no | yes | 8 |
| `main/sc/source/core/tool/odff*`, solver/CoinMP | solver backend | `double` | H | solver | **never** (separate project) | yes | — |
| `main/sc/source/core/tool/interpr5.cxx` | statistical distributions | `double` | I | statistics | individual audit required | yes | 9 |
| `main/sc/source/core/data/date*`, `interpr2.cxx` date functions | dates | `double` serial | J | date/time | no | yes | — |
| `main/sc/source/core/tool/interpr1.cxx` `CompareFunc` etc. | comparisons | `double` | K | comparisons | **yes** for BigFloat operands (Phase 6) | yes | 6 |
| `main/sc/source/core/data/table3.cxx:262` | `ScTable::IsLess` (sorter) | `IsValue()` → `double` | L | sorting | no; `IsValue()` keeps it a *number* (§7.4) | yes | 6 |
| `main/sc/source/core/data/conditio.cxx:638,855` | conditional format | `IsValue()` → `double`/text | E | conditional format | no; same mechanism (§7.4) | yes | 6 |
| `main/sc/source/core/tool/interpr1.cxx` `LOOKUP` helpers | comparisons | `svDouble`/`svString` | L | lookup | no | yes | 6 |
| `main/sc/source/core/tool/round*`, `MROUND`, `CEILING` | rounding | `double` | — | rounding | no in Phase 3; semantics must be documented first | yes | 6 |

## 4. Answers to the required audit questions (A–N)

**A. Where is `StackVar` used?** In 36 files, ~900 lines: `main/formula/inc/formula/token.hxx`
(declaration, `FormulaToken::eType`), `main/sc/inc/compiler.hxx:110-174` (`ScRawToken`
embeds it as the payload selector), `main/sc/inc/token.hxx` (Calc token ctors),
`main/sc/inc/formularesult.hxx`, `main/sc/source/core/inc/interpre.hxx:362-366,851,857`,
the interpreter sources, `main/sc/source/core/data/{cell,cell2,conditio,validat,documen4}.cxx`,
the Excel filters (`xeformula.cxx`, `xelink.cxx`, `xechart.cxx`, `xichart.cxx`,
`xlformula.cxx`), `main/sc/source/filter/xml/xmlexprt.cxx`, and the UNO token bridge
`tokenuno.cxx` / `chart2uno.cxx`.

**B. Which switches over `StackVar` need a new `svBigFloat` case?** Only those on the
result path, because a BigFloat token only ever appears as an intermediate or final
formula result:

* `main/sc/source/core/tool/interpr4.cxx:1043` `PopDouble()` — must not hit `default: SetError(errIllegalArgument)` by accident; the BigFloat case is added with an explicit, documented error.
* `main/sc/source/core/tool/interpr4.cxx:1884` `GetDouble()` and `:1971` `GetString()`.
* `main/sc/source/core/tool/interpr4.cxx:3900` — the *final* result switch inside `Interpret()`; its `default:` raises `errUnknownStackVariable`, so a BigFloat result would be destroyed. **Mandatory.**
* `main/sc/inc/formularesult.hxx:234` `ResolveToken()` — already correct via `default:`; a BigFloat is kept in `mpToken` (mandatory *not* to add it to the resolved arms, that would make every cell hold a token).
* `main/sc/inc/formularesult.hxx:481,509` `GetDouble()`/`GetString()` — narrowing arm added for the former.
* `main/sc/source/core/data/conditio.cxx:63` and `main/sc/source/core/tool/token.cxx:1416,1530` — array/conditional-format literal handling; not reachable, left alone deliberately.

**C. Which classes derive from `FormulaToken`?** `FormulaByteToken` (→ `FormulaFAPToken`,
`FormulaStringOpToken`), `FormulaDoubleToken`, `FormulaStringToken`, `FormulaIndexToken`,
`FormulaExternalToken`, `FormulaMissingToken`, `FormulaJumpToken`,
`FormulaSubroutineToken`, `FormulaUnknownToken`, `FormulaErrorToken`, and Calc's
`ScToken` (→ `ScSingleRefToken`, `ScDoubleRefToken`, `ScMatrixToken`,
`ScExternalSingleRefToken`, `ScExternalDoubleRefToken`, `ScExternalNameToken`,
`ScJumpMatrixToken`, `ScRefListToken`, `ScEmptyCellToken`,
`ScMatrixCellResultToken`, `ScMatrixFormulaCellToken`, `ScHybridCellToken`).
The new `ScBigFloatToken` derives from `ScToken`, exactly like every other Calc-specific
result token.

**D. Where are numeric tokens cloned?** `FormulaToken::Clone()` and every override
(`token.hxx:203,227,242,...`); `ScRawToken::Clone()` (`token.cxx:327`) for compiler
tokens; `ScFormulaResult`'s copy ctor clones only `ScMatrixFormulaCellToken`
(`formularesult.hxx:89-92`); `ScFormulaCell::InterpretTail` stores `p->GetResultToken()`
which shares (IncRef) rather than clones. `ScBigFloatToken::Clone()` is a real deep
copy of the decimal value.

**E. Where are `FormulaToken`s serialized?** `main/sc/source/filter/excel/xeformula.cxx:1161`
(`Factor()` → `ProcessDouble` for BIFF/OOXML), `main/formula/source/core/api/FormulaCompiler.cxx:1563`
(token → formula *string*, used for the OOXML `<f>` element), `main/sc/source/filter/excel/xelink.cxx:1287`
(external-ref cached value), `main/sc/source/filter/xml/xmlexprt.cxx:4179`
(ODF `office:cache-cell`). **A BigFloat never reaches any of these in Phase 3** — the
token exists only in the interpreter stack and in the cell result, and is written back
as its formula. `Factor()`'s `default:` silently emits an unknown type as a *function*
token, so Phase 7 must add an explicit case before any format claim is made.

**F. Where are `FormulaToken`s compared?** `FormulaToken::operator==`
(`formula/source/core/api/token.cxx:140`) compares `eType` first, so a BigFloat token is
never equal to a double token; each subclass overrides it, so `ScBigFloatToken::operator==`
compares the decimal values exactly. Additionally `cell.cxx:1656,1669` compare
*results* (not tokens) to decide whether the cell changed — that needed a new arm.

**G. Where does `ScFormulaResult` assume every numeric value is `svDouble`?**
`GetType():378` (`!mbToken ⇒ svDouble`, true for us because BigFloat always uses the
token arm), `IsValue():416` (**must change**), `GetDouble():489` (matrix upper-left
numeric gate, and the new narrowing arm), `GetString():518` (matrix upper-left text gate),
and `SetHybridString/Formula:573,587` (they capture a lossy `GetDouble()`; a BigFloat
cannot survive the hybrid import path, which is import-only and documented as such).

**H. Where does `ScFormulaCell` assume a numeric result is representable as `double`?**
`GetValue()`/`GetValueAlways()` (`cell2.cxx:413,428`), the `IsValue()`-gated display in
`cellform.cxx:131`, `GetStandardFormat` (`cell.cxx:1795`), CalcAsShown (`cell.cxx:1690`)
and the non-finite guard (`cell.cxx:1717`). The last two are *active* hazards: both are
gated on `IsValue()`, which a BigFloat now satisfies, and both would write a narrowed
double back into the result. Both are excluded explicitly.

**I. Where does Calc formatting convert numbers to String?**
`ScCellFormat::GetString` (`cellform.cxx:131-153`) → `SvNumberFormatter::GetOutputString(double, …)`
(`svl/source/numbers/zforlist.cxx:1524` → `SvNumberformat::GetOutputString(double …)`),
plus `GetInputString` (`cellform.cxx:205`) → `GetInputLineString`, and
`ScInterpreter::GetCellString` (`interpr4.cxx:487`) for the formula side.

**J. Where does ODF export convert numeric cells to XML?** In this checkout there is
**no ODF cell export** (`main/sc/source/filter/xml/xetable.cxx` does not exist; there is no
`sc/source/filter/oox/`). The only numeric writes are `XclExpNumberCell::SaveXml`
(`filter/excel/xetable.cxx:594`, `<c t="n"><v>` from a `double`) and
`WriteContents` (BIFF). ODF *external reference caches* are written in
`filter/xml/xmlexprt.cxx:4179` from `svDouble`.

**K. Where does ODF import parse numeric cells?** `filter/xml/xmlcelli.cxx:172` →
`SvXMLUnitConverter::convertDouble(double&, …)` → `rtl::math::stringToDouble`
(`xmloff/source/core/xmluconv.cxx:781`). The precision is lost at the parse, before Calc
sees the value.

**L. Which UNO APIs force double conversion?** Every numeric Calc API:
`XCell::getValue`, `ScTableSheetObj`, `ScCellObj` (`celluno.cxx`), the document's
`getDataArray`, the token bridge `tokenuno.cxx`, and the external-ref cache
`linkuno.cxx:1498`. These stay `double`; a string-based high-precision API is a
deliberate later decision, not an accident of this implementation.

**M. Which matrix APIs force double conversion?** All of them: `ScMatrix::fVal`
(`scmatrix.hxx:47`, with the comment "optimized for speed and double values"),
`PutDouble/GetDouble/FillDouble`, `ScInterpreter::CreateDoubleArr`,
`GetDoubleOrStringFromMatrix`, and `rangeseq.cxx`. Untouched in Phase 3; a BigFloat can
never enter a matrix in this phase, which is exactly why the prototype starts scalar.

**N. Which functions can safely support promotion?** In order, and only after the
scalar path is proven: the arithmetic operators and `ABS`/`SQRT`/`POWER`; then the
transcendental functions; then `SUM`/`PRODUCT`/`AVERAGE`/`MIN`/`MAX` (they go through
`ScMatrix` and need the matrix audit first). Comparisons (`=`, `<`, …) need the mixed
BigFloat/`double` semantics defined *before* implementation — see §7.

## 5. Architecture decisions

### 5.1 `svBigFloat` lives in the generic formula module

`formula::StackVarEnum` gains `svBigFloat` **after `svError`** (value 20). Rationale:
the generic module then owns only the *category*, while the value type stays in Calc.

* The only hard-coded numbering in the enum is `svMissing = 0x70`; there is no
  `StackVarCount`, no `sizeof(StackVarEnum)` and no array sized by the enum, so adding
  an enumerator before `svMissing` is safe.
* `StackVar` is `sal_uInt8` in production builds; 20 is far from the limit.
* Every switch over `StackVar` has a `default:`, so no switch becomes non-exhaustive.

The alternative (a Calc-private extension mechanism) would have required a parallel type
tag on every token and a second comparison in every `GetType()` consumer — strictly more
code and more places to get wrong.

### 5.2 The token, not the union

```
double  + double        -> double   (unchanged code path)
BigFloat + BigFloat     -> ScBigFloatToken
BigFloat token          -> ScFormulaResult::mpToken   (mbToken = true)
```

`ScFormulaResult` keeps its exact layout. `ResolveToken()`'s `default:` arm already does
the right thing; the union and the `double` fast path are untouched, so a document with
no BigFloat allocates and behaves exactly as before.

### 5.3 `ScBigFloatToken` derives from `ScToken`, not from `FormulaToken`

Every Calc-specific result token derives from `ScToken`; deriving from it gives the
token the same behaviour as `ScEmptyCellToken` and `ScHybridCellToken` for
`TextEqual`, `Is3DRef` and the reference accessors.

Public surface (deliberately small):

```cpp
const ScBigFloat &     GetBigFloat() const;   // exact value
const String &         GetString() const;     // full-precision decimal text
double                 GetDouble() const;     // EXPLICIT narrowing, never implicit
ScBigFloatToken *      Clone() const;
sal_Bool               operator==( const formula::FormulaToken& ) const;
```

There is **no** `operator double()` and no implicit conversion in either direction. The
`GetDouble()` override exists only because the virtual
`formula::FormulaToken::GetDouble()` is the interface `ScFormulaResult` already uses to
narrow, and its base implementation asserts. It is a documented narrowing conversion.

### 5.4 Boost is visible in exactly one header

`main/sc/inc/bigfloat.hxx` is the only Calc header that includes Boost. It is *not*
included by `formularesult.hxx`, `cell.hxx`, `interpre.hxx` or `token.hxx`, so the
boost-free public surface is preserved. Code that needs the value uses
`ScBigFloatToken` (a `ScToken`), and narrows through the existing virtual
`GetDouble()`.

### 5.5 Input never passes through `double`

`ScBigFloatFromString()` constructs the decimal from the text directly. No
`rtl::math::stringToDouble()`, no `atof`, no `strtod`. The only locale handling is
replacing the document's decimal separator with `'.'` and dropping grouping
separators *in the text*, never in a parsed number. The input is validated
character by character before Boost sees it, because Boost's string constructor
throws `std::runtime_error` on input it cannot represent (an exponent outside
its range, for example); `sc` is built with `-fexceptions`
(`gb_Library_add_exception_objects` in `main/sc/Library_sc.mk`), so the parser
catches that and reports an error instead.

### 5.6 Conversion directions, measured

* `ScBigFloat` → `double`: **not** implicit. `std::is_convertible<ScBigFloat,
  double>` is false, and there is a unit test pinning that down, because an
  implicit narrowing is exactly what would destroy the feature unnoticed.
* `double` → `ScBigFloat`: implicit, because Boost's `number` has a
  non-`explicit` constructor from floating point types. This direction is
  lossless, it is the promotion the plan asks for, and fighting it would mean
  wrapping the Boost type in a Calc class. It is documented here so that nobody
  is surprised by it.


## 6. Hazards found in the audit that had to be handled explicitly

| # | Location | Silent failure mode if ignored | Handling |
| --- | --- | --- | --- |
| 1 | `cell.cxx:1689` CalcAsShown | replaces a BigFloat result with a rounded `double` | condition excludes `svBigFloat`; precision-as-shown is not applied to BigFloat in this phase (§7) |
| 2 | `cell.cxx:1717` non-finite check | narrows a BigFloat just to test it | excluded; the interpreter maps non-finite BigFloat to a Calc error before the cell sees it |
| 3 | `cell.cxx:1656,1669` change detection | an unchanged-looking type with a *changed* value never repaints | explicit `svBigFloat` value comparison |
| 4 | `cell.cxx:1591` iteration convergence | a circular BigFloat formula never converges | explicit arm |
| 5 | `interpr4.cxx:4000` final result switch | `errUnknownStackVariable` — the result is destroyed | explicit `svBigFloat` case |
| 6 | `formularesult.hxx:416` `IsValue()` | the cell is treated as text everywhere | extended |
| 7 | `formularesult.hxx:493` `GetDouble()` `default:` | `0.0` with no diagnostic | explicit narrowing arm |
| 8 | `xeformula.cxx:1177` | a BigFloat would be exported as a *function* token | unreachable in Phase 3 (result is written as its formula); must be handled in Phase 7 |
| 9 | `token.cxx:369` `ScRawToken::Clone()` | a shorter copy if an unknown type were ever used | unreachable: a BigFloat is never a raw token |
| 10 | `compiler.cxx:4318` | an unknown type is reinterpreted as `svDoubleRef` | unreachable: no BigFloat token is ever created by the compiler |
| 11 | `interpr4.cxx` `PopDouble()` / `GetDouble()` | the double path would consume a BigFloat and drop digits | explicit case, `errIllegalArgument` / `errIllegalParameter`; promotion is Phase 5 |
| 12 | `ScMatrixFormulaCellToken::Assign` (`token.cxx:1113`) | a BigFloat upper-left result in a matrix formula cell would be dropped | not a hazard: the non-matrix branch keeps *any* token as the upper-left token |
| 13 | `interpr1.cxx` `ScIsValue()` | `ISVALUE` would be `FALSE` for a number that is a number | `case svBigFloat:` added next to `case svDouble:` |
| 14 | `interpr4.cxx` `PushCellResultToken()` | a reference to a BigFloat cell would be narrowed by `GetCellValue()` | the result token is read first, a BigFloat is pushed as a BigFloat |
| 15 | `interpr4.cxx` `GetCellString()` | a referenced BigFloat cell becomes a printed double | exact decimal text instead |
| 16 | `cell.cxx:1081` `CalcAfterLoad()` | the load-time infinity guard narrows a BigFloat result | skipped for `svBigFloat`, a BigFloat is finite by construction |
| 17 | `cell.cxx:1818` `GetStandardFormat()` | the standard-format lookup narrows a BigFloat to pick a number format | skipped, the format type is all it needs |
| 18 | `cell.cxx:1694` "initial results after loading" | an unchanged BigFloat after loading would mark the sheet modified | BigFloat arm added next to the `svDouble`/`approxEqual` one |
| 19 | `table3.cxx:262` sorter, `conditio.cxx:638` | a BigFloat cell would be classified as *text* and sorted/compared as such | not a hazard after #6: `IsValue()` now reports a number, see §7.4 |
| 20 | `interpr4.cxx` `PushBigFloatToken()` | `GetDoubleErrorValue()` decodes a Calc error out of the *bits* of a double; a Boost NaN carries no such code, so its payload would be read as an arbitrary error (measured: 32752) | NaN and infinity are told apart with `ScBigFloatIsNaN()` and mapped to `errNoValue` / `errIllegalFPOperation` |

## 7. Open semantic questions (deliberately not guessed)

* **Precision as shown** (`Tools ▸ Options ▸ Calc ▸ Calculation`): Phase 3 does **not**
  apply it to BigFloat values. Round-tripping the *displayed* decimal back into a
  BigFloat is the correct conceptual behaviour, but it needs a decimal-string number
  formatter (Phase 7); silently narrowing instead would destroy the feature.
* **Mixed comparisons** `BigFloat op double`: promoting the double to its exact binary
  value is *not* the same as its decimal display. Tests must be written before this is
  implemented (Phase 6).
* **Non-finite BigFloat**: mapped to Calc errors at push time (§26), not by leaking
  C++ `inf`/`nan` into the document.

### 7.1 The `IS*` family and type predicates

A predicate must answer for a BigFloat exactly as it answers for a double, no more and
no less. What the audit found in `interpr1.cxx`:

| Function | How `svDouble` is handled | What a BigFloat gets |
| --- | --- | --- |
| `ISVALUE` | explicit `case svDouble:` | **changed**: `case svBigFloat:` added to the same group |
| `ISNUMBER` | falls into `default: PopError()` | same `default`, identical by construction |
| `ISTEXT` (`IsString()`) | explicit `case svString:` only | `default`, so `FALSE` — correct |
| `ISLOGICAL` | `default:` tests `nCurFmtType` | same, and a BigFloat never sets it to `LOGICAL`, so `FALSE` — correct |
| `ISERROR`/`ISERR` | `default:` leaves `nRes` 0 | `FALSE` — correct |
| `ISREF`, `ISBLANK` | `default:` | `FALSE` — correct |
| `TYPE` | `default:` returns 1 | 1, i.e. a number — correct |

Nothing was "fixed" beyond `ISVALUE`: changing `ISNUMBER` to return `TRUE`
explicitly for a BigFloat would have made it answer differently from `ISNUMBER(1)`
in this tree, and a BigFloat is not a place to alter an existing predicate.

### 7.2 References to a BigFloat cell

`ScInterpreter::PushCellResultToken()` is the single funnel through which a *cell
reference* becomes a stack value (it serves both `=A1` as a final result and the
reference used by operators). It now reads the result token first, so

```
A1: =BIGFLOAT("1.2345…6789")
A2: =A1                 -> BigFloat, all digits shown
A2: =A1&"x"             -> the exact decimal text as the string
A2: =A1+1               -> Err:502, promotion is Phase 5
```

`ScInterpreter::GetCellString()` got the matching treatment, so a BigFloat cell
becomes text as its exact decimal representation rather than as a double printed
through the standard number format.

Ranges are *not* covered: a reference to a range is resolved into a `ScMatrix` of
doubles, which is the matrix audit of Phase 8. `ScDocument::GetValue()` and the UNO
`double` APIs still narrow, deliberately — see §6 and §3 row "L".

### 7.3 Saving a document: what actually happens

Traced, not assumed. For a formula cell whose result is a BigFloat:

* **ODF** (`main/sc/source/filter/xml/xmlexprt.cxx:2972`): `IsValue()` is true, so the
  cell is written as a *number* cell. The formula goes out with the storage grammar
  and therefore with the ODF function name `ORG.OPENOFFICE.BIGFLOAT("…")`; the
  cached value is written through `ScDocument::GetValue()`, i.e. **narrowed**.
* **BIFF/Excel** (`main/sc/source/filter/excel/xetable.cxx:794`): the cell is
  exported as a formula cell with the same function name, and the result is read
  with `GetValue()`, again **narrowed**. No assert, no dropped cell.
* Neither path can carry more digits, and that is not an AOO limitation: ODF
  declares `office:value` as `xsd:double`, and the Excel formats have a 15 digit
  limit built into their record layout. Writing 50 digits there would require a
  private extension, which the plan explicitly rules out without a standards
  basis.

So the formula is the canonical representation, exactly as §33 of the plan
anticipated: the exact value survives the round trip through the formula text, and
the cached number in the file is a display-level copy. The known limitation, stated
plainly: a document opened with "Recalculation on load: Never" shows the narrowed
value until the cell is recalculated. No file format claim of full precision is
made here.

### 7.4 Where a BigFloat cell is classified as a number, and where it is only compared as one

Two subsystems decide "number or text?" by asking `ScFormulaCell::IsValue()`:

* the sorter, `main/sc/source/core/data/table3.cxx:262-266`
* conditional formatting, `main/sc/source/core/data/conditio.cxx:638` and `:855`

Because the Phase 2 change made `IsValue()` true for a BigFloat result, a BigFloat
cell is **classified as a number** in both — it is not silently sorted or compared
as text, which is what extending the union alone would have caused. The comparison
itself then uses `GetValue()`, i.e. a narrowed double, so ordering and conditions
are decided on ~15 digits. That is the honest Phase 6/7 limitation: correct
behaviour, reduced precision, no misclassification. Fixing it means giving the
sorter and the condition evaluator a BigFloat branch, which is a change to their
comparison contract and belongs with the promotion work rather than with the type
introduction.

## 7a. Verification checklist for a real build

The steps this environment could not perform, in the order they should be run:

1. `./configure` (system Boost is not needed — the bundled 1.84.0 is fetched per
   `main/external_deps.lst`) and build `main/sc` plus the `sc` unit test target.
2. `main/sc/test/bigfloattests.cxx` must pass as part of the `sc` gtest binary.
3. In a real Calc: `=BIGFLOAT("1.234567890123456789012345678901234567890")` shows
   every digit; `=ISNUMBER(A1)` is `TRUE`; `=A1&""` is the exact decimal.
4. `=BIGFLOAT("10000000000000000000000000000000000001")-BIGFLOAT("10000000000000000000000000000000000000")`
   reports `Err:502` today, and must return `1` once Phase 5 lands.
5. Save as `.ods`, reopen, confirm the formula and the digits.
6. Run the pre-existing Calc test suites to confirm nothing regressed; the
   `double` path must be bit-identical, which is the strongest available check
   that ordinary calculations were not disturbed.



## 8. Function registration checklist (as mined from the source)

Verified against the most recent additions (`ARABIC`, `BITAND`…`BITRSHIFT`). There is
no function-pointer table and no function-metadata XML; registration is:

1. `main/formula/inc/formula/compiler.hrc` — `SC_OPCODE_BIGFLOAT` in the **one-parameter**
   range, and bump `SC_OPCODE_STOP_1_PAR`. The 1-par range matters: `GetParamCount()`
   derives the arity from the range, whereas the 2+ range would return the token's
   parameter byte, which the Calc compiler does not set.
2. `main/formula/inc/formula/opcode.hxx` — `ocBigFloat`.
3. `main/formula/source/core/resource/core_resource.src` — three entries: the ODFF
   list, the English list (used for ODF 1.0/1.1 storage and `XFunctionAccess`), and the
   localizable list (used for the UI). An OOo extension follows the
   `ORG.OPENOFFICE.*` convention in the ODFF list.
4. `main/sc/source/core/inc/interpre.hxx` — declare `ScBigFloat()`.
5. `main/sc/source/core/tool/interprN.cxx` — implement it.
6. `main/sc/source/core/tool/interpr4.cxx` — `case ocBigFloat: ScBigFloat(); break;`
   in the dispatch switch of `Interpret()`.
7. `main/sc/source/core/tool/parclass.cxx` — parameter classification (optional; the
   default is all-`Value` with no count check).
8. `main/sc/source/ui/src/scfuncs.src` — Function Wizard metadata (description,
   category, help id, parameter names).
9. `main/sc/inc/helpids.h`, `main/sc/util/hidother.src`, a `helpcontent2` topic.
10. `main/oox/source/xls/formulabase.cxx` and
    `main/sc/source/filter/excel/xlformula.cxx` — so XLSX/BIFF keep the name verbatim
    across a round trip.

No name collides: `BIGFLOAT`, `BIGADD`, `BIGSUB`, `BIGMUL`, `BIGDIV`, `BIGSQRT`,
`BIGPOW` do not occur anywhere in the tree.

## 9. Build and test notes

* Calc's C++ unit tests are **gtest** in `main/sc/test/`, registered in
  `main/sc/GoogleTest_sc.mk` (add the object to
  `gb_GoogleTest_add_exception_objects`) and gated by `ENABLE_UNIT_TESTS` (default on).
  There is no `main/sc/qa/unit` in this tree and no Boost.Test anywhere.
  `main/sc/test/bigfloattests.cxx` follows the one existing test
  (`stringutiltests.cxx`) and covers exact construction, 1/3, sqrt(2), the
  catastrophic-cancellation case, a 25x25 digit product, division, exponents,
  invalid input, locale separators, the missing implicit conversion to double,
  token clone/equality, and the 20/30/40/50 digit round trip.
* The functional suite in `test/` is Java/JUnit and runs against an installed
  build; it cannot exercise BigFloat until a full build exists.
* This checkout has no build tree (`main/configure`, `main/inc/` and all objects are
  absent) and no Boost. A full AOO build is also impossible here for a second,
  independent reason: the UNO C++ headers (`main/offapi/**/*.hpp`) are generated
  from `.idl` by `idlc` during the build, and this tree contains zero of them, so
  every Calc translation unit (all of them include `precompiled_sc.hxx`, which
  includes `com/sun/star/uno/*.hpp`) is uncompilable until a build has run.

### 9.1 What was actually verified

| Check | Result |
| --- | --- |
| `boost/multiprecision/cpp_dec_float.hpp` with `g++-15 -std=gnu++11 -O2 -fPIC -Wall -Wextra -Wshadow -Werror` (AOO's flags) | builds; produces the values in §2.1 |
| Same include with `-Werror`, no pragma | **fails** (`-Werror=cpp`), see §2.1 |
| The pragma workaround, GCC branch | builds with `-Werror` |
| `bigfloat.cxx` compiled with AOO's exact warning flags against the real AOO headers | clean |
| `bigfloat.cxx` logic: 30+ assertions (exact construction, cancellation, 1/3, sqrt 2, locale, exponent, 12 rejected inputs, double promotion/narrowing, finite/zero) | all pass |
| Every numeric expectation used in `bigfloattests.cxx` | verified against the real Boost library, not against a double |
| `sizeof(ScBigFloat)` | 56 bytes on x86-64 |
| Compilation of the sc/formula translation units | **not possible here**, generated UNO headers are missing (see §9.2) |
| Does the new code depend on any generated (idlc output) type? | **no** — no `com::sun::star::`, `uno::`, `Reference<>`, `Sequence<>` in any added line |
| Brace/paren balance of all 19 touched C++ files, compared against `HEAD` | identical to `HEAD` (caught one real error of mine, see below) |
| Every identifier the new code calls (69 API names) exists in the tree | all 69 resolve; a typo such as `GetLocaleData` vs `GetpLocaleData` cannot hide |
| Registration completeness: 20 required points (opcode id inside the 1-par range, `opcode.hxx`, all three name tables, dispatch, final-result case, `parclass`, declaration, definition, both filter tables, help id, wizard entry, help topic, both build files, `svBigFloat`) | all present and well formed; no unrelated `BIGFLOAT` name collision anywhere in `main/` |
| The plan's own checklist items that cannot run here | see §7a |
| The three opcode name tables stay positionally parallel (they are consumed by index) | 333/333/333 at `HEAD`, 334/334/334 now, one entry added to each at the same position |
| NaN vs infinity are distinguishable, as the error mapping requires | verified against real Boost: `1/0` is infinity, `inf/inf` and `0/0` are NaN |

### 9.2 Why there is no build here, and how far stubbing got

`main/offapi` contains no generated `.hpp` at all (5289 `.idl` sources, zero
generated headers), and every Calc translation unit includes `precompiled_sc.hxx`,
which includes UNO C++ headers. So a Calc TU is uncompilable until `configure` plus
an `idl` generation run has happened.

Hand-stubbing was attempted and abandoned deliberately, for a reason worth
recording:

* `com/sun/star/sheet/FormulaLanguage.hpp` — derivable. It is a constants-only IDL,
  so the stub was generated mechanically from the checked-in
  `main/offapi/com/sun/star/sheet/FormulaLanguage.idl`.
* `com/sun/star/uno/TypeClass.hdl` — derivable. Generated mechanically from the
  checked-in `main/cppu/inc/typelib/typeclass.h`, which is documented as binary
  compatible with the IDL enum.
* `com/sun/star/uno/XInterface.hpp` — **not** derivable. From here on the missing
  headers are idlc-generated *class* declarations whose exact shape (base classes,
  member signatures, export macros) is what a real compilation needs. Guessing them
  would produce a build that can go green while the real build fails, which is a
  worse outcome than no build at all, because it would be trusted.

The risk is bounded rather than eliminated by this: **no added line touches a
generated type** (§9.1), so the unverifiable surface is the pre-existing context of
the files I edited — HEAD code that already compiles — plus small, reviewed edits
inside it. The new logic itself lives in `bigfloat.hxx`/`bigfloat.cxx`, which do
compile and are tested here, and in the token, which is plain Calc code over
checked-in headers.

The brace/paren check earned its keep: it caught an unbalanced `parclass.cxx`
entry (`{{ Value }}` where every neighbour has `{{ Value }`), which would have
been a hard compile error. A compile would have caught it in seconds; a
careful read did not. That is the argument for the mechanical checks above and
against trusting review of a change this size without a build.

## 10. Behaviour established by the implementation (Phases 1–3)

* `=BIGFLOAT("1.234567890123456789012345678901234567890")` returns a
  `formula::svBigFloat` result. The cell result is a token, not the double in
  the union, so the value is stored and displayed at full precision.
* Display: the cell shows the value's exact decimal representation with the
  locale's decimal separator and an upper case exponent marker. It does **not**
  go through `SvNumberFormatter`, because svl can only format a `double`.
* Trailing zeros of the stored value are not shown: `1.50` is displayed as `1.5`.
  The value is identical either way, this is a property of the text.
* `ISNUMBER`-style classification: `ScFormulaResult::IsValue()` is true for a
  BigFloat, so the result is a Number, not Text.
* Combining a BigFloat with an ordinary number (`=BIGFLOAT("1.5")*2`) currently
  yields `Err:502` instead of a silently narrowed result. That is deliberate:
  promotion is Phase 5, and an error is honest where a wrong-looking answer is
  not.
* `Precision as shown` is not applied to BigFloat results (see §7).
* A BigFloat token becomes text (`&"…"` concatenation, or a text conversion) as
  its exact decimal representation, never as a printed double. This holds for a
  cell reference too, see §7.2.
* `ISVALUE(BIGFLOAT("1.5"))` is `TRUE`; `ISTEXT`, `ISLOGICAL`, `ISERROR`, `ISREF`
  and `ISBLANK` are `FALSE`; `TYPE` returns 1. A BigFloat answers every type
  predicate exactly like a double does, see §7.1.
* A reference to a BigFloat cell stays a BigFloat (`=A1` keeps every digit);
  arithmetic between a BigFloat and a number is not implemented yet and reports
  `Err:502` rather than narrowing.
* Non-finite high precision values are turned into a Calc error when they are
  pushed, so a NaN or infinity cannot reach a cell.

## 11. Memory

`sizeof(ScBigFloat)` is 56 bytes for `cpp_dec_float<50>` (measured, x86-64), and
`ScBigFloatToken` adds the token overhead plus a cached `String` for the decimal
text. For comparison, `ScFormulaResult` keeps `double mfValue` and a token
pointer, and `FormulaDoubleToken` holds a single `double`. A cell whose result is
a BigFloat therefore costs roughly ten times what a numeric cell costs today.

That is the price of not putting BigFloat in every cell, which is the whole
point: a document of ordinary formulas allocates exactly what it allocated
before, because the union still holds a plain `double` and no token is created
unless a formula actually produces a high precision result.

## 12. The milestone, step by step

`=BIGFLOAT("1.234567890123456789012345678901234567890")` in A1, traced through
the code that was actually written. Every step below was checked against the
source, this is not a sketch.

| Step | Where | What happens |
| --- | --- | --- |
| compile | `compiler.hrc:193` (`SC_OPCODE_BIGFLOAT 162`, inside the 1-par range), `opcode.hxx:176`, `parclass.cxx:195` | the name is known, the parameter is a `Value`, `GetParamCount()` returns 1 from the range |
| dispatch | `interpr4.cxx:3879` | `case ocBigFloat : ScBigFloat();` |
| argument | `interpr2.cxx:2560` `ScBigFloat()` | `GetString()` pops the literal as text, `ScBigFloatFromString()` parses it character by character with the document's separators, no double anywhere |
| push | `interpr4.cxx:1744` `PushBigFloatToken()` | non-finite check, then `PushTempTokenWithoutError(new ScBigFloatToken(...))`; the stack type is now `svBigFloat` |
| result format | `interpr4.cxx:4163` | every function defaults `nFuncFmtType` to `NUMBERFORMAT_NUMBER` (`:3542`), so the cell gets `NUMBERFORMAT_NUMBER`; the added `case svBigFloat:` is the same safety net `case svDouble:` has |
| hand-off | `cell.cxx:1678` / `cell.cxx:1704` | `aResult.SetToken( p->GetResultToken())` or `aResult.Assign(...)`; `ScFormulaResult`'s constructor parks an unknown type in `mpToken`, the `double` in the union stays untouched |
| guards | `cell.cxx:1707`, `:1741`, plus `cell.cxx:1081` and `:1818` | Precision-as-shown and the non-finite check skip `svBigFloat`, so neither can replace the token with a narrowed double |
| display | `cellform.cxx:137` | `IsValue()` is true, the BigFloat branch prints the exact decimal with the locale separator, `SvNumberFormatter` is not involved |
| reference | `interpr4.cxx:1010` `PushCellResultToken()` | `=A1` reads the result token and pushes a BigFloat, so the digits survive |
| arithmetic | `interpr4.cxx:1095` `PopDouble()` | `=A1+1` reports `Err:502`; promotion is Phase 5 and must not be faked by narrowing |


