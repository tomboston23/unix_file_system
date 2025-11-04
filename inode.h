#include "db.h"

uint32_t read_inodes_ct(FILE * f);

void set_inodes_ct(FILE * f, uint32_t val);

uint32_t calc_inode_addr(uint32_t pos);

void clr_inode_list(FILE * f);

void mark_inode(FILE * f, uint32_t pos, bool value);

uint32_t find_open_inode(FILE * f);

bool check_inode(FILE * f, uint32_t pos);

uint32_t get_cur_inode(FILE * f);

uint32_t change_cur_inode(FILE * f, uint32_t new_inode);

struct inode get_inode(FILE * f, uint32_t pos);

uint32_t alloc_inode(FILE * f, bool dir);

int find_free_ptr(FILE * f, uint32_t pos);

uint32_t alloc_db(FILE * f, uint32_t file);

void set_inode_name(FILE * f, uint32_t pos, char * name);

void get_inode_name(FILE * f, uint32_t pos, char * output);

uint32_t get_name_offset(uint32_t pos);

void write_inode(FILE * f, uint32_t pos, struct inode in);

void clear_inode(FILE * f, uint32_t pos);
