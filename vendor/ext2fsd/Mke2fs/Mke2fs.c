/*
 * PROJECT:          Mke2fs
 * FILE:             Disk.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "Mke2fs.h"

/* DEFINITIONS ***********************************************************/

#define MKE2FS_VERSION "0.04"

#define ZAP_BOOTBLOCK    TRUE

/* GLOBALS ***************************************************************/

int     average_fsize = 8192; /* 8K */
int     reserved_ratio = 5;
bool    noaction = false;
char    *creator_os = "winnt";
char    *volume_label = NULL;
char    *mount_dir = NULL;
char    *program_name = "Mke2fs";
char    *device_name = NULL;
char    *bad_blocks_filename = NULL;
int     cflag = 0;
int     verbose = 0;
int     quiet = 0;
int     force = 0;
int     super_only = 0;
int     journal_size = 0;
ULONG   fs_stride = 0;

BOOLEAN bLocked = FALSE;

/* FUNCTIONS *************************************************************/

#ifdef __cplusplus
extern "C" {
#endif
    void __declspec (naked) __cdecl _chkesp (void)
    {
	    _asm{
    	    jz	exit_chkesp
	        int	3
        exit_chkesp:
	        ret
        };
    }

#ifdef __cplusplus
}
#endif

static void usage(void)
{
    printf("Usage: Mke2fs [-c|-t|-l volumename] [-b block-size] "
    "[-f fragment-size]\n\t[-i bytes-per-inode] " /*  [-j] [-J journal-options] */
    " [-N number-of-inodes]\n\t[-m reserved-blocks-percentage] "
    "[-o creator-os] [-g blocks-per-group]\n\t[-L volume-label] "
    "[-M last-mounted-directory] [-O feature[,...]]\n\t"
    "[-r fs-revision] [-R raid_opts] [-qvSV] volumename [blocks-count]\n");

    printf("\nExample:\n\n"
           "1, Format a file (ext2.img) to ext2 file system.\n"
           "   Warning: Full path name of the file is needed.\n"
           "   Mke2fs -b 1024 c:\\temp\\ext2.img  [or]\n"
           "   Mke2fs -b 1024 \\??\\c:\\temp\\ext2.img\n\n"
           "2, Format volume x: to ext2 file system.\n"
           "   Mke2fs -b 4096 -L Lable x:    [or] \n"
           "   Mke2fs -b 4096 -L Lable \\??\\x:\n\n"    );

    exit(1);
}

int int_log2(int arg)
{
    int l = 0;

    arg >>= 1;

    while (arg)
    {
        l++;
        arg >>= 1;
    }

    return l;
}

int int_log10(unsigned int arg)
{
    int l;

    for (l=0; arg ; l++)
        arg = arg / 10;

    return l;
}


void proceed_question(void)
{
    char buf[256];
    const char *short_yes = "yY";

    fflush(stdout);
    fflush(stderr);
    printf("Mke2fs:  Proceed anyway? (y,n) ");
    buf[0] = 0;
    fgets(buf, sizeof(buf), stdin);

    if (strchr(short_yes, buf[0]) == 0)
        exit(1);
}

static char default_str[] = "default";

struct mke2fs_defaults {
    const char  *type;
    int     size;
    int     blocksize;
    int     inode_ratio;
} settings[] = {
    { default_str, 0, 4096, 8192 },
    { default_str, 512, 1024, 4096 },
    { default_str, 3, 1024, 8192 },
    { "journal", 0, 4096, 8192 },
    { "news", 0, 4096, 4096 },
    { "largefile", 0, 4096, 1024 * 1024 },
    { "largefile4", 0, 4096, 4096 * 1024 },
    { 0, 0, 0, 0},
};

void set_fs_defaults(const char *fs_type,
                PEXT2_SUPER_BLOCK super,
                int blocksize, int *inode_ratio)
{
    LONGLONG MegaSize;
    struct mke2fs_defaults *p;

    MegaSize = (LONGLONG)(super->s_blocks_count);
    MegaSize *= EXT2_BLOCK_SIZE(super);
    MegaSize /= (1024 * 1024);

    if (!fs_type)
        fs_type = default_str;

    for (p = settings; p->type; p++)
    {
        if ((strcmp(p->type, fs_type) != 0) &&
            (strcmp(p->type, default_str) != 0))
            continue;

        if ((p->size != 0) &&
            (MegaSize > (LONGLONG)p->size))
            continue;

        if (*inode_ratio == 0)
            *inode_ratio = p->inode_ratio;

        if (blocksize == 0)
        {
            super->s_log_frag_size = super->s_log_block_size =
                int_log2(p->blocksize >> EXT2_MIN_BLOCK_LOG_SIZE);
        }
    }

    if (blocksize == 0)
    {
        super->s_blocks_count /= EXT2_BLOCK_SIZE(super) / 1024;
    }
}

bool parse_raid_opts(const char *opts)
{
    char    *buf, *token, *next, *p, *arg;
    int     len;
    int     raid_usage = 0;

    len = strlen(opts);
    buf = (char *)malloc(len+1);
    if (!buf)
    {
        printf("Mke2fs: Couldn't allocate memory to parse raid options!\n");
        return false;
    }

    memcpy(buf, opts, len);
    buf[len] = 0;

    for (token = buf; token && *token; token = next)
    {
        p = strchr(token, ',');
        next = 0;
        if (p)
        {
            *p = 0;
            next = p+1;
        } 
        
        arg = strchr(token, '=');
        if (arg)
        {
            *arg = 0;
            arg++;
        }

        if (strcmp(token, "stride") == 0)
        {
            if (!arg)
            {
                raid_usage++;
                continue;
            }

            fs_stride = strtoul(arg, &p, 0);
            if (*p || (fs_stride == 0))
            {
                printf("Mke2fs: Invalid stride parameter.\n");
                raid_usage++;
                continue;
            }
        }
        else
        {
            raid_usage++;
        }
    }

    if (raid_usage)
    {
        printf("Mke2fs: \nBad raid options specified.\n\n"
            "Raid options are separated by commas, "
            "and may take an argument which\n"
            "\tis set off by an equals ('=') sign.\n\n"
            "Valid raid options are:\n"
            "\tstride=<stride length in blocks>\n\n");
        return false;
    }

    return true;
}   


