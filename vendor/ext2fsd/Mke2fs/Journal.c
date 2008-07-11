/*
 * PROJECT:          Mke2fs
 * FILE:             Inode.c
 * PROGRAMMER:       Matt Wu <mattwu@163.com>
 * HOMEPAGE:         http://ext2.yeah.net
 */

/* INCLUDES **************************************************************/

#include "Mke2fs.h"

/* DEFINITIONS ***********************************************************/

extern char *device_name;

/* FUNCTIONS *************************************************************/


/*
 * This function automatically sets up the journal superblock and
 * returns it as an allocated block.
 */
bool ext2fs_create_journal_superblock(PEXT2_FILESYS fs,
                       __u32 size, int flags,
                       char  **ret_jsb)
{
    bool        retval;
    journal_superblock_t    *jsb;

    if (size < 1024)
        return EXT2_ET_JOURNAL_TOO_SMALL;

    if ((retval = ext2fs_get_mem(fs->blocksize, (void **) &jsb)))
        return retval;

    memset (jsb, 0, fs->blocksize);

    jsb->s_header.h_magic = htonl(JFS_MAGIC_NUMBER);
    if (flags & EXT2_MKJOURNAL_V1_SUPER)
        jsb->s_header.h_blocktype = htonl(JFS_SUPERBLOCK_V1);
    else
        jsb->s_header.h_blocktype = htonl(JFS_SUPERBLOCK_V2);
    jsb->s_blocksize = htonl(fs->blocksize);
    jsb->s_maxlen = htonl(size);
    jsb->s_nr_users = htonl(1);
    jsb->s_first = htonl(1);
    jsb->s_sequence = htonl(1);
    memcpy(jsb->s_uuid, fs->ext2_sb->s_uuid, sizeof(fs->ext2_sb->s_uuid));
    /*
     * If we're creating an external journal device, we need to
     * adjust these fields.
     */
    if (fs->ext2_sb->s_feature_incompat &
        EXT3_FEATURE_INCOMPAT_JOURNAL_DEV) {
        jsb->s_nr_users = 0;
        if (fs->blocksize == 1024)
            jsb->s_first = htonl(3);
        else
            jsb->s_first = htonl(2);
    }

    *ret_jsb = (char *) jsb;
    return 0;
}

/*
 * This function writes a journal using POSIX routines.  It is used
 * for creating external journals and creating journals on live
 * filesystems.
 */
static bool write_journal_file(PEXT2_FILESYS fs, char *filename,
                    ULONG size, int flags)
{
    bool    retval;
    char        *buf = 0;
    int     i, fd, ret_size;

    if ((retval = ext2fs_create_journal_superblock(fs, size, flags, &buf)))
        return retval;

    /* Open the device or journal file */
    if ((fd = open(filename, O_WRONLY)) < 0) {
        retval = errno;
        goto errout;
    }

    /* Write the superblock out */
    retval = EXT2_ET_SHORT_WRITE;
    ret_size = write(fd, buf, fs->blocksize);
    if (ret_size < 0) {
        retval = errno;
        goto errout;
    }
    if (ret_size != fs->blocksize)
        goto errout;
    memset(buf, 0, fs->blocksize);

    for (i = 1; i < size; i++) {
        ret_size = write(fd, buf, fs->blocksize);
        if (ret_size < 0) {
            retval = errno;
            goto errout;
        }
        if (ret_size != fs->blocksize)
            goto errout;
    }
    close(fd);

    retval = 0;
errout:
    ext2fs_free_mem((void **) &buf);
    return retval;
}

/*
 * Helper function for creating the journal using direct I/O routines
 */
struct mkjournal_struct
{
    int     num_blocks;
    int     newblocks;
    char        *buf;
    bool    err;
};

