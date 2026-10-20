<#
.SYNOPSIS
    Builds the Scenery Editor X Windows installer (alpha test builds by default).

.DESCRIPTION
    Stages a Release build with everything it loads at run time, then compiles SceneryEditorX.nsi with NSIS.

    Staged into <OutputDir>\stage:
      - bin\SceneryEditorX.exe and every DLL it imports, recursively. Each one is looked up in the vcpkg Release
        bin folder, then bin\, then the Vulkan SDK (the Slang compiler). A DLL that imports the Debug C++ runtime
        stops the build: testers don't have it.
      - The Slang modules slang.dll loads by name (they are not in any import table).
      - The Visual C++ runtime DLLs the binaries import, copied beside the exe from the compiler's redist folder,
        so no separate VC++ redistributable is needed and per-user installs work without admin rights.
      - resources\, LICENSE.txt and licenses\ (the vcpkg copyright file of each staged third-party DLL).

    Also writes <OutputDir>\uninstall_files.nsh, the exact list of staged files the uninstaller removes.
    The installer lands in <OutputDir>\SceneryEditorX-<version>-<Channel>-Setup.exe.

.PARAMETER Build
    Builds the Release editor first (cmake --build <BuildDir> --config Release --target SceneryEditorX).

.PARAMETER BuildDir
    The CMake build folder, relative to the repository root. Default: build.

.PARAMETER Channel
    The release channel shown in the installer and its file name. Default: Alpha.

.PARAMETER OutputDir
    Where the stage folder and the installer go, relative to the repository root. Default: build\installer.

.EXAMPLE
    pwsh installer\windows\Build-Installer.ps1 -Build
