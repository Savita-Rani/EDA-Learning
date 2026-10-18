# GoogleTest — Reference Notes

General-purpose GoogleTest notes for revision/interviews — not tied to any
one phase of this project. See `docs/phases/phase1.md` for how it's
actually wired into *this* repo (`tests/test_gate.cpp`,
`tests/CMakeLists.txt`).

---

## 1. What it is

GoogleTest (`gtest`) is a C++ unit-testing framework from Google. Four
moving parts:

1. **Test registry** — a global list of every test, built automatically
   before `main()` runs.
2. **`TEST`/`TEST_F`/`TEST_P` macros** — define a test and register it.
3. **Assertion macros** — `EXPECT_*`/`ASSERT_*` — check conditions inside
   a running test.
4. **A runner** — iterates the registry, executes each test, reports
   results. `gtest_main` provides a ready-made `main()` for this.

---

## 2. Basic test: `TEST(Suite, Name)`

```cpp
TEST(GateTest, AND) {
    Gate g(GateType::AND);
    EXPECT_TRUE(g.evaluate(true, true));
    EXPECT_FALSE(g.evaluate(true, false));
}
```

- `Suite` groups related tests (shows as `GateTest.AND` in output).
- No forward declaration or registration list needed — the macro handles
  it via a trick described in §6.
- Each `TEST` compiles to its own hidden class derived from
  `::testing::Test`, with your `{ ... }` body becoming `TestBody()`.

---

## 3. Assertions

### EXPECT vs ASSERT

| | On failure | Use when |
|---|---|---|
| `EXPECT_*` | records failure, **keeps running** the rest of the test | default choice — see every failure in one run, not just the first |
| `ASSERT_*` | records failure, **returns immediately** from the enclosing function | a later line would crash/be meaningless if this check fails (e.g. a null pointer you're about to dereference) |

`ASSERT_*` works by literally expanding to a `return;` on failure — so it
can **only be used in a function returning `void`**. You cannot use
`ASSERT_EQ` inside a helper that returns `bool` or `int`; use `EXPECT_EQ`
there, or restructure so the assertion sits directly in `TestBody()`/a
`TEST()`/a void helper called from it with a `(void)` discard pattern —
in practice: keep `ASSERT_*` in test bodies or void helpers only.

### Common macros

| Macro | Checks |
|---|---|
| `EXPECT_TRUE(x)` / `EXPECT_FALSE(x)` | boolean condition |
| `EXPECT_EQ(a, b)` / `EXPECT_NE(a, b)` | equality / inequality (`==`/`!=`) |
| `EXPECT_LT/LE/GT/GE(a, b)` | ordering comparisons |
| `EXPECT_STREQ(a, b)` / `EXPECT_STRNE(a, b)` | C-string **content** equality (not pointer identity!) |
| `EXPECT_STRCASEEQ(a, b)` | case-insensitive C-string equality |
| `EXPECT_FLOAT_EQ(a, b)` / `EXPECT_DOUBLE_EQ(a, b)` | floating-point equality within a few ULPs |
| `EXPECT_NEAR(a, b, tol)` | floating-point equality within an explicit tolerance |
| `EXPECT_THROW(stmt, ExceptionType)` | `stmt` throws exactly `ExceptionType` |
| `EXPECT_ANY_THROW(stmt)` / `EXPECT_NO_THROW(stmt)` | throws *something* / throws nothing |
| `FAIL()` / `ADD_FAILURE()` | unconditionally fail (ASSERT-style / EXPECT-style) |
| `SUCCEED()` | explicitly mark success (rarely needed) |

**Classic gotcha:** `EXPECT_EQ(charPtrA, charPtrB)` on two `const char*`
compares **pointer addresses**, not string contents — two equal-looking
C-strings at different addresses will wrongly fail. Use `EXPECT_STREQ`
for C-strings, or compare `std::string`s with `EXPECT_EQ`.

**Custom failure messages:** stream onto any assertion with `<<` — only
printed if the check fails, a no-op on success:

```cpp
EXPECT_EQ(result, expected) << "mismatch for input " << input;
```

---

## 4. Fixtures: `TEST_F`

Use when multiple tests need the same setup/teardown. GoogleTest
constructs a **fresh fixture instance for every single test** (not
shared across tests in the suite) — this gives test isolation for free.

```cpp
class NetlistTest : public ::testing::Test {
protected:
    void SetUp() override {
        netlist = buildSampleNetlist();   // runs before EACH test
    }
    void TearDown() override {
        // runs after EACH test, even if it failed/threw
    }
    Netlist netlist;
};

TEST_F(NetlistTest, PropagatesSignal) {
    netlist.setInput("A", true);
    EXPECT_TRUE(netlist.output("Y"));
}

TEST_F(NetlistTest, HandlesFloatingInput) {
    EXPECT_FALSE(netlist.output("Y"));  // separate, freshly-built netlist
}
```

**`SetUp()`/`TearDown()` vs constructor/destructor:** prefer
constructor/destructor for simple state; use `SetUp()`/`TearDown()` when
setup can fail and you want it to show up as a test failure rather than
an uncaught exception from a constructor (GoogleTest can't safely report
constructor/destructor failures the same way).

---

## 5. Parameterized tests: `TEST_P`

Run the *same* test body against many inputs without copy-pasting:

```cpp
class GateTruthTableTest : public ::testing::TestWithParam<GateType> {};

TEST_P(GateTruthTableTest, MatchesExpected) {
    GateType type = GetParam();
    Gate g(type);
    EXPECT_EQ(g.evaluate(true, true), expected(type, true, true));
}

INSTANTIATE_TEST_SUITE_P(
    AllGates, GateTruthTableTest,
    ::testing::Values(GateType::AND, GateType::OR, GateType::XOR)
);
```

This is the pattern our hand-written `expectTruthTable()` helper function
in `tests/test_gate.cpp` could be refactored into later — six `TEST()`
calls become one `TEST_P` + one `INSTANTIATE_TEST_SUITE_P` list. Each
instantiated case still shows up as its own named test
(`AllGates/GateTruthTableTest.MatchesExpected/0`, etc.) and its own CTest
entry.

---

## 6. How registration actually works (static initialization trick)

```cpp
TEST(GateTest, AND) { ... }
```

expands to roughly:

```cpp
class GateTest_AND_Test : public ::testing::Test {
    void TestBody() override { ... }
};

static ::testing::TestInfo* const dummy_AND =
    ::testing::internal::MakeAndRegisterTestInfo(
        "GateTest", "AND", /* ... */,
        []{ return new GateTest_AND_Test; });
```

`dummy_AND` is a namespace-scope `static`, so the C++ standard guarantees
its initializer runs during **static initialization** — before `main()`
is called. That initializer call (`MakeAndRegisterTestInfo`) is what adds
the test to the global registry. This is why you never see a "list of all
tests" anywhere in the source: each `TEST(...)` macro silently registers
itself the moment the translation unit's static initializers run, purely
by existing in a linked object file.

`RUN_ALL_TESTS()` (called by `main()` — see §7) then just walks that
registry, constructs each test object, and calls `TestBody()` on it.

---

## 7. `main()` and `gtest_main`

You almost never write your own `main()`. Link against `GTest::gtest_main`
(a tiny library containing just this):

```cpp
int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
```

`InitGoogleTest` parses GoogleTest's own command-line flags (`--gtest_*`,
see §8) out of `argv` before your test binary sees the rest. Write your
own `main()` only if you need custom setup (e.g. initializing some global
resource) before tests run — call `InitGoogleTest`/`RUN_ALL_TESTS`
yourself in that case, and link `GTest::gtest` instead of `gtest_main`.

