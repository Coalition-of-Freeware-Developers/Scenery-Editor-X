# Installing Scenery Editor X

Scenery Editor X installs with a standard Windows setup program. It takes a minute, needs no administrator rights if you install it just for yourself, and checks that your graphics driver can run the editor before you start it.

## What you need

| | Requirement | Notes |
|---|---|---|
| **Operating system** | Windows 10 or 11, 64-bit | Setup refuses to run on 32-bit Windows or anything older than Windows 10. |
| **Graphics** | A graphics card and driver with **Vulkan 1.3** | Any NVIDIA, AMD or Intel GPU from the last several years, with a current driver. See [Checking Vulkan](#checking-vulkan). |
| **X-Plane** | X-Plane 12 (an X-Plane 11 install works for its libraries) | The editor reads X-Plane's libraries, airports and lighting tables from your install. It never changes X-Plane's own files. |
| **Memory** | 16 GB recommended | A large airport with thousands of objects can use several gigabytes on its own. |
| **Disk** | About 100 MB for the program, plus caches | Caches (airport index, thumbnails, compiled shaders) grow as you work and can always be deleted. |

Everything else the editor needs, including the Visual C++ runtime, is installed beside it. The only system-wide requirements are the Vulkan runtime and a Vulkan 1.3 graphics driver.

## Installing

Test builds are published as a single file named like `SceneryEditorX-0.2.28.529-Alpha-Setup.exe`.

<!-- class: steps -->
1. **Run the setup program.** Windows may warn that the publisher is unknown (test builds aren't code-signed yet): choose **More info › Run anyway**.
2. **Read the welcome page.** It reminds you that this is a test build: keep backups of your projects and scenery.
3. **Accept the license.** Scenery Editor X is released under the MIT License.
4. **Choose who it's for.**
   - **Anyone who uses this computer** installs into `Program Files` and needs an administrator. Choose this if you want setup to be able to install the Vulkan runtime for you.
   - **Only for me** installs into `%LOCALAPPDATA%\Programs\Scenery Editor X` and needs no administrator rights.
5. **Choose components.** The editor itself and the `.edX` / `.sex` file associations are always installed. **Desktop shortcut** is optional. A Start menu entry is always added.
6. **Choose the folder**, or keep the default, and click **Install**.
7. **Let setup check Vulkan.** It runs the editor's own Vulkan check (see below) and tells you if your driver needs updating.
8. **Finish.** Leave **Start Scenery Editor X** ticked to launch it straight away. The **Report a problem** link opens the project's issue tracker.

After installing, double-clicking a `.edX` or `.sex` project file opens it in Scenery Editor X.

> [!TIP]
> Install **Only for me** if you don't have an administrator account or just want to try the editor. Everything works the same way; only the install folder and who can use it differ.

## Checking Vulkan

Scenery Editor X draws with Vulkan 1.3. Setup checks for it, and the editor checks again every time it starts. When something is missing you see one of these:

| What setup or the editor says | What it means | What to do |
|---|---|---|
| The Vulkan runtime (`vulkan-1.dll`) is missing or older than 1.3 | The Vulkan loader that graphics drivers install isn't there or is old. | Install the latest driver for your graphics card. An all-users setup can also install LunarG's Vulkan Runtime for you; it asks you to accept its license. |
| No graphics driver on this PC offers Vulkan | The driver doesn't support Vulkan at all (common with the basic display driver Windows installs). | Install the graphics card maker's driver: [NVIDIA](https://www.nvidia.com/drivers), [AMD](https://www.amd.com/support) or [Intel](https://www.intel.com/content/www/us/en/support/detect.html). |
| The driver offers a Vulkan older than 1.3 | The driver, or the card, is too old. | Update the driver. If the newest driver doesn't add Vulkan 1.3, the graphics card is too old for Scenery Editor X. |

You can run the same check yourself from a command prompt in the install folder:

```bat
SceneryEditorX.exe --check-vulkan
echo %ERRORLEVEL%
```

The exit code is `0` when everything is ready. The other codes are listed under [Command-line options](command-line.md#vulkan-check-exit-codes).

## Updating, repairing and removing

Run a newer setup program over an installed copy and it skips the first-install pages and offers:

- **Update to *version*:** replaces the program files. Shortcuts and file associations stay as they are.
- **Repair *version*:** when the same version is installed, reinstalls every program file, the file associations, the shortcuts and the Add/Remove Programs entry.
- **Replace it with the older *version*:** when the installed copy is newer than the setup. Projects saved with the newer version may not open in the older one.
- **Uninstall Scenery Editor X.**

Your settings, projects and unsaved-project recovery data are kept whichever you choose. If Scenery Editor X is running, setup asks you to save your work and close it first.

If the editor was installed for all users and you aren't an administrator, setup can't update that copy. It offers to install a separate copy just for you instead.

### Uninstalling

Uninstall from **Windows Settings › Apps › Installed apps**, or from the maintenance page of any setup program. The uninstaller removes exactly the files it installed, its shortcuts and the file associations.

It also offers **Settings, logs and caches**, unticked by default. Tick it to remove:

- `%APPDATA%\SceneryEditorX`: settings, preferences, recent projects and logs
- `%LOCALAPPDATA%\SceneryEditorX`: caches and the project recovery data
- `%TEMP%\SceneryEditorX`: temporary files

> [!WARNING]
> Removing settings, logs and caches also deletes unsaved-project recovery data. Open and save any project the editor offered to recover before you tick it. Your project files and scenery packs themselves are never touched by the uninstaller.

## Unattended installs

The setup program accepts these switches, for scripted installs on several machines:

| Switch | Effect |
|---|---|
| `/S` | Silent: no pages, default choices. |
| `/CurrentUser` | Install for the current user only. |
| `/AllUsers` | Install for all users (needs an elevated prompt). |
| `/D=C:\Path\To\Folder` | Install folder. Must be the last switch, without quotes. |

```bat
SceneryEditorX-0.2.28.529-Alpha-Setup.exe /S /CurrentUser
```

A silent install never shows the Vulkan Runtime offer; install a current graphics driver instead.

## Next steps

Start the editor from the Start menu. The [next chapter](first-launch.md) explains what it asks you the first time and how it finds X-Plane.
