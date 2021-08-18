param(
    [string] $AppName
)

if (-not(Test-Path "build-windows-x64")) {
    New-Item "build-windows-x64" -ItemType Directory
}

Push-Location "build-windows-x64"
cmake -G="Visual Studio 16 2019" ..
cmake --build . -t $AppName
Pop-Location

& build-windows-x64/bin/Debug/Demo.exe
