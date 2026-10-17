param([Parameter(Mandatory=$true)][string]$GameDirectory,[string]$PythonExecutable='python',[string]$DragonFFDirectory,[string]$OutputDirectory=(Join-Path $env:LOCALAPPDATA 'GroveCraft'))
$ErrorActionPreference='Stop'
$packageRoot=$PSScriptRoot
$expected='aa07fcdc8723a2831f77a35491dea6cdd58f6733e3ac69a10ddf5cdf95034fe3'
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
if((Get-FileHash -LiteralPath (Join-Path $game 'gta-sa.exe')).Hash.ToLowerInvariant() -ne $expected){throw 'Unsupported GTA executable. This alpha supports only the documented NewSteam R2 LV build; no game conversion is performed.'}
if(Get-Process gta-sa,gta_sa -ErrorAction SilentlyContinue){throw 'Close GTA before preparing the world cache.'}
$cache=[IO.Path]::GetFullPath($OutputDirectory)
New-Item -ItemType Directory -Force -Path $cache|Out-Null
if(!$DragonFFDirectory){$DragonFFDirectory=Join-Path $cache 'dependencies/DragonFF'}
$pin='b3bd7aa65979faa125555c794fc483980080dbff'
if(!(Test-Path -LiteralPath $DragonFFDirectory)){
    New-Item -ItemType Directory -Force -Path (Split-Path $DragonFFDirectory -Parent)|Out-Null
    & git clone --no-checkout https://github.com/Parik27/DragonFF.git $DragonFFDirectory
    if($LASTEXITCODE -ne 0){throw 'Could not clone the official DragonFF reader.'}
    & git -C $DragonFFDirectory checkout --detach $pin
    if($LASTEXITCODE -ne 0){throw 'Could not select the pinned DragonFF revision.'}
}
$actual=& git -C $DragonFFDirectory rev-parse HEAD
if($LASTEXITCODE -ne 0 -or $actual.Trim() -ne $pin){throw 'DragonFF checkout does not match the pinned revision.'}
$changes=& git -C $DragonFFDirectory status --porcelain --untracked-files=no
if($LASTEXITCODE -ne 0 -or $changes){throw 'DragonFF has local changes; preserve that checkout and select a clean one.'}
$venv=Join-Path $cache 'tools/python'
$python=Join-Path $venv 'Scripts/python.exe'
if(!(Test-Path -LiteralPath $python)){
    & $PythonExecutable -m venv $venv
    if($LASTEXITCODE -ne 0){throw 'Python with venv support is required. Select it with -PythonExecutable.'}
}
& $python -m pip install --disable-pip-version-check 'numpy==2.5.3'
if($LASTEXITCODE -ne 0){throw 'Could not install NumPy in the private GroveCraft environment.'}
& $python (Join-Path $packageRoot 'tools/build-world-surface-cache.py') --game-directory $game --output-directory $cache --dragonff $DragonFFDirectory
if($LASTEXITCODE -ne 0){throw 'Surface cache generation failed; GTA files were only read.'}
Write-Host "Private map cache prepared: $cache. Do not upload surface.bin or manifest.json."
