#include "inode.h"

void mark_inode(FILE * f, uint32_t pos, bool value){
    if (pos >= NUM_INODES) return;
    int byteoffset = INODE_LIST_OFFSET + pos/8;
    int bitoffset = pos % 8;
    uint8_t byte;
    fseek(f, byteoffset, SEEK_SET);
    fread(&byte, sizeof(uint8_t), 1, f);
    if (value){
        byte |= 1 << bitoffset; //set bit
    } else {
        byte &= ~ (1 << bitoffset); // unset bit
    }

    fseek(f, byteoffset, SEEK_SET);
    fwrite(&byte, sizeof(uint8_t), 1, f);
}

void clr_inode_list(FILE * f){ //set all inodes to available
    fseek(f, INODE_LIST_OFFSET, SEEK_SET);
    clr_8_bytes(f);
    set_inodes_ct(f, 0);
}

bool check_inode(FILE * f, uint32_t pos){
    if ((long unsigned) pos >= NUM_INODES) return false;
    int byteoffset = INODE_LIST_OFFSET + pos/8;
    int bitoffset = pos % 8;
    uint8_t byte;
    fseek(f, byteoffset, SEEK_SET);
    fread(&byte, sizeof(uint8_t), 1, f);
    return (byte & (1 << bitoffset));
}

uint32_t read_inodes_ct(FILE * f){
    struct superblock s;
    fseek(f, SUPERBLOCK_OFFSET, SEEK_SET);
    uint32_t ct;
    fread(&s, sizeof(struct superblock), 1, f);
    ct = s.u_inodes_count;
    return ct;
}

void set_inodes_ct(FILE * f, uint32_t val){
    struct superblock s;
    fseek(f, SUPERBLOCK_OFFSET, SEEK_SET);
    fread(&s, sizeof(uint32_t), 1, f);
    s.u_inodes_count = val;
}

uint32_t find_open_inode(FILE * f){
    fseek(f, INODE_LIST_OFFSET, SEEK_SET);
    uint8_t list[INODE_LIST_SIZE];
    fread(list, sizeof(uint8_t), INODE_LIST_SIZE, f);


    for(long unsigned i = 0; i < INODE_LIST_SIZE; i++){

        uint8_t byte = list[i];

        for(long unsigned bit = 0; bit < 8; bit++){

            if (!(byte & (1 << bit))){
                return i*8 + bit;
            }
        }
    }


    return 0;
}

uint32_t calc_inode_addr(uint32_t pos){
    return pos * INODE_SIZE;
}

uint32_t change_cur_inode(FILE * f, uint32_t new_inode){ // sets dir to new_dir, returns old dir as well.
    uint32_t cur_inode = get_cur_inode(f);
    fseek(f, CUR_OFFSET, SEEK_SET);
    fwrite(&new_inode, sizeof(int), 1, f);
    return cur_inode;
}

uint32_t get_cur_inode(FILE * f){
    fseek(f, CUR_OFFSET, SEEK_SET);
    uint32_t cur_inode;
    fread(&cur_inode, sizeof(int), 1, f);
    return cur_inode;
}