/*
 *  ParseCmd: Parase the command line of mke2fs
 */

bool parase_cmd(int argc, char *argv[], PEXT2_FILESYS Ext2Sys)
{
    PEXT2_SUPER_BLOCK Ext2Sb = Ext2Sys->ext2_sb;

    int c;
    int     size;
    char    *tmp;
    int     blocksize = 4096;
    int     inode_ratio = 0;
    int     reserved_ratio = 5;
    ULONG   num_inodes = 0;
    bool    retval = TRUE;
    char    *raid_opts = 0;
    const char *    fs_type = 0;
    int     default_features = 1;
    LONGLONG    dev_size;

    memset(Ext2Sb, 0, sizeof(EXT2_SUPER_BLOCK));
    Ext2Sb->s_rev_level = 1;  /* Create revision 1 filesystems now */
    Ext2Sb->s_feature_incompat |= EXT2_FEATURE_INCOMPAT_FILETYPE;
    Ext2Sb->s_feature_ro_compat |= EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER;
    Ext2Sb->s_feature_ro_compat |= EXT2_FEATURE_RO_COMPAT_LARGE_FILE;

    while ((c = getopt (argc, argv,
            "b:cf:g:i:jl:m:no:qr:R:s:tvI:J:ST:FL:M:N:O:V")) != EOF)
    switch (c)
    {
        case 'b':

            blocksize = strtoul(optarg, &tmp, 0);
            
            if ( blocksize < EXT2_MIN_BLOCK_SIZE ||
                 blocksize > 32768 ||
                 *tmp )
            {
                printf("Mke2fs: wrong blocksize: %d ...\n", blocksize);
                return false;
            }

            if (blocksize > EXT2_MAX_BLOCK_SIZE) {
                int yes = 0;
                printf("Mke2fs: Your block size is bigger than Linux default maximum\n"
                       "        block size: %d > %d. Would you like to conintue (Y/N) ? ",
                                blocksize, EXT2_MAX_BLOCK_SIZE);
                yes = getchar();
                if (yes != 'Y' &&  yes != 'y') {
                    return false;
                }
            }

            break;

        case 'c':   /* Check for bad blocks */
        case 't':   /* deprecated */

            cflag++;

            break;

        case 'f':

            size = strtoul(optarg, &tmp, 0);

            if (size < 1024 || size > 4096 || *tmp)
            {
                printf("Mke2fs: bad fragment size - %s", optarg);

                exit(1);
            }

            Ext2Sb->s_log_frag_size =
                int_log2(size >> EXT2_MIN_BLOCK_LOG_SIZE);

            break;

        case 'g':

            Ext2Sb->s_blocks_per_group = strtoul(optarg, &tmp, 0);

            if (*tmp)
            {
                exit(1);
            }

            if ((Ext2Sb->s_blocks_per_group % 8) != 0)
            {
                // _("blocks per group must be multiple of 8"));
                exit(1);
            }

            break;

        case 'i':

            inode_ratio = strtoul(optarg, &tmp, 0);

            if (inode_ratio < 1024 || inode_ratio > 4096 * 1024 ||
                *tmp)
            {
                printf("Mke2fs: bad inode ratio - %s", optarg);
                exit(1);
            }

            break;


        case 'J':

//          parse_journal_opts(optarg);


        case 'j':

            Ext2Sb->s_feature_compat |=
                EXT3_FEATURE_COMPAT_HAS_JOURNAL;

            if (!journal_size)
                journal_size = -1;

            {
                printf("Mke2fs:  current Mke2fs does not support journal yet.\n" );

                exit(1);
            }
            
            break;

        case 'l':

            bad_blocks_filename = (char *) malloc(strlen(optarg)+1);

            if (!bad_blocks_filename)
            {
                printf("Mke2fs: in malloc for bad_blocks_filename");
                exit(1);
            }

            memcpy(bad_blocks_filename, optarg, strlen(optarg));
            bad_blocks_filename[strlen(optarg)] = 0;

            break;

        case 'm':

            reserved_ratio = strtoul(optarg, &tmp, 0);

            if (reserved_ratio > 50 || *tmp)
            {
                printf("Mke2fs: bad reserved blocks percent - %s", optarg);
                exit(1);
            }

            break;

        case 'n':

            noaction++;

            break;

        case 'o':

            creator_os = optarg;

            break;

        case 'r':

            Ext2Sb->s_rev_level = atoi(optarg);

            if (Ext2Sb->s_rev_level == EXT2_GOOD_OLD_REV)
            {
                Ext2Sb->s_feature_incompat = 0;
                Ext2Sb->s_feature_compat = 0;
                Ext2Sb->s_feature_ro_compat = 0;
            }

            break;

        case 's':   /* deprecated */

            if (atoi(optarg))
            {
                Ext2Sb->s_feature_ro_compat |=
                    EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER;
            }
            else 
            {
                Ext2Sb->s_feature_ro_compat &=
                    ~EXT2_FEATURE_RO_COMPAT_SPARSE_SUPER;
            }

            break;

#ifdef EXT2_DYNAMIC_REV
        case 'I':

            Ext2Sb->s_inode_size = atoi(optarg);

            break;
#endif

        case 'N':

            num_inodes = atoi(optarg);

            break;

        case 'v':

            verbose = 1;

            break;

        case 'q':

            quiet = 1;

            break;

        case 'F':

            force = 1;

            break;

        case 'L':

            volume_label = optarg;

            break;

        case 'M':

            mount_dir = optarg;

            break;
/*
        case 'O':

            if (!strcmp(optarg, "none") || default_features)
            {
                pExt2Sb->s_feature_compat = 0;
                pExt2Sb->s_feature_incompat = 0;
                pExt2Sb->s_feature_ro_compat = 0;
                default_features = 0;
            }

            if (!strcmp(optarg, "none"))
                break;

            if (e2p_edit_feature(optarg,
                        &pExt2Sb->s_feature_compat,
                        ok_features))
            {
                fprintf(stderr,
                    _("Invalid filesystem option set: %s\n"), optarg);
                exit(1);
            }

            break;
*/
        case 'R':

            raid_opts = optarg;

            break;

        case 'S':

            super_only = 1;

            break;

        case 'T':

            fs_type = optarg;

            break;

        case 'V':
            /* Print version number and exit */
            printf("Mke2fs: \tUsing EXT2_ET_BASE.\n");

            exit(0);

        default:

            usage();
    }

    if (optind == argc)
        usage();

    device_name = argv[optind];

    optind++;

    if (optind < argc)
    {
        unsigned long tmp2  = strtoul(argv[optind++], &tmp, 0);

        if ((*tmp) || (tmp2 > 0xfffffffful))
        {
            printf("Mke2fs: bad blocks count - %s", argv[optind - 1]);
            exit(1);
        }

        Ext2Sb->s_blocks_count = tmp2;
    }

    if (optind < argc)
        usage();
   
    if (!NT_SUCCESS(Ext2OpenDevice(Ext2Sys, device_name)))
    {
        printf("Mke2fs:  Failed to open volume %s ...\n", device_name);

        return FALSE;
    }

    if (!NT_SUCCESS(Ext2GetMediaInfo(Ext2Sys)))
    {
        IO_STATUS_BLOCK             IoStatus;
        FILE_BASIC_INFORMATION      FBI;
        FILE_STANDARD_INFORMATION   FSI;
        
        NtQueryInformationFile(
            Ext2Sys->MediaHandle,
            &IoStatus,
            (PVOID) &FBI,
            sizeof(FILE_BASIC_INFORMATION),
            FileBasicInformation  );

        if (NT_SUCCESS(IoStatus.Status))
        {
            Ext2Sys->bFile = TRUE;

            NtQueryInformationFile(
                Ext2Sys->MediaHandle,
                &IoStatus,
                (PVOID) &FSI,
                sizeof(FILE_STANDARD_INFORMATION),
                FileStandardInformation  );

            if (NT_SUCCESS(IoStatus.Status) && (!FSI.Directory))
            {
                if (FSI.AllocationSize.QuadPart < 0x100000)
                {
                    printf("Mke2fs: too small file %s ...\n", device_name);

                    Ext2CloseDevice(Ext2Sys);

                    return false;
                }

                Ext2Sys->PartInfo.PartitionLength = FSI.AllocationSize;

                Ext2Sys->DiskGeometry.Cylinders.QuadPart = 
                        FSI.AllocationSize.QuadPart / (0x8000);
                Ext2Sys->DiskGeometry.TracksPerCylinder  = 4;
                Ext2Sys->DiskGeometry.SectorsPerTrack    = 16;
                Ext2Sys->DiskGeometry.BytesPerSector     = 512;
            }
            else
            {
                printf("Mke2fs:  Error get file info about volume %s, ...\n", device_name);

                Ext2CloseDevice(Ext2Sys);
            }
        }
        else
        {
            printf("Mke2fs:  Error get media info about volume %s, ...\n", device_name);

            Ext2CloseDevice(Ext2Sys);

            return FALSE;
        }
    }

    if (raid_opts)
    {
        if (!parse_raid_opts(raid_opts))
            return false;
    }

    /*
     * If there's no blocksize specified and there is a journal
     * device, use it to figure out the blocksize
     */
/*
    if (blocksize == 0 && journal_device)
    {
        ext2_filsys jfs;

        retval = ext2fs_open(journal_device,
                     EXT2_FLAG_JOURNAL_DEV_OK, 0,
                     0, unix_io_manager, &jfs);
        if (!retval)
        {
            com_err(program_name, retval,
                _("while trying to open journal device %s\n"),
                journal_device);
            exit(1);
        }

        blocksize = jfs->blocksize;
        pExt2Sb->s_log_block_size =
            int_log2(blocksize >> EXT2_MIN_BLOCK_LOG_SIZE);

        ext2fs_close(jfs);
    }
*/
    if (Ext2Sb->s_feature_incompat & EXT3_FEATURE_INCOMPAT_JOURNAL_DEV)
    {
        if (!fs_type)
            fs_type = "journal";

        reserved_ratio = 0;
        Ext2Sb->s_feature_incompat = EXT3_FEATURE_INCOMPAT_JOURNAL_DEV;
        Ext2Sb->s_feature_compat = 0;
        Ext2Sb->s_feature_ro_compat = 0;
    }

    if (Ext2Sb->s_rev_level == EXT2_GOOD_OLD_REV &&
        (Ext2Sb->s_feature_compat || Ext2Sb->s_feature_ro_compat ||
         Ext2Sb->s_feature_incompat))
    {
        Ext2Sb->s_rev_level = 1;  /* Create a revision 1 filesystem */
    }

//  check_mount(device_name, force, "filesystem");


    Ext2Sb->s_log_block_size =
        int_log2(blocksize >> EXT2_MIN_BLOCK_LOG_SIZE);

    Ext2Sb->s_log_frag_size = Ext2Sb->s_log_block_size;

    if (noaction && Ext2Sb->s_blocks_count)
    {
        dev_size = (LONGLONG) Ext2Sb->s_blocks_count;
        retval = TRUE;
    }
    else
    {
        dev_size = 0;
        if (Ext2Sys->Extents) {
            DWORD i = 0;
            dev_size = 0;
            for (i=0; i < Ext2Sys->Extents->NumberOfDiskExtents; i++) {
                dev_size += Ext2Sys->Extents->Extents[i].ExtentLength.QuadPart;
            }
            
        } else {
            if (Ext2Sys->PartInfo.PartitionType != PARTITION_LDM) {
                dev_size = Ext2Sys->PartInfo.PartitionLength.QuadPart;
            }
        }

        if (dev_size)
        {
            dev_size /= EXT2_BLOCK_SIZE(Ext2Sb);
            retval = TRUE;
        }
        else
        {
            retval = FALSE;
        }
    }

    if (!retval)
    {
        printf("Mke2fs: error while trying to determine filesystem size");

        Ext2CloseDevice(Ext2Sys);

        exit(1);
    }

    if (!Ext2Sb->s_blocks_count)
    {
        if (dev_size == 0)
        {
            printf("Mke2fs: Device size reported to be zero.  "
                  "Invalid partition specified, or\n\t"
                  "partition table wasn't reread "
                  "after running fdisk, due to\n\t"
                  "a modified partition being busy "
                  "and in use.  You may need to reboot\n\t"
                  "to re-read your partition table.\n"
                  );

             Ext2CloseDevice(Ext2Sys);
             exit(1);
        }

        Ext2Sb->s_blocks_count = (ULONG) dev_size;
    }
    else if (!force && (Ext2Sb->s_blocks_count > dev_size))
    {
        printf("Mke2fs:  Filesystem larger than apparent filesystem size.");
        proceed_question();
    }

    /*
     * If the user asked for HAS_JOURNAL, then make sure a journal
     * gets created.
     */
    if ((Ext2Sb->s_feature_compat & EXT3_FEATURE_COMPAT_HAS_JOURNAL) &&
        !journal_size)
        journal_size = -1;


    if (!inode_ratio) {
        average_fsize = blocksize;
    } else {
        average_fsize = inode_ratio;
    }

    /*
     * Calculate number of blocks to reserve
     */
    Ext2Sb->s_r_blocks_count = (Ext2Sb->s_blocks_count * reserved_ratio) / 100;

    return true;
}

