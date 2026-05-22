param(
    [string]$Version = "v1.0.0",
    [string]$Configuration = "Release",
    [string]$BuildDir = "build_oop_current",
    [string]$OSGeo4WRoot = "D:/OSGeo4W",
    [string]$Libxml2Root = "D:/OOP_2026/libxml2"
)

$ErrorActionPreference = "Stop"

function Resolve-FullPath {
    param([string]$Path)
    return [System.IO.Path]::GetFullPath((Join-Path (Get-Location) $Path))
}

function Assert-Exists {
    param(
        [string]$Path,
        [string]$Label
    )
    if (-not (Test-Path -LiteralPath $Path)) {
        throw "$Label not found: $Path"
    }
}

function Copy-DirectoryClean {
    param(
        [string]$Source,
        [string]$Destination
    )
    Assert-Exists $Source "Source directory"
    if (Test-Path -LiteralPath $Destination) {
        Remove-Item -LiteralPath $Destination -Recurse -Force
    }
    New-Item -ItemType Directory -Path (Split-Path -Parent $Destination) -Force | Out-Null
    Copy-Item -LiteralPath $Source -Destination $Destination -Recurse -Force
}

function Copy-FileSet {
    param(
        [string]$SourceDir,
        [string]$Filter,
        [string]$DestinationDir
    )
    Assert-Exists $SourceDir "Source directory"
    New-Item -ItemType Directory -Path $DestinationDir -Force | Out-Null
    Get-ChildItem -LiteralPath $SourceDir -Filter $Filter -File | ForEach-Object {
        Copy-Item -LiteralPath $_.FullName -Destination (Join-Path $DestinationDir $_.Name) -Force
    }
}

$repoRoot = (Get-Location).Path
$buildRoot = Resolve-FullPath $BuildDir
$exePath = Join-Path $buildRoot "$Configuration/FirstQT.exe"
$distRoot = Join-Path $repoRoot "dist"
$packageName = "GeoAI-$Version-win64"
$packageDir = Join-Path $distRoot $packageName
$zipPath = Join-Path $distRoot "$packageName.zip"

$osgeoRootFull = [System.IO.Path]::GetFullPath($OSGeo4WRoot)
$libxmlRootFull = [System.IO.Path]::GetFullPath($Libxml2Root)
$qtRoot = Join-Path $osgeoRootFull "apps/Qt5"
$qgisRoot = Join-Path $osgeoRootFull "apps/qgis-ltr"
$gdalRoot = Join-Path $osgeoRootFull "apps/gdal"
$windeployqt = Join-Path $qtRoot "bin/windeployqt.exe"

Assert-Exists $exePath "Built executable"
Assert-Exists $windeployqt "windeployqt"
Assert-Exists $qgisRoot "QGIS runtime"
Assert-Exists $gdalRoot "GDAL runtime"
Assert-Exists (Join-Path $osgeoRootFull "share/proj") "PROJ data"
Assert-Exists (Join-Path $libxmlRootFull "bin") "libxml2 bin"

if (Test-Path -LiteralPath $packageDir) {
    Remove-Item -LiteralPath $packageDir -Recurse -Force
}
New-Item -ItemType Directory -Path $packageDir -Force | Out-Null

Copy-Item -LiteralPath $exePath -Destination (Join-Path $packageDir "FirstQT.exe") -Force
Copy-DirectoryClean (Join-Path $repoRoot "resources/icons") (Join-Path $packageDir "resources/icons")

& $windeployqt --release --compiler-runtime --verbose 1 (Join-Path $packageDir "FirstQT.exe")
if ($LASTEXITCODE -ne 0) {
    throw "windeployqt failed with exit code $LASTEXITCODE"
}

Copy-FileSet (Join-Path $osgeoRootFull "bin") "*.dll" $packageDir
Copy-FileSet (Join-Path $qgisRoot "bin") "qgis*.dll" $packageDir
Copy-FileSet (Join-Path $libxmlRootFull "bin") "*.dll" $packageDir

Copy-DirectoryClean (Join-Path $qgisRoot "plugins") (Join-Path $packageDir "apps/qgis-ltr/plugins")
Copy-DirectoryClean (Join-Path $qgisRoot "resources") (Join-Path $packageDir "apps/qgis-ltr/resources")
Copy-DirectoryClean (Join-Path $qgisRoot "svg") (Join-Path $packageDir "apps/qgis-ltr/svg")
Copy-DirectoryClean (Join-Path $qgisRoot "i18n") (Join-Path $packageDir "apps/qgis-ltr/i18n")
Copy-DirectoryClean (Join-Path $qgisRoot "qtplugins") (Join-Path $packageDir "apps/qgis-ltr/qtplugins")
Copy-DirectoryClean (Join-Path $qtRoot "plugins") (Join-Path $packageDir "apps/Qt5/plugins")
Copy-DirectoryClean (Join-Path $gdalRoot "share/gdal") (Join-Path $packageDir "apps/gdal/share/gdal")
Copy-DirectoryClean (Join-Path $gdalRoot "lib/gdalplugins") (Join-Path $packageDir "apps/gdal/lib/gdalplugins")
Copy-DirectoryClean (Join-Path $osgeoRootFull "share/proj") (Join-Path $packageDir "share/proj")

$certFile = Join-Path $osgeoRootFull "bin/curl-ca-bundle.crt"
if (Test-Path -LiteralPath $certFile) {
    New-Item -ItemType Directory -Path (Join-Path $packageDir "bin") -Force | Out-Null
    Copy-Item -LiteralPath $certFile -Destination (Join-Path $packageDir "bin/curl-ca-bundle.crt") -Force
}

if (Test-Path -LiteralPath $zipPath) {
    Remove-Item -LiteralPath $zipPath -Force
}
Compress-Archive -LiteralPath $packageDir -DestinationPath $zipPath -CompressionLevel Optimal

Write-Host "Package directory: $packageDir"
Write-Host "Package zip: $zipPath"
