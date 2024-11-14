/*
    Author: Simone Stefanello
    Name: clib-clone.c
    Description: install repos outside the wiki in clib git repo 
                 (repos not intended to be downloaded by clib)
*/

#define MAX_CHAR 100
#define MAX_URL 200
#define MAX_COMMAND 400
#define MAX_PATH 800
#define NOT_FOUND_GIT "404: Not Found"
#define INVALID_REQUEST_GIT "400: Invalid Request"
#define HOME_ENV_VARIABLE "HOME"
#define GITHUB_REPO_URL "https://github.com/"

#ifndef GITHUB_CONTENT_URL
    #define GITHUB_CONTENT_URL "https://raw.githubusercontent.com/"
#endif

// debug mkdir
#include <errno.h>

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include "clib-package.h"
#include "http-get/http-get.h"
#include "clib-clone.h"
#include "logger/logger.h"

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
        printf("Error: impossible to allocate memory");
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

    // author/name
    char author[MAX_CHAR] = "";
    char name[MAX_CHAR] = "";

    // http_get request struct initialized to zero
    http_get_response_t *res = NULL;

    // parsing package name author/name
    char * token = strtok(package_name, "/");

    // TODO: strnlen of author != 0
    if (token != NULL)
    {
        strcat(author, token);
        token = strtok(NULL, "/");
        // TODO: strnlen of name != 0
        if (token != NULL )
        {
            strcat(name, token);
        }
        else
        {
            // syntax error
            return 2;
        }
    }
    else
    {
        // syntax error
        return 2;
    }

    
    // build url of repo without version
    char *url_repo = clib_package_url_withour_ver(author, name);

    // build url of manifest (clib.json first, package.json second)
    char tmp_url_package[MAX_URL];
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

// git clone repo to /home/$USER/.cache/clib/packages
// TODO: instead of /home/$USER/.cache/clib/packages, copy to something/root_project/deps/package_name
// restriction: clib install SHOULD BE used in a clib project dir (so it should contain at least deps dir)
// TODO: extract function to parse author and name from package_name_original
// return 0 if success
int git_clone(char *package_name_original)
{
    char package_name[MAX_CHAR] = "";

    // build path to /home/$USER/.cache
    char path_cache[MAX_PATH] = "";
    strcat(path_cache, getenv(HOME_ENV_VARIABLE));
    strcat(path_cache, "/.cache/clib/packages/");

    // mkdir for package_name clone
    struct stat sb;
    if (stat(path_cache, &sb) == 0 && S_ISDIR(sb.st_mode))
    {
        // doesn't destroy original package_name
        strcpy(package_name, package_name_original);

        // author/name
        char author[MAX_CHAR] = "";
        char name[MAX_CHAR] = "";
        
        // control if author and name != NULL
        // and length of author and name != 0
        char * token = strtok(package_name, "/");
        if (token != NULL && strnlen(token, MAX_CHAR) != 0)
        {
            // TODO: check for weird char
            strcat(author, token);
            token = strtok(NULL, "/");
            if (token != NULL && strnlen(token, MAX_CHAR) != 0)
            {
                strcat(name, token);
            }
            else
            {
                // syntax error
                return 2;
            }
        }
        else
        {
            // syntax error
            return 2;
        }

        // name dir: author_name_version
        char path_package_name[MAX_CHAR] = "";
        strcat(path_package_name, author);
        strcat(path_package_name, "_");
        strcat(path_package_name, name);
        
        strcat(path_cache, path_package_name); 
        // TODO: add version
        if (mkdir(path_cache, 0755) == -1)
        {
            logger_error("error", "%s", strerror(errno));
            logger_error("error", "impossible to create %s dir", path_cache);
            return -1;
        }

        // call git clone

        // build url for repo
        char url_repo[MAX_URL] = GITHUB_REPO_URL;
        strcat(url_repo, author);
        strcat(url_repo, "/");
        strcat(url_repo, name);
        
        int r = fork_git(url_repo, path_cache);

        return r;
    }
    else
    {
        // dir clib doesn't exist 
        logger_error("error", "clib directory in /.cache doesn't exist");

        // TODO: create dir if not existing
        return -1;
    }

    return 0;
}