
/***********************************************************************
 Name:	  macLabel
 Created: 01-06-2013
 Author:  McUsr
 
 Usage:   
 Sets or returns the finder Label for a file, it is somewhat naive, but 
 should do the job correctly  99.99% of the time. It doesn't work on 
 read-only or locked files, that is it doesn't override any of the usal
 behavour, and it only works if the file is on a HFS+ filesystem.

 It should however work for OS X from 10.0 to present, at least on Intel
 platforms.

 I got the idea from the Carbon version I found in Scott Anguish's repo.
 His article about uti's is by the way the best there is.

 Command lines to compile:
    gcc -Os -c filetypes.c 
    gcc -Os -o macLabel macLabel.c filetypes.o 
 
 DISCLAIMER
 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 */

#include <sys/mount.h>
#include <sys/attr.h>
#include <sys/vnode.h>
#include <sys/errno.h>
#include <sys/stat.h>
#include <unistd.h>
#include <getopt.h>
#include <stdlib.h>
#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdbool.h>
#include <filetypes.h>
	
#define HFSPLUS 0x11

#define USER_ERROR		2
#define INTERNAL_ERR	4

#define c_none   0 
#define c_grey   2
#define c_green  4
#define c_purple 6
#define c_blue   8
#define c_yellow 10
#define c_red    12
#define c_orange 14

#define c_col_min 0
#define c_col_max 7

#define  valid_colornumber( aColor )\
(((int)aColor >= (int)c_col_min ) &&  ((int)aColor <= (int)c_col_max )) 


const char *colors[] = {
	"none",
	"grey",
	"green",
	"purple",
	"blue",
	"yellow",
	"red",
	"orange"
};

static int foundColor( char *color ) ;

size_t MAX_PATH;
static void usage(void) ;
static void version(void) ;
static void copyright(void) ;


static bool okFilename(char *fn) ;

static bool fonhfsplus(char *fname ) ;
#define  bailout_if_not_on_hfsplus(x)\
do {\
	if (fonhfsplus(x) == false ) {\
		fprintf(stderr,"macLabel: the file %s is not on a HFS+ filesystem. bailing out.\n",x);\
		exit(EXIT_FAILURE);\
	}\
} while (0) 

static unsigned int GetLabel(const char *path) ;
static int SetLabel(const char *path, unsigned int color ) ;

int	listLabels(char *arg,int color, bool showNumber, bool multi, char delim ) ;
int	listAllLabels(char *arg,int color, bool showNumber, bool multi, char delim ) ;
int findLabels(char *arg,int color, bool showNumber, bool multi, char delim  ) ;
int testLabels(char *arg,int color, bool showNumber, bool multi, char delim ) ;
int setLabels(char *arg,int color, bool showNumber,bool multi, char delim ) ;

static struct option longopts[] = {
	{"help", no_argument, NULL, 'h'},
	{"usage", no_argument, NULL, 'u'},
	{"copyright", no_argument, NULL, 'c'},
	{"version", no_argument, NULL, 'V'},
	{"list", no_argument, NULL, 'l'},
	{"all", no_argument, NULL, 'a'},
	{"set", required_argument, NULL, 's'},
	{"test", required_argument,NULL, 't'},
    {"which",no_argument,NULL, 'w'},
    {"verbose",no_argument,NULL, 'v' },
	{"number", no_argument, NULL, 'n'},
	{"delimiter", required_argument, NULL, 'd'},
	{NULL, 0, NULL, 0},
};

#define CMD_SET  	 0x1
#define CMD_TEST 	 0x2
#define CMD_FIND 	 0x4
#define CMD_LIST 	 0x8
#define CMD_LIST_ALL 0x10
#define CMD_VERBOSE  0x20
#define CMD_NUMBER   0x40

#define set_verbose(x)\
	do {\
		if ((x & CMD_NUMBER)==CMD_NUMBER )\
			;\
		else\
			x |= CMD_VERBOSE ;	\
	} while(0)
	
