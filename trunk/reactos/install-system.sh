mkdir -p $1/reactos
mkdir -p $1/reactos/system32
mkdir -p $1/reactos/system32/drivers
mkdir -p $1/reactos/bin
cp loaders/dos/loadros.com $1
cp ntoskrnl/ntoskrnl.exe $1
cp services/fs/vfat/vfatfs.sys $1
cp services/dd/ide/ide.sys $1
cp services/dd/keyboard/keyboard.sys $1/reactos/system32/drivers
cp services/dd/blue/blue.sys $1/reactos/system32/drivers
cp services/dd/vga/vgamp.sys $1/reactos/system32/drivers
cp services/dd/vidport/vidport.sys $1/reactos/system32/drivers
