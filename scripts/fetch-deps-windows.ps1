# Downloads prebuilt libmpv and FFmpeg for Windows into deps/ and creates the MSVC import library for libmpv.
# Run from a "Developer PowerShell for VS 2022" (or any shell where lib.exe and dumpbin.exe are on PATH),
# or let the script locate them through vswhere.
#
# The builds are pinned by URL and SHA-256. The upstream projects delete old builds after a while; to update,
# pick new assets from https://github.com/shinchiro/mpv-winbuild-cmake/releases and
# https://github.com/BtbN/FFmpeg-Builds/releases and update the constants below.

param(
    [string]$DepsDir = (Join-Path $PSScriptRoot "..\deps")
)

$ErrorActionPreference = "Stop"

$MpvUrl = "https://github.com/shinchiro/mpv-winbuild-cmake/releases/download/20261014/mpv-dev-x86_64-20261014-git-ec38361c56.7z"
$MpvSha256 = "325555b4e0b1d87fb7c92c96010776581599c64e4988052cb9d05e130b52f939"
$FfmpegUrl = "https://github.com/BtbN/FFmpeg-Builds/releases/download/autobuild-2026-10-14-13-03/ffmpeg-n8.1.3-18-g46d899d7a0-win64-lgpl-shared-8.1.zip"
$FfmpegSha256 = "38dd3fb973c2e10723489e2173135d60f569953e70a04b885446175f9e9380d6"

function Get-VerifiedDownload([string]$Url, [string]$Sha256, [string]$DownloadDir) {
    $path = Join-Path $DownloadDir ([System.IO.Path]::GetFileName($Url))

    if (-not (Test-Path $path) -or (Get-FileHash $path -Algorithm SHA256).Hash -ne $Sha256) {
        Write-Host "Downloading $Url"
        & curl.exe -sfL -o $path $Url

        if ($LASTEXITCODE -ne 0) {
            throw "Download failed: $Url"
        }
    }

    $actual = (Get-FileHash $path -Algorithm SHA256).Hash

    if ($actual -ne $Sha256) {
        throw "SHA-256 mismatch for ${path}: $actual"
    }

    return $path
}

function Find-MsvcTool([string]$Name) {
    $command = Get-Command $Name -ErrorAction SilentlyContinue

    if ($command) {
        return $command.Source
    }

    $vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
    $tool = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 `
        -find "VC\Tools\MSVC\*\bin\Hostx64\x64\$Name" | Select-Object -First 1

    if (-not $tool) {
        throw "$Name not found: install Visual Studio 2022 Build Tools with the C++ workload"
    }

    return $tool
}

# 7-Zip when installed (the tar of Windows Server 2022 cannot read 7z archives), otherwise Windows' bsdtar,
# which reads both 7z and zip on Windows 11.
function Expand-ArchiveTo([string]$Archive, [string]$Destination) {
    $sevenZip = Get-Command 7z.exe -ErrorAction SilentlyContinue

    if (-not $sevenZip -and (Test-Path "$env:ProgramFiles\7-Zip\7z.exe")) {
        $sevenZip = Get-Command "$env:ProgramFiles\7-Zip\7z.exe"
    }

    if ($sevenZip) {
        & $sevenZip.Source x -y "-o$Destination" $Archive | Out-Null
    } else {
        & (Join-Path $env:SystemRoot "System32\tar.exe") -xf $Archive -C $Destination
    }

    if ($LASTEXITCODE -ne 0) {
        throw "Extraction failed: $Archive"
    }
}

$DepsDir = [System.IO.Path]::GetFullPath($DepsDir)
$downloadDir = Join-Path $DepsDir "downloads"
$mpvDir = Join-Path $DepsDir "mpv"
$ffmpegDir = Join-Path $DepsDir "ffmpeg"
New-Item -ItemType Directory -Force $downloadDir | Out-Null

$mpvArchive = Get-VerifiedDownload $MpvUrl $MpvSha256 $downloadDir
$ffmpegArchive = Get-VerifiedDownload $FfmpegUrl $FfmpegSha256 $downloadDir

Remove-Item -Recurse -Force $mpvDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $mpvDir | Out-Null
Expand-ArchiveTo $mpvArchive $mpvDir

Remove-Item -Recurse -Force $ffmpegDir -ErrorAction SilentlyContinue
$ffmpegExtractDir = Join-Path $downloadDir "ffmpeg-extract"
Remove-Item -Recurse -Force $ffmpegExtractDir -ErrorAction SilentlyContinue
New-Item -ItemType Directory -Force $ffmpegExtractDir | Out-Null
Expand-ArchiveTo $ffmpegArchive $ffmpegExtractDir
Move-Item (Get-ChildItem $ffmpegExtractDir -Directory | Select-Object -First 1).FullName $ffmpegDir
Remove-Item -Recurse -Force $ffmpegExtractDir

# libmpv ships only a MinGW import library: build mpv.lib for MSVC from the DLL's export table
$dumpbin = Find-MsvcTool "dumpbin.exe"
$lib = Find-MsvcTool "lib.exe"
$dll = Get-ChildItem $mpvDir -Filter "libmpv-*.dll" | Select-Object -First 1

if (-not $dll) {
    throw "libmpv-*.dll not found in $mpvDir"
}

$exports = & $dumpbin /exports $dll.FullName |
    Where-Object { $_ -match "^\s+\d+\s+[0-9A-F]+\s+[0-9A-F]{8}\s+(\S+)" } |
    ForEach-Object { $Matches[1] }
$definitionPath = Join-Path $mpvDir "mpv.def"
$libraryPath = Join-Path $mpvDir "mpv.lib"
$definition = @("LIBRARY $($dll.Name)", "EXPORTS") + $exports
Set-Content -Path $definitionPath -Value $definition -Encoding ascii
& $lib /nologo "/def:$definitionPath" /machine:x64 "/out:$libraryPath" | Out-Null

if ($LASTEXITCODE -ne 0) {
    throw "lib.exe failed"
}

Write-Host "Dependencies are ready in $DepsDir"
