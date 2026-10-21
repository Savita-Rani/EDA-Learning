# scripts/ci.ps1 — Phase 11: one-command local CI pipeline.
#
# Configure -> build -> test, with the exit code propagating properly
# (0 = everything passed, nonzero = something failed) so this is usable
# as an actual gate, not just a human-readable report. This is the
# script .github/workflows/ci.yml's Windows job runs — kept as a
# standalone script (not inlined into the workflow YAML) specifically
# so it also works for a plain local run, with no GitHub Actions
# involved at all.
#
# Usage:
#   powershell -File scripts/ci.ps1
#   powershell -File scripts/ci.ps1 -Coverage     # also runs a coverage build/report
#
# Does NOT attempt a sanitizer build here — on this project's MinGW/
# ucrt64 toolchain, -fsanitize=address,undefined fails to link
# (confirmed directly; see docs/phases/phase11.md). The CI workflow's
# Linux job covers that instead, where GCC's sanitizer runtimes are
# actually available.

param(
    [switch]$Coverage
)

$ErrorActionPreference = "Stop"

function Invoke-Step {
    param([string]$Name, [scriptblock]$Action)
    Write-Host "=== $Name ===" -ForegroundColor Cyan
    & $Action
    if ($LASTEXITCODE -ne 0) {
        Write-Host "=== CI FAILED at: $Name ===" -ForegroundColor Red
        exit $LASTEXITCODE
    }
}

Invoke-Step "Configure" { cmake -S . -B build -G "MinGW Makefiles" }
Invoke-Step "Build" { cmake --build build }

Write-Host "=== Test ===" -ForegroundColor Cyan
Push-Location build
ctest --output-on-failure
$testExitCode = $LASTEXITCODE
Pop-Location

if ($testExitCode -ne 0) {
    Write-Host "=== CI FAILED at: Test ===" -ForegroundColor Red
    exit $testExitCode
}

if ($Coverage) {
    Invoke-Step "Coverage: configure" { cmake -S . -B build-coverage -G "MinGW Makefiles" -DEDA_ENABLE_COVERAGE=ON }
    Invoke-Step "Coverage: build" { cmake --build build-coverage }

    Write-Host "=== Coverage: run tests ===" -ForegroundColor Cyan
    Push-Location build-coverage
    ctest
    $coverageTestExitCode = $LASTEXITCODE
    Pop-Location

    if ($coverageTestExitCode -ne 0) {
        Write-Host "=== CI FAILED at: Coverage test run ===" -ForegroundColor Red
        exit $coverageTestExitCode
    }

    Write-Host "=== Coverage: report (backend/src/*.cpp only) ===" -ForegroundColor Cyan
    & "$PSScriptRoot\coverage-report.ps1"
}

Write-Host "=== CI PASSED ===" -ForegroundColor Green
exit 0
