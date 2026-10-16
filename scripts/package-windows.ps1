# Builds the portable Windows release: dist/af-video-player-<version>-windows-x64.zip (P-10).
# With -Local: the developer's local build in dist/windows/af-video-player, without a version and an archive.
# Requires: Visual Studio 2022 Build Tools, CMake, Qt (QT_ROOT_DIR), deps/ from fetch-deps-windows.ps1.

param(
    [string]$Version,
    [switch]$Local
)

$ErrorActionPreference = "Stop"

if (-not $Local -and -not $Version) {
    throw "Pass -Version for a release package or -Local for a local build"
}

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$buildDir = Join-Path $root "build\package-windows"

if ($Local) {
    $stageDir = Join-Path $root "dist\windows\af-video-player"
} else {
    $name = "af-video-player-$Version-windows-x64"
    $stageDir = Join-Path $root "dist\$name"
    $archive = Join-Path $root "dist\$name.zip"
}

function Invoke-Checked([scriptblock]$Command) {
    & $Command

    if ($LASTEXITCODE -ne 0) {
        throw "Command failed: $Command"
    }
}

Invoke-Checked { cmake -S $root -B $buildDir -G "Visual Studio 17 2022" -A x64 "-DCMAKE_PREFIX_PATH=$env:QT_ROOT_DIR" }
Invoke-Checked { cmake --build $buildDir --config Release --target af-video-player }

# A previous build that cannot be removed (the player is running) stops the script.
if (Test-Path $stageDir) {
    Remove-Item -Recurse -Force $stageDir
}

Invoke-Checked { cmake --install $buildDir --config Release --prefix $stageDir }

if ($Local) {
    Write-Host "Created $stageDir"

    return
}

Remove-Item -Force $archive -ErrorAction SilentlyContinue
Compress-Archive -Path $stageDir -DestinationPath $archive

Write-Host "Created $archive"
