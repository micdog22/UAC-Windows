# Exemplo de execução remota (PowerShell Remoting/WinRM)
param(
  [string[]]$Computers,
  [ValidateSet('NeverNotify','FullDisable','Rollback')]
  [string]$Mode = 'NeverNotify'
)

$arg = switch ($Mode) {
  'NeverNotify' { '--never-notify' }
  'FullDisable' { '--full-disable' }
  'Rollback'    { '--rollback' }
}

Invoke-Command -ComputerName $Computers -ScriptBlock {
  param($arg)
  Start-Process -FilePath "$env:SystemDrive\uacctl\uacctl.exe" -ArgumentList $arg -Wait
} -ArgumentList $arg
