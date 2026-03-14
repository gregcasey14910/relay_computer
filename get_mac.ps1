$port = New-Object System.IO.Ports.SerialPort("COM4", 115200)
$port.ReadTimeout = 500
$port.Open()
$output = ""
$deadline = (Get-Date).AddSeconds(8)
while ((Get-Date) -lt $deadline) {
    try {
        $line = $port.ReadLine()
        $output += $line + "`n"
        if ($line -match "MAC Address") { break }
    } catch {}
}
$port.Close()
Write-Output $output
