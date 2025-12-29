# PowerShell profile for DigitShowDST development
# This profile is loaded automatically by VS Code terminal

# Load VS Code PowerShell integration if running inside VS Code
if ($env:TERM_PROGRAM -eq "vscode") {
    if (Get-Command code -ErrorAction SilentlyContinue) {
        $integrationPath = code --locate-shell-integration-path pwsh
        if ($integrationPath -and (Test-Path $integrationPath)) {
            . $integrationPath
        }
        else {
            Write-Verbose "VS Code shell integration path not found: $integrationPath"
        }
    }
    else {
        Write-Verbose "'code' command not found; skipping VS Code shell integration."
    }
}

# Initialize Visual Studio Developer Environment
& 'C:\Program Files (x86)\Microsoft Visual Studio\18\BuildTools\Common7\Tools\Launch-VsDevShell.ps1' -SkipAutomaticLocation
