Mke2fs for Winnt/2k/xp V0.02
    Format a file or volume to ext2 format file system.

Author:   Matt <mattwu@163.com>
Homepage: http://ext2.yeah.net

Usage: Mke2fs [-c|-t|-l filename] [-b block-size] [-f fragment-size]
        [-i bytes-per-inode]  [-N number-of-inodes]
        [-m reserved-blocks-percentage] [-o creator-os] [-g blocks-per-group]
        [-L volume-label] [-M last-mounted-directory] [-O feature[,...]]
        [-r fs-revision] [-R raid_opts] [-qvSV] volumename [blocks-count]

Example:

1, Format a file (ext2.img) to ext2 file system.
   Warnings: Full path name of the file is needed.

   Mke2fs -b 1024 c:\temp\ext2.img  [or]
   Mke2fs -b 1024 \??\c:\temp\ext2.img

2, Format volume x: to ext2 file system.
   Mke2fs -b 4096 -L Lable x:    [or]
   Mke2fs -b 4096 -L Lable \??\x:
