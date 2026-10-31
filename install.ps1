# Copies xinput1_3.dll beside the game and gives L3/R3 their own icons.
# Needs PsychoPad's XI_Icons art already in the game folder.
param([Parameter(Mandatory)][string]$GameDir)
$ErrorActionPreference = 'Stop'

$dll = Join-Path $PSScriptRoot 'build\xinput1_3.dll'
$icons = Join-Path $GameDir 'WorkResource\Textures\Icons'
$table = Join-Path $icons 'UI_Icons\IconPrefsPC.ini'
$backup = "$table.psychopadx.bak"

foreach ($required in $dll, (Join-Path $GameDir 'Psychonauts.exe'), $table,
                      (Join-Path $icons 'XI_Icons\XI_JoystickLeft.tga'),
                      (Join-Path $icons 'XI_Icons\XI_JoystickRight.tga')) {
    if (-not (Test-Path -LiteralPath $required)) { throw "Missing: $required" }
}
if (Get-Process -Name Psychonauts -ErrorAction SilentlyContinue) { throw 'Close Psychonauts first.' }

if (-not (Test-Path -LiteralPath $backup)) { Copy-Item -LiteralPath $table -Destination $backup }

$file = Get-Item -LiteralPath $table
$wasReadOnly = $file.IsReadOnly
$file.IsReadOnly = $false
$text = [IO.File]::ReadAllText($table, [Text.Encoding]::Default)
$text = $text -replace '(?m)^(Joy11,\s*WorkResource\\Textures\\icons\\XI_Icons\\)XI_Joystick_00\.tga', '$1XI_JoystickLeft.tga'
$text = $text -replace '(?m)^(Joy12,\s*WorkResource\\Textures\\icons\\XI_Icons\\)XI_Joystick_00\.tga', '$1XI_JoystickRight.tga'
[IO.File]::WriteAllText($table, $text, [Text.Encoding]::Default)
$file.IsReadOnly = $wasReadOnly

Copy-Item -LiteralPath $dll -Destination (Join-Path $GameDir 'xinput1_3.dll') -Force
Write-Output "Installed to $GameDir"
