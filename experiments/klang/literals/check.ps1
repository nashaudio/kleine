# Run from the repository root in an x64 Visual Studio developer PowerShell.
param(
    [string]$Compiler = 'cl.exe',
    [string]$Output = 'build/literals/msvc',
    [switch]$SkipApplication
)
$ErrorActionPreference = 'Stop'
New-Item -ItemType Directory -Force $Output | Out-Null
$flags = @('/nologo', '/std:c++17', '/EHsc', '/O2', '/DNOMINMAX', '/W0', '/Iinclude')

function Compile([string]$Name, [string[]]$Arguments, [bool]$Reject = $false) {
    # Windows PowerShell treats native stderr as errors even for expected diagnostics.
    $ErrorActionPreference = 'Continue'
    & $Compiler @flags @Arguments *> "$Output/$Name.log"
    $result = $LASTEXITCODE
    $ErrorActionPreference = 'Stop'
    if (($Reject -and $result -eq 0) -or (!$Reject -and $result -ne 0)) {
        Get-Content "$Output/$Name.log" -Tail 40
        throw "Unexpected compiler result for ${Name}: $result"
    }
    if ($Reject) {
        Write-Output "$Name rejected as expected (see log)."
    } else {
        Write-Output "$Name compiled."
    }
}

Compile 'probe' @('experiments/klang/literals/probe.cpp', "/Fo$Output/probe.obj", "/Fe$Output/probe.exe")
& "$Output/probe.exe"
if ($LASTEXITCODE -ne 0) { throw 'Literal probe failed' }

Compile 'codegen' @('/c', '/FA', "/Fa$Output/codegen.asm", 'experiments/klang/literals/codegen.cpp', "/Fo$Output/codegen.obj")

foreach ($case in @('F_SUFFIX', 'NO_IMPORT', 'MIXED_MAX', 'TYPED_ROUTE', 'OUTPUT_SCALE')) {
    Compile $case @('/c', "/DREJECT_$case", 'experiments/klang/literals/rejections.cpp', "/Fo$Output/$case.obj") $true
}

# Verify the unchanged application's source with the suffixes in scope.
if (!$SkipApplication) {
    Compile 'compatibility' @('/c', '/MD', '/DNDEBUG', '/FIexperiments/klang/literals/compatibility.h', 'kleine.cpp', "/Fo$Output/kleine.obj")
}
