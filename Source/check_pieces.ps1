Add-Type -AssemblyName System.Drawing
Get-ChildItem -Path "D:\Project\app\Ludo-Family\ludo-native\assets\images\pieces\piece-*.png" | ForEach-Object {
    $img = [System.Drawing.Image]::FromFile($_.FullName)
    Write-Host "$($_.Name): $($img.Width)x$($img.Height)"
    $img.Dispose()
}
