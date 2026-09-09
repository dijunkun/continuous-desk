$ErrorActionPreference = 'Stop'

function Get-CrossDeskSigningFiles {
  # Only binaries built from this project's sources. Never include third-party DLLs.
  return @('crossdesk.exe', 'crossdesk_service.exe', 'crossdesk_session_helper.exe', 'wgc_plugin.dll')
}

function Find-CrossDeskSignTool {
  $command = Get-Command signtool.exe -ErrorAction SilentlyContinue
  if ($command) {
    return $command.Source
  }
  $sdk = Join-Path ${env:ProgramFiles(x86)} 'Windows Kits/10/bin'
  $tool = Get-ChildItem "$sdk/*/x64/signtool.exe" |
    Sort-Object { [version]$_.Directory.Parent.Name } |
    Select-Object -Last 1
  if (!$tool) {
    throw 'SignTool from the Windows SDK is required to verify release signatures.'
  }
  return $tool.FullName
}

function Assert-CrossDeskSignature {
  param(
    [Parameter(Mandatory)] [string]$Path,
    [Parameter(Mandatory)] [ValidateSet('test-signing', 'release-signing')] [string]$SigningPolicy
  )

  if (!(Test-Path -LiteralPath $Path -PathType Leaf)) {
    throw "Missing signed file: $Path"
  }
  $signature = Get-AuthenticodeSignature -LiteralPath $Path
  if (!$signature.SignerCertificate) {
    throw "File has no Authenticode signing certificate: $Path"
  }
  Write-Host "Signature: $Path | $($signature.Status) | $($signature.SignerCertificate.Subject)"
  if ($SigningPolicy -eq 'test-signing') {
    # Self-signed test certificates are not trusted by stock GitHub runners.
    # HashMismatch, NotSigned, UnknownError, etc. must still stop the build.
    if ($signature.Status -notin @('Valid', 'NotTrusted')) {
      throw "Invalid test signature on ${Path}: $($signature.Status)"
    }
    return
  }

  if ($signature.Status -ne 'Valid') {
    throw "Release signature is not trusted on ${Path}: $($signature.Status)"
  }
  if (!$signature.TimeStamperCertificate) {
    throw "Release signature has no timestamp: $Path"
  }
  $signTool = Find-CrossDeskSignTool
  & $signTool verify /pa /all /v $Path
  if ($LASTEXITCODE -ne 0) {
    throw "SignTool verification failed for $Path (exit $LASTEXITCODE)."
  }
}
