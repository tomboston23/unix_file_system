#include "fs.h"

int str_check_valid(char * s, bool file){
    char invalid_chars[] = {'.', ',', '/', '\\', '\"', '\'', ':', '>', '<', '?', '*', ' '};
    int invalid_size = sizeof(invalid_chars) / sizeof(char);
    int dotcnt = 0;
    for(int i = 0; *(s + i); i++){
        for(int j = 0; j < invalid_size; j++){
            if (*(s+i) == invalid_chars[j]) {
                if (j == 0) dotcnt++;
                else return false;
            }
        }
        if (dotcnt > (int)file) return false; // allow 1 . for files, 0 for directories
    }
    return true;
}

char** str_split(char* a_str, const char a_delim)
{
    char** result    = 0;
    size_t count     = 0;
    char* tmp        = a_str;
    char* last_comma = 0;
    char delim[2];
    delim[0] = a_delim;
    delim[1] = 0;

    /* Count how many elements will be extracted. */

    while (*tmp)
    {
        if (a_delim == *tmp)
        {
            count++;
            last_comma = tmp;
        }
        tmp++;
    }

    /* Add space for trailing token. */
    count += last_comma < (a_str + strlen(a_str) - 1);

    /* Add space for terminating null string so caller
       knows where the list of returned strings ends. */
    count++;

    result = malloc(sizeof(char*) * count);

    if (result)
    {
        size_t idx  = 0;
        char* token = strtok(a_str, delim);

        while (token)
        {
            *(result + idx++) = strdup(token);
            token = strtok(0, delim);
        }

        *(result + idx) = 0;
    }

    return result;
}


bool str_compare(char * b, char * c){
    int i = 0;
    while(true){
        if (b[i] != c[i]){
            return false;
        }
        if (b[i] == 0 && c[i] == 0) return true;
        i++;
    }
    return true;
}


int find(FILE * f, char * selection){
    uint32_t inode_id = get_cur_inode(f);
    struct inode cur_inode = get_inode(f, inode_id);

    uint32_t child_files[12] = {0};
    int files = 0;
    int dirs = 0; 
    uint32_t child_dirs[12] = {0};

    char name[64] = {0};

    for(int j = 0; j < 12; j++){
        uint32_t p = cur_inode.ptrs[j];
        if (p){
            struct inode child = get_inode(f, p);
            if (child.dir){
                child_dirs[dirs] = p;
                dirs++;
            } else {
                child_files[files] = p;
                files++;
            }
        }
    }
    
    for(int j = 0; j < files; j++){
        uint32_t val = child_files[j];

        if(!val) continue;
        get_inode_name(f, val, name);
        // printf("dir: %d\n", dir);
        if (str_compare(name, selection)){
            return (int)val;
        }
    }
    for(int k = 0; k < dirs; k++){
        uint32_t val = child_dirs[k];
        if(!val) continue;
        get_inode_name(f, val, name);
        if(str_compare(name, selection)){
            return DIRECTORY;
        }
    }

    return NOTFOUND;
}



void help(){
    char help_text[] =
        "List of commands:\n"
        "\t-- cd [directory name]: change directory\n"
        "\t-- mkdir [directory name]: create directory\n"
        "\t-- touch [file name]: create file\n"
        "\t-- read [file name]: read from current file\n"
        "\t-- write [file name]: write to current file\n"
        "\t-- ls: list contents of current directory\n"
        "\t-- rm [file/dir name]: remove file or directory\n"
        "\t-- q: quit\n";

    printf("%s", help_text);
}

int cd(FILE * f, char * b){
    char ** tokens = str_split(b, '/');

    uint32_t num = get_cur_inode(f);

    for(int i = 0; *(tokens + i); i++){
        uint32_t inode_id = get_cur_inode(f);
        struct inode cur_inode = get_inode(f, inode_id);
        uint32_t child_dirs[12] = {0};
        uint32_t parent = cur_inode.parent_pos;

        int dirs = 0; 
        for(int j = 0; j < 12; j++){
            uint32_t p = cur_inode.ptrs[j];
            if (p){
                struct inode child = get_inode(f, p);
                if (child.dir){
                    child_dirs[dirs] = p;
                    dirs++;
                }
            }
        }

        if (!dirs && !parent) {
            return EMPTY;
        }


        char selection[64];
        strncpy(selection, tokens[i], sizeof(selection) - 1);
        char dots[3] = "..";
        if (str_compare(selection, dots)){
            if (parent){
                change_cur_inode(f, parent);
                continue;
            }
            else return NOTFOUND;
        }

        bool good = false;
        char name[64] = {0};

        for(int j = 0; j < 12; j++){
            uint32_t val = child_dirs[j];
            if(!val) continue;
            get_inode_name(f, val, name);

            if (str_compare(name, selection)){
                num = val;
                good = true;
                break;
            }
        }
        
        if (!good) {
            num = inode_id;
            change_cur_inode(f, num);
            return NOTFOUND;
        }

        change_cur_inode(f, num);
    }
    return SUCCESS;
}