/*
 * Helper function which zeros out _num_ blocks starting at _blk_.  In
 * case of an error, the details of the error is returned via _ret_blk_
 * and _ret_count_ if they are non-NULL pointers.  Returns 0 on
 * success, and an error code on an error.
 *
 * As a special case, if the first argument is NULL, then it will
 * attempt to free the static zeroizing buffer.  (This is to keep
 * programs that check for memory leaks happy.)
 */
bool zero_blocks(PEXT2_FILESYS fs, ULONG blk, ULONG num,
                 ULONG *ret_blk, ULONG *ret_count)
{
    ULONG       j, count, next_update, next_update_incr;
    static unsigned char        *buf;
    bool        retval;

    /* If fs is null, clean up the static buffer and return */
    if (!fs)
    {
        if (buf)
        {
            free(buf);
            buf = 0;
        }
        return true;
    }

#define STRIDE_LENGTH 8

    /* Allocate the zeroizing buffer if necessary */
    if (!buf)
    {
        buf = (unsigned char *) malloc(fs->blocksize * STRIDE_LENGTH);
        if (!buf)
        {
            printf("Mke2fs: while allocating zeroizing buffer");
            return false;
        }
        memset(buf, 0, fs->blocksize * STRIDE_LENGTH);
    }

    /* OK, do the write loop */
    next_update = 0;
    next_update_incr = num / 100;
    if (next_update_incr < 1)
        next_update_incr = 1;

    for (j=0; j < num; j += STRIDE_LENGTH, blk += STRIDE_LENGTH)
    {
        if (num-j > STRIDE_LENGTH)
            count = STRIDE_LENGTH;
        else
            count = num - j;

        retval = NT_SUCCESS(Ext2WriteDisk(
                        fs,
                        ((ULONGLONG)blk * fs->blocksize),
                        count * fs->blocksize,
                        buf));

        if (!retval)
        {
            if (ret_count)
                *ret_count = count;

            if (ret_blk)
                *ret_blk = blk;

            return retval;
        }
    }

    return true;
}   