#>
[CmdletBinding()]
param(
    [switch]$Build,
    [string]$BuildDir = 'build',
    [string]$Channel = 'Alpha',
    [string]$OutputDir = 'build\installer'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$Root = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$BuildPath = Join-Path $Root $BuildDir
$OutputPath = Join-Path $Root $OutputDir
$StagePath = Join-Path $OutputPath 'stage'
$ExeName = 'SceneryEditorX.exe'

# DLLs the Slang compiler loads by name at run time (slang.dll only imports kernel32)
$SlangModules = @('slang-compiler.dll', 'slang-glslang.dll', 'slang-glsl-module.dll')

# Never shipped: the Vulkan loader comes with the graphics driver
$DriverDlls = @('vulkan-1.dll')

$VcRuntimePattern = '^(msvcp140.*|vcruntime140.*|concrt140|vccorlib140|vcomp140)\.dll$'
$DebugRuntimePattern = '^(msvcp140.*d|vcruntime140.*d|ucrtbased|concrt140d|vccorlib140d|vcomp140d)\.dll$'

function Write-Step([string]$Text) { Write-Host "==> $Text" -ForegroundColor Cyan }

# Tools ------------------------------------------------------

function Get-CMakeCacheValue([string]$Name) {
    $cache = Join-Path $BuildPath 'CMakeCache.txt'
    if (-not (Test-Path $cache)) { throw "No CMake cache at $cache. Configure the build first." }
    $line = Select-String -Path $cache -Pattern "^$([regex]::Escape($Name)):[^=]*=(.*)$" | Select-Object -First 1
    if ($line) { return $line.Matches[0].Groups[1].Value }
    return $null
}

function Get-MakeNsis {
    $command = Get-Command makensis.exe -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    foreach ($key in 'HKLM:\SOFTWARE\WOW6432Node\NSIS', 'HKLM:\SOFTWARE\NSIS') {
        $folder = $null
        try { $folder = Get-ItemPropertyValue -Path $key -Name '(default)' } catch { }
        if ($folder -and (Test-Path (Join-Path $folder 'makensis.exe'))) { return Join-Path $folder 'makensis.exe' }
    }
    $default = Join-Path ${env:ProgramFiles(x86)} 'NSIS\makensis.exe'
    if (Test-Path $default) { return $default }
    throw 'NSIS 3 was not found. Install it from https://nsis.sourceforge.io (makensis.exe on PATH also works).'
}

# The toolset that built the editor: dumpbin sits beside its linker, the VC++ runtime under its VC\Redist
# (Visual Studio generators cache the linker's path, not the compiler's)
$Linker = Get-CMakeCacheValue 'CMAKE_LINKER'
if (-not $Linker) { $Linker = Get-CMakeCacheValue 'CMAKE_CXX_COMPILER' }
if (-not $Linker -or -not (Test-Path $Linker)) { throw "The MSVC toolset in the CMake cache ($Linker) doesn't exist." }
$ToolFolder = Split-Path $Linker
$Dumpbin = Join-Path $ToolFolder 'dumpbin.exe'
if (-not (Test-Path $Dumpbin)) { throw "dumpbin.exe was not found in $ToolFolder." }

$VcRoot = (Resolve-Path (Join-Path $ToolFolder '..\..\..\..\..\..')).Path     # <VS>\VC\Tools\MSVC\<ver>\bin\Hostx64\x64
$CrtFolder = Get-ChildItem (Join-Path $VcRoot 'Redist\MSVC') -Directory |
    Where-Object { $_.Name -match '^\d+(\.\d+)+$' } |
    Sort-Object { [version]$_.Name } -Descending |
    ForEach-Object { Get-ChildItem (Join-Path $_.FullName 'x64') -Directory -Filter 'Microsoft.VC*.CRT' -ErrorAction SilentlyContinue } |
    Select-Object -First 1
if (-not $CrtFolder) { throw "No x64 Visual C++ runtime under $VcRoot\Redist\MSVC." }

$Triplet = Get-CMakeCacheValue 'VCPKG_TARGET_TRIPLET'
if (-not $Triplet) { $Triplet = 'x64-windows' }
$VcpkgInstalled = Get-CMakeCacheValue 'VCPKG_INSTALLED_DIR'
if (-not $VcpkgInstalled) { $VcpkgInstalled = Join-Path $BuildPath 'vcpkg_installed' }
$VcpkgBin = Join-Path $VcpkgInstalled "$Triplet\bin"

if (-not $env:VULKAN_SDK) { throw 'VULKAN_SDK is not set: the Slang compiler DLLs come from the Vulkan SDK.' }
$SdkBin = Join-Path $env:VULKAN_SDK 'Bin'

$SearchFolders = @($VcpkgBin, (Join-Path $Root 'bin'), $SdkBin) | Where-Object { Test-Path $_ }

# Build --------------------------------------------------------

if ($Build) {
    Write-Step 'Building the Release editor'
    & cmake --build $BuildPath --config Release --target SceneryEditorX
    if ($LASTEXITCODE -ne 0) { throw "The Release build failed (exit code $LASTEXITCODE)." }
}

$Exe = Join-Path $Root "bin\$ExeName"
if (-not (Test-Path $Exe)) {
    throw "No Release build at $Exe. Build it (-Build, or the Release configuration in Visual Studio) first."
}
$ExeItem = Get-Item $Exe
$Version = $ExeItem.VersionInfo.ProductVersion
Write-Step "Packaging $ExeName $Version ($Channel), built $($ExeItem.LastWriteTime)"

# Stage --------------------------------------------------------

if (Test-Path $StagePath) { Remove-Item $StagePath -Recurse -Force }
New-Item -ItemType Directory -Path $StagePath | Out-Null

function Get-Imports([string]$Path) {
    $lines = & $Dumpbin /nologo /dependents $Path
    if ($LASTEXITCODE -ne 0) { throw "dumpbin failed on $Path." }
    # Both normal and delay-load dependencies: a delay-loaded DLL is still needed once it's called
    return $lines | Where-Object { $_ -match '^\s+(\S+\.dll)\s*$' } | ForEach-Object { $Matches[1] }
}

function Test-SystemDll([string]$Name) {
    if ($Name -match '^(api|ext)-ms-win-') { return $true }
    if ($DriverDlls -contains $Name.ToLowerInvariant()) { return $true }
    if ($Name -match $VcRuntimePattern) { return $false }
    return Test-Path (Join-Path $env:WINDIR "System32\$Name")
}

function Find-Dll([string]$Name) {
    if ($Name -match $VcRuntimePattern) {
        $path = Join-Path $CrtFolder.FullName $Name
        if (Test-Path $path) { return $path }
        throw "$Name is not in the Visual C++ runtime folder $($CrtFolder.FullName)."
    }
    foreach ($folder in $SearchFolders) {
        $path = Join-Path $folder $Name
        if (Test-Path $path) { return $path }
    }
    throw "$Name was not found in: $($SearchFolders -join ', ')."
}

$Staged = @{}      # DLL name (lower case) -> source path
$Queue = New-Object System.Collections.Generic.Queue[string]
$Queue.Enqueue($Exe)

function Add-Binary([string]$Source) {
    $name = Split-Path $Source -Leaf
    if ($Staged.ContainsKey($name.ToLowerInvariant())) { return }
    $Staged[$name.ToLowerInvariant()] = $Source
    Copy-Item $Source -Destination $StagePath
    $Queue.Enqueue($Source)
}

$Staged[$ExeName.ToLowerInvariant()] = $Exe
Copy-Item $Exe -Destination $StagePath

Write-Step 'Resolving DLL dependencies'
while ($Queue.Count -gt 0) {
    $binary = $Queue.Dequeue()
    $binaryName = Split-Path $binary -Leaf
    foreach ($import in Get-Imports $binary) {
        if ($import -match $DebugRuntimePattern) {
            throw "$binary imports $import, the Debug C++ runtime. Package Release binaries only (a Debug DLL in bin\?)."
        }
        if (Test-SystemDll $import) { continue }
        Add-Binary (Find-Dll $import)
    }
    if ($binaryName -ieq 'slang.dll') {
        foreach ($module in $SlangModules) { Add-Binary (Find-Dll $module) }
    }
}
$Staged.GetEnumerator() | Sort-Object Name | ForEach-Object { Write-Host ("    {0,-28} {1}" -f $_.Key, $_.Value) }

Write-Step 'Copying resources and licenses'
Copy-Item (Join-Path $Root 'resources') -Destination $StagePath -Recurse
Copy-Item (Join-Path $Root 'LICENSE.txt') -Destination $StagePath

# Third-party notices: vcpkg records which port installed each DLL, and keeps that port's copyright file
$LicensePath = Join-Path $StagePath 'licenses'
New-Item -ItemType Directory -Path $LicensePath | Out-Null
$Notes = New-Object System.Collections.Generic.List[string]
$Notes.Add('Third-party components shipped with Scenery Editor X')
$Notes.Add('')
$Owners = @{}
foreach ($list in Get-ChildItem (Join-Path $VcpkgInstalled 'vcpkg\info') -Filter "*_$Triplet.list" -ErrorAction SilentlyContinue) {
    $port = $list.BaseName -replace "_[^_]+_$([regex]::Escape($Triplet))$", ''
    foreach ($line in Get-Content $list.FullName) {
        if ($line -match "^$([regex]::Escape($Triplet))/bin/([^/]+\.dll)$") { $Owners[$Matches[1].ToLowerInvariant()] = $port }
    }
}
$Ports = $Staged.GetEnumerator() |
    Where-Object { $_.Value.StartsWith($VcpkgBin, [StringComparison]::OrdinalIgnoreCase) -and $Owners.ContainsKey($_.Key) } |
    ForEach-Object { $Owners[$_.Key] } | Sort-Object -Unique
foreach ($port in $Ports) {
    $copyright = Join-Path $VcpkgInstalled "$Triplet\share\$port\copyright"
    if (Test-Path $copyright) {
        Copy-Item $copyright -Destination (Join-Path $LicensePath "$port.txt")
        $Notes.Add("$port`t(licenses\$port.txt)")
    }
    else {
        Write-Warning "vcpkg port $port has no copyright file."
    }
}
if ($Staged.Keys | Where-Object { $_ -like 'slang*' }) {
    $Notes.Add("slang`t(slang*.dll, from the Vulkan SDK) Apache License 2.0 with LLVM Exceptions, https://github.com/shader-slang/slang/blob/master/LICENSE")
}
if ($Staged.Keys | Where-Object { $_ -match $VcRuntimePattern }) {
    $Notes.Add("Microsoft Visual C++ runtime`t(msvcp140*.dll, vcruntime140*.dll) redistributed under the Visual Studio license terms")
}
[IO.File]::WriteAllLines((Join-Path $LicensePath 'README.txt'), $Notes)

# Uninstall list -------------------------------------------------

Write-Step 'Writing the uninstall list'
$ListPath = Join-Path $OutputPath 'uninstall_files.nsh'
$Lines = New-Object System.Collections.Generic.List[string]
$Lines.Add('; Generated by installer\windows\Build-Installer.ps1: the files staged for this installer, which its uninstaller removes.')
$Lines.Add('!macro SEDX_UNINSTALL_FILES')
$Files = Get-ChildItem $StagePath -Recurse -File | ForEach-Object { $_.FullName.Substring($StagePath.Length + 1) } | Sort-Object
foreach ($file in $Files) { $Lines.Add("  Delete `"`$INSTDIR\$($file.Replace('$', '$$'))`"") }
$Folders = Get-ChildItem $StagePath -Recurse -Directory | Sort-Object { $_.FullName.Length } -Descending |
    ForEach-Object { $_.FullName.Substring($StagePath.Length + 1) }
foreach ($folder in $Folders) { $Lines.Add("  RMDir `"`$INSTDIR\$($folder.Replace('$', '$$'))`"") }
$Lines.Add('!macroend')
[IO.File]::WriteAllLines($ListPath, $Lines, (New-Object System.Text.UTF8Encoding($true)))

# Installer -------------------------------------------------------

$MakeNsis = Get-MakeNsis
$Script = Join-Path $PSScriptRoot 'SceneryEditorX.nsi'
$OutFile = Join-Path $OutputPath "SceneryEditorX-$Version-$Channel-Setup.exe"
Write-Step "Compiling the installer with $MakeNsis"
& $MakeNsis /V3 /INPUTCHARSET UTF8 "/DSTAGE_DIR=$StagePath" "/DFILE_LIST=$ListPath" "/DCHANNEL=$Channel" "/DVERSION=$Version" "/DOUT_FILE=$OutFile" $Script
if ($LASTEXITCODE -ne 0) { throw "makensis failed (exit code $LASTEXITCODE)." }

$size = (Get-Item $OutFile).Length / 1MB
Write-Step ("Done: {0} ({1:N1} MB, {2} files)" -f $OutFile, $size, $Files.Count)
