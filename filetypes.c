/***********************************************************************


    Name:    
    Created: 04-29-2013
    Author:  McUsr

    Usage:   

    DISCLAIMER
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.



***********************************************************************/
#if 0 == 1
#define TESTING
#include <stdlib.h>
#endif 
#include <sys/stat.h>
#include <errno.h>
#include <stdio.h>
#include <filetypes.h>

/*
returns 1 if the file is a regular file,
        0 if it isn't,
       -1 if not exists.
*/

static struct stat statbuf ;

int isSymLink(char *pathname ) 
{
	if (lstat(pathname, &statbuf) < 0) { /* stat error file may not exist */
		errno=0;
		return -1;
	}
	if (S_ISLNK(statbuf.st_mode) == 1)	/* not a directory */
		return 1;	
	return 0;
}
int isExec(char *pathname )
{
	if (stat(pathname, &statbuf) < 0) { /* stat error file may not exist */
		errno=0;
		return -1;
	}
	if (( statbuf.st_mode & S_IXUSR ) ==  S_IXUSR )	/*  is executable */
		return 1;	
	return 0;

}
int isDir(char *pathname ) 
{
	if (stat(pathname, &statbuf) < 0) { /* stat error file may not exist */
		errno=0;
		return -1;
	}
	if (S_ISDIR(statbuf.st_mode) == 1)	/* not a directory */
		return 1;	
	return 0;
}

int isregFile(char *pathname ) 
{
	if (stat(pathname, &statbuf) < 0) { /* stat error file may not exist */
		errno=0;
		return -1;
	}
	if (S_ISREG(statbuf.st_mode) == 1)	/* not a directory */
		return 1;	
	return 0;
}
#ifdef TESTING
int main( int argc, char *argv[] )
{ 
	if (argc == 1 ) exit(1) ;
	if (isSymLink(argv[1]) > 0 )
		printf("%s : is a sym-linked file\n",argv[1]) ;
	else if (isDir(argv[1]) > 0)
		printf("%s : is a directory\n",argv[1]) ;
	else if ( isExec(argv[1] )) 
		printf("%s : is an executable file\n",argv[1]) ;
	else if (isregFile(argv[1])>0) 
		printf("%s : is a regular file\n",argv[1]) ;
	else
		printf("%s : is none of above\n",argv[1]) ;
	return 0 ;
}
#endif 

