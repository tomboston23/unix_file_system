#include "utils.h"

void clr_8_bytes(FILE * f){ // this function assumes you have already seeked to the appropriate place
    char zeros[8] = {0};
    fwrite(zeros, sizeof(char), 8, f);
}

void clr_db(FILE * f, uint32_t db_addr){
    for(int i = 0; i < BLOCK_SIZE; i+=8){
        uint32_t addr = db_addr + i;
        fseek(f, addr, SEEK_SET);
        clr_8_bytes(f);
    }
}