---

## 8. Useful command-line flags

Run any GoogleTest binary directly (`./eda_tests.exe <flag>`):

| Flag | Effect |
|---|---|
| `--gtest_list_tests` | print every registered test name, run nothing |
| `--gtest_filter=Suite.Name` | run only matching tests (supports `*` wildcards, `-` to exclude: `--gtest_filter=-Suite.Slow*`) |
| `--gtest_repeat=N` | run the whole suite N times (good for flaky-test hunting) |
| `--gtest_shuffle` | randomize test order (catches hidden inter-test dependencies) |
| `--gtest_output=xml:report.xml` | write JUnit-style XML (for CI dashboards) |
| `--gtest_break_on_failure` | drop into the debugger at the first failure instead of continuing |
| `--gtest_also_run_disabled_tests` | run tests named `DISABLED_*` (normally skipped) |

---

## 9. CMake integration

Two ways to bring GoogleTest into a CMake project:

**A. System/package-manager install, `find_package` (what this project
uses):**
```cmake
find_package(GTest CONFIG REQUIRED)
add_executable(my_tests test_foo.cpp)
target_link_libraries(my_tests PRIVATE GTest::gtest_main)
include(GoogleTest)
gtest_discover_tests(my_tests)
```
Fast (no download/rebuild of gtest itself), but requires it installed on
every machine that builds the project (here: `pacman -S
mingw-w64-ucrt-x86_64-gtest`).

**B. `FetchContent` (downloads + builds gtest as part of your build):**
```cmake
include(FetchContent)
FetchContent_Declare(googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
FetchContent_MakeAvailable(googletest)
target_link_libraries(my_tests PRIVATE gtest_main)
```
No separate install step, reproducible across machines, but needs network
access on first configure and adds gtest to your own build time.

**`add_test()` vs `gtest_discover_tests()`:** plain `add_test(NAME x
COMMAND my_tests)` registers the *whole binary* as one CTest entry — pass
only if every `TEST()` inside passes. `gtest_discover_tests` runs
`--gtest_list_tests` at build time and registers **each** `TEST()`
individually, so a CI dashboard shows exactly which one broke.

