# Use GitVersion command line tool to increment the version number

param (
    [string]$OutputFile = ".\version.h",
    [string]$ProjectDirectory = "."
)

Push-Location $ProjectDirectory

$gitVersion = GitVersion.exe | ConvertFrom-Json
$semver = $gitVersion.SemVer
$majorVersion = $gitVersion.Major
$minorVersion = $gitVersion.Minor

Set-Content -Path $OutputFile "#define SemanticVersion ""$semver"""
Add-Content -Path $OutputFile "#define MajorVersion ($majorVersion)"
Add-Content -Path $OutputFile "#define MinorVersion ($minorVersion)"
Write-Debug "Version $semver written to $OutputFile"
Write-Host "-DSEMANTIC_VERSION=$semver"
Pop-Location

