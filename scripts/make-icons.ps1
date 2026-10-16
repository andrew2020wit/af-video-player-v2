# Generates the application icons from packaging/icons/af-video-player.svg (I-84):
# af-video-player.ico (Windows), af-video-player.icns (macOS) and png/ (the window icon on Linux).
# The SVG is rendered by Qt itself (scripts/make-icons.qml); the generated files are committed.
# Requires: Qt (QT_ROOT_DIR) with the Qt SVG module.

$ErrorActionPreference = "Stop"

$root = [System.IO.Path]::GetFullPath((Join-Path $PSScriptRoot ".."))
$iconDir = Join-Path $root "packaging\icons"
$svg = Join-Path $iconDir "af-video-player.svg"
$tempDir = Join-Path ([System.IO.Path]::GetTempPath()) "afvp-icons"

# Windows and Linux icons take the whole canvas; on macOS the icon body is about 80% of it with a margin.
$windowsSizes = 16, 20, 24, 32, 40, 48, 64, 256
$linuxSizes = 16, 24, 32, 48, 64, 128, 256
$macSizes = 16, 32, 64, 128, 256, 512, 1024
$macScale = 0.86

$jobs = @()
$jobs += $windowsSizes + $linuxSizes | Sort-Object -Unique | ForEach-Object { "full-${_}:${_}:1" }
$jobs += $macSizes | ForEach-Object { "mac-${_}:${_}:$macScale" }

if (Test-Path $tempDir) {
    Remove-Item -Recurse -Force $tempDir
}

New-Item -ItemType Directory $tempDir | Out-Null

# Device pixel ratio 1, so that a PNG of N pixels is the SVG rendered at N pixels.
$env:QT_ENABLE_HIGHDPI_SCALING = "0"
$qml = Join-Path $env:QT_ROOT_DIR "bin\qml.exe"
$process = Start-Process -FilePath $qml -Wait -PassThru -NoNewWindow `
    -ArgumentList (@("`"$PSScriptRoot\make-icons.qml`"", "--", "`"$($svg -replace '\\', '/')`"",
        "`"$($tempDir -replace '\\', '/')`"") + $jobs)

if ($process.ExitCode -ne 0) {
    throw "qml failed with exit code $($process.ExitCode)"
}

function Read-Png([string]$Name) {
    # The comma keeps PowerShell from unrolling the array into the pipeline.
    return , [System.IO.File]::ReadAllBytes((Join-Path $tempDir "$Name.png"))
}

function Get-BigEndian([uint32]$Value) {
    $bytes = [System.BitConverter]::GetBytes($Value)
    [System.Array]::Reverse($bytes)

    return , $bytes
}

# ICO with PNG images (Windows Vista and later).
$images = $windowsSizes | ForEach-Object { , (Read-Png "full-$_") }
$stream = New-Object System.IO.MemoryStream
$writer = New-Object System.IO.BinaryWriter $stream
$writer.Write([uint16]0)
$writer.Write([uint16]1)
$writer.Write([uint16]$windowsSizes.Count)
$offset = 6 + 16 * $windowsSizes.Count

for ($i = 0; $i -lt $windowsSizes.Count; $i++) {
    $size = $windowsSizes[$i] % 256
    $writer.Write([byte]$size)
    $writer.Write([byte]$size)
    $writer.Write([byte]0)
    $writer.Write([byte]0)
    $writer.Write([uint16]1)
    $writer.Write([uint16]32)
    $writer.Write([uint32]$images[$i].Length)
    $writer.Write([uint32]$offset)
    $offset += $images[$i].Length
}

$images | ForEach-Object { $writer.Write($_) }
$writer.Flush()
[System.IO.File]::WriteAllBytes((Join-Path $iconDir "af-video-player.ico"), $stream.ToArray())

# ICNS with PNG images: the type of each element and the pixel size of its image.
$icnsTypes = [ordered]@{
    "icp4" = 16; "ic11" = 32; "icp5" = 32; "ic12" = 64; "ic07" = 128;
    "ic13" = 256; "ic08" = 256; "ic14" = 512; "ic09" = 512; "ic10" = 1024
}
$stream = New-Object System.IO.MemoryStream
$writer = New-Object System.IO.BinaryWriter $stream
$writer.Write([System.Text.Encoding]::ASCII.GetBytes("icns"))
$writer.Write([byte[]](0, 0, 0, 0))

foreach ($type in $icnsTypes.Keys) {
    $png = Read-Png "mac-$($icnsTypes[$type])"
    $writer.Write([System.Text.Encoding]::ASCII.GetBytes($type))
    $writer.Write([byte[]](Get-BigEndian ($png.Length + 8)))
    $writer.Write($png)
}

$writer.Flush()
$bytes = $stream.ToArray()
[System.Array]::Copy([byte[]](Get-BigEndian $bytes.Length), 0, $bytes, 4, 4)
[System.IO.File]::WriteAllBytes((Join-Path $iconDir "af-video-player.icns"), $bytes)

# PNG for the window icon on Linux.
$pngDir = Join-Path $iconDir "png"

if (Test-Path $pngDir) {
    Remove-Item -Recurse -Force $pngDir
}

New-Item -ItemType Directory $pngDir | Out-Null
$linuxSizes | ForEach-Object { Copy-Item (Join-Path $tempDir "full-$_.png") (Join-Path $pngDir "$_.png") }

Remove-Item -Recurse -Force $tempDir
Write-Host "Created the icons in $iconDir"
