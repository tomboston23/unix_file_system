#include "fs.h"

void mkfs(FILE * f){
    fseek(f, 0, SEEK_SET); // set to beginning of file
    struct superblock s;
    s.u_db_count = 0;
    s.u_inodes_count = 0;
    fwrite(&s, sizeof(struct superblock), 1, f);
    clr_inode_list(f);
    clr_db_list(f);
    mark_db(f, 0, true);
    mark_inode(f, 0, true);
    mark_db(f, 1, true); // alloc space for names
    fseek(f, NAMES_LIST_OFFSET, SEEK_SET);
    char i = 0;
    fwrite(&i, sizeof(char), BLOCK_SIZE/sizeof(char), f); //clear name list
    change_cur_inode(f, 0);
    uint32_t ret = alloc_inode(f, true);
    if(!ret) return;
    // printf("directory allocated at: %u\n", ret);
    char name[64] = "";
    set_inode_name(f, ret, name);
    change_cur_inode(f, ret);
}



FILE * fsopen(){
    FILE * f = fopen("disk.img", "rb+");
    if (!f) {
        perror("open");
        printf("Failed to open file\n");
        return NULL;
    }
    return f;
} 

void write_helper(char * buffer, size_t size, FILE * f, uint32_t file){
    // printf("write_helper called\n");
    struct inode i = get_inode(f, file);
    uint8_t ptr = i.cur_ptr;
    uint16_t pos = i.ptr_pos;
    int rem_size = 0;
    if (pos + size > BLOCK_SIZE){
        rem_size = size;
        size = BLOCK_SIZE - pos;
        rem_size -= size;
    }
    uint32_t db_id = i.ptrs[ptr];
    if (!db_id) {
        db_id = alloc_db(f, file);
        i.size++;
        i.ptrs[ptr] = db_id;
    }
    
    fseek(f, calc_db_addr(db_id) + pos, SEEK_SET);
    i.ptr_pos += size;
    fwrite(buffer, sizeof(char), size, f);

    if(!rem_size) {
        write_inode(f, file, i); // really important ! save our work
        // printf("File size: dbs: %d; chars: %d\n", i.size, i.ptr_pos);
        return;
    }

    i.cur_ptr++;
    db_id = i.ptrs[ptr+1];
    if(!db_id){
        db_id = alloc_db(f, file);
        i.size++;
        i.ptrs[ptr+1] = db_id;
    }

    fseek(f, calc_db_addr(db_id), SEEK_SET);
    i.ptr_pos = rem_size;
    fwrite(buffer + size, sizeof(char), rem_size, f);
    
    write_inode(f, file, i); // really important ! save our work
    // printf("File size: dbs: %d; chars: %d\n", i.size, i.ptr_pos);
    
}

void fswrite(FILE * f, uint32_t file){
    int num_lines;
    printf("How many lines would you like to write: ");
    scanf("%d", &num_lines);
    for(int i = 0; i <= num_lines; i++){
        char line[LINE_LENGTH];
        fgets(line, sizeof(line), stdin);

        if(!i) continue; // fgets gets triggered on the first iteration before you type anything
        //hence the for loop being inclusive

        if (line[strlen(line) - 1] != '\n') {
            printf("Warning: input was likely truncated.\n");
            line[strlen(line) - 1] = '\n';
        }
        write_helper(line, strlen(line), f, file);
    }
}   

void fsread(FILE * f, uint32_t file){
    char buffer[BLOCK_SIZE];
    struct inode file_inode = get_inode(f, file);
    for(int i = 0; i <= file_inode.cur_ptr; i++){
        uint32_t db = file_inode.ptrs[i];
        uint32_t addr = calc_db_addr(db);
        fseek(f, addr, SEEK_SET);
        fread(buffer, sizeof(char), BLOCK_SIZE, f);
        if (strlen(buffer) > 0){
            printf("%s", buffer);
        }
    }
}   



