# Removes the DLL and restores the icon table. Profiles the DLL changed keep a
# .psychopadx.bak beside them; they aren't restored, so later rebinds survive.
param([Parameter(Mandatory)][string]$GameDir)
$ErrorActionPreference = 'Stop'

if (Get-Process -Name Psychonauts -ErrorAction SilentlyContinue) { throw 'Close Psychonauts first.' }

$table = Join-Path $GameDir 'WorkResource\Textures\Icons\UI_Icons\IconPrefsPC.ini'
$backup = "$table.psychopadx.bak"
if (Test-Path -LiteralPath $backup) {
    Set-ItemProperty -LiteralPath $table -Name IsReadOnly -Value $false
    Move-Item -LiteralPath $backup -Destination $table -Force
}
foreach ($name in 'xinput1_3.dll', 'psychopadx.log') {
    $path = Join-Path $GameDir $name
    if (Test-Path -LiteralPath $path) { Remove-Item -LiteralPath $path }
}
Write-Output "Removed from $GameDir"
