#include "utils.h"

void clr_db_list(FILE * f);

void mark_db(FILE * f, uint32_t pos, bool value);

uint32_t find_open_db(FILE * f);

bool check_db(FILE * f, uint32_t pos);

uint32_t read_db_ct(FILE * f);

void set_db_ct(FILE * f, uint32_t val);

uint32_t calc_db_addr(uint32_t pos);

void clear_db(FILE * f, uint32_t pos);