#define copy_optarg(x,y)\
	do {\
		y=malloc(strlen(x)+1) ;\
		if (y == NULL ) {\
			perror("Error during malloc of mem for optarg");\
			exit(EXIT_FAILURE);\
		} else {\
			y=strcpy(y,x);\
		}\
	} while (0)
		
#define clear_prev_cmd( x ) x &=~(0x1E)

#define clear_list_type(x) x &=~(0x60)
#define show_number_for_color(x) ((x & CMD_NUMBER) == CMD_NUMBER)
#define be_verbose(x) ((x & CMD_VERBOSE) == CMD_VERBOSE)
		
void main (int argc, char *argv[] ) {	
    int 			cmd_err 		= EXIT_SUCCESS ;
	bool			redir			= (bool)(!isatty(STDIN_FILENO)),
					multiple		= false;
	u_int16_t 		optype = 0;
	unsigned char 	theColor=0;
	char 			delimiter='\t';
	
	assert(__LITTLE_ENDIAN__);

	MAX_PATH= (size_t) pathconf(".",_PC_PATH_MAX) ;
	if (argc < 2 && redir == false ) {
		usage() ;
		exit(USER_ERROR) ;
	} else if ( argc < 2 && redir == true )  {
		optype = (CMD_VERBOSE | CMD_LIST ) ;	
		// we'll list files from stdin.
	} else {
		// parse argument list.
    	
        extern int optind ;
    	int	ch;
    	char *colorParm	= NULL;
    			
    	while ((ch = getopt_long(argc, argv, "hucVlans:t:w:vd:", longopts,NULL )) != -1) {
    		switch (ch) {
    			case 'h':
    				usage() ;
    				exit(0) ;
    			case 'u':
    				usage() ;
    				exit(0) ;
    			case 'c':
    				copyright();
    				exit(0) ;
    			case 'V':
    				version() ;
    				exit(0);
    			case 'l':
					clear_prev_cmd(optype);
					set_verbose(optype);
					optype |= CMD_LIST;
    				break;
    			case 'a':
					clear_prev_cmd(optype) ;
					set_verbose(optype);
					optype |= CMD_LIST_ALL;
    				break;
    			case 's':
					clear_prev_cmd(optype) ;
					optype |= CMD_SET ;
					copy_optarg(optarg,colorParm);
    				break;
				case 't': 
					clear_prev_cmd(optype) ;
					set_verbose(optype);
					optype |= CMD_TEST ;
					copy_optarg(optarg,colorParm);
    				break;	
    			case 'w':
					clear_prev_cmd(optype) ;
					set_verbose(optype);
					optype |= CMD_FIND ;
					copy_optarg(optarg,colorParm);
    				break;	
    			case 'v':
				    clear_list_type(optype);
    				optype |= CMD_VERBOSE ;
    				break;
    			case 'n':
				    clear_list_type(optype);
    				optype |= CMD_NUMBER ;
    				break;
				case 'd':
					delimiter=optarg[0] ;
					break;
    		}
    	}
		// checks for any optype, sets l if none given.
		if (!(optype & 0x1F)) optype |= CMD_LIST ;

		// checks for any color we picked up here.
		if (colorParm) {
			if ( show_number_for_color(optype)) {
				theColor=(unsigned char)strtol(colorParm,NULL,10); 
				if (valid_colornumber( (int) theColor )==false) {
					free(colorParm) ;
					fprintf(stderr,"macLabel: \"%d\" is not a valid number for a color.",(int)theColor);
					fprintf(stderr," Valid numbers are 0-7 (macLabel -h)\n") ;
					exit(USER_ERROR);
				}
			} else if ((theColor=foundColor( colorParm)) == 255 ) {
				
				theColor=(unsigned char)strtol(colorParm,NULL,10); 
				if ( theColor == 0 && errno == EINVAL ) {
					fprintf(stderr,"macLabel: \"%s\" is not a valid  color.\nValid colors are:\n",colorParm);
					fprintf(stderr,"None,\nGrey,\nGreen,\nPurple,\nBlue,\nYellow,\nRed,\nOrange.\n") ;
					free(colorParm) ;
					exit(USER_ERROR);
				} else if (valid_colornumber( (int) theColor )==false) {
					free(colorParm) ;
					fprintf(stderr,"macLabel: \"%d\" is not a valid number for a color.",(int)theColor);
					fprintf(stderr,"Valid numbers are 0-7 (macLabel -h)\n") ;
					exit(USER_ERROR);
				}
			}
			free(colorParm) ;
		}
			
    	if (optind<(argc-1) || redir == true )  {
    		multiple = true;
    	} else if (optind<argc) 
    		multiple = false ;
    	else {
			fprintf(stderr,"macLabel: there are no files to operate upon.\n") ;
    		usage() ;
    		exit(USER_ERROR) ;
    	}
	}
	
	bool asNumber = (( optype & CMD_NUMBER ) == CMD_NUMBER ) ;
	// arrange the correct operation
	int (*doit)(char *arg,int color, bool showNumber,bool multi, char delim ) = NULL,
		main_cmd= (optype & (~(0x60))) ;
	if (main_cmd == CMD_LIST ) { 
		doit= &listLabels;
	} else if ( main_cmd == CMD_LIST_ALL )
		doit= &listAllLabels;
	else if ( main_cmd == CMD_FIND )
		doit= &findLabels;
	else if ( main_cmd == CMD_TEST )
		doit= &testLabels;
	else if ( main_cmd == CMD_SET )
		doit= &setLabels;
	else {
		fprintf(stderr,"macLabel: can't happen! no valid command from main\n") ;
		exit(INTERNAL_ERR ) ;
	}
	
   	setbuf(stdout,NULL) ;
	size_t slen;
	if (multiple == false ) {
		if ((slen=strlen(argv[optind])) >= MAX_PATH ) {
			fprintf(stderr,"macLabel: Pathname too long\n");
			exit(USER_ERROR) ;
		} else {
			
			if (okFilename(argv[optind]) == true ) {
				bailout_if_not_on_hfsplus(argv[optind]) ;
				cmd_err = doit(argv[optind],theColor,asNumber,multiple,delimiter);
			} 
			exit(cmd_err) ;
		}
	} else if (multiple == true ) {
		int final_code;
		if (redir == true ) {
			char *buf = malloc( (size_t) MAX_PATH );
			if (buf == NULL ) {
				perror("macLabel %s: error during malloc in main.");
				exit(INTERNAL_ERR);
			}
			while((buf=fgets(buf,MAX_PATH+1,stdin))!=NULL) {
				buf[strlen(buf)-1] = '\0'  ;
				if ( buf )
				if (okFilename(buf) == true ) {
					bailout_if_not_on_hfsplus(buf) ;
					cmd_err = doit(buf,theColor,asNumber,multiple, delimiter);
					final_code = (cmd_err == INTERNAL_ERR ) ?  INTERNAL_ERR  : EXIT_SUCCESS  ;
				}
			}
			free(buf) ;
			buf=NULL ;
			exit(final_code) ;
		} else {
			int last_arg = argc -1 ;
						
			while ( optind <= last_arg ) {
				if (okFilename(argv[optind]) == true ) {
					bailout_if_not_on_hfsplus(argv[optind]) ;
					cmd_err = doit(argv[optind++],theColor,asNumber,multiple, delimiter);
					final_code = (cmd_err == INTERNAL_ERR ) ?  INTERNAL_ERR  : EXIT_SUCCESS  ;
				}
			}
			exit(final_code) ;
		}
	}
}

