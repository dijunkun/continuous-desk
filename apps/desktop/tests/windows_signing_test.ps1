# Portable tests for artifact handling and signature validation decisions.
# Get-AuthenticodeSignature and SignTool are simulated; CI validates real files.
$ErrorActionPreference = 'Stop'
$scripts = Join-Path $PSScriptRoot '../scripts/windows'
. "$scripts/signpath-common.ps1"
$artifactScript = Join-Path $scripts 'signpath-artifact.ps1'
$testDirectory = Join-Path ([IO.Path]::GetTempPath()) "crossdesk-signing-test-$([guid]::NewGuid())"
New-Item -ItemType Directory -Path $testDirectory | Out-Null
$checks = 0

function Assert-True([bool]$Condition, [string]$Message) {
  if (!$Condition) { throw $Message }
  $script:checks++
}

function Assert-Throws([scriptblock]$Action, [string]$Expected) {
  try { & $Action } catch {
    if ($_.Exception.Message -notlike "*$Expected*") { throw }
    $script:checks++
    return
  }
  throw "Expected an error containing: $Expected"
}

function New-TestZip([string]$Path, [string[]]$Names) {
  $zip = [IO.Compression.ZipFile]::Open($Path, [IO.Compression.ZipArchiveMode]::Create)
  try {
    foreach ($name in $Names) {
      $writer = [IO.StreamWriter]::new($zip.CreateEntry($name).Open())
      try { $writer.Write("signed $name") } finally { $writer.Dispose() }
    }
  } finally { $zip.Dispose() }
}

$mockStatus = 'NotTrusted'
$mockBadFile = ''
$mockCertificate = [pscustomobject]@{ Subject = "CN=Test certificate for 'CrossDesk [OSS]'" }
$mockTimestamp = $null
function Get-AuthenticodeSignature {
  param([string]$LiteralPath)
  $status = if ([IO.Path]::GetFileName($LiteralPath) -eq $mockBadFile) { 'HashMismatch' } else { $mockStatus }
  return [pscustomobject]@{
    Status = $status
    SignerCertificate = $mockCertificate
    TimeStamperCertificate = $mockTimestamp
  }
}

try {
  $files = @(Get-CrossDeskSigningFiles)
  $build = Join-Path $testDirectory 'build'
  New-Item -ItemType Directory -Path $build | Out-Null
  foreach ($name in $files) { Set-Content -LiteralPath (Join-Path $build $name) -Value "original $name" }
  Set-Content -LiteralPath (Join-Path $build 'third-party.dll') -Value 'upstream'
  $unsigned = Join-Path $testDirectory 'unsigned.zip'
  & $artifactScript -Operation PrepareBinaries -Directory $build -Artifact $unsigned
  $zip = [IO.Compression.ZipFile]::OpenRead($unsigned)
  try {
    $entries = @($zip.Entries | ForEach-Object { $_.FullName })
    Assert-True ($entries.Count -eq 4 -and @(Compare-Object $files $entries).Count -eq 0) 'Signing input must include only the four project binaries.'
  } finally { $zip.Dispose() }

  [xml]$config = Get-Content -Raw (Join-Path $scripts 'signpath-binaries.xml')
  $configured = @($config.SelectNodes('//*[local-name()="include"]') | ForEach-Object { $_.path })
  Assert-True (@(Compare-Object $files $configured).Count -eq 0) 'SignPath XML must cover exactly the files in the submitted ZIP.'

  Remove-Item -LiteralPath (Join-Path $build 'wgc_plugin.dll')
  Assert-Throws { & $artifactScript -Operation PrepareBinaries -Directory $build -Artifact (Join-Path $testDirectory 'missing.zip') } 'Missing CrossDesk binary'
  Set-Content -LiteralPath (Join-Path $build 'wgc_plugin.dll') -Value 'original wgc_plugin.dll'

  $signed = Join-Path $testDirectory 'signed.zip'
  New-TestZip $signed $files
  $mockBadFile = 'wgc_plugin.dll'
  Assert-Throws { & $artifactScript -Operation RestoreBinaries -Directory $build -Artifact $signed -SigningPolicy test-signing } 'HashMismatch'
  foreach ($name in $files) {
    Assert-True ((Get-Content -Raw (Join-Path $build $name)).Trim() -eq "original $name") 'Failed verification must not replace any output.'
  }
  $mockBadFile = ''
  & $artifactScript -Operation RestoreBinaries -Directory $build -Artifact $signed -SigningPolicy test-signing
  foreach ($name in $files) {
    Assert-True ((Get-Content -Raw (Join-Path $build $name)) -eq "signed $name") 'Verified signed files must replace the unsigned build output.'
  }
  Assert-True ((Get-Content -Raw (Join-Path $build 'third-party.dll')).Trim() -eq 'upstream') 'Third-party libraries must remain untouched.'

  $badArchives = @(
    @($files[0], $files[1], $files[2]),
    @($files[0], $files[1], $files[2], '../wgc_plugin.dll'),
    @($files[0], $files[1], $files[2], $files[0]),
    @($files[0], $files[1], $files[2], $files[3], 'third-party.dll')
  )
  for ($i = 0; $i -lt $badArchives.Count; $i++) {
    $badZip = Join-Path $testDirectory "bad-$i.zip"
    New-TestZip $badZip $badArchives[$i]
    Assert-Throws { & $artifactScript -Operation RestoreBinaries -Directory $build -Artifact $badZip -SigningPolicy test-signing } 'Signed archive must contain exactly'
  }

  $file = Join-Path $build 'crossdesk.exe'
  foreach ($mockStatus in @('NotSigned', 'HashMismatch', 'UnknownError')) {
    Assert-Throws { Assert-CrossDeskSignature $file test-signing } 'Invalid test signature'
  }
  $mockStatus = 'Valid'
  $mockCertificate = $null
  Assert-Throws { Assert-CrossDeskSignature $file test-signing } 'no Authenticode signing certificate'
  $mockCertificate = [pscustomobject]@{ Subject = 'CN=SignPath Foundation' }
  $mockStatus = 'NotTrusted'
  Assert-Throws { Assert-CrossDeskSignature $file release-signing } 'Release signature is not trusted'
  $mockStatus = 'Valid'
  Assert-Throws { Assert-CrossDeskSignature $file release-signing } 'no timestamp'
  $mockTimestamp = [pscustomobject]@{ Subject = 'CN=Timestamp authority' }

  # Only the real SignTool process is replaced; exercise the actual exit-code check.
  $fakeSignTool = Join-Path $testDirectory 'signtool.ps1'
  function Find-CrossDeskSignTool { return $fakeSignTool }
  Set-Content $fakeSignTool '$global:LASTEXITCODE = 1'
  Assert-Throws { Assert-CrossDeskSignature $file release-signing } 'SignTool verification failed'
  Set-Content $fakeSignTool '$global:LASTEXITCODE = 0'
  Assert-CrossDeskSignature $file release-signing
  $checks++

  Write-Host "Passed $checks Windows signing checks."
} finally {
  Remove-Item -LiteralPath $testDirectory -Recurse -Force
}
