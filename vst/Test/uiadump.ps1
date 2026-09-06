# Launch the standalone build and walk its UI Automation tree, which is what a
# screen reader on Windows actually reads. Prints every element that is a
# keyboard stop, with the control type and name it would be announced by.
$ErrorActionPreference = "Stop"
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes

$exe = "C:\Users\turtl\Documents\GitHub\vocalwriter\vst\build\VocalWriterVoice_artefacts\Release\Standalone\VocalWriter Voice.exe"
$proc = Start-Process -FilePath $exe -PassThru
Start-Sleep -Seconds 6

try {
    $root = [System.Windows.Automation.AutomationElement]::RootElement
    $cond = New-Object System.Windows.Automation.PropertyCondition(
        [System.Windows.Automation.AutomationElement]::ProcessIdProperty, $proc.Id)
    $win = $root.FindFirst([System.Windows.Automation.TreeScope]::Children, $cond)
    if (-not $win) { Write-Output "NO WINDOW FOUND"; exit 1 }
    Write-Output ("window: '" + $win.Current.Name + "'")

    $all = $win.FindAll([System.Windows.Automation.TreeScope]::Descendants,
        [System.Windows.Automation.Condition]::TrueCondition)
    Write-Output ("elements: " + $all.Count)
    Write-Output ""
    Write-Output "control type          keyboard?  name / value"
    Write-Output "--------------------  ---------  ------------------------------"
    foreach ($e in $all) {
        $c = $e.Current
        $type = $c.ControlType.ProgrammaticName -replace 'ControlType\.',''
        $kb = if ($c.IsKeyboardFocusable) { "FOCUS   " } else { "        " }
        $val = ""
        $vp = $null
        if ($e.TryGetCurrentPattern([System.Windows.Automation.ValuePattern]::Pattern, [ref]$vp)) {
            $val = " = '" + $vp.Current.Value + "'"
        }
        $rp = $null
        if ($e.TryGetCurrentPattern([System.Windows.Automation.RangeValuePattern]::Pattern, [ref]$rp)) {
            $val = " = " + $rp.Current.Value
        }
        Write-Output ("{0,-20}  {1}  '{2}'{3}" -f $type, $kb, $c.Name, $val)
    }
}
finally {
    if (-not $proc.HasExited) { $proc.Kill() }
}
