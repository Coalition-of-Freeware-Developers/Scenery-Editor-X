<#
PowerShell helper: build project and (optionally) invoke RenderDoc CLI to capture.
Usage:
  ./build_and_capture.ps1 -Build -Capture
#>

param(
  [switch]$Build,
  [switch]$Capture
)

if ($Build) {
  if (-Not (Test-Path build)) { New-Item -ItemType Directory -Path build | Out-Null }
  pushd build
  cmake .. -DCMAKE_BUILD_TYPE=Debug
  cmake --build . --config Debug
  popd
}

if ($Capture) {
  # Ensure validation layers enabled for the run
  $env:VK_INSTANCE_LAYERS = "VK_LAYER_KHRONOS_validation"
  $env:VK_LOADER_DEBUG = "all"

  $exe = Join-Path (Resolve-Path .) "build/bin/vulkan_sample.exe"
  if (-Not (Test-Path $exe)) { Write-Error "Executable not found: $exe"; exit 1 }

  # If RenderDoc CLI is available (renderdoccmd.exe), use it to capture a single frame.
  $renderdocCmd = "renderdoccmd.exe"
  if (Get-Command $renderdocCmd -ErrorAction SilentlyContinue) {
    Write-Host "Starting RenderDoc capture via renderdoccmd..."
    & $renderdocCmd capture --capfile my_capture.rdc --hookexe $exe -- $exe
  } else {
    Write-Host "RenderDoc CLI not found. You can open RenderDoc and attach to the running process or install RenderDoc and use renderdoccmd.exe to capture."
    & $exe
  }
}
