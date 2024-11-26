/*
    Author: Simone Stefanello
    Name: clib-clone.h
    Description: install repos outside the wiki in clib git repo 
                 (repos not intended to be downloaded by clib)
*/

#ifndef MAX_CHAR
    #define MAX_CHAR 200
#endif

#define MAX_PATH 800
#define MAX_VER 15

struct pkg;

int *check_manifest_for_packages(int n, char *pkgs[]);

int check_manifest_online(char *package_name);

int check_errors(int r, char *pkg_name);

// git clone command
int git_clone(char * package_name, char *path, char *version);

// parsing package name
int cc_parse_author(char *package_name_original, char *author);

int cc_parse_name(char *package_name_original, char *name, int r);

int cc_parse_version(char *package_name_original, char *version, int r);

// clib update functions
int check_local_manifest_and_rm_package(char *package_name_original);

int *is_clib_repo(int n, char *pkgs[]);

int fork_checkout(char *path, char *version);