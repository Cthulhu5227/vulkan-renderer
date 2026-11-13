# Build, then run with the MSYS2 validation layer visible to the Vulkan loader.
$ErrorActionPreference = "Stop"
cmake --preset debug | Out-Null
cmake --build --preset debug
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$env:VK_ADD_LAYER_PATH = "C:\msys64\ucrt64\bin"
& "$PSScriptRoot\build\app.exe"z