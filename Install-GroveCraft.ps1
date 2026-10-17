param(
    [Parameter(Mandatory=$true)][string]$GameDirectory,
    [string]$MinecraftDirectory=(Join-Path $env:APPDATA '.minecraft-grovecraft'),
    [string]$AsiLoaderPath,[string]$SilentPatchDirectory,[string]$FabricApiJar,
    [string]$PythonExecutable='python',[string]$DragonFFDirectory,
    [switch]$SkipWorldCache,[switch]$PlanOnly
)
$ErrorActionPreference='Stop'
$package=$PSScriptRoot
$game=(Resolve-Path -LiteralPath $GameDirectory).Path
$minecraft=[IO.Path]::GetFullPath($MinecraftDirectory)
$state=Join-Path $env:LOCALAPPDATA 'GroveCraft'
$receiptPath=Join-Path $state 'installation.json'
$expectedGame='aa07fcdc8723a2831f77a35491dea6cdd58f6733e3ac69a10ddf5cdf95034fe3'
function Hash([string]$path){return (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
function Silent-Bytes([string]$path){
    $original='908e1d12e36ba809a8ce584be0bd29239349968259a5de593f9b0cc71dbb352b'
    $corrected='bb5e6d9e3bb3cae5eaf131a0902f81ffb128b33ebdd97949d1af4a3a6d516c62'
    $hash=Hash $path;$data=[IO.File]::ReadAllBytes($path)
    if($hash -eq $original){
        $old=[Text.Encoding]::ASCII.GetBytes('50 68 ? ? ? ? A3'+[char]0)
        $new=[Text.Encoding]::ASCII.GetBytes('5068????A3????E8'+[char]0)
        if($old.Length -ne $new.Length -or [Text.Encoding]::ASCII.GetString($data,0x5e0a4,$old.Length) -ne [Text.Encoding]::ASCII.GetString($old)){throw 'SilentPatch compatibility pattern differs.'}
        [Array]::Copy($new,0,$data,0x5e0a4,$new.Length)
    }elseif($hash -ne $corrected){throw 'Unsupported SilentPatch source.'}
    $digest=[Security.Cryptography.SHA256]::Create()
    try{$actual=([BitConverter]::ToString($digest.ComputeHash($data))).Replace('-','').ToLowerInvariant()}finally{$digest.Dispose()}
    if($actual -ne $corrected){throw 'SilentPatch compatibility conversion failed verification.'}
    return ,$data
}
function Write-Json([string]$path,$value){[IO.File]::WriteAllText($path,($value|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))}
function Assert-Closed {
    if(Get-Process gta-sa,gta_sa,MinecraftLauncher -ErrorAction SilentlyContinue){throw 'Close GTA and the Minecraft Launcher before installation.'}
    if(Get-Process java,javaw -ErrorAction SilentlyContinue){throw 'Close Java games/tools before installation so their files are not in use.'}
}
Assert-Closed
if((Hash (Join-Path $game 'gta-sa.exe')) -ne $expectedGame){throw 'Unsupported GTA build. Read README.md: this alpha supports only NewSteam R2 LV with the documented SHA-256. No downgrade or executable replacement is provided.'}
if(Test-Path -LiteralPath $receiptPath){throw "An installation receipt already exists: $receiptPath. Verify/uninstall that installation first; saves are preserved."}
if(Test-Path -LiteralPath (Join-Path $state 'installed-v1.txt')){throw 'Existing GroveCraft settings require review before a new installation.'}
$manifest=Get-Content -LiteralPath (Join-Path $package 'SHA256SUMS.json') -Raw|ConvertFrom-Json
foreach($entry in $manifest.files.PSObject.Properties){
    if($entry.Name -match '(^[/\\]|(^|[/\\])\.\.([/\\]|$)|:)'){throw 'Unsafe package manifest path.'}
    $file=Join-Path $package $entry.Name
    if(!(Test-Path -LiteralPath $file -PathType Leaf) -or (Hash $file) -ne $entry.Value){throw "Damaged package file: $($entry.Name)"}
}
$rows=New-Object Collections.Generic.List[object]
function Add-File([string]$source,[string]$destination,[string]$expected,[switch]$SilentCompatibility){
    if(!(Test-Path -LiteralPath $source -PathType Leaf)){throw "Missing required file: $source"}
    $hash=Hash $source
    if($SilentCompatibility){$null=Silent-Bytes $source;$hash='bb5e6d9e3bb3cae5eaf131a0902f81ffb128b33ebdd97949d1af4a3a6d516c62'}
    if($expected -and $hash -ne $expected){throw "Dependency does not match the tested version: $source"}
    $exists=Test-Path -LiteralPath $destination
    $previous=if($exists){Hash $destination}else{$null}
    $replace=$SilentCompatibility -and $exists -and $previous -eq '908e1d12e36ba809a8ce584be0bd29239349968259a5de593f9b0cc71dbb352b'
    if($exists -and $previous -ne $hash -and !$replace){throw "Refusing to replace an existing file: $destination"}
    $rows.Add([pscustomobject]@{Source=$source;Path=$destination;SHA256=$hash;Created=(!$exists);Replaced=[bool]$replace;OriginalSHA256=$previous;BackupPath=$null;SilentCompatibility=[bool]$SilentCompatibility})
}
Add-File (Join-Path $package 'native/GroveCraft.asi') (Join-Path $game 'GroveCraft.asi') ''
Add-File (Join-Path $package 'minecraft/grovecraft-guest-0.1.0-alpha.1.jar') (Join-Path $minecraft 'mods/grovecraft-guest-0.1.0-alpha.1.jar') ''
$loader=Join-Path $game 'dinput8.dll'
if(Test-Path -LiteralPath $loader){$AsiLoaderPath=$loader}
if(!$AsiLoaderPath){throw 'Pass -AsiLoaderPath pointing to the x86 Ultimate ASI Loader 9.7.4 dinput8.dll from its official release.'}
Add-File $AsiLoaderPath $loader 'd5a059aa467a7a7127c8f6169f79fa63ff0f55986ee9eb2fd9a281bebf2aa2e6'
$silent=Join-Path $game 'SilentPatchSA.asi'
if(Test-Path -LiteralPath $silent){$silentSource=$silent}else{
    if(!$SilentPatchDirectory){throw 'Pass -SilentPatchDirectory with the extracted official SilentPatchSA 34.1 release.'}
    $silentSource=Join-Path $SilentPatchDirectory 'SilentPatchSA.asi'
}
Add-File $silentSource $silent 'bb5e6d9e3bb3cae5eaf131a0902f81ffb128b33ebdd97949d1af4a3a6d516c62' -SilentCompatibility
$silentIni=Join-Path $game 'SilentPatchSA.ini'
if(!(Test-Path -LiteralPath $silentIni)){
    if(!$SilentPatchDirectory){throw 'SilentPatchSA.ini is missing; pass -SilentPatchDirectory.'}
    Add-File (Join-Path $SilentPatchDirectory 'SilentPatchSA.ini') $silentIni ''
}
if(!$FabricApiJar){throw 'Pass -FabricApiJar with Fabric API 0.161.0+26.3 from its official project.'}
Add-File $FabricApiJar (Join-Path $minecraft 'mods/fabric-api-0.161.0+26.3.jar') '86f16178a3cecc887a85a4cfe9a79d92fa7341d8f39b5951a4d6ad800ab657a6'
$allowed=@($rows|Where-Object {$_.Path.StartsWith((Join-Path $minecraft 'mods'),[StringComparison]::OrdinalIgnoreCase)}|ForEach-Object {[IO.Path]::GetFileName($_.Path)})
if(Test-Path -LiteralPath (Join-Path $minecraft 'mods')){
    $other=Get-ChildItem -LiteralPath (Join-Path $minecraft 'mods') -File|Where-Object {$_.Extension -eq '.jar' -and $_.Name -notin $allowed}
    if($other){throw 'Select a dedicated Minecraft game directory without unrelated mods.'}
}
$plan=[pscustomobject]@{Version='0.1.0-alpha.1';GameDirectory=$game;MinecraftDirectory=$minecraft;StateDirectory=$state;Files=$rows.ToArray();WorldCacheRequired=(!$SkipWorldCache)}
if($PlanOnly){$plan|ConvertTo-Json -Depth 8;return}
New-Item -ItemType Directory -Force -Path $state|Out-Null
$installLock=[IO.File]::Open((Join-Path $state '.install.lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
try{
if(Test-Path -LiteralPath $receiptPath){throw 'Another installation already created a receipt; no game files changed.'}
if(!$SkipWorldCache){
    & (Join-Path $package 'Setup-WorldCache.ps1') -GameDirectory $game -PythonExecutable $PythonExecutable -DragonFFDirectory $DragonFFDirectory -OutputDirectory $state
}
$cacheManifest=Join-Path $state 'manifest.json'
$surface=Join-Path $state 'surface.bin'
if(!(Test-Path -LiteralPath $cacheManifest) -or !(Test-Path -LiteralPath $surface)){throw 'Prepare the private world cache before installing.'}
$cache=Get-Content -LiteralPath $cacheManifest -Raw|ConvertFrom-Json
if(!$cache.private_game_derived_cache -or $cache.file_sha256 -ne (Hash $surface)){throw 'Private world cache failed verification.'}
# Back up native saves/settings before installing either half of the bridge.
$documents=[Environment]::GetFolderPath('MyDocuments')
$nativeSaves=Join-Path $documents 'GTA San Andreas User Files'
$backup=Join-Path $state ('backups/native-before-'+(Get-Date).ToUniversalTime().ToString('yyyyMMdd-HHmmssfff')+'.zip')
if(Test-Path -LiteralPath $nativeSaves){
    New-Item -ItemType Directory -Force -Path (Split-Path $backup -Parent)|Out-Null
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    [IO.Compression.ZipFile]::CreateFromDirectory($nativeSaves,$backup)
}else{$backup=$null}
foreach($row in $rows){if($row.Replaced){
    $row.BackupPath=Join-Path $state ('backups/SilentPatch-before-'+(Get-Date).ToUniversalTime().ToString('yyyyMMdd-HHmmssfff')+'.asi')
    New-Item -ItemType Directory -Force -Path (Split-Path $row.BackupPath -Parent)|Out-Null
    Copy-Item -LiteralPath $row.Path -Destination $row.BackupPath
    if((Hash $row.BackupPath) -ne $row.OriginalSHA256){throw 'Original plugin backup failed.'}
}}
$receipt=[ordered]@{Schema='GroveCraftInstall1';Version=$plan.Version;State='installing';GameDirectory=$game;MinecraftDirectory=$minecraft;Backup=$backup;Files=@($rows|Select-Object Path,SHA256,Created,Replaced,OriginalSHA256,BackupPath)}
Write-Json $receiptPath $receipt
try{
    foreach($row in $rows){
        if(!$row.Created -and !$row.Replaced){continue}
        if($row.Replaced -and (Hash $row.Path) -ne $row.OriginalSHA256){throw 'Plugin changed after preflight; original preserved.'}
        New-Item -ItemType Directory -Force -Path (Split-Path $row.Path -Parent)|Out-Null
        if($row.SilentCompatibility){[IO.File]::WriteAllBytes($row.Path,(Silent-Bytes $row.Source))}
        else{Copy-Item -LiteralPath $row.Source -Destination $row.Path}
        if((Hash $row.Path) -ne $row.SHA256){throw "Installed hash mismatch: $($row.Path)"}
    }
    [IO.File]::WriteAllText((Join-Path $state 'installed-v1.txt'),"GroveCraftRelease1`n",(New-Object Text.UTF8Encoding($false)))
    $receipt.State='installed';Write-Json $receiptPath $receipt
}catch{
    foreach($row in $rows){if($row.Created -and (Test-Path -LiteralPath $row.Path) -and (Hash $row.Path) -eq $row.SHA256){Remove-Item -LiteralPath $row.Path}}
    foreach($row in $rows){if($row.Replaced -and (Test-Path -LiteralPath $row.Path) -and (Hash $row.Path) -eq $row.SHA256 -and (Hash $row.BackupPath) -eq $row.OriginalSHA256){Copy-Item -LiteralPath $row.BackupPath -Destination $row.Path}}
    $marker=Join-Path $state 'installed-v1.txt';if(Test-Path -LiteralPath $marker){Remove-Item -LiteralPath $marker}
    $receipt.State='failed-rolled-back';Write-Json $receiptPath $receipt
    throw
}
Write-Host "Installed. In the official Minecraft Launcher create Fabric 26.3 with game directory: $minecraft"
Write-Host 'Add JVM argument --enable-native-access=ALL-UNNAMED, start that profile, then start GTA normally. Read README.md before starting a campaign.'
}finally{$installLock.Dispose()}
