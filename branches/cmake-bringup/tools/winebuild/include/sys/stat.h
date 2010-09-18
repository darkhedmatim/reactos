#ifndef _INC_STAT
#define _INC_STAT

#include <sys/types.h>

#ifndef _STAT_DEFINED
#define _STAT_DEFINED

#pragma pack(push,_CRT_PACKING)
  struct stat {
    _dev_t st_dev;
    _ino_t st_ino;
    unsigned short st_mode;
    short st_nlink;
    short st_uid;
    short st_gid;
    _dev_t st_rdev;
    _off_t st_size;
    time_t st_atime;
    time_t st_mtime;
    time_t st_ctime;
  };
#pragma pack(pop)

#endif /* !_STAT_DEFINED */

#define S_IFMT 0xF000
#define S_IFDIR 0x4000
#define S_IFIFO 0x1000
#define S_IFCHR 0x2000
#define S_IFBLK 0x3000
#define S_IFREG 0x8000

#define	S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define	S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define	S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define	S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define	S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)

  _CRTIMP int __cdecl stat(const char *_Filename,struct stat *_Stat);
  _CRTIMP int __cdecl fstat(int _Desc,struct stat *_Stat);

#endif
