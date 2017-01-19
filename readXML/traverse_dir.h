#ifndef TRAVERSE_DIR_H
#define TRAVERSE_DIR_H

#include <vector>
#include <string>
using namespace std;

#ifdef   _WIN32  
#include   <io.h>  
#include   <sys/types.h>  
#else
#include <dirent.h>
#endif 

#define   _MAX_FNAME   256 

#ifdef   _WIN32  

struct   dirent
{
	long   d_ino;
	off_t   d_off;
	unsigned   short   d_reclen;
	char   d_name[_MAX_FNAME + 1];
};

typedef   struct
{
	long   handle;
	short   offset;
	short   finished;
	struct   _finddata_t   fileinfo;
	char   *dir;
	struct   dirent   dent;
}   DIR;

DIR*     opendir(const   char   *);
struct   dirent   *   readdir(DIR   *);
int   closedir(DIR   *);
#endif 

int traverse_dir(string path, vector<string>& files, bool recursive = false, bool includedir = false);

#endif