bool zap_sector(PEXT2_FILESYS Ext2Sys, int sect, int nsect)
{
    unsigned char *buf;
    ULONG         *magic;
    NTSTATUS      status;
    bool          rc = false;

    buf = (unsigned char *)malloc(SECTOR_SIZE*nsect);
    if (!buf)
    {
        printf("Mke2fs: Out of memory erasing sectors %d-%d\n",
               sect, sect + nsect - 1);
        return false;
    }

#define BSD_DISKMAGIC   (0x82564557UL)  /* The disk magic number */
#define BSD_MAGICDISK   (0x57455682UL)  /* The disk magic number reversed */
#define BSD_LABEL_OFFSET        64

    if (sect == 0)
    {
        Ext2ReadDisk(
                  Ext2Sys, 
                  (LONGLONG)0,
                  SECTOR_SIZE,
                  buf);

        // Check for a BSD disklabel, and don't erase it if so
        magic = (ULONG *) (buf + BSD_LABEL_OFFSET);
        if ((*magic == BSD_DISKMAGIC) ||   (*magic == BSD_MAGICDISK))
                goto clean_up;
    }

    memset(buf, 0, (ULONG)nsect * SECTOR_SIZE);

    // Write buf to disk
    status = Ext2WriteDisk( Ext2Sys,
                   (LONGLONG)(sect * SECTOR_SIZE),
                   (ULONG)nsect * SECTOR_SIZE,
                   buf );
    if (!NT_SUCCESS(status)) {
        printf("Mke2fs: failed to write disk, is it readonly ?\n");
    }

    rc = true;

clean_up:

    free(buf);
    
    return rc;
}

/*
 * Set the S_CREATOR_OS field.  Return true if OS is known,
 * otherwise, 0.
 */
bool set_os(PEXT2_SUPER_BLOCK sb, char *os)
{
    if (isdigit (*os))
        sb->s_creator_os = atoi (os);
    else if (strcmp(os, "linux") == 0)
        sb->s_creator_os = EXT2_OS_LINUX;
    else if (strcmp(os, "GNU") == 0 || strcmp(os, "hurd") == 0)
        sb->s_creator_os = EXT2_OS_HURD;
    else if (strcmp(os, "masix") == 0)
        sb->s_creator_os = EXT2_OS_MASIX;
    else if (strcmp(os, "freebsd") == 0)
        sb->s_creator_os = EXT2_OS_FREEBSD;
    else if (strcmp(os, "LITES") == 0)
        sb->s_creator_os = EXT2_OS_LITES;
    else if (strcmp(os, "winnt") == 0)
        sb->s_creator_os = EXT2_OS_WINNT;
    else
        return false;
    return true;
}

