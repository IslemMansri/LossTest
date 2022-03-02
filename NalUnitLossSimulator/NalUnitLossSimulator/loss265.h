#ifndef _loss_
#define _loss__

/*Edited By Islem Mansri Silx 28/06/2020: In order to work with the Annex B byte stream format for HEVC.*/
/*Tested with Bitstreams from HM16.17*/
#define _CRT_SECURE_NO_DEPRECATE

#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <math.h>
#include <float.h>
#include <ctype.h>

#define OPTIONCHAR '-'
#define MAXNALUNITSIZE 1000000
#define HM16_17_ANCHOR
#define OLD_ERR_PAT_FORMAT
//#define HEVC_Slice_Level

//Silx:	Define the very first structures (3 files, nal unit structure, count of nalu type read and written)  

static FILE *errfile, *infile, *outfile;

static unsigned char nalu[MAXNALUNITSIZE];	//Silx: define the nalu buffer to contain the nalu.

static int nalutypecountread[64], nalutypecountwrite[64];		//Silx: it is the 64 total types of NalUnits
																
static int nalusread = 0, naluswritten = 0;
static int debuglevel = 1;				// default, can be overwritten by -d
static int applyerrors = 1;				// default, parameter sets make it, can be overridden by -a

void parsecmd(int ac, char *av[]);		// parses commandline and opens files, writes global statics
int readnalu(unsigned char *buf);		// returns size of read nal unit
void writenalu(unsigned char *buf, int size);		// write nalu
int ZB_startcode();						// reads in infile, returns 1 if current pos in file infile is a ZB_startcode,
										// and 0 otherwise.  No side effects but cleanup()and exit() when end of
										// input bitstream is reached at start code boundary.
int nal_unit_type(unsigned char *buf);		// returns the nal_unit_type of the nal unit in buf
int naluloss();							// return 1 if nal unit is lost, 0 otherwise.
void cleanup();							// closes files, print statistics, etc.





#endif // !_loss_
