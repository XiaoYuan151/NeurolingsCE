Param(
    [string]$PackageDir = "out/package",
    [string]$OutputRoot = "out/installer",
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

function Get-RelativePath {
    Param(
        [Parameter(Mandatory = $true)]
        [string]$BasePath,

        [Parameter(Mandatory = $true)]
        [string]$Path
    )

    $baseWithSeparator = $BasePath.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
    if ($Path.StartsWith($baseWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        return $Path.Substring($baseWithSeparator.Length)
    }

    return $Path
}

function New-StableGuid {
    Param(
        [Parameter(Mandatory = $true)]
        [string]$Value
    )

    $md5 = [System.Security.Cryptography.MD5]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Value)
        $hash = $md5.ComputeHash($bytes)
    } finally {
        $md5.Dispose()
    }

    $hash[6] = ($hash[6] -band 0x0F) -bor 0x30
    $hash[8] = ($hash[8] -band 0x3F) -bor 0x80

    return [System.Guid]::new($hash).ToString()
}

function New-WixId {
    Param(
        [Parameter(Mandatory = $true)]
        [string]$Prefix,

        [Parameter(Mandatory = $true)]
        [string]$Value
    )

    $sha1 = [System.Security.Cryptography.SHA1]::Create()
    try {
        $bytes = [System.Text.Encoding]::UTF8.GetBytes($Value)
        $hash = $sha1.ComputeHash($bytes)
    } finally {
        $sha1.Dispose()
    }

    $hex = -join ($hash | ForEach-Object { $_.ToString("x2") })
    return "${Prefix}_$($hex.Substring(0, 24))"
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

function Get-DirectoryXml {
    Param(
        [Parameter(Mandatory = $true)]
        [string]$DirectoryPath,

        [Parameter(Mandatory = $true)]
        [string]$PackageRoot,

        [Parameter(Mandatory = $true)]
        [AllowEmptyCollection()]
        [System.Collections.Generic.List[string]]$ComponentRefs,

        [Parameter(Mandatory = $true)]
        [hashtable]$Context
    )

    $builder = [System.Text.StringBuilder]::new()

    $subDirectories = Get-ChildItem -LiteralPath $DirectoryPath -Directory | Sort-Object Name
    foreach ($subDirectory in $subDirectories) {
        $relativePath = Get-RelativePath -BasePath $PackageRoot -Path $subDirectory.FullName
        $directoryId = New-WixId -Prefix "DIR" -Value $relativePath
        [void]$builder.AppendLine("    <Directory Id=""$directoryId"" Name=""$(Escape-Xml $subDirectory.Name)"">")
        [void]$builder.Append((Get-DirectoryXml -DirectoryPath $subDirectory.FullName -PackageRoot $PackageRoot -ComponentRefs $ComponentRefs -Context $Context))
        [void]$builder.AppendLine("    </Directory>")
    }

    $files = Get-ChildItem -LiteralPath $DirectoryPath -File | Sort-Object Name
    foreach ($file in $files) {
        $relativePath = Get-RelativePath -BasePath $PackageRoot -Path $file.FullName
        $normalizedRelativePath = $relativePath.Replace('\', '/')
        $componentId = New-WixId -Prefix "CMP" -Value $normalizedRelativePath
        $fileId = New-WixId -Prefix "FIL" -Value $normalizedRelativePath
        $guid = New-StableGuid -Value "component:$normalizedRelativePath"
        $source = Escape-Xml $file.FullName

        $componentCondition = ""
        if (
            $normalizedRelativePath.StartsWith("neurolingsce-skill/", [System.StringComparison]::OrdinalIgnoreCase) -or
            $normalizedRelativePath.StartsWith("neurolingsce-companion/", [System.StringComparison]::OrdinalIgnoreCase)
        ) {
            $componentCondition = ' Condition="NEUROLINGSCE_INSTALL_SKILL = 1"'
        }

        [void]$builder.AppendLine("    <Component Id=""$componentId"" Guid=""$guid""$componentCondition>")
        [void]$builder.AppendLine("      <File Id=""$fileId"" Source=""$source"" KeyPath=""yes"">")
        [void]$builder.AppendLine("      </File>")
        [void]$builder.AppendLine("    </Component>")
        $ComponentRefs.Add($componentId)

        if ($file.Name.Equals($Context.MainExecutableName, [System.StringComparison]::OrdinalIgnoreCase)) {
            $Context.MainExecutableFileId = $fileId
        }

        if (
            -not $Context.InstallSkillScriptFileId -and
            $normalizedRelativePath.Equals("neurolingsce-skill/scripts/install_to_codex_home.ps1", [System.StringComparison]::OrdinalIgnoreCase)
        ) {
            $Context.InstallSkillScriptFileId = $fileId
        }

        if ($file.Name.Equals("vc_redist.x64.exe", [System.StringComparison]::OrdinalIgnoreCase)) {
            $Context.VcRedistFileId = $fileId
        }
    }

    return $builder.ToString()
}

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$versionFile = Join-Path $repoRoot "VERSION.txt"
$iconPath = Join-Path $PSScriptRoot "assets/neurolingsce.ico"
$licenseRtfPath = Join-Path $PSScriptRoot "assets/license_en-US.rtf"

if (-not (Test-Path -LiteralPath $versionFile)) {
    throw "VERSION.txt not found at $versionFile."
}

if (-not (Test-Path -LiteralPath $iconPath)) {
    throw "Installer icon not found at $iconPath."
}

if (-not (Test-Path -LiteralPath $licenseRtfPath)) {
    throw "Installer license RTF not found at $licenseRtfPath."
}

$productName = Get-VersionValue -FilePath $versionFile -Key "APP_NAME"
$manufacturer = Get-VersionValue -FilePath $versionFile -Key "APP_COMPANY"
$description = Get-VersionValue -FilePath $versionFile -Key "APP_DESCRIPTION"
$appExecutable = Get-VersionValue -FilePath $versionFile -Key "APP_EXECUTABLE"
$productVersion = Get-VersionValue -FilePath $versionFile -Key "VERSION"

$resolvedPackageDir = Resolve-PackageDirectory -InputPath $PackageDir -ExecutableName $appExecutable
$packageName = Split-Path -Leaf $resolvedPackageDir

$resolvedOutputRoot = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $OutputRoot))
$generatedRoot = Join-Path $resolvedOutputRoot "wix\$packageName"
$productSourcePath = Join-Path $generatedRoot "Product.generated.wxs"
$filesSourcePath = Join-Path $generatedRoot "PackageFiles.generated.wxs"
$outputMsiPath = Join-Path $resolvedOutputRoot "$packageName.msi"

