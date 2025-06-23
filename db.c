#include "db.h"

void clr_db_list(FILE * f){
    fseek(f, DB_LIST_OFFSET, SEEK_SET);
    for(uint32_t offset = 0; offset < NUM_DATABLOCKS / 8; offset+= 8){
        uint32_t pos = offset + DB_LIST_OFFSET;
        fseek(f, pos, SEEK_SET);
        clr_8_bytes(f);
    }
    set_db_ct(f, 0);
}

uint32_t read_db_ct(FILE * f){
    struct superblock s;
    fseek(f, SUPERBLOCK_OFFSET, SEEK_SET);
    uint32_t ct;
    fread(&s, sizeof(uint32_t), 1, f);
    ct = s.u_db_count;
    return ct;
}

void set_db_ct(FILE * f, uint32_t val){
    struct superblock s;
    fseek(f, SUPERBLOCK_OFFSET, SEEK_SET);
    fread(&s, sizeof(uint32_t), 1, f);
    s.u_db_count = val;
}

void mark_db(FILE * f, uint32_t pos, bool value){
    if (pos >= NUM_DATABLOCKS) return;
    int byteoffset = DB_LIST_OFFSET + pos/8;
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

uint32_t find_open_db(FILE * f){
    uint32_t ct = read_db_ct(f);
    if (ct >= NUM_DATABLOCKS) printf("full\n");

    for(int i = 0; i < NUM_DATABLOCKS; i++){
        if (!check_db(f, i)){
            return i;
        }
    }
    return 0;
}

bool check_db(FILE * f, uint32_t pos){
    if ((long unsigned) pos >= NUM_INODES) return false;
    int byteoffset = DB_LIST_OFFSET + pos/8;
    int bitoffset = pos % 8;
    uint8_t byte;
    fseek(f, byteoffset, SEEK_SET);
    fread(&byte, sizeof(uint8_t), 1, f);
    return (byte & (1 << bitoffset));
}

uint32_t calc_db_addr(uint32_t pos){
    return pos * BLOCK_SIZE + DATA_BLOCK_OFFSET;
}

void clear_db(FILE * f, uint32_t pos){
    if (pos >= NUM_DATABLOCKS) return;
    uint32_t addr = calc_db_addr(pos);
    uint64_t z = 0;
    fseek(f, addr, SEEK_SET);
    fwrite(&z, sizeof(uint64_t), BLOCK_SIZE/sizeof(uint64_t), f);
}