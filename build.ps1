# Build the genisis project.
# Run this from Developer PowerShell for VS.

param(
    [switch]$Clean,
    [switch]$OpenUf2
)

$projectRoot = "C:\Users\OwenS\OneDrive\Documents\Programs\NCSU ASME SDC\ASME_SDC_2526_C++\genisis"
$buildDir = Join-Path $projectRoot "build"
$ninjaPath = "C:\Users\OwenS\.pico-sdk\ninja\v1.12.1\ninja.exe"
$uf2Path = Join-Path $buildDir "genisis.uf2"

$env:PICO_SDK_PATH = "$HOME\.pico-sdk\sdk\2.2.0"
$env:PICO_TOOLCHAIN_PATH = "$HOME\.pico-sdk\toolchain\14_2_Rel1"

# Make sure this is being run from a VS developer shell.
if (-not (Get-Command cl -ErrorAction SilentlyContinue)) {
    Write-Error "cl not found. Run this from Developer PowerShell for VS."
    exit 1
}

Push-Location $projectRoot

try {
    # Optional full clean rebuild.
    if ($Clean -and (Test-Path $buildDir)) {
        Write-Host "Cleaning build folder..."
        Remove-Item -Recurse -Force $buildDir
    }

    # Create build folder if needed.
    if (-not (Test-Path $buildDir)) {
        New-Item -ItemType Directory -Path $buildDir | Out-Null
    }

    Set-Location $buildDir

    # Configure only if this build folder has not been configured yet.
    if (-not (Test-Path "build.ninja")) {
        Write-Host "Configuring project..."
        cmake -G Ninja -DCMAKE_MAKE_PROGRAM="$ninjaPath" ..
        if ($LASTEXITCODE -ne 0) {
            exit $LASTEXITCODE
        }
    }

    Write-Host "Building project..."
    cmake --build . -j
    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }

    if (Test-Path $uf2Path) {
        Write-Host ""
        Write-Host "Build succeeded."
        Write-Host "UF2 file: $uf2Path"

        if ($OpenUf2) {
            Start-Process explorer.exe "/select,`"$uf2Path`""
        }
    }
    else {
        Write-Warning "Build finished, but genisis.uf2 was not found."
    }
}
finally {
    Pop-Location
}