static int foundColor( char *color ) {
	size_t slen=strlen(color);
	assert(slen >0 ) ;
	char *lowercaseCol=malloc(slen+1) ;
	if (lowercaseCol == NULL ) {
		perror("FoundColor: Couldn't allocate mem.") ;
		exit(INTERNAL_ERR);
	}
	lowercaseCol=strcpy(lowercaseCol,color) ;	
	int i;
	for (i=0;i<slen;i++) 
		lowercaseCol[i]=tolower(lowercaseCol[i]);

	for (i=0;i<8;i++) {
		if(!strcmp(colors[i],lowercaseCol))
			break;
	}
	free(lowercaseCol) ;
	if (i<8)
		return i;
	else
		return -1;
}

static bool okFilename(char *fn) {
	struct stat buf;
		// removes those char's from the end of a file name for that we have 
		// gotten a "pretty ls- listing as input.
		// we -don't handle long ls listings very well...
	if ( fn[strlen(fn)-1] == '@' ) {
		fn[strlen(fn)-1]= '\0' ;
	}
	if (  fn[strlen(fn)-1] == '*' ) {
		fn[strlen(fn)-1]= '\0' ;
	}
	if (strlen(fn) == 2 && (!strcmp( fn,".." ))) return false ;
		// Check that we aren't dealing with a dot-file.
	if ( fn[0] != '.' || ( fn[0] == '.' && strlen(fn) > 1
					&& ( fn[1] == '.' ||  fn[1] == '/')))  {
			// dotfiles starts with a '.' but if the next char is another
		 	//	'.' or '/' then its a relative path.
		if (lstat(fn, &buf) < 0) return false ;
		if (S_ISREG(buf.st_mode))
			return true ;
		else if (S_ISDIR(buf.st_mode))
			return true ;
		else 
			return false;
	} else {
		return false ;
	}
}