static int mkjournal_proc(PEXT2_FILESYS     fs,
               ULONG        *blocknr,
               e2_blkcnt_t      blockcnt,
               ULONG        ref_block,
               int          ref_offset,
               void         *priv_data)
{
    struct mkjournal_struct *es = (struct mkjournal_struct *) priv_data;
    ULONG   new_blk;
    static ULONG    last_blk = 0;
    bool    retval;
    
    if (*blocknr) {
        last_blk = *blocknr;
        return 0;
    }
    retval = ext2fs_new_block(fs, last_blk, 0, &new_blk);
    if (retval) {
        es->err = retval;
        return BLOCK_ABORT;
    }
    if (blockcnt > 0)
        es->num_blocks--;

    es->newblocks++;
    retval = io_channel_write_blk(fs->io, new_blk, 1, es->buf);

    if (blockcnt == 0)
        memset(es->buf, 0, fs->blocksize);

    if (retval) {
        es->err = retval;
        return BLOCK_ABORT;
    }
    *blocknr = new_blk;
    last_blk = new_blk;
    ext2fs_block_alloc_stats(fs, new_blk, +1);

    if (es->num_blocks == 0)
        return (BLOCK_CHANGED | BLOCK_ABORT);
    else
        return BLOCK_CHANGED;
    
}

/*
 * This function creates a journal using direct I/O routines.
 */
static bool write_journal_inode(PEXT2_FILESYS fs, ext2_ino_t journal_ino,
                     ULONG size, int flags)
{
    char            *buf;
    bool        retval;
    struct ext2_inode   inode;
    struct mkjournal_struct es;

    LARGE_INTEGER   SysTime;
    
    NtQuerySystemTime(&SysTime);

    if ((retval = ext2fs_create_journal_superblock(fs, size, flags, &buf)))
        return retval;
    
    if ((retval = ext2_read_bitmaps(fs)))
        return retval;

    if ((retval = ext2fs_read_inode(fs, journal_ino, &inode)))
        return retval;

    if (inode.i_blocks > 0)
        return EEXIST;

    es.num_blocks = size;
    es.newblocks = 0;
    es.buf = buf;
    es.err = 0;

    retval = ext2fs_block_iterate2(fs, journal_ino, BLOCK_FLAG_APPEND,
                       0, mkjournal_proc, &es);
    if (es.err) {
        retval = es.err;
        goto errout;
    }

    if ((retval = ext2fs_read_inode(fs, journal_ino, &inode)))
        goto errout;

    inode.i_size += fs->blocksize * size;
    inode.i_blocks += (fs->blocksize / SECTOR_SIZE) * es.newblocks;
    inode.i_mtime = inode.i_ctime = ext2_unix_time(SysTime.QuadPart);
    inode.i_links_count = 1;
    inode.i_mode = LINUX_S_IFREG | 0600;

    if ((retval = ext2fs_write_inode(fs, journal_ino, &inode)))
        goto errout;
    retval = 0;

errout:
    ext2fs_free_mem((void **) &buf);
    return retval;
}

/*
 * This function adds a journal device to a filesystem
 */