void show_stats(PEXT2_FILESYS fs)
{
    PEXT2_SUPER_BLOCK s = fs->ext2_sb;
    char        buf[80];
    ULONG       group_block;
    int         i, need, col_left;
    
    memset(buf, 0, sizeof(buf));
    strncpy(buf, s->s_volume_name, sizeof(s->s_volume_name));
    printf("Filesystem Statics ...\n\n");
    printf("     Filesystem label=%s\n", buf);
    printf("     OS type: ");
    switch (s->s_creator_os)
    {
        case EXT2_OS_LINUX: printf ("Linux");
            break;
        case EXT2_OS_HURD:  printf ("GNU/Hurd");
            break;
        case EXT2_OS_MASIX: printf ("Masix");
            break;
        case EXT2_OS_FREEBSD: printf ("FreeBsd");
            break;
        case EXT2_OS_LITES: printf ("Lites");
            break;
        case EXT2_OS_WINNT: printf ("Winnt");
            break;
        default:
            printf (("(Unknown)"));
    }
    printf("\n");
    printf("     Block size=%u (log=%u)\n", fs->blocksize,
        s->s_log_block_size);
    printf(("     Fragment size=%u (log=%u)\n"), fs->fragsize,
        s->s_log_frag_size);
    printf(("     %u inodes, %u blocks\n"), s->s_inodes_count,
           s->s_blocks_count);
    printf(("     %u blocks (%2.2f%%) reserved for the super user\n"),
        s->s_r_blocks_count,
           100.0 * s->s_r_blocks_count / s->s_blocks_count);
    printf(("     First data block=%u\n"), s->s_first_data_block);

    if (fs->group_desc_count > 1)
        printf(("     %u block groups\n"), fs->group_desc_count);
    else
        printf(("     %u block group\n"), fs->group_desc_count);

    printf(("     %u blocks per group, %u fragments per group\n"),
           s->s_blocks_per_group, s->s_frags_per_group);
    printf(("     %u inodes per group\n"), s->s_inodes_per_group);

    if (fs->group_desc_count == 1)
    {
        printf("\n\n");
        return;
    }
    
    printf(("     Superblock backups stored on blocks: "));
    group_block = s->s_first_data_block;
    col_left = 0;

    for (i = 1; (ULONG)i < fs->group_desc_count; i++)
    {
        group_block += s->s_blocks_per_group;

        if (!ext2_bg_has_super(s, i))
            continue;

        if (i != 1)
            printf(", ");

        need = int_log10(group_block) + 2;

        if (need > col_left)
        {
            printf("\n     ");
            col_left = 72;
        }

        col_left -= need;
        printf("%u", group_block);
    }

    printf("\n\n");
}

bool ext2_mkdir( PEXT2_FILESYS fs, 
                 ULONG parent,
                 ULONG inum, 
                 char *name,
                 ULONG *no,
                 PEXT2_INODE pid )
{
    bool            retval;
    EXT2_INODE      parent_inode, inode;
    ULONG           ino = inum;
    //ULONG         scratch_ino;
    ULONG           blk;
    char            *block = 0;
    int             filetype = 0;

    LARGE_INTEGER   SysTime;
    
    NtQuerySystemTime(&SysTime);

    /*
     * Allocate an inode, if necessary
     */
    if (!ino)
    {
        retval = ext2_new_inode(fs, parent, LINUX_S_IFDIR | 0755, 0, &ino);
        if (!retval)
            goto cleanup;
    }

    if (no)
        *no = ino;

    /*
     * Allocate a data block for the directory
     */
    retval = ext2_new_block(fs, 0, 0, &blk);
    if (!retval)
        goto cleanup;

    /*
     * Create a scratch template for the directory
     */
    retval = ext2_new_dir_block(fs, ino, parent, &block);
    if (!retval)
        goto cleanup;

    /*
     * Get the parent's inode, if necessary
     */
    if (parent != ino)
    {
        retval = ext2_load_inode(fs, parent, &parent_inode);
        if (!retval)
            goto cleanup;
    }
    else
    {
        memset(&parent_inode, 0, sizeof(parent_inode));
    }

    /*
     * Create the inode structure....
     */
    memset(&inode, 0, sizeof(EXT2_INODE));
    inode.i_mode = (USHORT)(LINUX_S_IFDIR | (0777 & ~fs->umask));
    inode.i_uid = inode.i_gid = 0;
    inode.i_blocks = fs->blocksize / 512;
    inode.i_block[0] = blk;
    inode.i_links_count = 2;
    inode.i_ctime = inode.i_atime = inode.i_mtime = ext2_unix_time(SysTime.QuadPart);
    inode.i_size = fs->blocksize;

    /*
     * Write out the inode and inode data block
     */
    retval = ext2_write_block(fs, blk, block);
    if (!retval)
        goto cleanup;

    retval = ext2_save_inode(fs, ino, &inode); 
    if (!retval)
        goto cleanup;

    if (pid)
    {
        *pid = inode;
    }

    if (parent != ino)
    {
        /*
         * Add entry for this inode to parent dir 's block
         */

        if (fs->ext2_sb->s_feature_incompat & EXT2_FEATURE_INCOMPAT_FILETYPE)
                filetype = EXT2_FT_DIR;

        retval = ext2_add_entry(fs, parent, ino, filetype, name);

        if (!retval)
            goto cleanup;

        /*
         * Update parent inode's counts
         */

        parent_inode.i_links_count++;
        retval = ext2_save_inode(fs, parent, &parent_inode);
        if (!retval)
            goto cleanup;

    }
    
    /*
     * Update accounting....
     */
    ext2_block_alloc_stats(fs, blk, +1);
    ext2_inode_alloc_stats2(fs, ino, +1, 1);

cleanup:

    if (block)
    {
        free(block);
        block = NULL;
    }

    return retval;
}

bool create_root_dir(PEXT2_FILESYS fs)
{
    bool        retval;
    EXT2_INODE  inode;

    retval = ext2_mkdir(fs, EXT2_ROOT_INO, EXT2_ROOT_INO, 0, NULL, &inode);
    
    if (!retval)
    {
        printf("Mke2fs: while creating root dir");
        return false;
    }

    {
/*
        EXT2_INODE  newInode;
        retval = ext2_load_inode(fs, EXT2_ROOT_INO, &newInode);
        
        if (!retval)
        {
            printf("Mke2fs: while reading root inode");
            return false;
        }

        ASSERT(memcmp(&newInode, inode, sizeof(EXT2_INODE)) == 0);
*/

        inode.i_uid = 0;    
        inode.i_gid = 0;

        retval = ext2_save_inode(fs, EXT2_ROOT_INO, &inode);
        if (!retval)
        {
            printf("Mke2fs: while setting root inode ownership");
            return false;
        }
    }

    return true;
}