/* TODO: not entirely safe before checks upon length
	of individual segments of paths. */
static bool fonhfsplus(char *fname ) {
   struct statfs stat;
   char buf[FILENAME_MAX+1];
   strncpy(buf,fname,FILENAME_MAX);
	
    if (!statfs(buf, &stat)) {
         if (stat.f_type == HFSPLUS ) 
			 return true;
		 else 
			 return false;
    } else {
		fprintf(stderr,"%s: problems call fstat on %s : the file may not exist\n",
				__PRETTY_FUNCTION__,buf);
        perror("fonhfsplus");
		exit(INTERNAL_ERR);
    }
}

typedef struct attrlist attrlist_t;

struct FInfo2CommonAttrBuf {
	fsobj_type_t	objType;
	char			finderInfo[32];
};

typedef struct FInfo2CommonAttrBuf FInfo2CommonAttrBuf;

struct FInfo2AttrBuf {
	u_int32_t		length;
	FInfo2CommonAttrBuf common;
};

typedef struct FInfo2AttrBuf FInfo2AttrBuf;
#define LABEL_OFFSET 9

static unsigned int GetLabel(const char *path)
{
	int			err;
	unsigned char colorCode=0;
	attrlist_t	attrList;
	FInfo2AttrBuf attrBuf;

	memset(&attrList,0, sizeof(attrList));
	attrList.bitmapcount= ATTR_BIT_MAP_COUNT;
	attrList.commonattr= ATTR_CMN_OBJTYPE | ATTR_CMN_FNDRINFO;

	err = getattrlist(path, &attrList, &attrBuf, sizeof(attrBuf), 0);
	if (err != 0) {
		perror("macLabel: something went wrong when trying to get the fileattributes list:");
		exit(INTERNAL_ERR);
	} else {
		assert(attrBuf.length == sizeof(attrBuf));
		colorCode = (unsigned char) attrBuf.common.finderInfo[LABEL_OFFSET] ;
	}

	return (unsigned int ) (colorCode>>1);
}


struct FInfoAttrBuf {
	u_int32_t       length;
    fsobj_type_t    objType;
    char            finderInfo[32];
} ;

typedef struct FInfoAttrBuf FInfoAttrBuf;

