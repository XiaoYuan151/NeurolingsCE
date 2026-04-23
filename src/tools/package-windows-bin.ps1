Param(
    [string]$SourceDir = "out/build/x64-Release/bin",
    [string]$OutputRoot = "out/package",
    [switch]$IncludeLogs,
    [switch]$SkipZip,
    [switch]$SkipVcRedist
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

function Should-ExcludePath {
    Param(
        [Parameter(Mandatory = $true)]
        [System.IO.FileSystemInfo]$Item,

        [Parameter(Mandatory = $true)]
        [string]$SourceRoot,

        [Parameter(Mandatory = $true)]
        [bool]$IncludeLogsValue,

        [Parameter(Mandatory = $true)]
        [bool]$SkipVcRedistValue
    )

    $sourceRootWithSeparator = $SourceRoot.TrimEnd('\', '/') + [System.IO.Path]::DirectorySeparatorChar
    $itemFullName = $Item.FullName

    if ($itemFullName.StartsWith($sourceRootWithSeparator, [System.StringComparison]::OrdinalIgnoreCase)) {
        $relativePath = $itemFullName.Substring($sourceRootWithSeparator.Length)
    } else {
        $relativePath = $Item.Name
    }

    $relativePath = $relativePath.Replace('\', '/')

    if (-not $IncludeLogsValue) {
        if ($relativePath -eq "log" -or $relativePath.StartsWith("log/")) {
            return $true
        }

        if ($relativePath -in @("shijima_stdout.txt", "shijima_stderr.txt")) {
            return $true
        }
    }

    if ($SkipVcRedistValue -and $relativePath -eq "vc_redist.x64.exe") {
        return $true
    }

    return $false
}

function Copy-CompanionSkills {
    Param(
        [Parameter(Mandatory = $true)]
        [string]$RepoRoot,

        [Parameter(Mandatory = $true)]
        [string]$StageDir
    )

    $skillDirectories = @(
        "neurolingsce-skill",
        "neurolingsce-companion"
    )

    $filesToCopy = @(
        "SKILL.md",
        "agents/openai.yaml",
        "scripts/find_neurolingsce_cli.py",
        "scripts/summon_companion.py",
        "scripts/install_to_codex_home.ps1"
    )

    foreach ($skillDirectory in $skillDirectories) {
        $skillSourceRoot = Join-Path $RepoRoot $skillDirectory
        if (-not (Test-Path -LiteralPath $skillSourceRoot)) {
            Write-Warning "Skill directory not found at $skillSourceRoot. Skipping packaging for $skillDirectory."
            continue
        }

        $skillTargetRoot = Join-Path $StageDir $skillDirectory
        foreach ($relativePath in $filesToCopy) {
            $sourcePath = Join-Path $skillSourceRoot $relativePath
            if (-not (Test-Path -LiteralPath $sourcePath)) {
                Write-Warning "Expected skill file not found: $sourcePath"
                continue
            }

            $targetPath = Join-Path $skillTargetRoot $relativePath
            $targetParent = Split-Path -Parent $targetPath
            New-Item -ItemType Directory -Path $targetParent -Force | Out-Null
            Copy-Item -LiteralPath $sourcePath -Destination $targetPath -Force
        }
    }
}

$repoRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$versionFile = Join-Path $repoRoot "VERSION.txt"

if (-not (Test-Path -LiteralPath $versionFile)) {
    throw "VERSION.txt not found at $versionFile."
}

$appName = Get-VersionValue -FilePath $versionFile -Key "APP_NAME"
$version = Get-VersionValue -FilePath $versionFile -Key "VERSION"

$resolvedSourceDir = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $SourceDir))
if (-not (Test-Path -LiteralPath $resolvedSourceDir)) {
    throw "Source directory not found: $resolvedSourceDir"
}

$resolvedOutputRoot = [System.IO.Path]::GetFullPath((Join-Path $repoRoot $OutputRoot))
$packageName = "${appName}_windows_x86_64_v${version}"
$stageDir = Join-Path $resolvedOutputRoot $packageName
$zipPath = Join-Path $resolvedOutputRoot "$packageName.zip"

New-Item -ItemType Directory -Path $resolvedOutputRoot -Force | Out-Null

if (Test-Path -LiteralPath $stageDir) {
    Remove-Item -LiteralPath $stageDir -Recurse -Force
}

if ((-not $SkipZip) -and (Test-Path -LiteralPath $zipPath)) {
    Remove-Item -LiteralPath $zipPath -Force
}

New-Item -ItemType Directory -Path $stageDir -Force | Out-Null

Get-ChildItem -LiteralPath $resolvedSourceDir -Force | ForEach-Object {
    if (Should-ExcludePath -Item $_ -SourceRoot $resolvedSourceDir -IncludeLogsValue $IncludeLogs.IsPresent -SkipVcRedistValue $SkipVcRedist.IsPresent) {
        return
    }

    Copy-Item -LiteralPath $_.FullName -Destination $stageDir -Recurse -Force
}

Copy-CompanionSkills -RepoRoot $repoRoot -StageDir $stageDir

if (-not $SkipZip) {
    Compress-Archive -Path (Join-Path $stageDir '*') -DestinationPath $zipPath -CompressionLevel Optimal
}

Write-Host "Packaged source : $resolvedSourceDir"
Write-Host "Staged output   : $stageDir"

if (-not $SkipZip) {
    Write-Host "Zip archive     : $zipPath"
}
