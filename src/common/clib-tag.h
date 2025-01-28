/*
    Author: Simone Stefanello
    Name: test-tag.c
    Description: implement a "namespace" by insert in every function identifier a prefix for the library,
                 used as an agreement in C programming. 
*/

#define MAX_LINE 1000
#define MAX_LINES 100
#define MAX_ROW 5
#define TAB_DELIM "\t"
#define NUM_SED_COMMANDS 3

typedef struct
{
    char identifier[MAX_CHAR];
    char line[MAX_ROW];
    char kind[MAX_CHAR];
} Tag;

int fork_ctags(char *functions_path,
              char *path_to_source);

void substitute_files(char *path, char *path_to_sed_script, char *path_to_modified, char *author_dep, char *name_dep);

void find_files(char *path, char *functions_path, int *log_check);

void find_dir(const char *path);

int rows_tags_file(char *path_tags);

Tag *parse_tags_file(const char *path_tags, int rows);