static int SetLabel(const char *path, unsigned int color )
{
	int			err;
	unsigned char colorCode=0;
	attrlist_t	attrList;
	FInfoAttrBuf attrBuf;

	memset(&attrList,0, sizeof(attrList));
	attrList.bitmapcount= ATTR_BIT_MAP_COUNT;
	attrList.commonattr= ATTR_CMN_OBJTYPE | ATTR_CMN_FNDRINFO;

	err = getattrlist(path, &attrList, &attrBuf, sizeof(attrBuf), 0);
	if (err != 0) {
		perror("macLabel: something went wrong when trying to get the fileattributes list:");
		exit(INTERNAL_ERR);
	} else {
		assert(attrBuf.length == sizeof(attrBuf));
		colorCode = (unsigned char) attrBuf.finderInfo[LABEL_OFFSET] ;
		memcpy(&attrBuf.finderInfo[LABEL_OFFSET],(unsigned char *)&color,1 );
		attrList.commonattr= ATTR_CMN_FNDRINFO;
		err = setattrlist(
			path,
			&attrList,
			attrBuf.finderInfo,
			sizeof(attrBuf.finderInfo),
			0
		);
	}
	return err;
}


// lists all files with  labels that are set
int	listLabels(char *arg,int color, bool showNumber, bool multi, char delim ){
	unsigned int theColor = GetLabel(arg) ;	
	if (theColor > 0 ) {
		if (showNumber) 
			fprintf(stdout,"%s%c\%d\n",arg,delim,theColor ) ;
		else
			fprintf(stdout,"%s%c\%s\n",arg,delim,colors[theColor] ) ;
	}
	return 0 ;
}

// lists all files with  labels that are set no label included.
int	listAllLabels(char *arg,int color, bool showNumber, bool multi, char delim ) {
	unsigned int theColor = GetLabel(arg) ;	
	if (theColor >= 0 ) {
		// theColor=(theColor>>1); // internally in GetLabel
		if (showNumber) 
			fprintf(stdout,"%s%c\%d\n",arg,delim,theColor ) ;
		else
			fprintf(stdout,"%s%c\%s\n",arg,delim,colors[theColor] ) ;
	}
	return 0 ;
}

// lists all files with a given label. (-w option)
int findLabels(char *arg,int color, bool showNumber, bool multi, char delim ) {
	unsigned int theColor = GetLabel(arg) ;	
	if (theColor == color ) {
		if (showNumber) 
			fprintf(stdout,"%s%c\%d\n",arg,delim,theColor ) ;
		else
			fprintf(stdout,"%s%c\%s\n",arg,delim,colors[theColor] ) ;
		if ( multi == false ) 
			return color ;
	}
	return 0 ;
}

// tests if a file has  a given label.
int testLabels(char *arg,int color, bool showNumber, bool multi, char delim  ) {
	unsigned int theColor = GetLabel(arg) ;	
	if (theColor == color ) {
		if (showNumber) 
			fprintf(stdout,"%s%c\%d\n",arg,delim,theColor ) ;
		else
			fprintf(stdout,"%s%c\%s\n",arg,delim,colors[theColor] ) ;
		if ( multi == false ) 
			return 0 ;
	}
	if ( multi == false ) 
		return 1 ;
	else
		return 0 ;
}

// sets a label for a file.
int setLabels(char *arg,int color, bool showNumber, bool multi, char  delimiter ) {
	int retval, save_err;
	color<<=1;	
	if ((retval= SetLabel(arg,  color))<0 ) {
		int save_err = errno ;
		fprintf(stderr,"macLabel an error occured during setting the label of %s\n",arg);
		fprintf(stderr,"%s\n",strerror(save_err));
		exit(INTERNAL_ERR);
	}
	return 0 ;
}