$resolvedWixExe = Resolve-ExistingCommand -Candidates @(
    $WixExe,
    "C:\Program Files\WiX Toolset v7.0\bin\wix.exe"
)

if (-not $resolvedWixExe) {
    throw "WiX 7 CLI not found. Ensure 'wix.exe' is on PATH or pass -WixExe with the full path, for example 'C:\Program Files\WiX Toolset v7.0\bin\wix.exe'."
}

$schemaNamespace = "http://wixtoolset.org/schemas/v4/wxs"
$templatePath = Join-Path $PSScriptRoot "Product.v4.template.wxs"

if (-not (Test-Path -LiteralPath $templatePath)) {
    throw "WiX template not found at $templatePath."
}

New-Item -ItemType Directory -Path $generatedRoot -Force | Out-Null
New-Item -ItemType Directory -Path $resolvedOutputRoot -Force | Out-Null

$componentRefs = [System.Collections.Generic.List[string]]::new()
$context = @{
    MainExecutableName = "$appExecutable.exe"
    MainExecutableFileId = $null
    InstallSkillScriptFileId = $null
    VcRedistFileId = $null
    ProductName = $productName
    Description = $description
}

$directoryXml = Get-DirectoryXml -DirectoryPath $resolvedPackageDir -PackageRoot $resolvedPackageDir -ComponentRefs $componentRefs -Context $context
if (-not $context.MainExecutableFileId) {
    throw "Could not find main executable '$($context.MainExecutableName)' in $resolvedPackageDir."
}

