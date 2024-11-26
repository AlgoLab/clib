/*
    Author: Simone Stefanello
    Name: clib-clone.c
    Description: install repos outside the wiki in clib git repo 
                 (repos not intended to be downloaded by clib)
*/

#define MAX_CHAR 100
#define MAX_URL 200
#define MAX_COMMAND 400

// for ntfw
#define _GNU_SOURCE
#define _XOPEN_SOURCE 500

#define NOT_FOUND_GIT "404: Not Found"
#define INVALID_REQUEST_GIT "400: Invalid Request"

#define HOME_ENV_VARIABLE "HOME"
#define PWD_ENV_VARIABLE "PWD"

#define GITHUB_REPO_URL "https://github.com/"
#ifndef GITHUB_CONTENT_URL
    #define GITHUB_CONTENT_URL "https://raw.githubusercontent.com/"
#endif

#define DEFAULT_VERSION "0.1.0"

#include <stdlib.h>
#include <string.h>
#include <ftw.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <errno.h>
#include <fcntl.h>
#include "clib-package.h"
#include "http-get/http-get.h"
#include "clib-clone.h"
#include "logger/logger.h"
#include "parson/parson.h"
#include "fs/fs.h"

// functions declaration
int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf);

int rm_rf(char *path);

int check_local_manifest_and_rm_package(char *package_name_original);

int *is_clib_repo(int n, char *pkgs[]);

// return a pointer to array of int (length: n) with responses
// of check_manifest_online()
// 0 if found, -1 if not found
int *check_manifest_for_packages(int n,  char *pkgs[])
{
    // answers for every package
    int *res = (int *)malloc(n * sizeof(int)); 

    if (res != NULL)
    {
        for (int i = 0; i < n; i++)
        {
            // check for every package if there is a manifest online
            res[i] = check_manifest_online(pkgs[i]);
        }
    }
    else
    {
        // res = NULL
        return res;
    }
    return res;
}

// return 0 -> manifest found
// return -1 -> manifest not found
// return 1 -> http_get problem
// ToDo: add control for clib.json
int check_manifest_online(char *package_name_original)
{
    // doesn't destroy original package_name
    char package_name[MAX_CHAR];
    strcpy(package_name, package_name_original);

    // author/name@version
    char author[MAX_CHAR] = "";
    char name[MAX_CHAR] = "";
    char version[MAX_CHAR] = "";

    // http_get request struct initialized to zero
    http_get_response_t *res = NULL;

    // parsing package name author/name
    int r_auth = cc_parse_author(package_name_original, author);
    if (r_auth > 0)
    {
        // syntax error
        // remember to add generic error
        return 2;
    }

    int r_name = cc_parse_name(package_name_original, name, r_auth);
    if (r_name > 0)
    {
        // syntax error
        // remember to add generic error
        return 2;
    }

    int r_ver;
    if (r_name == 0)
    {
        r_ver = cc_parse_version(package_name_original, version, r_name);
    }

    if (r_ver != 0 && r_ver != 2)
    {
        // syntax error
        return 2;
    }
    
    char url_repo[MAX_URL] = "";
    // build url of repo without version
    strncat(url_repo, clib_package_url_withour_ver(author, name), MAX_URL - 1);

    // build url of manifest (clib.json first, package.json second)
    char tmp_url_package[MAX_URL] = "";
    //char tmp_url_clib[MAX_URL];

    // master branch for package.json
    strcpy(tmp_url_package, url_repo);
    strcat(tmp_url_package, "/master/package.json");

    // http_get request
    res = http_get(tmp_url_package);

    if (res == NULL)
    {
        // Problem with http_get
        return 1;
    }

    // res->data == "404: Not Found" or "400: Invalid Request"
    // NB: it is possible to automatically check this error throw res->status
    // package.json not found
    if (strcmp(res->data, NOT_FOUND_GIT) == 0 || 
        strcmp(res->data, INVALID_REQUEST_GIT) == 0)
    {
        return -1;
    }
    else
    {
        // manifest exist
        return 0;
    }

    // error
    return 1;
}