void usage(void) {

	fprintf(stderr,"maclabel: version 1.0 Copyright © 2013 McUsr and put into Public Domain\n");
	fprintf(stderr,"  under Gnu GPL 2.0\n");
	fprintf(stderr,"Usage: macLabel [options]  [color] [1 to n ..file arguments or from stdin\n");
	fprintf(stderr,"       specified by posix path, one file on each line.]\n");
	fprintf(stderr,"Options\n");
	fprintf(stderr,"-------\n");
	fprintf(stderr,"  macLabel [ - -huVclastwvnd ]\n");
	fprintf(stderr,"  macLabel [ --help,--usage,--copyright,--version,--list,--all,--set\n");
	fprintf(stderr,"             --test,--which,--verbose, --number, --delimiter ]\n");
	fprintf(stderr,"Details\n");
	fprintf(stderr,"-------\n");
	fprintf(stderr,"  Label color can be set, returned or tested for.  Colors are numbers\n");
	fprintf(stderr,"  by -n or English  as -v or by default. Another delimiter than '\\t' (tab)\n") ;
	fprintf(stderr,"  can be chosen to separate files from colors/numbers. Only files on a HFS+\n");
	fprintf(stderr,"  filesystem, can be operated upon. And the files must of course exist.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"  -l  [file1 ..file1] list files and their label color.\n");
	fprintf(stderr,"  -a  [file1 ..file1] list all files and their label color (none).\n");
	fprintf(stderr,"  -s  [color] [file1 ..file1] set label to color. \n");
	fprintf(stderr,"      -sn signalizes that color is set by number.\n");
	fprintf(stderr,"	  \n");
	fprintf(stderr,"  -t [color] [file 1 .. file n] test for label of file.\n");
	fprintf(stderr,"     exit code is zero if single file and success.\n");
	fprintf(stderr,"    -v output to stdout automatically set.\n");
	fprintf(stderr,"	-n returns the color as number.\n");
	fprintf(stderr,"	 \n");
	fprintf(stderr,"  -w [color] [file 1 ..file n] : returns files with a given color.\n");
	fprintf(stderr,"     exit code is colorcode if single file.\n");
	fprintf(stderr,"     -v is automatically set.\n");
	fprintf(stderr,"     -n returns the color as number.\n");
	fprintf(stderr,"	 \n");
	fprintf(stderr,"  -n [number] for color with the -s or --set option. (See below).\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Valid colors and numbers representing them\n");
	fprintf(stderr,"-------------------------------------------\n");
	fprintf(stderr,"  0 None   1 Grey\n");
	fprintf(stderr,"  2 Green  3 Purple\n");
	fprintf(stderr,"  4 Blue   5 Yellow\n");
	fprintf(stderr,"  6 Red    7 Orange\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"Examples of usage\n");
	fprintf(stderr,"-----------------\n");
	fprintf(stderr,"  macLabel -w [file 1 .. file n]\n");
	fprintf(stderr,"  Returns [file color] to stdout.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"  macLabel -wn [file 1 .. file n]\n");
	fprintf(stderr,"  Returns [file nr] to stdout.\n");
	fprintf(stderr,"  \n");
	fprintf(stderr,"  macLabel -w [file 1]\n");
	fprintf(stderr,"  Returns the colornumber as exitcode.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"  macLabel -sn 6 [file 1]\n");
	fprintf(stderr,"  Sets the Finder Label of file 1 to red.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"  macLabel -s red [file 1]\n");
	fprintf(stderr,"  Sets the Finder Label of file 1 to red.\n");
	fprintf(stderr,"\n");
	fprintf(stderr,"  macLabel -t red [file 1 .. file n]\n");
	fprintf(stderr,"  Returns 0 if red is the label of file 1.\n");
	fprintf(stderr,"  Sends [file color] to stdout when  more than one file.\n");
	fprintf(stderr,"  \n");
	fprintf(stderr,"  macLabel -tn red [file 1 .. file n]\n");
	fprintf(stderr,"  Sends [file color-number] to stdout when more than one.\n");
}

static void version(void) {
	fprintf(stderr,"macLabel version 1.0 © 2013 Copyright McUsr and put into Public Domain under GNU Gpl.\n") ;
}

static void copyright(void) {
	version() ;
}