void rm(FILE * f, uint32_t id){ // remove the file with the specified id
    // we are assuming id has been removed from parent directory's pointer list already
    char name[64] = {0};
    mark_inode(f, id, false); // clear occupied bit
    set_inode_name(f, id, name); // clear name
    struct inode in = get_inode(f, id);
    for(int i = 0; i < 12; i++){ // clear ptrs list -- easy because it's a file
        uint32_t db = in.ptrs[i];
        if (!db) continue;
        mark_db(f, db, false);
        clear_db(f, db);
    }
    
    clear_inode(f, id);

    printf("file removed\n");
    return;
}

int create(FILE * f, bool dir, char * p){

    uint32_t cache = get_cur_inode(f);

    char ** tokens = str_split(p, '/');

    size_t size = 0;
    while (tokens[size] != NULL) {
        size++;
    }

    for(size_t i = 0; i < size-1; i++){
        int r = cd (f, tokens[i]);
        if (r == NOTFOUND || r == EMPTY){
            char name[NAME_SIZE];
            uint32_t cur = get_cur_inode(f);
            get_inode_name(f, cur, name);
            printf("Directory %s has no sub-directory %s\n", name, tokens[i]);
            return r;
        }
    }

    char name[64] = {0};
    strncpy(name, tokens[size-1], sizeof(name) - 1);
    if (!str_check_valid(name, (int)!dir)){
        return INVALID;
    }
    int b = find(f, name);
    if(b >= 0 && !dir){
        // trying to create a duplicate file name
        change_cur_inode(f, cache);
        return DUPLICATE;
        
    } else if (b == DIRECTORY && dir){
        // trying to create a duplicate directory name
        change_cur_inode(f, cache);
        return DUPLICATE;
    }
    uint32_t ret = alloc_inode(f, dir);
    if (ret == 0) return SUCCESS;
    set_inode_name(f, ret, name);
    change_cur_inode(f, cache);
    // printf("changing cur inode back to %s\n", inodename);
    return SUCCESS;
}

void read_write(FILE * f, bool w, char * p){

    uint32_t cache = get_cur_inode(f);

    char ** tokens = str_split(p, '/');

    size_t size = 0;
    while (tokens[size] != NULL) {
        size++;
    }

    for(size_t i = 0; i < size-1; i++){
        int r = cd (f, tokens[i]);
        if (r == NOTFOUND || r == EMPTY){
            char name[NAME_SIZE];
            uint32_t cur = get_cur_inode(f);
            get_inode_name(f, cur, name);
            printf("Directory %s has no sub-directory %s\n", name, tokens[i]);
            return;
        }
    }

    char selection[64] = {0};

    strncpy(selection, tokens[size-1], sizeof(selection) - 1);

    int r = find(f, selection);
    if (r > 0){
        if(w)fswrite(f, r);
        else fsread(f,r);
    }  else if (r == DIRECTORY){
        printf("Cannot access %s: Is a directory\n", selection);
    } else if (r == NOTFOUND){
        printf("Cannot access %s: File/directory not found!\n", selection);
    } else {
        printf("unexpected behavior in read/write\n");
    }

    change_cur_inode(f, cache);
}


void rmfile(FILE * f, char * p){

    char ** tokens = str_split(p, '/');
    uint32_t cache = get_cur_inode(f);
    size_t size = 0;
    while (tokens[size] != NULL) {
        size++;
    }

    for(size_t i = 0; i < size-1; i++){
        int r = cd (f, tokens[i]);
        if (r == NOTFOUND || r == EMPTY){
            char name[NAME_SIZE];
            uint32_t cur = get_cur_inode(f);
            get_inode_name(f, cur, name);
            printf("Directory %s has no sub-directory %s\n", name, tokens[i]);
        }
    }


    char selection[64] = {0};
    strncpy(selection, tokens[size-1], sizeof(selection) - 1);
    int r = find(f, selection);
    if (r > 0){
        uint32_t parent = get_cur_inode(f);
        struct inode p = get_inode(f, parent);

        //clear parent ptr
        for(int i = 0; i < NUM_PTRS; i++){
            if (p.ptrs[i] == (unsigned)r){
                p.ptrs[i] = 0;
            }
        }
        write_inode(f, parent, p);
        rm(f, r);
    } else if (r == DIRECTORY){
        printf("Cannot remove %s: Is a directory\n", selection);
    } else if (r == NOTFOUND){
        printf("Cannot remove %s: File/directory not found!\n", selection);
    }

    change_cur_inode(f, cache);
}