int check_errors(int r, char *pkg_name)
{
    int i = 0;

    switch(r)
    {
        case -1:    logger_warn("warning", 
                    "package.json or clib.json not found for %s, git cloning",
                    pkg_name);
                        
                    int response = git_clone(pkg_name, NULL, NULL);

                    if (response >= 1)
                    {
                        logger_error("error", 
                        "git calling failed");
                    }
                    else if (response == -1)
                    {
                        logger_error("error", 
                        "impossible to create directories for %s package", 
                        pkg_name);
                    }
                    else if (response == -2)
                    {
                        logger_error("error",
                        "impossible create manifest package.json for %s package",
                        pkg_name);
                    }

                    i = 1;
                    break;
                    // if clib designed package
        case 0:     break;
        case 1:     logger_error("error", 
                    "internet problem");
                    logger_error("error", 
                    "package %s not installed", 
                    pkg_name);

                    i = 1;
                    break;
        case 2:     logger_error("error", 
                    "argument syntax error for %s package",
                    pkg_name);

                    i = 1;
                    break;
        default:    logger_error("error",
                    "generic error");

                    i = 1;
    }

    return i;
}

// if WEXITSTATUS == 0 -> git clone successfull
// if WEXITSTATUS != 0 -> git clone unsuccessfull
int fork_git(char *url_repo, char *path)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        // error to fork git
        logger_error("error", "impossible to call git clone");
        // 1 is a generic return error 
        return 1;
    }
    else if (pid == 0)
    {
        // redirect stdout to dev/null for not printing
        // result of git
        int fd = open("/dev/null", O_WRONLY);
        if (fd < 0) {
            perror("open /dev/null failed");
            exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 stdout failed");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDERR_FILENO) < 0) {
            perror("dup2 stderr failed");
            exit(EXIT_FAILURE);
        }

        close(fd);

        // array with git clone args
        // It must be NULL terminated
        char *arg[] = {"git", "clone", url_repo, path, NULL};

        // Child process call: git
        execvp(arg[0], arg);

        // execvp never return if process success, if not returns 1
        exit(EXIT_FAILURE);
    }
    else
    {   
        // Parent process
        int status;

        int r = waitpid(pid, &status, 0);

        if (r == -1)
        {
            // generic error
            return 1;
        }

        if (WIFEXITED(status))
        {
            // if WEXITSTATUS(status) = 0 no problem, if != 0 problem
            return WEXITSTATUS(status);
        }
        else
        {
            return 0;
        }
    }
}

// return 1 if cannot open file
// return 2 if there are a json problem
// return 0 if it's ok
int create_manifest(char *author, char *name, char *version, char *path)
{
    // name, author, version, clib_repo : 1 or 0
    JSON_Value *root_value = json_value_init_object();
    JSON_Object *root_object = json_value_get_object(root_value);

    char *json_string = NULL;

    json_object_set_string(root_object, "author", author);
    json_object_set_string(root_object, "name", name);

    if (version == NULL)
    {
        json_object_set_string(root_object, "version", DEFAULT_VERSION);
    }
    else
    {
        json_object_set_string(root_object, "version", version);
    }

    // 0 if repo isn't a clib repo
    // this function is called when repo isn't a clib designed repo
    json_object_set_number(root_object, "clib_repo", 0);

    json_string = json_serialize_to_string_pretty(root_value);

    FILE *fp;

    const char manifest_name[] = "/package.json";
    
    // adding to path /package.json
    char tmp_path[MAX_PATH] = "";
    strncat(tmp_path, path, MAX_PATH - 1);
    strncat(tmp_path, manifest_name, strlen(manifest_name));

    // write mode: if not exist, create it
    // if exist, recreate it
    fp = fopen(tmp_path, "w");

    if (fp == NULL)
    {
        // cannot open file
        return 1;
    }

    if (json_string == NULL)
    {
        // json problem
        fclose(fp);
        return 2;
    }
    
    // write to file json
    fputs(json_string, fp);

    // ok
    fclose(fp);

    logger_info("info", 
    "manifest package.json for %s/%s succesfully created",
    author, name);

    return 0;
}

