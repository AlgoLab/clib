/*
    Author: Simone Stefanello
    Name: clib-clone.h
    Description: install repos outside the wiki in clib git repo 
                 (repos not intended to be downloaded by clib)
*/

int *check_manifest_for_packages(int n, char *pkgs[]);

int check_manifest_online(char *package_name);

// git clone command
int git_clone(char * package_name);

// parsing package name
int cc_parse_author(char *package_name_original, char *author);

int cc_parse_name(char *package_name_original, char *name, int r);

int cc_parse_version(char *package_name_original, char *version, int r);