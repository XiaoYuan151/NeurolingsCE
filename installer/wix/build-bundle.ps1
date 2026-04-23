Param(
    [string]$PackageDir = "out/package",
    [string]$InstallerRoot = "out/installer",
    [string]$WixExe = "wix",
    [switch]$GenerateOnly
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version Latest

function Get-VersionValue {
    Param(
        [Parameter(Mandatory = $true)]
        [string]$FilePath,

        [Parameter(Mandatory = $true)]
        [string]$Key
    )

    $match = Select-String -Path $FilePath -Pattern "^$Key=(.+)$" | Select-Object -First 1
    if (-not $match) {
        throw "Could not find '$Key' in $FilePath."
    }

    return $match.Matches[0].Groups[1].Value.Trim()
}

function Escape-Xml {
    Param(
        [AllowNull()]
        [string]$Value
    )

    if ($null -eq $Value) {
        return ""
    }

    return [System.Security.SecurityElement]::Escape($Value)
}

function Resolve-ExistingCommand {
    Param(
        [Parameter(Mandatory = $true)]
        [string[]]$Candidates
    )

    foreach ($candidate in $Candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) {
            continue
        }

        $command = Get-Command $candidate -ErrorAction SilentlyContinue
        if ($command) {
            return $command.Source
        }

        if (Test-Path -LiteralPath $candidate) {
            return [System.IO.Path]::GetFullPath($candidate)
        }
    }

    return $null
}

function Resolve-PackageDirectory {
    Param(
        [Parameter(Mandatory = $true)]
        [string]$InputPath,

        [Parameter(Mandatory = $true)]
        [string]$ExecutableName
    )

    $fullInputPath = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $InputPath))
    if (-not (Test-Path -LiteralPath $fullInputPath)) {
        throw "Package path not found: $fullInputPath"
    }

    $inputItem = Get-Item -LiteralPath $fullInputPath
    if (-not $inputItem.PSIsContainer) {
        throw "Package path must be a directory: $fullInputPath"
    }

    $candidateExe = Join-Path $fullInputPath "$ExecutableName.exe"
    if (Test-Path -LiteralPath $candidateExe) {
        return $fullInputPath
    }

    $packageCandidates = Get-ChildItem -LiteralPath $fullInputPath -Directory |
        Where-Object { Test-Path -LiteralPath (Join-Path $_.FullName "$ExecutableName.exe") } |
        Sort-Object LastWriteTime -Descending

    if (-not $packageCandidates) {
        throw "No packaged application directory containing $ExecutableName.exe was found under $fullInputPath."
    }

    return $packageCandidates[0].FullName
}

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$versionFile = Join-Path $repoRoot "VERSION.txt"
$templatePath = Join-Path $PSScriptRoot "Bundle.v4.template.wxs"
$licenseRtfPath = Join-Path $PSScriptRoot "assets/license_en-US.rtf"
$iconPath = Join-Path $PSScriptRoot "assets/neurolingsce.ico"

if (-not (Test-Path -LiteralPath $versionFile)) {
    throw "VERSION.txt not found at $versionFile."
}

if (-not (Test-Path -LiteralPath $templatePath)) {
    throw "WiX bundle template not found at $templatePath."
}

if (-not (Test-Path -LiteralPath $licenseRtfPath)) {
    throw "Installer license RTF not found at $licenseRtfPath."
}

if (-not (Test-Path -LiteralPath $iconPath)) {
    throw "Installer icon not found at $iconPath."
}

$productName = Get-VersionValue -FilePath $versionFile -Key "APP_NAME"
$manufacturer = Get-VersionValue -FilePath $versionFile -Key "APP_COMPANY"
$appExecutable = Get-VersionValue -FilePath $versionFile -Key "APP_EXECUTABLE"
$productVersion = Get-VersionValue -FilePath $versionFile -Key "VERSION"

$resolvedPackageDir = Resolve-PackageDirectory -InputPath $PackageDir -ExecutableName $appExecutable
$packageName = Split-Path -Leaf $resolvedPackageDir

$resolvedInstallerRoot = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $InstallerRoot))
$generatedRoot = Join-Path $resolvedInstallerRoot "wix\$packageName"
$bundleSourcePath = Join-Path $generatedRoot "Bundle.generated.wxs"
$outputBundlePath = Join-Path $resolvedInstallerRoot "$packageName-setup.exe"
$msiPath = Join-Path $resolvedInstallerRoot "$packageName.msi"
$vcRedistPath = Join-Path $resolvedPackageDir "vc_redist.x64.exe"

if (-not (Test-Path -LiteralPath $msiPath)) {
    throw "MSI not found at $msiPath. Build the MSI first."
}

if (-not (Test-Path -LiteralPath $vcRedistPath)) {
    throw "vc_redist.x64.exe not found in packaged directory: $resolvedPackageDir"
}

$resolvedWixExe = Resolve-ExistingCommand -Candidates @(
    $WixExe,
    "C:\Program Files\WiX Toolset v7.0\bin\wix.exe"
)

if (-not $resolvedWixExe) {
    throw "WiX 7 CLI not found. Ensure 'wix.exe' is on PATH or pass -WixExe with the full path."
}

New-Item -ItemType Directory -Path $generatedRoot -Force | Out-Null
New-Item -ItemType Directory -Path $resolvedInstallerRoot -Force | Out-Null

$bundleTemplate = Get-Content -LiteralPath $templatePath -Raw
$replacementMap = @{
    "{{ProductName}}" = Escape-Xml $productName
    "{{Manufacturer}}" = Escape-Xml $manufacturer
    "{{Version}}" = Escape-Xml $productVersion
    "{{BundleUpgradeCode}}" = "30fe5c95-8d19-447c-8eef-d53a11da3d4e"
    "{{IconPath}}" = Escape-Xml $iconPath
    "{{LicenseRtfPath}}" = Escape-Xml $licenseRtfPath
    "{{VcRedistPath}}" = Escape-Xml $vcRedistPath
    "{{MsiPath}}" = Escape-Xml $msiPath
}

foreach ($entry in $replacementMap.GetEnumerator()) {
    $bundleTemplate = $bundleTemplate.Replace($entry.Key, $entry.Value)
}

Set-Content -LiteralPath $bundleSourcePath -Value $bundleTemplate -Encoding UTF8

Write-Host "Package source : $resolvedPackageDir"
Write-Host "MSI source     : $msiPath"
Write-Host "Bundle source  : $bundleSourcePath"
Write-Host "Bundle output  : $outputBundlePath"

if ($GenerateOnly) {
    Write-Host "GenerateOnly   : enabled (skipping wix build)"
    return
}

if (Test-Path -LiteralPath $outputBundlePath) {
    Remove-Item -LiteralPath $outputBundlePath -Force
}

& $resolvedWixExe build `
    $bundleSourcePath `
    -arch x64 `
    -ext WixToolset.BootstrapperApplications.wixext `
    -ext WixToolset.Util.wixext `
    -out $outputBundlePath

if ($LASTEXITCODE -ne 0) {
    throw "wix build failed with exit code $LASTEXITCODE."
}

Write-Host "Built bundle   : $outputBundlePath"
