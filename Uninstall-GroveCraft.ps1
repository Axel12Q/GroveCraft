param([switch]$VerifyOnly)
$ErrorActionPreference='Stop'
if(Get-Process gta-sa,gta_sa,java,javaw -ErrorAction SilentlyContinue){throw 'Close GTA and Java applications before uninstalling.'}
$state=Join-Path $env:LOCALAPPDATA 'GroveCraft'
$path=Join-Path $state 'installation.json'
$installLock=[IO.File]::Open((Join-Path $state '.install.lock'),[IO.FileMode]::OpenOrCreate,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
try{
$record=Get-Content -LiteralPath $path -Raw|ConvertFrom-Json
if($record.Schema -ne 'GroveCraftInstall1' -or $record.State -notin @('installed','installing','failed-rolled-back')){throw 'Unexpected installation receipt; no files changed.'}
$sep=[IO.Path]::DirectorySeparatorChar
$gameRoot=[IO.Path]::GetFullPath($record.GameDirectory).TrimEnd($sep)+$sep
$modsRoot=[IO.Path]::GetFullPath((Join-Path $record.MinecraftDirectory 'mods')).TrimEnd($sep)+$sep
$roots=@($gameRoot,$modsRoot)
$allowed=@('GroveCraft.asi','dinput8.dll','SilentPatchSA.asi','SilentPatchSA.ini','grovecraft-guest-0.1.0-alpha.1.jar','fabric-api-0.161.0+26.3.jar')
foreach($file in $record.Files){
    $full=[IO.Path]::GetFullPath($file.Path)
    if([IO.Path]::GetFileName($full) -notin $allowed -or $file.Created -isnot [bool] -or $file.SHA256 -notmatch '^[a-f0-9]{64}$'){throw 'Invalid file entry in receipt.'}
    if(!($roots|Where-Object {$full.StartsWith($_,[StringComparison]::OrdinalIgnoreCase)})){throw 'Receipt points outside the recorded installation.'}
    if(Test-Path -LiteralPath $full){if((Get-FileHash -LiteralPath $full).Hash.ToLowerInvariant() -ne $file.SHA256){throw "Changed file preserved: $full. Review before uninstalling."}}
    if($file.Replaced){
        $backup=[IO.Path]::GetFullPath($file.BackupPath)
        if([IO.Path]::GetFileName($full) -ne 'SilentPatchSA.asi' -or !$backup.StartsWith(([IO.Path]::GetFullPath((Join-Path $state 'backups')).TrimEnd($sep)+$sep),[StringComparison]::OrdinalIgnoreCase) -or (Get-FileHash -LiteralPath $backup).Hash.ToLowerInvariant() -ne $file.OriginalSHA256){throw 'Original SilentPatch backup cannot be verified; no files removed.'}
    }
}
if($VerifyOnly){Write-Host 'Uninstall paths and hashes verified; no files changed.';return}
foreach($file in $record.Files){if($file.Created -and (Test-Path -LiteralPath $file.Path)){Remove-Item -LiteralPath $file.Path}}
foreach($file in $record.Files){if($file.Replaced){Copy-Item -LiteralPath $file.BackupPath -Destination $file.Path}}
$marker=Join-Path $state 'installed-v1.txt'
if(Test-Path -LiteralPath $marker){Remove-Item -LiteralPath $marker}
$record.State='uninstalled';$archived=Join-Path $state ('uninstalled-'+(Get-Date).ToUniversalTime().ToString('yyyyMMdd-HHmmssfff')+'.json')
[IO.File]::WriteAllText($archived,($record|ConvertTo-Json -Depth 8),(New-Object Text.UTF8Encoding($false)))
Remove-Item -LiteralPath $path
Write-Host 'Removed only files created by this installation. Campaigns, Minecraft worlds, native saves, backups and generated cache remain preserved.'
}finally{$installLock.Dispose()}
