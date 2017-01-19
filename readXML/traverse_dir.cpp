#include   <stdio.h>  
#include   <fcntl.h>  
#include   <sys/types.h>
#include   <ctype.h>
#include   <stdlib.h>
#include   <errno.h>
#include   <sys/stat.h>
#include   "traverse_dir.h"

#ifdef   _WIN32  
DIR*   opendir(const   char   *dir)
{
	DIR   *dp;
	char   filespec[_MAX_FNAME];
	long   handle;
	int   index;
	strncpy(filespec, dir, _MAX_FNAME);
	index = strlen(filespec) - 1;
	if (index >= 0 && (filespec[index] == '/' || filespec[index] == '\\'))
		filespec[index] = '\0';
	strcat(filespec, "/*");   
	dp = (DIR   *)malloc(sizeof(DIR));
	dp->offset = 0;
	dp->finished = 0;
	dp->dir = _strdup(dir);
	if ((handle = _findfirst(filespec, &(dp->fileinfo)))   <   0)
	{
		if (errno == ENOENT)
			dp->finished = 1;
		else
			return   NULL;
	}
	dp->handle = handle;
	return   dp;
}

struct   dirent*   readdir(DIR   *dp)
{
	if (!dp || dp->finished)
		return   NULL;
	if (dp->offset != 0)
	{
		if (_findnext(dp->handle, &(dp->fileinfo))   <   0)
		{
			dp->finished = 1;
			return   NULL;
		}
	}
	dp->offset++;
	strncpy(dp->dent.d_name, dp->fileinfo.name, _MAX_FNAME);
	dp->dent.d_ino = 1;
	dp->dent.d_reclen = strlen(dp->dent.d_name);
	dp->dent.d_off = dp->offset;
	return   &(dp->dent);
}

int   closedir(DIR   *dp)
{
	if (!dp)
		return   0;
	_findclose(dp->handle);
	if (dp->dir)
	{
		free(dp->dir);
		dp->dir = NULL;
	}
	if (dp)
	{
		free(dp);
		dp = NULL;
	}
	return   0;
}
#endif 

int traverse_dir(string path, vector<string>& files, bool recursive, bool includedir)
{
	if (path.empty())
		return -1;
	DIR* dir;
	dir = opendir(path.c_str());
	if (dir == NULL)
		return -2;
	struct dirent *direntry;
	struct stat st;

	char temp[256] = { 0 };
	while (direntry = readdir(dir))
	{
		if (!strcmp(direntry->d_name, ".") || !strcmp(direntry->d_name, ".."))
			continue;

		if (path[path.size() - 1] != '/')
			sprintf(temp, "%s/%s", path.c_str(), direntry->d_name);
		else
			sprintf(temp, "%s%s", path.c_str(), direntry->d_name);


		if (stat(temp, &st) == 0)
		{
#ifdef _WIN32
			if ((st.st_mode&_S_IFDIR) == _S_IFDIR)
			{
				if (recursive)
				{
					traverse_dir(temp, files, recursive, includedir);
				}
			}
#else
			if (S_ISDIR(st.st_mode))
			{
				if (recursive)
				{
					traverse_dir(temp, files, recursive);
				}
			}
#endif
			else
			{
				if (includedir)
				{
					//查询gz zip xml文件
					if ((strstr(temp, ".gz") != NULL 
						|| strstr(temp, ".zip") != NULL 
						|| strstr(temp, ".xml") != NULL
						) && strstr(temp,"_MRO_") !=NULL  )   //只取MRO
						files.push_back(temp);
				}
				else
				{
					string file = temp;
					files.push_back(file.substr(file.find_last_of('/') + 1));//only need file name			
				}
			}
		}
	}
	closedir(dir);
	return (int)files.size();
}