bool create_lost_and_found(PEXT2_FILESYS Ext2Sys)
{
    bool        retval;
    ULONG       ino;
    char        *name = "lost+found";
    int         lpf_size = 0;
    EXT2_INODE  inode;
    ULONG       dwBlk = 0;
    BOOLEAN     bExt= TRUE;

    PEXT2_DIR_ENTRY dir;

    char *      buf;

    buf = (char *) malloc (Ext2Sys->blocksize);
    if (!buf)
    {
        bExt = FALSE;
    }
    else
    {
        memset(buf, 0, Ext2Sys->blocksize);

        dir = (PEXT2_DIR_ENTRY) buf;
        dir->rec_len = Ext2Sys->blocksize;
    }

    Ext2Sys->umask = 077;
    retval = ext2_mkdir(Ext2Sys, EXT2_ROOT_INO, 0, name, &ino, &inode);
    
    if (!retval)
    {
        printf("Mke2fs: while creating /lost+found.\n");
        return false;
    }

    if (!bExt)
        goto errorout;

    lpf_size = inode.i_size;

    while(TRUE)
    {
        if (lpf_size >= 16*1024)
            break;
        
        retval = ext2_alloc_block(Ext2Sys, 0, &dwBlk);

        if (! retval)
        {
            printf("Mke2fs: create_lost_and_found: error alloc block.\n");
            break;
        }

        retval = ext2_expand_inode(Ext2Sys, &inode, dwBlk);
        if (!retval)
        {
            printf("Mke2fs: errors when expanding /lost+found.\n");
            break;
        }

        ext2_write_block(Ext2Sys, dwBlk, buf);

        inode.i_blocks += (Ext2Sys->blocksize/SECTOR_SIZE);
        lpf_size += Ext2Sys->blocksize;
    }

    {
        inode.i_size = lpf_size;

        ASSERT( (inode.i_size/Ext2Sys->blocksize) == 
                Ext2DataBlocks(Ext2Sys, inode.i_blocks/(Ext2Sys->blocksize/SECTOR_SIZE)));

        ASSERT( (inode.i_blocks/(Ext2Sys->blocksize/SECTOR_SIZE)) == 
                Ext2TotalBlocks(Ext2Sys, inode.i_size/Ext2Sys->blocksize));

    }

    ext2_save_inode(Ext2Sys, ino, &inode);

errorout:

    if (buf)
    {
        free(buf);
    }

    return true;
}

/*
 * This function forces out the primary superblock.  We need to only
 * write out those fields which we have changed, since if the
 * filesystem is mounted, it may have changed some of the other
 * fields.
 *
 * It takes as input a superblock which has already been byte swapped
 * (if necessary).
 *
 */

bool write_primary_superblock(PEXT2_FILESYS Ext2Sys, PEXT2_SUPER_BLOCK super)
{
    bool bRet;    

    bRet = NT_SUCCESS(Ext2WriteDisk(
                           Ext2Sys,
                           ((LONGLONG)SUPERBLOCK_OFFSET),
                           SUPERBLOCK_SIZE, (PUCHAR)super));
            


    return bRet;
}


/*
 * Updates the revision to EXT2_DYNAMIC_REV
 */
void ext2_update_dynamic_rev(PEXT2_FILESYS fs)
{
    PEXT2_SUPER_BLOCK sb = fs->ext2_sb;

    if (sb->s_rev_level > EXT2_GOOD_OLD_REV)
        return;

    sb->s_rev_level = EXT2_DYNAMIC_REV;
    sb->s_first_ino = EXT2_GOOD_OLD_FIRST_INO;
    sb->s_inode_size = EXT2_GOOD_OLD_INODE_SIZE;
    /* s_uuid is handled by e2fsck already */
    /* other fields should be left alone */
}


bool ext2_flush(PEXT2_FILESYS fs)
{
    ULONG       i,j,maxgroup,sgrp;
    ULONG       group_block;
    bool        retval;
    char        *group_ptr;
    unsigned long fs_state;
    PEXT2_SUPER_BLOCK super_shadow = 0;
    PEXT2_GROUP_DESC group_shadow = 0;

    LARGE_INTEGER   SysTime;
    
    NtQuerySystemTime(&SysTime);
    
    fs_state = fs->ext2_sb->s_state;

    fs->ext2_sb->s_wtime = ext2_unix_time(SysTime.QuadPart);
    fs->ext2_sb->s_block_group_nr = 0;

    super_shadow = fs->ext2_sb;
    group_shadow = fs->group_desc;
    
    /*
     * Write out master superblock.  This has to be done
     * separately, since it is located at a fixed location
     * (SUPERBLOCK_OFFSET).
     */
    retval = write_primary_superblock(fs, super_shadow);
    if (!retval)
        goto errout;

    /*
     * If this is an external journal device, don't write out the
     * block group descriptors or any of the backup superblocks
     */
    if (fs->ext2_sb->s_feature_incompat &
        EXT3_FEATURE_INCOMPAT_JOURNAL_DEV)
    {
        retval = false;
        goto errout;
    }

    /*
     * Set the state of the FS to be non-valid.  (The state has
     * already been backed up earlier, and will be restored when
     * we exit.)
     */
    fs->ext2_sb->s_state &= ~EXT2_VALID_FS;

    /*
     * Write out the master group descriptors, and the backup
     * superblocks and group descriptors.
     */
    group_block = fs->ext2_sb->s_first_data_block;
    maxgroup = fs->group_desc_count;

    for (i = 0; i < maxgroup; i++)
    {
        if (!ext2_bg_has_super(fs->ext2_sb, i))
            goto next_group;

        sgrp = i;
        if (sgrp > ((1 << 16) - 1))
            sgrp = (1 << 16) - 1;

        fs->ext2_sb->s_block_group_nr = (USHORT) sgrp;

        if (i !=0 )
        {
            retval = NT_SUCCESS(Ext2WriteDisk(
                                fs,
                                ((ULONGLONG)group_block * fs->blocksize),
                                SUPERBLOCK_SIZE, (PUCHAR)super_shadow));

            if (!retval)
            {
                goto errout;
            }
        }

        if (super_only)
            goto next_group;

        group_ptr = (char *) group_shadow;

        for (j=0; j < fs->desc_blocks; j++)
        {

            retval = NT_SUCCESS(Ext2WriteDisk(
                                fs,
                                ((ULONGLONG)(group_block+1+j) * fs->blocksize),
                                fs->blocksize, (PUCHAR) group_ptr));

            if (!retval)
            {
                goto errout;
            }

            group_ptr += fs->blocksize;
        }

    next_group:
        group_block += EXT2_BLOCKS_PER_GROUP(fs->ext2_sb);

    }

    fs->ext2_sb->s_block_group_nr = 0;

    /*
     * If the write_bitmaps() function is present, call it to
     * flush the bitmaps.  This is done this way so that a simple
     * program that doesn't mess with the bitmaps doesn't need to
     * drag in the bitmaps.c code.
     */
    retval = ext2_write_bitmaps(fs);
    if (!retval)
        goto errout;

    /*
     * Flush the blocks out to disk
     */

    // retval = io_channel_flush(fs->io);

errout:

    fs->ext2_sb->s_state = (USHORT) fs_state;

    return retval;
}