---

## 10. Best practices

- **One logical thing per test.** Several `EXPECT_*` calls checking
  facets of the *same* behavior is fine; testing two unrelated behaviors
  in one `TEST()` makes failures harder to diagnose.
- **Tests must not depend on execution order or on each other's state.**
  GoogleTest doesn't guarantee order by default (and `--gtest_shuffle`
  actively randomizes it) — shared mutable global state between tests is
  a common source of flaky failures.
- **Prefer `EXPECT_*` over `ASSERT_*`** unless a later line would be
  meaningless or crash after a failure — you want to see all failures in
  one run, not just the first.
- **Make test names describe behavior, not implementation**, e.g.
  `HandlesEmptyInput` rather than `Test1`.
- **Recompute expected values independently of the code under test** when
  feasible — a test that calls the same logic it's checking (or a
  refactored copy of it) can't catch a shared bug in that logic. (This is
  exactly what `expected()` does in `tests/test_gate.cpp` — it reimplements
  each truth table differently from `Gate.cpp`.)
- **Keep fixtures lean.** `SetUp()` reruns for *every* test in the suite —
  expensive setup there slows the whole suite down linearly.

---

## 11. Quick comparison with other C++ test frameworks

| | GoogleTest | Catch2 | doctest |
|---|---|---|---|
| Style | macro-based (`TEST`, `EXPECT_EQ`) | macro-based, more "natural language" (`REQUIRE`, `SECTION`) | header-only, very similar to Catch2 |
| Setup | separate library, needs linking | header-only (single include) | header-only, extremely fast to compile |
| Mocking | gMock ships alongside it | none built-in | none built-in |
| Common use | Google-ecosystem projects, large codebases, CI-heavy | smaller projects, fast iteration | projects wanting near-zero compile overhead |

---

## 12. Interview-style Q&A

**Q: What's the difference between `EXPECT_EQ` and `ASSERT_EQ`?**
A: Both check equality and record a failure on mismatch. `EXPECT_EQ`
lets the test continue running afterward; `ASSERT_EQ` returns
immediately from the enclosing function, so later lines in that test
don't execute. `ASSERT_*` only works in `void`-returning functions
because it expands to a bare `return;`.

**Q: How does GoogleTest know which tests exist without a registration
list?**
A: Each `TEST(...)` macro expands to a test class plus a
namespace-scope `static` variable whose initializer registers that test
with a global registry. Static initializers run before `main()`, so by
the time `RUN_ALL_TESTS()` executes, every `TEST()` that was compiled
into the binary has already added itself.

**Q: Why would `EXPECT_EQ(a, b)` fail for two C-strings that look
identical?**
A: If `a`/`b` are `const char*`, `EXPECT_EQ` compares pointer values via
`operator==`, not the characters they point to. Two separately-allocated
but textually identical C-strings have different addresses. Use
`EXPECT_STREQ` for content comparison, or compare `std::string`s instead.

**Q: What's a test fixture and when do you need one?**
A: A class derived from `::testing::Test` (used via `TEST_F`) that
provides shared `SetUp()`/`TearDown()` logic, re-run fresh before/after
*each* test using it. Use one when multiple tests need the same non-trivial
setup (e.g. constructing a populated data structure) and you want that
setup isolated per-test rather than copy-pasted or shared/mutated across
tests.

**Q: How do you run just one test out of a large suite?**
A: `./binary --gtest_filter=SuiteName.TestName` (supports `*` wildcards
and `-` to exclude patterns).

**Q: How would you test that a function throws a specific exception?**
A: `EXPECT_THROW(someCall(), std::invalid_argument);` — fails if it
throws a different type, or doesn't throw at all.

**Q: What's the difference between `add_test()` and
`gtest_discover_tests()` in CMake?**
A: `add_test()` registers one CTest entry for the whole test binary —
CTest only knows "pass" or "fail" for the entire executable.
`gtest_discover_tests()` runs the binary with `--gtest_list_tests` at
build time and registers each individual `TEST()`/`TEST_P()` case as its
own CTest entry, so failures are attributable to a specific test by name.

**Q: Why prefer `EXPECT_*` over `ASSERT_*` as a default?**
A: `EXPECT_*` keeps running after a failure, so a single test run
surfaces every problem instead of stopping at the first one — faster
feedback loop when several things are broken at once. Reach for
`ASSERT_*` specifically when continuing would be meaningless or unsafe
(e.g., the next line dereferences a pointer that was just asserted
non-null).

---

## See also

- `docs/phases/phase1.md` — how this project actually wires GoogleTest in
  (`tests/test_gate.cpp`, `tests/CMakeLists.txt`, `gtest_discover_tests`).
- `docs/DECISIONS.md`, 2026-10-18 "GoogleTest brought forward to Phase 1"
  — why it arrived this early instead of waiting for Phase 11.
