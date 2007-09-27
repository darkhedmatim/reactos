echo off
copy uniata.sys "%SystemRoot%\System32\drivers"
start /wait regedit -s uniata_w2k.reg
echo Universal ATA driver installed
echo please reboot your computer to complete installation
pause
