#include <stdint.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define BLOCK_SIZE 4096
#define FILE_SIZE 1048576
#define SUPERBLOCK_OFFSET 0
#define INODE_SIZE sizeof(struct inode)
#define INODE_OFFSET 64 //allow first 64 bits to be empty
#define NUM_INODES BLOCK_SIZE / INODE_SIZE
#define NUM_DATABLOCKS FILE_SIZE / BLOCK_SIZE
#define INODE_LIST_SIZE NUM_INODES / 8
#define DB_LIST_SIZE NUM_DATABLOCKS / 8
#define CUR_OFFSET DB_LIST_OFFSET + DB_LIST_SIZE
#define DATA_BLOCK_OFFSET BLOCK_SIZE
#define INODE_LIST_OFFSET sizeof(struct superblock)
#define DB_LIST_OFFSET INODE_LIST_OFFSET + NUM_INODES/8
#define NAMES_LIST_OFFSET BLOCK_SIZE
#define NAME_SIZE INODE_SIZE

#define NUM_PTRS 12

#define LINE_LENGTH 136

#define HELP 0
#define CD 1
#define MKDIR 2
#define TOUCH 3
#define READ 4
#define WRITE 5
#define RM 6
#define LS 7
#define QUIT 8

#define SUCCESS 0
#define NOTFOUND -1
#define DIRECTORY -2
#define EMPTY -3
#define INVALID -4
#define DUPLICATE -5


struct inode {
    uint32_t pos;
    uint32_t size;
    int parent_pos;
    uint32_t ptrs[NUM_PTRS]; // List of POSITIONS IN INODE, rather than POINTERS
    uint8_t dir; // 1 if directory, 0 if file
    uint8_t cur_ptr;
    uint16_t ptr_pos;
}; // 64 bytes

struct superblock {
    uint32_t u_db_count;
    uint32_t u_inodes_count;
};

void clr_8_bytes(FILE * f);

void clr_db(FILE * f, uint32_t db_addr);