uint32_t alloc_inode(FILE * f, bool dir){ // allocates a file / directory in the inodes block
    // newly allocated inodes are empty and have a size of zero
    //check if full
    uint32_t ct = read_inodes_ct(f);
    // printf("inodes count: %u\n", ct);
    if (ct >= NUM_INODES) {
        printf("Error: No inodes available!\n");
        return 0;
    }

    //find open inode position
    uint32_t pos = find_open_inode(f);
    // printf("New alloc: %u\n", pos);
    
    if (!pos) {
        printf("File system is full!\n");
        return 0;
    }

    uint32_t d = get_cur_inode(f);
    int free = -1;

    uint32_t parent = 0;
    uint32_t parent_addr = 0;

    if (d){
        parent = d;
        parent_addr = calc_inode_addr(parent);
        free = find_free_ptr(f, parent);
        if (free == -1){
            printf("Current directory full!\n");
            scanf("%*s");
            return 0;
        }
        // printf("free: %d\n", free);
    }

    // mark it as used
    set_inodes_ct(f, ct+1);
    uint32_t addr = calc_inode_addr(pos);
    mark_inode(f, pos, true);

    struct inode i;
    i.pos = pos;
    i.size = 0;
    i.dir = (uint8_t) dir;
    i.cur_ptr = 0;
    i.ptr_pos = 0;
    // printf("inode at pos %u set dir to %u\n", i.pos, i.dir);
    i.parent_pos = parent;
    memset(i.ptrs, 0, 12*sizeof(uint32_t)); // clear ptrs

    // printf("parent_addr: %u\n", parent_addr);

    if(parent_addr){
        struct inode p = get_inode(f, parent);
        // printf("inode values:\npos: %u\nsize: %u\nparent_pos: %d\ndir: %u\n", p.pos, p.size, p.parent_pos, p.dir);

        if (!p.dir) {
            printf("You need to be in a valid directory!\n");
            return 0;
        }
        p.size += 1;
        p.ptrs[free] = pos;
        // printf("placing inode %u in free: %u\n", pos, free);
        write_inode(f, parent, p);
    }

    fseek(f, addr, SEEK_SET);
    fwrite(&i, sizeof(struct inode), 1, f);

    set_inodes_ct(f, ct+1);

    // printf("inode allocated at: %u\n", pos);

    return pos;
}

int find_free_ptr(FILE * f, uint32_t pos){
    struct inode in = get_inode(f, pos);

    for(int i = 0; i < 12; i++){
        if (!in.ptrs[i]){
            return i;
        }
    }
    return -1;
}

struct inode get_inode(FILE * f, uint32_t pos){
    struct inode in;
    uint32_t addr = calc_inode_addr(pos);
    fseek(f, addr, SEEK_SET);
    fread(&in, sizeof(struct inode), 1, f);
    return in;
}

void write_inode(FILE * f, uint32_t pos, struct inode in){
    if (pos >= NUM_INODES) return;
    uint32_t addr = calc_inode_addr(pos);
    fseek(f, addr, SEEK_SET);
    fwrite(&in, sizeof(struct inode), 1, f);
}

void clear_inode(FILE * f, uint32_t pos){
    if (pos >= NUM_INODES) return;
    uint32_t addr = calc_inode_addr(pos);
    uint64_t z = 0;
    fseek(f, addr, SEEK_SET);
    fwrite(&z, sizeof(uint64_t), 1, f);
}


uint32_t alloc_db(FILE * f, uint32_t file){
    uint32_t ct = read_db_ct(f);

    if (ct >= NUM_DATABLOCKS) {
        printf("No datablocks available!");
        return 0;
    }

    uint32_t pos = find_open_db(f);
    if (!pos){
        printf("File system is full!");
        return 0;
    }

    struct inode cur_inode = get_inode(f, file);

    int free = -1;

    // if(!inode_id || cur_inode.dir) {
    //     printf("You need to be in a file inode!\n");
    //     return 0;
    // }

    free = find_free_ptr(f, file);
    if(free == -1){
        printf("Current file is full!\n");
        return 0;
    }

    //all checks passed
    write_inode(f, file, cur_inode);

    mark_db(f, pos, true);

    set_db_ct(f, ct+1);

    clr_db(f, calc_db_addr(pos));

    return pos;
}


uint32_t get_name_offset(uint32_t pos){
    return NAMES_LIST_OFFSET + NAME_SIZE*pos;
}

void set_inode_name(FILE * f, uint32_t pos, char * name){
    uint32_t addr = get_name_offset(pos);
    fseek(f, addr, SEEK_SET);
    name[NAME_SIZE - 1] = '\0';
    fwrite(name, sizeof(char), NAME_SIZE, f);
    // printf("\n%s name written for pos: %u\n", name, addr);
}

void get_inode_name(FILE * f, uint32_t pos, char * output){ //put it in output
    uint32_t addr = get_name_offset(pos);
    fseek(f, addr, SEEK_SET);
    fread(output, sizeof(char), NAME_SIZE, f);
    output[NAME_SIZE - 1] = '\0';
    // printf("\n%s name retrieved for pos: %u\n", output, addr);
}