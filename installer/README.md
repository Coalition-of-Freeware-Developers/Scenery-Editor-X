# Windows installer

`installer/windows/` builds a Windows setup program for test builds of Scenery Editor X (alpha by default).

| File | What it is |
| --- | --- |
| `Build-Installer.ps1` | Stages a Release build and compiles the installer. Start here. |
| `SceneryEditorX.nsi` | The NSIS installer script. |
| `FileAssociations.nsh` | Registers the editor as the handler of `.edX` and `.sex` project files. |

## Building it

You need an MSVC build folder (the `windows-*` presets or `SetupProject.bat`), the Vulkan SDK with `VULKAN_SDK`
set, and [NSIS 3](https://nsis.sourceforge.io).

```powershell
pwsh installer\windows\Build-Installer.ps1 -Build
```

`-Build` builds the Release editor first. Leave it out to package the `bin\SceneryEditorX.exe` you already have.
`-Channel Beta` changes the channel shown in the installer and its file name.

The setup program lands in `build\installer\SceneryEditorX-<version>-<channel>-Setup.exe`. The version comes from
`source\resource.h`.

The script stages, in `build\installer\stage`:

- `SceneryEditorX.exe` and every DLL it imports, followed recursively with `dumpbin`. DLLs come from the vcpkg
  Release `bin` folder, then `bin\`, then the Vulkan SDK. A DLL that imports the Debug C++ runtime stops the build,
  because testers don't have it.
- The Slang modules `slang.dll` loads by name: `slang-compiler.dll`, `slang-glslang.dll` and `slang-glsl-module.dll`.
- The Visual C++ runtime DLLs, copied beside the exe from the compiler's `VC\Redist` folder. Testers don't need the
  VC++ redistributable, and per-user installs don't need admin rights.
- `resources\`, `LICENSE.txt`, and `licenses\` with the vcpkg copyright file of each third-party DLL.

It also writes `build\installer\uninstall_files.nsh`, the exact file list the uninstaller removes.

LunarG's signed Vulkan Runtime installer (`VULKAN_SDK\Helpers\VulkanRT.exe`) is embedded in setup too. Setup doesn't
install it with the editor: it only runs it when the runtime is missing (see Prerequisites). It adds about 9 MB to
setup; `-NoVulkanRuntime` leaves it out.

## Prerequisites

Everything the editor loads is installed beside it, including the Visual C++ runtime and the Slang shader compiler.
Two things it needs live outside its folder:

| Prerequisite | What setup does |
| --- | --- |
| Vulkan runtime (`vulkan-1.dll`), 1.3 or newer | Graphics drivers install it. If it's missing or too old, an all-users install offers to run LunarG's Vulkan Runtime installer, which asks the user to accept its license. A per-user or silent install can't run it, so setup says how to fix it. |
| A graphics driver with Vulkan 1.3 | Only the card's maker can provide it. Setup warns and lists the NVIDIA, AMD and Intel driver pages. |

Setup checks both by running `SceneryEditorX.exe --check-vulkan`. The check opens no window and writes no log; its
exit code says what's missing:

| Exit code | Meaning |
| --- | --- |
| 0 | A GPU with Vulkan 1.3 |
| 2 | No usable Vulkan runtime (`vulkan-1.dll` missing or damaged) |
| 3 | Vulkan runtime older than 1.3 |
| 4 | No graphics driver offers Vulkan |
| 5 | GPUs found, none with Vulkan 1.3 |

The editor runs the same check at startup. If it fails, the editor shows a message naming what to install instead of
crashing.

X-Plane 11 or 12 can't be bundled. The editor finds an existing install itself, through X-Plane's install records,
Steam, the registry and the usual folders.

## What the installer does

- Installs for all users in `Program Files\Scenery Editor X` (needs admin), or for the current user only in
  `%LOCALAPPDATA%\Programs\Scenery Editor X` (no admin). Admins choose on the install-mode page.
- Associates `.edX` (JSON project) and `.sex` (binary project) with the editor. Double-clicking either one runs
  `SceneryEditorX.exe --project "<file>"`. The editor is also listed under Open with and Settings > Apps > Default apps.
- Adds a Start menu entry, an optional desktop shortcut, and an Add/Remove Programs entry.
- Checks for Vulkan 1.3 after installing, and can install the Vulkan runtime (see Prerequisites). Refuses 32-bit
  Windows and anything older than Windows 10.
- On an upgrade, the previous version's uninstaller removes the files that version installed first.
- Uninstalling removes only the files it installed, plus its registry entries and shortcuts. Settings, logs, caches
  and unsaved-project recovery data (`%APPDATA%\SceneryEditorX`, `%LOCALAPPDATA%\SceneryEditorX`) are removed only
  if that box is ticked. Project files are never touched.

Setup command line: `/S` silent, `/AllUsers` or `/CurrentUser`, `/D=<folder>` install folder (unquoted, last).
The uninstaller takes `/S`, plus `/AllUsers` or `/CurrentUser`.

## Testing an installer without a UAC prompt

A per-user install doesn't need elevation, but the setup asks for it when an admin runs it. To skip that while
testing:

```powershell
$env:__COMPAT_LAYER = 'RunAsInvoker'
.\build\installer\SceneryEditorX-0.2.27.528-Alpha-Setup.exe /S /CurrentUser
```

To try the "no Vulkan driver" path on a machine that has one, point the Vulkan loader at a driver list that doesn't
exist. Setup and the editor both inherit it:

```powershell
$env:VK_DRIVER_FILES = 'C:\no\such\icd.json'
.\build\installer\stage\SceneryEditorX.exe --check-vulkan
Remove-Item Env:VK_DRIVER_FILES
```

If Windows still opens a project with another app after installing, the user picked that app with "Always" before.
Choose Scenery Editor X once under Open with > Choose another app > Always. Installers aren't allowed to change that
choice.
