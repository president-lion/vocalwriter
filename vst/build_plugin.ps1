# Configures and builds the VocalWriter Voice plugin.
#
# Imports the Visual Studio x64 build environment (cl.exe and the bundled
# Ninja), then builds with CMake. JUCE is not vendored here -- it is 111 MB of
# someone else's repository -- so it is cloned on the first build.

$ErrorActionPreference = "Stop"
Set-Location -Path $PSScriptRoot

$vswhere = "${env:ProgramFiles(x86)}\Microsoft Visual Studio\Installer\vswhere.exe"
if (-not (Test-Path $vswhere)) { Write-Error "vswhere.exe not found; is Visual Studio installed?" }
$vsRoot = & $vswhere -latest -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if (-not $vsRoot) { Write-Error "Visual Studio with the C++ toolset was not found." }
$vcvars = Join-Path $vsRoot "VC\Auxiliary\Build\vcvars64.bat"
if (-not (Test-Path $vcvars)) { Write-Error "vcvars64.bat not found at $vcvars" }
cmd /c "`"$vcvars`" && set" | ForEach-Object {
    if ($_ -match "^(.*?)=(.*)$") { Set-Item -Path "Env:\$($matches[1])" -Value $matches[2] }
}

if (-not (Test-Path "libs/JUCE/CMakeLists.txt")) {
    Write-Host "Cloning JUCE 8.0.13..."
    git clone --depth 1 --branch 8.0.13 https://github.com/juce-framework/JUCE.git libs/JUCE
    if ($LASTEXITCODE -ne 0) { Write-Error "Could not clone JUCE." }
}
if (-not (Test-Path "../engine/include/vw_editor.h")) {
    Write-Error "The engine submodule is missing. Run: git submodule update --init engine"
}

cmake -S . -B build -G "Ninja" -DCMAKE_BUILD_TYPE=Release
if ($LASTEXITCODE -ne 0) { Write-Error "CMake configure failed." }
cmake --build build
if ($LASTEXITCODE -ne 0) { Write-Error "Build failed." }

Write-Host ""
Write-Host "Built." -ForegroundColor Green
$vst3 = "build/VocalWriterVoice_artefacts/Release/VST3/VocalWriter Voice.vst3"
if (Test-Path $vst3) { Write-Host "VST3: $((Resolve-Path $vst3).Path)" }
Write-Host "To install: copy the .vst3 folder into C:\Program Files\Common Files\VST3\ and re-scan in Reaper."
