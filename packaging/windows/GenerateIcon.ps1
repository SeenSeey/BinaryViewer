[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string] $OutputPath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

Add-Type -AssemblyName System.Drawing
Add-Type @'
using System;
using System.Runtime.InteropServices;

public static class BinaryViewerNativeIcon
{
    [DllImport("user32.dll", CharSet = CharSet.Auto)]
    public static extern bool DestroyIcon(IntPtr handle);
}
'@

$fullOutputPath = [System.IO.Path]::GetFullPath($OutputPath)
$outputDirectory = [System.IO.Path]::GetDirectoryName($fullOutputPath)
[System.IO.Directory]::CreateDirectory($outputDirectory) | Out-Null

$bitmap = $null
$graphics = $null
$font = $null
$brush = $null
$format = $null
$icon = $null
$stream = $null
$iconHandle = [IntPtr]::Zero

try {
    $size = 256
    $bitmap = [System.Drawing.Bitmap]::new(
        $size,
        $size,
        [System.Drawing.Imaging.PixelFormat]::Format32bppArgb
    )
    $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
    $graphics.Clear([System.Drawing.Color]::Transparent)
    $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::AntiAlias
    $graphics.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::AntiAliasGridFit

    # This is the same U+2723 symbol shown beside "Binary Viewer" in the main UI.
    $font = [System.Drawing.Font]::new(
        'Segoe UI Symbol',
        174,
        [System.Drawing.FontStyle]::Regular,
        [System.Drawing.GraphicsUnit]::Pixel
    )
    $brush = [System.Drawing.SolidBrush]::new(
        [System.Drawing.Color]::FromArgb(255, 20, 20, 19)
    )
    $format = [System.Drawing.StringFormat]::new()
    $format.Alignment = [System.Drawing.StringAlignment]::Center
    $format.LineAlignment = [System.Drawing.StringAlignment]::Center

    $bounds = [System.Drawing.RectangleF]::new(0, -8, $size, $size)
    $graphics.DrawString([char] 0x2723, $font, $brush, $bounds, $format)

    $iconHandle = $bitmap.GetHicon()
    $icon = [System.Drawing.Icon]::FromHandle($iconHandle)
    $stream = [System.IO.File]::Create($fullOutputPath)
    $icon.Save($stream)
}
finally {
    if ($null -ne $stream) { $stream.Dispose() }
    if ($null -ne $icon) { $icon.Dispose() }
    if ($iconHandle -ne [IntPtr]::Zero) {
        [BinaryViewerNativeIcon]::DestroyIcon($iconHandle) | Out-Null
    }
    if ($null -ne $format) { $format.Dispose() }
    if ($null -ne $brush) { $brush.Dispose() }
    if ($null -ne $font) { $font.Dispose() }
    if ($null -ne $graphics) { $graphics.Dispose() }
    if ($null -ne $bitmap) { $bitmap.Dispose() }
}

if (-not (Test-Path -LiteralPath $fullOutputPath -PathType Leaf) -or
    (Get-Item -LiteralPath $fullOutputPath).Length -eq 0) {
    throw "The Windows icon was not created: $fullOutputPath"
}
