param([string]$CompilerDirectory,[switch]$Release)
$ErrorActionPreference='Stop'
$root = Split-Path $PSScriptRoot -Parent
if(!$CompilerDirectory){$CompilerDirectory=Join-Path $root 'runtime\llvm-mingw-20261006-ucrt-x86_64\bin'}
$clang = Join-Path $CompilerDirectory 'i686-w64-mingw32-clang.exe'
$cxx = Join-Path $CompilerDirectory 'i686-w64-mingw32-clang++.exe'
$groveCompileFlags=@()
$groveLinkFlags=@()
if($Release){$groveCompileFlags=@('-DNDEBUG',"-ffile-prefix-map=$root=.","-ffile-prefix-map=$($root.Replace('\','/'))=.");$groveLinkFlags=@('-Wl,--no-insert-timestamp')}
$build = Join-Path $root 'build\host'
New-Item -ItemType Directory -Force $build | Out-Null
$objects = @()
foreach($source in @('buffer.c','hook.c','trampoline.c','hde\hde32.c')) {
    $object = Join-Path $build (([IO.Path]::GetFileNameWithoutExtension($source))+'.o')
    & $clang @groveCompileFlags -O2 -c (Join-Path $root "vendor\minhook\src\$source") -o $object
    if($LASTEXITCODE -ne 0){throw "Compilation failed: $source"}
    $objects += $object
}
& $cxx @groveCompileFlags @groveLinkFlags -std=c++17 -O2 -Wall -Wextra -shared -static -I (Join-Path $root 'vendor\minhook\include') -I (Join-Path $root 'vendor\libertycraft\protocol') (Join-Path $root 'src\bridge.cpp') @objects -ld3d9 -luser32 -lgdi32 -ladvapi32 -lole32 -o (Join-Path $build 'GroveCraft.asi')
if($LASTEXITCODE -ne 0){throw 'Host compilation failed'}
Get-FileHash (Join-Path $build 'GroveCraft.asi') -Algorithm SHA256