bool create_journal_dev(PEXT2_FILESYS fs)
{
    bool        retval = false;
    char        *buf = NULL;
    ULONG       blk;
    ULONG       count;

/*
    retval = ext2_create_journal_superblock(fs,
                  fs->ext2_sb->s_blocks_count, 0, &buf);
*/

    if (!retval)
    {
        printf("Mke2fs: ext2_create_journal_dev: while initializing journal superblock.\n");
        return false;
    }

    if (!quiet)
        printf("Mke2fs: Zeroing journal device: \n"),

    retval = zero_blocks(fs, 0, fs->ext2_sb->s_blocks_count,
                 &blk, &count);

    zero_blocks(0, 0, 0, 0, 0);

    if (!retval)
    {
        printf("Mke2fs: create_journal_dev: while zeroing journal device (block %u, count %d).\n",
            blk, count);
        return false;
    }

    retval = NT_SUCCESS(Ext2WriteDisk(
                    fs,
                    ((ULONGLONG)blk * (fs->ext2_sb->s_first_data_block+1)),
                    fs->blocksize, (unsigned char *)buf));

    if (!retval)
    {
        printf("Mke2fs: create_journal_dev: while writing journal superblock.\n");
        return false;
    }

    return true;
}

#define BLOCK_BITS (Ext2Sys->ext2_sb->s_log_block_size + 10)

ULONG
Ext2DataBlocks(PEXT2_FILESYS Ext2Sys, ULONG TotalBlocks)
{
    ULONG   dwData[4] = {1, 1, 1, 1};
    ULONG   dwMeta[4] = {0, 0, 0, 0};
    ULONG   DataBlocks = 0;
    ULONG   i, j;

    if (TotalBlocks <= EXT2_NDIR_BLOCKS)
    {
        return TotalBlocks;
    }

    TotalBlocks -= EXT2_NDIR_BLOCKS;

    for (i = 0; i < 4; i++)
    {
        dwData[i] = dwData[i] << ((BLOCK_BITS - 2) * i);

        if (i > 0)
        {
            dwMeta[i] = 1 + (dwMeta[i - 1] << (BLOCK_BITS - 2));
        }
    }

    for( i=1; (i < 4) && (TotalBlocks > 0); i++)
    {
        if (TotalBlocks >= (dwData[i] + dwMeta[i]))
        {
            TotalBlocks -= (dwData[i] + dwMeta[i]);
            DataBlocks  += dwData[i];
        }
        else
        {
            ULONG   dwDivide = 0;
            ULONG   dwRemain = 0;

            for (j=i; (j > 0) && (TotalBlocks > 0); j--)
            {
                dwDivide = (TotalBlocks - 1) / (dwData[j-1] + dwMeta[j-1]);
                dwRemain = (TotalBlocks - 1) % (dwData[j-1] + dwMeta[j-1]);

                DataBlocks += (dwDivide * dwData[j-1]);
                TotalBlocks = dwRemain;
            }
        }
    }

    return (DataBlocks + EXT2_NDIR_BLOCKS);
}


ULONG
Ext2TotalBlocks(PEXT2_FILESYS Ext2Sys, ULONG DataBlocks)
{
    ULONG   dwData[4] = {1, 1, 1, 1};
    ULONG   dwMeta[4] = {0, 0, 0, 0};
    ULONG   TotalBlocks = 0;
    ULONG   i, j;

    if (DataBlocks <= EXT2_NDIR_BLOCKS)
    {
        return DataBlocks;
    }

    DataBlocks -= EXT2_NDIR_BLOCKS;

    for (i = 0; i < 4; i++)
    {
        dwData[i] = dwData[i] << ((BLOCK_BITS - 2) * i);

        if (i > 0)
        {
            dwMeta[i] = 1 + (dwMeta[i - 1] << (BLOCK_BITS - 2));
        }
    }

    for( i=1; (i < 4) && (DataBlocks > 0); i++)
    {
        if (DataBlocks >= dwData[i])
        {
            DataBlocks  -= dwData[i];
            TotalBlocks += (dwData[i] + dwMeta[i]);
        }
        else
        {
            ULONG   dwDivide = 0;
            ULONG   dwRemain = 0;

            for (j=i; (j > 0) && (DataBlocks > 0); j--)
            {
                dwDivide = (DataBlocks) / (dwData[j-1]);
                dwRemain = (DataBlocks) % (dwData[j-1]);

                TotalBlocks += (dwDivide * (dwData[j-1] + dwMeta[j-1]) + 1);
                DataBlocks = dwRemain;
            }
        }
    }

    return (TotalBlocks + EXT2_NDIR_BLOCKS);
}


