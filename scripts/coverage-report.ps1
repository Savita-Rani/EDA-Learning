# scripts/coverage-report.ps1 — Phase 11: summarizes gcov line coverage
# for backend/src/*.cpp ONLY (not system headers, not vendored ImGui,
# not GoogleTest itself) from an already-built, already-tested
# build-coverage/ directory.
#
# Usage (after `cmake -S . -B build-coverage -DEDA_ENABLE_COVERAGE=ON`,
# building, and running ctest there — or just: .\scripts\ci.ps1 -Coverage):
#   powershell -File scripts/coverage-report.ps1

$ErrorActionPreference = "Stop"
$buildDir = "build-coverage"

if (-not (Test-Path $buildDir)) {
    Write-Host "No '$buildDir' directory found. Run first:" -ForegroundColor Red
    Write-Host "  cmake -S . -B $buildDir -G `"MinGW Makefiles`" -DEDA_ENABLE_COVERAGE=ON"
    Write-Host "  cmake --build $buildDir"
    Write-Host "  cd $buildDir; ctest; cd .."
    exit 1
}

$gcdaFiles = Get-ChildItem -Path $buildDir -Recurse -Filter "*.gcda" |
    Where-Object { $_.FullName -match "[\\/]backend[\\/]" }

$totalLines = 0
$totalExecuted = 0
$rows = @()

foreach ($gcda in $gcdaFiles) {
    Push-Location $gcda.DirectoryName
    $output = & gcov $gcda.Name 2>&1 | Out-String
    Pop-Location

    $lines = $output -split "`r?`n"
    for ($i = 0; $i -lt $lines.Length; $i++) {
        if ($lines[$i] -match "backend[\\/]src[\\/]([A-Za-z0-9_]+\.cpp)'") {
            $fileName = $Matches[1]
            if ($i + 1 -lt $lines.Length -and $lines[$i + 1] -match "Lines executed:([\d.]+)% of (\d+)") {
                $pct = [double]$Matches[1]
                $count = [int]$Matches[2]
                $executed = [math]::Round($count * $pct / 100.0)
                $totalLines += $count
                $totalExecuted += $executed
                $rows += [PSCustomObject]@{ File = $fileName; Percent = $pct; Lines = $count }
            }
        }
    }
}

if ($rows.Count -eq 0) {
    Write-Host "No backend/src/*.cpp coverage data found under '$buildDir'." -ForegroundColor Red
    exit 1
}

$rows | Sort-Object Percent | Format-Table -AutoSize

$aggregate = [math]::Round(100.0 * $totalExecuted / $totalLines, 2)
Write-Host "Aggregate backend/src/*.cpp line coverage: $aggregate% ($totalExecuted / $totalLines lines)" -ForegroundColor Green