// git clone repo to $PWD/deps/package_name
// restriction: $PWD must be a clib project root dir (it must contain /deps dir)
// return 0 if success
int git_clone(char *package_name_original, char *path, char *version_e)
{
    char package_name[MAX_CHAR] = "";

    // build path to -/project_root/deps/package_name
    char path_cache[MAX_PATH] = "";
    
    // install packages in  home/$USER/.cache/clib/packages
    // ---------------------------------------------
    // strcat(path_cache, getenv(HOME_ENV_VARIABLE));
    // strcat(path_cache, "/.cache/clib/packages/");

    // build path to -/project_root/deps/package_name
    strncat(path_cache, getenv(PWD_ENV_VARIABLE), MAX_PATH - 1);
    strncat(path_cache, "/deps/", MAX_PATH - 1);

    // mkdir for package_name clone in /deps
    struct stat sb;
    if (stat(path_cache, &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        // doesn't destroy original package_name
        strncpy(package_name, package_name_original, MAX_CHAR - 1);

        // author/name@version
        char author[MAX_CHAR] = "";
        char name[MAX_CHAR] = "";
        char version[MAX_URL] = "";
        
        // control if author and name != NULL
        // and length of author and name != 0
        int r_auth = cc_parse_author(package_name_original, author);
        if (r_auth > 0)
        {
            // syntax error
            // remember to add generic error
            return 2;
        }

        int r_name = cc_parse_name(package_name_original, name, r_auth);
        if (r_name > 0)
        {
            // syntax error
            // remember to add generic error
            return 2;
        }

        int r_ver;
        if (r_name == 0)
        {
            r_ver = cc_parse_version(package_name_original, version, r_name);
        }

        if (r_ver != 0 && r_ver != 2)
        {
            // syntax error
            return 2;
        }

        // name dir: author_name
        // char path_package_name[MAX_CHAR] = "";

        // strcat(path_package_name, author);
        // strcat(path_package_name, "_");
        // strcat(path_package_name, name);

        // if there is a version, append the version
        /*
        if (strnlen(version, MAX_CHAR) > 0)
        {
            // author_name_version
            strcat(path_package_name, "_");
            strcat(path_package_name, version);
        }
        */
        
        // name dir in /deps/: only name
        strcat(path_cache, name); 
        // TODO: add version
        if (mkdir(path_cache, 0755) == -1)
        {
            // if dir already exist
            // TODO: try mkdir with author_name
            if (errno == EEXIST)
            {
                logger_error("error", "directory %s already existing", path_cache);
                logger_error("error", "impossible to create %s dir", path_cache);
            }
            else
            {
                logger_error("error", "%s", strerror(errno));
                logger_error("error", "impossible to create %s dir", path_cache);
            }
            
            // logger_error("error", "%s", strerror(errno));
            // logger_error("error", "impossible to create %s dir", path_cache);
            return -1;
        }

        // call git clone

        // build url for repo
        char url_repo[MAX_URL] = GITHUB_REPO_URL;
        strcat(url_repo, author);
        strcat(url_repo, "/");
        strcat(url_repo, name);
        
        int r = fork_git(url_repo, path_cache);

        int r_cm;
        // if fork_git terminated succesfully
        // create_manifest()
        if (r == 0)
        {
            if (strlen(version) != 0)
            {
                r_cm = create_manifest(author, name, version, path_cache);
            }
            else
            {
                // if there isn't a version, pass NULL 
                r_cm = create_manifest(author, name, NULL, path_cache);
            }
            if (r_cm > 0)
            {
                // error in creating the manifest
                return -2;
            }
            
            if (path != NULL)
            {
                // need path for check_errors_update in clib-update.c
                strncat(path, path_cache, MAX_PATH - 1);
                if (strnlen(version, MAX_VER) > 0)
                {
                    strncat(version_e, version, MAX_VER - 1);
                }
            }
        }

        return r;
    }
    else
    {
        // dir clib doesn't exist 
        // logger_error("error", "clib directory in /.cache doesn't exist");

        // not a clib root dir project
        logger_error("error", "deps directory not exist, %s not a clib root project directory",
        getenv(PWD_ENV_VARIABLE));

        // TODO: create dir if not existing
        return -1;
    }

    return 0;
}

// ---------------------------------------
// Parsing author, name, version functions
// ---------------------------------------
//
// parsing author
// return 1 if syntax error
// return 2 general problem
int cc_parse_author(char * package_name_original, char *author)
{
    char package_name[MAX_CHAR] = "";

    strncat(package_name, package_name_original, MAX_CHAR - 1);

    char *token = strtok(package_name, "/");

    if (token == NULL || strnlen(token, MAX_CHAR) == 0)
    {
        // syntax error
        return 1;
    }

    if (author == NULL)
    {
        // problem with author
        return 2;
    }

    strcat(author, token);

    // ok
    return 0;
}

// return 1 if syntax error
// return 2 if there is a general problem
// return -1 if ok and there isn't version
// return 0 if ok and there is a version
int cc_parse_name(char *package_name_original, char *name, int r)
{
    char package_name[MAX_CHAR] = "";

    strncat(package_name, package_name_original, MAX_CHAR - 1);

    char *token = strtok(package_name, "/");

    if (name == NULL)
    {
        // problem
        return 2;
    }

    if (token == NULL || r != 0)
    {   
        // syntax error
        return 1;
    }

    token = strtok(NULL, "/");
    if (token == NULL || strnlen(token, MAX_CHAR) == 0)
    {
        // syntax error
        return 1;
    }
    
    char tmp[MAX_CHAR] = "";
    strcat(tmp, token);

    token = strtok(token, "@");
    if (token == NULL)
    {
        // if author/name without version
        strcat(name, tmp);
        
        // ok
        return -1;
    }
    
    // if author/name@version
    strcat(name, token);
    return 0;
}

// return 1 if there is a syntax error
// return 2 if there isn't a version
// return if author/name@version
int cc_parse_version(char *package_name_original, char *version, int r)
{
    char package_name[MAX_CHAR] = "";

    strncat(package_name, package_name_original, MAX_CHAR - 1);

    char *token = strtok(package_name, "/");

    if (token == NULL || r != 0)
    {   
        // syntax error
        return 1;
    }

    token = strtok(NULL, "/");
    if (token == NULL || strnlen(token, MAX_CHAR) == 0)
    {
        // syntax error
        return 1;
    }
    
    char tmp[MAX_CHAR] = "";
    strcat(tmp, token);

    token = strtok(token, "@");
    if (token == NULL)
    {
        // problem
        return 1;
    }

    token = strtok(NULL, "@");

    if (token == NULL)
    {
        // if author/name without version
        return 2;
    }

    if (strnlen(token, MAX_CHAR) == 0)
    {
        // problem
        return 1;
    }

    strcat(version, token);
    // ok
    return 0;
}

// ------------------------------
// functions for clib-update
// ------------------------------

// return 0: checked manifest and rm -rf dir ok
// return -1: package_name_original syntax error
// return -2: cannot open manifest package.json
// return -3: problem with json manifest or json object: clib_repo
// return -4: problem with name dir
// return -5: problem to erase dir
// return -6: problem with git cloning
// return -7: isn't a clib project directory
int check_local_manifest_and_rm_package(char *package_name_original)
{
    // doesn't destroy package_name_original
    char package_name[MAX_CHAR] = "";
    strncat(package_name, package_name_original, MAX_CHAR - 1);

    // build path to /deps/package_name
    char name[MAX_CHAR] = "";
    char path[MAX_PATH] = "";

    strncat(path, getenv(PWD_ENV_VARIABLE), MAX_PATH - 1);
    strncat(path, "/deps/", MAX_CHAR);

    // check if there is deps dir
    struct stat sb;
    if (stat(path, &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        int r_name = cc_parse_name(package_name, name, 0);
        if (r_name != -1 && r_name != 0)
        {
            // syntax error
            return -1;
        }

        // ------------------------------
        // Parsing package.json
        // 
        // TODO: extract this part of code in another function
        // ------------------------------

        // open package.json
        char path_tmp[MAX_PATH] = "";

        strncat(path, name, MAX_PATH - 1);
        strncpy(path_tmp, path, MAX_CHAR);

        // now I have path_tmp e path -> $PWD/deps/name
        char manifest_name[] = "/package.json";
        strncat(path_tmp, manifest_name, MAX_CHAR);

        JSON_Value *root_value;
        JSON_Object *object;

        root_value = json_parse_file(path_tmp);

        if (root_value == NULL)
        {
            // cannot open manifest package.json
            return -2;
        }

        object = json_value_get_object(root_value);

        // json_clib_repo is a flag initialized to 1
        // because if there is a clib_repo flag in manifest
        // json_clib_repo = 0
        // if there isn't this flag in json json_clib_repo = 1
        // and return error -3
        char json_name[MAX_CHAR] = "";
        int json_clib_repo = 1;

        if (object != NULL)
        {
            // parsed json value "name", "clib_repo"
            strncat(json_name, json_object_get_string(object, "name"), MAX_CHAR - 1);
            // expected result if ok: 0
            json_clib_repo = (int)json_object_get_number(object, "clib_repo");
        }
        else
        {
            // problem with json 
            return -3;
        }

        if (json_clib_repo != 0)
        {
            // problem with json_clib_repo
            return -3;
        }

        struct stat sb1;
        if (stat(path, &sb1) == 0 && S_ISDIR(sb.st_mode))
        {  
            int r = rm_rf(path);

            if (r != 0)
            {
                // impossible to remove dir
                return -5;
            }

            return 0;
        }
        else
        {
            // error with name dir
            return -4;
        }

    }
    else
    {
        // isn't a clib proj dir
        return -7;
    }
}

int *is_clib_repo(int n, char *pkgs[])
{
    // answers for every package
    int *res = (int *)malloc(n * sizeof(int)); 

    if (res != NULL)
    {
        for (int i = 0; i < n; i++)
        {
            // check for every package if there is a manifest online
            res[i] = check_local_manifest_and_rm_package(pkgs[i]);
        }
    }
    else
    {
        // res = NULL
        return res;
    }
    return res;
}

int unlink_cb(const char *fpath, const struct stat *sb, int typeflag, struct FTW *ftwbuf)
{
    int rv = remove(fpath);

    if (rv)
        perror(fpath);

    return rv;
}

int rm_rf(char *path)
{
    return nftw(path, unlink_cb, 64, FTW_DEPTH | FTW_PHYS);
}

// array with git clone args
// It must be NULL terminated
int fork_checkout(char *path, char *version)
{
    pid_t pid = fork();

    if (pid < 0)
    {
        logger_error("error", "impossible to fork git checkout");
        // 1 is a generic return error 
        return 1;
    }
    else if (pid == 0)
    {
        // redirect stdout to dev/null for not printing
        // result of git
        int fd = open("/dev/null", O_WRONLY);
        if (fd < 0) {
            perror("open /dev/null failed");
            exit(EXIT_FAILURE);
        }

        if (dup2(fd, STDOUT_FILENO) < 0) {
            perror("dup2 stdout failed");
            exit(EXIT_FAILURE);
        }
        if (dup2(fd, STDERR_FILENO) < 0) {
            perror("dup2 stderr failed");
            exit(EXIT_FAILURE);
        }

        close(fd);

        char tags[MAX_CHAR] = "tags/";
        strncat(tags, version, MAX_VER - 1);
        // create correct input array (git -C path/to/dir checkout version)
        char *args[] = {"git", "-C", path, "checkout", tags};

        execvp(args[0], args);

        // execvp never return if process success, if not returns 1
        exit(EXIT_FAILURE);
    }
    else
    {   
        // Parent process
        int status;

        int r = waitpid(pid, &status, 0);

        if (r == -1)
        {
            // generic error
            return 1;
        }

        if (WIFEXITED(status))
        {
            // if WEXITSTATUS(status) = 0 no problem, if != 0 problem
            return WEXITSTATUS(status);
        }
        else
        {
            return 0;
        }
    }
}





































