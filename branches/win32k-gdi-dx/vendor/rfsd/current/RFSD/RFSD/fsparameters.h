#ifndef __FS_PARAMETERS_H__
#define __FS_PARAMETERS_H__

#include "reiserfs.h"				// Simplified ReiserFS header
//#include <linux/reiserfs_fs.h>	// Full ReiserFS header


typedef struct reiserfs_super_block_v1		RFSD_SUPER_BLOCK, *PRFSD_SUPER_BLOCK;
typedef struct stat_data					RFSD_INODE, *PRFSD_INODE;

#define RFSD_CALLBACK(name)	NTSTATUS(* name )(ULONG BlockNumber, PVOID pContext)


typedef struct block_head					RFSD_BLOCK_HEAD, *PRFSD_BLOCK_HEAD;		// [mark]
typedef struct reiserfs_de_head				RFSD_DENTRY_HEAD, *PRFSD_DENTRY_HEAD;	// [mark]
typedef struct item_head					RFSD_ITEM_HEAD, *PRFSD_ITEM_HEAD;		// [mark]
typedef struct reiserfs_key					RFSD_KEY_ON_DISK, *PRFSD_KEY_ON_DISK;
typedef struct reiserfs_cpu_key				RFSD_KEY_IN_MEMORY, *PRFSD_KEY_IN_MEMORY;		
typedef struct disk_child					RFSD_DISK_NODE_REF, *PRFSD_DISK_NODE_REF; 

#define RFSD_NAME_LEN				255			/// Default length of buffers for filenames (although filenames may be longer)

#define SUPER_BLOCK_OFFSET              REISERFS_DISK_OFFSET_IN_BYTES
#define SUPER_BLOCK_SIZE                sizeof(RFSD_SUPER_BLOCK)

#define RFSD_ROOT_PARENT_ID			1			/// Part of the key for the root node
#define RFSD_ROOT_OBJECT_ID			2			/// Part of the key for the root node
#define RFSD_IS_ROOT_KEY(x)			(x.k_dir_id  == RFSD_ROOT_PARENT_ID && x.k_objectid  == RFSD_ROOT_OBJECT_ID)
#define RFSD_IS_PTR_TO_ROOT_KEY(x)	(x->k_dir_id == RFSD_ROOT_PARENT_ID && x->k_objectid == RFSD_ROOT_OBJECT_ID)

typedef short RFSD_KEY_COMPARISON;
typedef __u16 RFSD_KEY_VERSION;

#define RFSD_KEY_VERSION_1			0
#define RFSD_KEY_VERSION_2			1
#define RFSD_KEY_VERSION_UNKNOWN	7

// Results of a key comparison (as returned by CompareKeys)
#define RFSD_KEYS_MATCH			0
#define RFSD_KEY_SMALLER		-1
#define RFSD_KEY_LARGER			1


#define	RFSD_LEAF_BLOCK_LEVEL	1

#endif