void ls(FILE * f){
    uint32_t inode_id = get_cur_inode(f);
    struct inode cur_inode = get_inode(f, inode_id);

    uint32_t child_files[12] = {0};
    int files = 0;
    int dirs = 0; 
    uint32_t child_dirs[12] = {0};
    char name[64] = {0};

    for(int j = 0; j < 12; j++){
        uint32_t p = cur_inode.ptrs[j];
        if (p){
            struct inode child = get_inode(f, p);
            if (child.dir){
                child_dirs[dirs] = p;
                dirs++;
            } else {
                child_files[files] = p;
                files++;
            }
        }
    }

    for(int j = 0; j < dirs; j++){
        // printf("directory found at index %d\n", j);
        uint32_t dir = child_dirs[j];
        get_inode_name(f, dir, name);
        printf("\033[34m%s\033[0m\n", name);
    }

    for(int k = 0; k < files; k++){
        // printf("file found at index %d\n", k);
        uint32_t file = child_files[k];
        get_inode_name(f, file, name);
        printf("%s\n", name);
    }
    // printf("\n");
}


int subroutine(FILE * f, int i, char ** b){

    size_t size = 0;
    while (b[size] != NULL) {
        size++;
    }

    // printf("size: %ld\n", size);

    int r;

    switch(i){
        case HELP:
            if (size > 1) {
                printf("Too many input arguments\n");
                break;
            }
            help();
            break;
        case CD:
            uint32_t cache = get_cur_inode(f);
            if (size != 2){
                printf("Invalid input arguments!\n");
                break;
            }
            char * name = malloc(strlen(b[1]));
            strncpy(name, b[1], strlen(b[1]));
            r = cd(f, b[1]);
            if (r == NOTFOUND){
                printf("Directory %s not found!\n", name);
                change_cur_inode(f, cache);
            }
            break;
        case MKDIR:
            if (size != 2){
                printf("Invalid input arguments!\n");
                break;
            }
            r = create(f, true, b[1]);
            if (r == INVALID){
                printf("Directory name contains invalid characters!\n");
            } else if (r == DUPLICATE){
                printf("Directory already exists!\n");
            }
            break;
        case TOUCH:
            if (size != 2){
                printf("Invalid input arguments!\n");
                break;
            }
            r = create(f, false, b[1]);
            if (r == INVALID){
                printf("File name contains invalid characters!\n");
            } else if (r == DUPLICATE){
                printf("File already exists!\n");
            }
            break;
        case READ:
            if (size != 2){
                printf("Invalid input arguments!\n");
                break;
            }
            read_write(f,false, b[1]);

            break;
        case WRITE:
            if (size != 2){
                printf("Invalid input arguments!\n");
                break;
            }
            read_write(f, true, b[1]);
            break;
        case RM:
            if (size != 2){
                printf("Invalid input arguments!\n");
                break;
            }
            rmfile(f, b[1]);
            break;
        case LS:
            if (size > 1){
                printf("Too many input arguments\n");
                break;
            }
            ls(f);
            break;
        case QUIT:
            if (size > 1){
                printf("Too many input arguments\n");
                break;               
            }
            return 0;
    }

    return 1;
}





int main(){
    FILE * file = fsopen();
    mkfs(file);

    char b[100] = {0};

    printf("\nEnter your command - type h for help:\n");

    char commands[][6] = {"h", "cd", "mkdir", "touch", "read", "write", "rm", "ls", "q"};


    while(true){
        printf("\033[32mpath\033[0m:");
        uint32_t in = get_cur_inode(file);
        uint32_t path[NAME_SIZE] = {0};
        int i = 0;
        while(true){
            path[i] = in;
            // printf("%u added to path\n", in);
            in = get_inode(file, in).parent_pos;
            if (!in) break;
            i++;
        }
        // printf("finished while loop\n");
        char pathname[NAME_SIZE] = {0};
        printf("\033[34m");
        if (i == 0) printf("~");
        for(int j = i-1; j >= 0; j--){
            uint32_t num = path[j];
            get_inode_name(file, num, pathname);
            printf("/%s", pathname);
        }
        printf("\033[0m$ ");
        fgets(b, 100, stdin);

        size_t len = strlen(b) - 1;
        while(len > 0){
            if(b[len] == '\n' || b[len] == ' '){
                b[len] = '\0';
            } else {
                break;
            }
        }

        char**tokens = str_split(b, ' ');

        size_t size = 0;
        while (tokens[size] != NULL) {
            size++;
        }

        // if (tokens)
        // {
        //     int i;
        //     for (i = 0; *(tokens + i); i++)
        //     {
        //         char * t = *(tokens + i);
        //         printf("[");
        //         for (int j = 0; *(t + j); j++){
        //             unsigned char c = *(t+j);
        //             printf("%u ", (unsigned)c);
        //         }
        //         printf("]");
        //     }
        //     printf("\n");
        // }

        for(int i = 0; i < 9; i++){
            if(str_compare(*tokens, commands[i])){
                if(!subroutine(file, i, tokens)) return 0;
                break;
            }
        }
        for (i = 0; *(tokens + i); i++)
        {
            free(*(tokens+i));
        }
        free(tokens);
    }
    return 0;
} 