bool ext2fs_add_journal_device(PEXT2_FILESYS fs, PEXT2_FILESYS journal_dev)
{
    struct stat st;
    bool    retval;
    char        buf[1024];
    journal_superblock_t    *jsb;
    int     i, start;
    __u32       nr_users;

    /* Make sure the device exists and is a block device */
    if (stat(journal_dev->device_name, &st) < 0)
        return errno;
    
    if (!S_ISBLK(st.st_mode))
        return EXT2_ET_JOURNAL_NOT_BLOCK; /* Must be a block device */

    /* Get the journal superblock */
    start = 1;
    if (journal_dev->blocksize == 1024)
        start++;
    if ((retval = io_channel_read_blk(journal_dev->io, start, -1024, buf)))
        return retval;

    jsb = (journal_superblock_t *) buf;
    if ((jsb->s_header.h_magic != (unsigned) ntohl(JFS_MAGIC_NUMBER)) ||
        (jsb->s_header.h_blocktype != (unsigned) ntohl(JFS_SUPERBLOCK_V2)))
        return EXT2_ET_NO_JOURNAL_SB;

    if (ntohl(jsb->s_blocksize) != fs->blocksize)
        return EXT2_ET_UNEXPECTED_BLOCK_SIZE;

    /* Check and see if this filesystem has already been added */
    nr_users = ntohl(jsb->s_nr_users);
    for (i=0; i < nr_users; i++) {
        if (memcmp(fs->ext2_sb->s_uuid,
               &jsb->s_users[i*16], 16) == 0)
            break;
    }
    if (i >= nr_users) {
        memcpy(&jsb->s_users[nr_users*16],
               fs->ext2_sb->s_uuid, 16);
        jsb->s_nr_users = htonl(nr_users+1);
    }

    /* Writeback the journal superblock */
    if ((retval = io_channel_write_blk(journal_dev->io, start, -1024, buf)))
        return retval;
    
    fs->ext2_sb->s_journal_inum = 0;
    fs->ext2_sb->s_journal_dev = st.st_rdev;
    memcpy(fs->ext2_sb->s_journal_uuid, jsb->s_uuid,
           sizeof(fs->ext2_sb->s_journal_uuid));
    fs->ext2_sb->s_feature_compat |= EXT3_FEATURE_COMPAT_HAS_JOURNAL;
    ext2fs_mark_super_dirty(fs);
    return 0;
}

/*
 * This function adds a journal inode to a filesystem, using either
 * POSIX routines if the filesystem is mounted, or using direct I/O
 * functions if it is not.
 */
bool ext2fs_add_journal_inode(PEXT2_FILESYS fs, ULONG size, int flags)
{
    bool        retval;
    ext2_ino_t      journal_ino;
    struct stat     st;
    char            jfile[1024];
    int         fd, mount_flags, f;

    if ((retval = ext2fs_check_mount_point(fs->device_name, &mount_flags,
                           jfile, sizeof(jfile)-10)))
        return retval;

    if (mount_flags & EXT2_MF_MOUNTED) {
        strcat(jfile, "/.journal");

        /*
         * If .../.journal already exists, make sure any 
         * immutable or append-only flags are cleared.
         */
#if defined(HAVE_CHFLAGS) && defined(UF_NODUMP)
        (void) chflags (jfile, 0);
#else
#if HAVE_EXT2_IOCTLS
        fd = open(jfile, O_RDONLY);
        if (fd >= 0) {
            f = 0;
            ioctl(fd, EXT2_IOC_SETFLAGS, &f);
            close(fd);
        }
#endif
#endif

        /* Create the journal file */
        if ((fd = open(jfile, O_CREAT|O_WRONLY, 0600)) < 0)
            return errno;

        if ((retval = write_journal_file(fs, jfile, size, flags)))
            goto errout;
        
        /* Get inode number of the journal file */
        if (fstat(fd, &st) < 0)
            goto errout;

#if defined(HAVE_CHFLAGS) && defined(UF_NODUMP)
        retval = fchflags (fd, UF_NODUMP|UF_IMMUTABLE);
#else
#if HAVE_EXT2_IOCTLS
        f = EXT2_NODUMP_FL | EXT2_IMMUTABLE_FL;
        retval = ioctl(fd, EXT2_IOC_SETFLAGS, &f);
#endif
#endif
        if (retval)
            goto errout;
        
        close(fd);
        journal_ino = st.st_ino;
    } else {
        journal_ino = EXT2_JOURNAL_INO;
        if ((retval = write_journal_inode(fs, journal_ino,
                          size, flags)))
            return retval;
    }
    
    fs->ext2_sb->s_journal_inum = journal_ino;
    fs->ext2_sb->s_journal_dev = 0;
    memset(fs->ext2_sb->s_journal_uuid, 0,
           sizeof(fs->ext2_sb->s_journal_uuid));
    fs->ext2_sb->s_feature_compat |= EXT3_FEATURE_COMPAT_HAS_JOURNAL;

    ext2fs_mark_super_dirty(fs);
    return 0;
errout:
    close(fd);
    return retval;
}