int main(int argc, char* argv[])
{
    
    BOOLEAN    bRet;
    NTSTATUS   Status;

    // Super Block: 1024 bytes long
    EXT2_SUPER_BLOCK Ext2Sb;

    // File Sys Structure
    EXT2_FILESYS     FileSys;

    PEXT2_FILESYS    Ext2Sys = &FileSys;

    printf("\nMke2fs for Winnt/2k/xp V"MKE2FS_VERSION"\n"
           "    Format a file or volume to ext2 format file system.\n"
           "\nAuthor:   Matt <mattwu@163.com>"
           "\nHomepage: http://ext2.yeah.net\n\n");

    memset(Ext2Sys, 0, sizeof(EXT2_FILESYS));

    Ext2Sys->ext2_sb = &(Ext2Sb);

    // Parase the command line
    bRet = parase_cmd(argc, argv, Ext2Sys);
    if (! bRet) goto clean_up;

    if (!Ext2Sys->bFile)
    {
        Status = Ext2LockVolume(Ext2Sys);

        if (NT_SUCCESS(Status))
        {
            bLocked = TRUE;
        }
    }

    // Initialize 
    if (!ext2_initialize_sb(Ext2Sys))
    {
        printf("Mke2fs: failed to initialize super_block ...\n");
        goto clean_up;
    }

    printf("\nPress ENTER to format %s\n", device_name);
    fgetc(stdin);

    if (!noaction)
        zap_sector(Ext2Sys, 2, 6);

    /*
     * Generate a UUID for it...
     */
    {
        __u8  uuid[16];
        uuid_generate(&uuid[0]);
        memcpy(&Ext2Sb.s_uuid[0], &uuid[0], 16);
    }

    /*
     * Add "jitter" to the superblock's check interval so that we
     * don't check all the filesystems at the same time.  We use a
     * kludgy hack of using the UUID to derive a random jitter value.
     */
    {
        int i, val;

        for (i = 0, val = 0 ; i < sizeof(Ext2Sb.s_uuid); i++)
            val += Ext2Sb.s_uuid[i];

        Ext2Sb.s_max_mnt_count += val % EXT2_DFL_MAX_MNT_COUNT;
    }

    /*
     * Override the creator OS, if applicable
     */
    if (creator_os && !set_os(&Ext2Sb, creator_os))
    {
        goto clean_up;
    }

    /*
     * For the Hurd, we will turn off filetype since it doesn't
     * support it.
     */
    if (Ext2Sb.s_creator_os == EXT2_OS_HURD)
        Ext2Sb.s_feature_incompat &=
            ~EXT2_FEATURE_INCOMPAT_FILETYPE;

    /*
     * Set the volume label...
     */
    if (volume_label)
    {
        memset(Ext2Sb.s_volume_name, 0,
               sizeof(Ext2Sb.s_volume_name));
        strncpy(Ext2Sb.s_volume_name, volume_label,
            sizeof(Ext2Sb.s_volume_name));
    }

    /*
     * Set the last mount directory
     */
    if (mount_dir)
    {
        memset(Ext2Sb.s_last_mounted, 0,
               sizeof(Ext2Sb.s_last_mounted));
        strncpy(Ext2Sb.s_last_mounted, mount_dir,
            sizeof(Ext2Sb.s_last_mounted));
    }

    if (!quiet)
        ext2_print_super( &Ext2Sb);

    if (!quiet || noaction)
        show_stats(Ext2Sys);

    if (noaction)
        goto clean_up;

/*
    if (Ext2Sb.s_feature_incompat &
        EXT3_FEATURE_INCOMPAT_JOURNAL_DEV)
    {
        create_journal_dev(fs);
        goto clean_up;
    }

    if (bad_blocks_filename)
        read_bb_file(fs, &bb_list, bad_blocks_filename);

    if (cflag)
        test_disk(fs, &bb_list);

    handle_bad_blocks(fs, bb_list);
*/

    Ext2Sys->stride = fs_stride;

    bRet = ext2_allocate_tables(Ext2Sys);

    if (!bRet)
    {
        goto clean_up;
    }

    if (super_only)
    {
        Ext2Sb.s_state |= EXT2_ERROR_FS;
    }
    else
    {
        /* rsv must be a power of two (64kB is MD RAID sb alignment) */
        ULONG rsv = 65536 / Ext2Sys->blocksize;
        ULONG blocks = Ext2Sb.s_blocks_count;
        ULONG start;
        ULONG ret_blk;

#ifdef ZAP_BOOTBLOCK
        if (!zap_sector(Ext2Sys, 0, 2)) {
            goto clean_up;
        }
#endif

        /*
         * Wipe out any old MD RAID (or other) metadata at the end
         * of the device.  This will also verify that the device is
         * as large as we think.  Be careful with very small devices.
         */

        start = (blocks & ~(rsv - 1));
        if (start > rsv)
            start -= rsv;

        if (start > 0)
            bRet = zero_blocks(Ext2Sys, start, blocks - start, &ret_blk, NULL);

        if (!bRet)
        {
            printf("Mke2fs: failed to clear block %xh.\n", ret_blk);
            goto clean_up;
        }

        write_inode_tables(Ext2Sys);

        create_root_dir(Ext2Sys);
        create_lost_and_found(Ext2Sys);

        ext2_reserve_inodes(Ext2Sys);

/*
        create_bad_block_inode(Ext2Sys, bb_list);
*/

        create_bad_block_inode(Ext2Sys, NULL);
    }

//no_journal:
    if (!quiet)
        printf("Writing superblocks and filesystem accounting information ... \n");

    if (!ext2_flush(Ext2Sys))
    {
        bRet = false;
        printf("Warning, had trouble writing out superblocks.\n");
        goto clean_up;
    }

    if (!quiet)
        printf("Writing superblocks and filesystem accounting information done!\n");

    printf("\n\nThanks for using Mke2fs V" MKE2FS_VERSION " by Matt.\n\n");

clean_up:

    // Clean up ...
    ext2_free_group_desc(Ext2Sys);

    ext2_free_block_bitmap(Ext2Sys);
    ext2_free_inode_bitmap(Ext2Sys);

    if (bad_blocks_filename)
        free(bad_blocks_filename);

    if (bRet)
    {
        if (!Ext2Sys->bFile) {
            Ext2DisMountVolume(Ext2Sys);
        }
    }
    else
    {
        if(bLocked)
        {
            Ext2UnLockVolume(Ext2Sys);
        }
    }

    Ext2CloseDevice(Ext2Sys);

    return (!bRet);
}

