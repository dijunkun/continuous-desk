param(
  [Parameter(Mandatory)]
  [ValidateSet('PrepareBinaries', 'RestoreBinaries', 'VerifyInstaller')]
  [string]$Operation,
  [Parameter(Mandatory)] [string]$Artifact,
  [string]$Directory,
  [ValidateSet('test-signing', 'release-signing')] [string]$SigningPolicy
)

. "$PSScriptRoot/signpath-common.ps1"

if ($Operation -ne 'PrepareBinaries' -and !$SigningPolicy) {
  throw 'SigningPolicy is required when verifying signed artifacts.'
}
if ($Operation -eq 'VerifyInstaller') {
  Assert-CrossDeskSignature -Path $Artifact -SigningPolicy $SigningPolicy
  return
}
if (!$Directory -or !(Test-Path -LiteralPath $Directory -PathType Container)) {
  throw "Binaries directory does not exist: $Directory"
}

$files = @(Get-CrossDeskSigningFiles)
$temporaryDirectory = Join-Path ([IO.Path]::GetTempPath()) "crossdesk-signing-$([guid]::NewGuid())"
New-Item -ItemType Directory -Path $temporaryDirectory | Out-Null
try {
  if ($Operation -eq 'PrepareBinaries') {
    foreach ($name in $files) {
      $source = Join-Path $Directory $name
      if (!(Test-Path -LiteralPath $source -PathType Leaf)) {
        throw "Missing CrossDesk binary: $source"
      }
      Copy-Item -LiteralPath $source -Destination (Join-Path $temporaryDirectory $name)
    }
    $artifactPath = [IO.Path]::GetFullPath($Artifact)
    New-Item -ItemType Directory -Path (Split-Path -Parent $artifactPath) -Force | Out-Null
    Compress-Archive -Path "$temporaryDirectory/*" -DestinationPath $artifactPath -Force
    return
  }

  # Inspect the ZIP before extraction: reject missing, duplicate or unexpected
  # entries (including paths) so only the four intended files can be replaced.
  $archive = [IO.Compression.ZipFile]::OpenRead([IO.Path]::GetFullPath($Artifact))
  try {
    $entries = @($archive.Entries | ForEach-Object { $_.FullName })
    $uniqueEntries = @($entries | Sort-Object -Unique)
    if ($entries.Count -ne $files.Count -or $uniqueEntries.Count -ne $files.Count -or
        @(Compare-Object $files $entries).Count -ne 0) {
      throw "Signed archive must contain exactly: $($files -join ', ')"
    }
  } finally {
    $archive.Dispose()
  }
  Expand-Archive -LiteralPath $Artifact -DestinationPath $temporaryDirectory

  # Verify every file before replacing any build output.
  foreach ($name in $files) {
    Assert-CrossDeskSignature -Path (Join-Path $temporaryDirectory $name) -SigningPolicy $SigningPolicy
  }
  foreach ($name in $files) {
    Copy-Item -LiteralPath (Join-Path $temporaryDirectory $name) -Destination (Join-Path $Directory $name) -Force
  }
} finally {
  Remove-Item -LiteralPath $temporaryDirectory -Recurse -Force
}