if (-not $context.InstallSkillScriptFileId) {
    Write-Warning "Companion skill install script was not found in $resolvedPackageDir. The user-skill installation option will not work correctly."
}

if (-not $context.VcRedistFileId) {
    Write-Warning "vc_redist.x64.exe was not found in $resolvedPackageDir. MSI will be generated without a valid VC runtime custom action target."
}

$componentRefsXml = ($componentRefs | ForEach-Object { "      <ComponentRef Id=""$_"" />" }) -join [Environment]::NewLine
$filesXml = @"
<?xml version="1.0" encoding="utf-8"?>
<Wix xmlns="$schemaNamespace">
  <Fragment>
    <DirectoryRef Id="INSTALLFOLDER">
$directoryXml  </DirectoryRef>
  </Fragment>

  <Fragment>
    <ComponentGroup Id="PackageFiles">
$componentRefsXml
    </ComponentGroup>
  </Fragment>
</Wix>
"@

Set-Content -LiteralPath $filesSourcePath -Value $filesXml -Encoding UTF8

$productTemplate = Get-Content -LiteralPath $templatePath -Raw
$replacementMap = @{
    "{{ProductName}}" = Escape-Xml $productName
    "{{Manufacturer}}" = Escape-Xml $manufacturer
    "{{Version}}" = Escape-Xml $productVersion
    "{{UpgradeCode}}" = "9d12b5a2-dcc6-4ef5-a833-9d57ffde8ca1"
    "{{IconPath}}" = Escape-Xml $iconPath
    "{{LicenseRtfPath}}" = Escape-Xml $licenseRtfPath
    "{{InstallFolderName}}" = Escape-Xml $productName
    "{{ShortcutCleanupComponentGuid}}" = New-StableGuid -Value "shortcut-cleanup:$productName"
    "{{StartMenuShortcutComponentGuid}}" = New-StableGuid -Value "start-menu-shortcut:$productName"
    "{{DesktopShortcutComponentGuid}}" = New-StableGuid -Value "desktop-shortcut:$productName"
    "{{Description}}" = Escape-Xml $description
    "{{MainExecutableFileId}}" = Escape-Xml $context.MainExecutableFileId
    "{{InstallSkillScriptFileId}}" = Escape-Xml $context.InstallSkillScriptFileId
    "{{VcRedistFileId}}" = Escape-Xml $context.VcRedistFileId
    "{{ExitLaunchText}}" = Escape-Xml "Launch $productName after setup"
}

foreach ($entry in $replacementMap.GetEnumerator()) {
    $productTemplate = $productTemplate.Replace($entry.Key, $entry.Value)
}

Set-Content -LiteralPath $productSourcePath -Value $productTemplate -Encoding UTF8

Write-Host "Package source : $resolvedPackageDir"
Write-Host "Generated WiX  : $generatedRoot"
Write-Host "MSI output     : $outputMsiPath"
Write-Host "WiX mode       : wix build"

if ($GenerateOnly) {
    Write-Host "GenerateOnly   : enabled (skipping wix build)"
    return
}

if (Test-Path -LiteralPath $outputMsiPath) {
    Remove-Item -LiteralPath $outputMsiPath -Force
}

& $resolvedWixExe build `
    $productSourcePath `
    $filesSourcePath `
    -arch x64 `
    -ext WixToolset.UI.wixext `
    -ext WixToolset.Util.wixext `
    -out $outputMsiPath

if ($LASTEXITCODE -ne 0) {
    throw "wix build failed with exit code $LASTEXITCODE."
}

Write-Host "Built MSI      : $outputMsiPath"
