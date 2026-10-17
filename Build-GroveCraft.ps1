param([Parameter(Mandatory=$true)][string]$CompilerDirectory,[string]$JavaHome=$env:JAVA_HOME,[string]$PythonExecutable='python',[switch]$Offline)
$ErrorActionPreference='Stop'
$root=$PSScriptRoot
if(!$JavaHome -or !(Test-Path -LiteralPath (Join-Path $JavaHome 'bin/java.exe'))){throw 'Pass -JavaHome with a Java 25 installation.'}
$env:JAVA_HOME=$JavaHome
function Get-PinnedSource([string]$name,[string]$url,[string]$pin){
    $target=Join-Path $root "vendor/$name"
    if(!(Test-Path -LiteralPath $target)){
        New-Item -ItemType Directory -Force -Path (Split-Path $target -Parent)|Out-Null
        & git clone --no-checkout $url $target
        if($LASTEXITCODE -ne 0){throw "Clone failed: $name"}
        & git -C $target checkout --detach $pin
        if($LASTEXITCODE -ne 0){throw "Checkout failed: $name"}
    }
    $actual=& git -C $target rev-parse HEAD
    if($LASTEXITCODE -ne 0 -or $actual.Trim() -ne $pin){throw "Unexpected revision: $name"}
    $changes=& git -C $target status --porcelain --untracked-files=no
    if($LASTEXITCODE -ne 0 -or $changes){throw "Preserve modified dependency first: $name"}
}
Get-PinnedSource 'libertycraft' 'https://github.com/mrborghini/libertycraft' '0fe0f2c9aae2ba974cb077e282b091a38f1ca48f'
Get-PinnedSource 'minhook' 'https://github.com/TsudaKageyu/minhook' '8af6b4acae5a9388fd742b56fa79ece89d96f823'
if(!(Test-Path -LiteralPath (Join-Path $root 'build/guest'))){
    & $PythonExecutable (Join-Path $root 'tools/prepare_guest.py')
}else{& $PythonExecutable (Join-Path $root 'tools/sync-guest.py')}
if($LASTEXITCODE -ne 0){throw 'Guest preparation failed.'}
& (Join-Path $root 'tools/build-host.ps1') -CompilerDirectory $CompilerDirectory -Release
$gradleArgs=@('-p',(Join-Path $root 'build/guest'),'test','build','-Pversion=0.1.0-alpha.1','--no-daemon','--max-workers=2','--console=plain')
if($Offline){$gradleArgs+='--offline'}
& (Join-Path $root 'build/guest/gradlew.bat') @gradleArgs
if($LASTEXITCODE -ne 0){throw 'Guest tests/build failed.'}
Write-Host 'Built build/host/GroveCraft.asi and build/guest/build/libs/grovecraft-guest-0.1.0-alpha.1.jar. Game files are not included.'
