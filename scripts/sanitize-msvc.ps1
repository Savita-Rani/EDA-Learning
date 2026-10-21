# scripts/sanitize-msvc.ps1 — Phase 11: AddressSanitizer via MSVC.
#
# This project's PRIMARY toolchain (MinGW/ucrt64 g++) has no
# libasan/libubsan at all (confirmed directly — see
# docs/phases/phase11.md). MSVC ships a working AddressSanitizer
# (VS2019 16.9+) — this script configures, builds, and runs the test
# suite under it, using a SEPARATE build directory and SKIPPING
# frontend/ (GLFW/GoogleTest installed via pacman are MinGW-ABI only,
# not MSVC-compatible; GoogleTest is fetched fresh from source for
# this path instead — see tests/CMakeLists.txt).
#
# Two environment quirks this script works around (both discovered by
# actually running this, not anticipated in advance):
#   1. The ASan runtime (clang_rt.asan_dynamic-x86_64.dll) ships next
#      to cl.exe but isn't on PATH by default — instrumented binaries
#      fail to even start (STATUS_DLL_NOT_FOUND) without it.
#   2. In this specific environment, ASan's own Windows API
#      interception (how it hooks malloc/etc. to track allocations)
#      hits an "Interception failure" and aborts immediately — almost
#      certainly some other process-level hooking already present in
#      this sandboxed environment conflicting with ASan's own hooks.
#      ASAN_WIN_CONTINUE_ON_INTERCEPTION_FAILURE=1 (suggested by ASan's
#      own error message) lets it proceed regardless; every one of this
#      project's 90 tests then passes normally, and a deliberately
#      broken standalone repro was confirmed to still be caught
#      correctly (see docs/phases/phase11.md for the full captured
#      report) — so the one failed hook doesn't appear to be hiding
#      real detections.
#
# Usage: powershell -File scripts/sanitize-msvc.ps1

$ErrorActionPreference = "Stop"

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) {
    Write-Host "vswhere.exe not found - no Visual Studio installer present." -ForegroundColor Red
    exit 1
}

$vsInstallPath = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsInstallPath) {
    Write-Host "No Visual Studio installation with the C++ (VC.Tools.x86.x64) workload found." -ForegroundColor Red
    Write-Host "Install it via the Visual Studio Installer, or use the ubuntu-latest CI job instead (see .github/workflows/ci.yml)."
    exit 1
}

$msvcVersionDirs = Get-ChildItem -Path "$vsInstallPath\VC\Tools\MSVC" -Directory | Sort-Object Name -Descending
if ($msvcVersionDirs.Count -eq 0) {
    Write-Host "No MSVC toolset found under '$vsInstallPath\VC\Tools\MSVC'." -ForegroundColor Red
    exit 1
}
$asanRuntimeDir = Join-Path $msvcVersionDirs[0].FullName "bin\Hostx64\x64"

Write-Host "Using MSVC toolset: $($msvcVersionDirs[0].Name)" -ForegroundColor Cyan
Write-Host "ASan runtime directory: $asanRuntimeDir" -ForegroundColor Cyan

$env:PATH = "$asanRuntimeDir;" + $env:PATH
$env:ASAN_WIN_CONTINUE_ON_INTERCEPTION_FAILURE = "1"

$buildDir = "build-msvc-sanitize"

Write-Host "=== Configure (Visual Studio 17 2022, x64, AddressSanitizer) ===" -ForegroundColor Cyan
cmake -S . -B $buildDir -G "Visual Studio 17 2022" -A x64 -DEDA_ENABLE_SANITIZERS=ON
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "=== Build ===" -ForegroundColor Cyan
cmake --build $buildDir --config Debug
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "=== Test (under AddressSanitizer) ===" -ForegroundColor Cyan
Push-Location $buildDir
ctest -C Debug --output-on-failure
$testExitCode = $LASTEXITCODE
Pop-Location

if ($testExitCode -ne 0) {
    Write-Host "=== ASan run FAILED ===" -ForegroundColor Red
    exit $testExitCode
}

Write-Host "=== ASan run PASSED (90/90 as of Phase 11) ===" -ForegroundColor Green
exit 0
