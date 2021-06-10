

/*Edited By Islem Mansri Silx 28/06/2020: In order to work with the Annex B byte stream format for HEVC.*/
					/*Tested with Bitstreams from HM16.17*/

//#include "stdafx.h"
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

static FILE *errfile, *infile, *outfile;
static unsigned char nalu[MAXNALUNITSIZE];
static int nalutypecountread[64], nalutypecountwrite[64];		//Silx: what is this ? is the 64 refere to the total types of NalUnits?

static int nalusread = 0, naluswritten = 0;
static int debuglevel = 1;				// default, can be overwritten by -d
static int applyerrors = 1;				// default, parameter sets make it, can be overridden by -a

void parsecmd(int ac, char *av[]);		// parses commandline and opens files, writes global statics
int readnalu(unsigned char *buf);		// returns size of read nal unit
void writenalu(unsigned char *buf, int size);		// write nalu
int nal_unit_type(unsigned char *buf);		// returns the nal_unit_type of the nal unit in buf
int naluloss();							// return 1 if nal unit is lost, 0 otherwise.
int ZB_startcode();						// reads in infile, returns 1 if current pos in file infile is a ZB_startcode,
// and 0 otherwise.  No side effects but cleanup()and exit() when end of
// input bitstream is reached at start code boundary.
void cleanup();							// closes files, print statistics, etc.




int main(int argc, char* argv[]) {
	int nalusize, nalutype, nalucount = 0;		//Silx:	declare int types for the NalUnit
	int apply;

	// note: the application logic herein is using the NAL unit types of WD5-d6 (JCTVC-G1103-d6).
	// nal_unit_type() "translates" from earlier syntax, i.e. from HM_5_0_ANCHORS
	//

	parsecmd(argc, argv);	//Silx: checked in my case with mode -a (applyerrors is the resuts of parsecmd)

	while (1) {
		nalusize = readnalu(nalu);			
		nalutype = nal_unit_type(nalu);
		nalucount++;

		if (debuglevel >1)
			printf("NAL unit %5d, length %5d, with type %2d ",
			nalucount, nalusize, nalutype);
		apply = 1;


		//Silx: edited////////////////////////////
		
		/*switch (applyerrors) {
		case 0:		// everything goes through (debug)
			apply = 0;
			break;

		case 1:		// Parameter sets not exposed to errors
			apply = !(nalutype == 7 || nalutype == 8);
			break;

		case 2:		//  + IDR not exposed
			apply = !(nalutype == 7 || nalutype == 8 || nalutype == 5);
			break;

		case 3:		//  + CRA not exposed
			apply = !(nalutype == 7 || nalutype == 8 || nalutype == 5 || nalutype == 4);
			break;

		case 4:		//  + normal slices not exposed
			apply = !(nalutype == 7 || nalutype == 8 || nalutype == 5 || nalutype == 4 || nalutype == 1);
			break;

		case 5:		// everything exposed
			apply = 1;
			break;

			// create your own cases here; don't forget the "break"

		default:
			fprintf(stderr, "wrong Apply Errors parameter %d\n", applyerrors);
		}*/

		switch (applyerrors) {
		case 0:		// everything goes through (debug)
			apply = 0;
			break;

		case 1:		// Parameter sets (VPS, SPS, PPS, 32, 33, 34) not exposed to errors
			apply = !(nalutype == 32 || nalutype == 33 || nalutype == 34);
			break;

		case 2:		//  + IDR (19, 20) not exposed
			apply = !(nalutype == 32 || nalutype == 33 || nalutype == 34 || nalutype == 19 || nalutype == 20);
			break;

		case 3:		//  + CRA not exposed
			apply = !(nalutype == 32 || nalutype == 33 || nalutype == 34 || nalutype == 19 || nalutype == 20 || nalutype == 21);
			break;

		case 4:		// everything exposed
			apply = 1;
			break;

			// create your own cases here; don't forget the "break"

		default:
			fprintf(stderr, "wrong Apply Errors parameter %d\n", applyerrors);
		}
		///////////////////////////////////////////////////////////


		if (!apply)	{			// error patterns are not being applied...
			if (debuglevel>1)
				printf("... written\n");
			writenalu(nalu, nalusize);
		}
		else if (!naluloss())	{ // error patterns applied, but no error found in pattern file
			if (debuglevel>1)
				printf("... written\n");
			writenalu(nalu, nalusize);			//Silx: I think this is where the error patter is applied to the bitstream when -a5 is used.
		}
		else					//  pattern applied and pattern file says nalu is to be discarded
		if (debuglevel>1)
			printf("... discarded\n");

	}
	fprintf(stderr, "You should never see this\n");
	getchar();
}

int readnalu(unsigned char *buf) {
	int c, pos = 0; //Silx: pos is the position I think.


	//Silx: Check if current position in NAL unit is a ZB_start code.
	if (!ZB_startcode()) {		// can be no ZB_startcode in file at this position or EOF     //Silx: EOF EndOfFile!
		if (feof(infile)) {																//Silx: Check if current position is a start code or not (1 yes, 0 no).
			// EOF found at the position of the first octet of the start code.
			// This is the correct position for an EOF in the bitstream, so clean up
			// and exit successfully
			cleanup();
			exit(0);
		}
		else {
			fprintf(stderr, "ZB_startcode expected at octet offset %d expected, exiting\n", ftell(infile));
			exit(-1);
		}
	}

	// read ZB_startcode
	if (fread(&buf[pos], 1, 4, infile) != 4) {     //Silx: read 4 elements of size 1 bye from infile and put it into buf (read 4 bytes -> ZB_startcode)
		fprintf(stderr, "cannot read ZB_startcode at octet offset %d or thereabouts, existing\n", ftell(infile));
		exit(-1);
	}
	pos += 4;

	// read NAL unit

	while (!ZB_startcode()) {
		if ((c = getc(infile)) == EOF) {
			break;
		}
		buf[pos++] = (unsigned char)c;
		if (pos >= MAXNALUNITSIZE) {
			fprintf(stderr, "Found NAL unit of more than %d bits, can't deal with that,\
														 use slices :-) or recompile with larger NALUNITSIZE, exiting\n", MAXNALUNITSIZE);
			exit(-1);
		}
	}
	nalusread++;
	nalutypecountread[nal_unit_type(buf)]++;
	if (debuglevel>3)
		printf("NAL unit length %d at pos %d first bytes of nalu: %x %x %x %x %x %x",
		pos, ftell(infile) - pos, buf[0], buf[1], buf[2], buf[3], buf[4], buf[5]);
	return pos;
}

void writenalu(unsigned char *buf, int size) {
	if (fwrite(buf, 1, size, outfile) != size) {
		fprintf(stderr, "Cannot write %d bytes to outfile, exiting\n", size);
		exit(-1);
	}
	naluswritten++;
	nalutypecountwrite[nal_unit_type(buf)]++;
}

int ZB_startcode() {		//ZB_startcode is the zero Byte + startcode

	int c0, c1, c2, c3;
	if ((c0 = fgetc(infile)) == EOF)
		return 0;
	if (c0 != 0) {
		if (fseek(infile, -1, SEEK_CUR) != 0) {
			fprintf(stderr, "fseek() fails in ZB_startcode() (0)\n");
			exit(-1);
		}
		return 0;	// not a ZB_startcode
	}

	if ((c1 = fgetc(infile)) == EOF)
		return 0;
	if (c1 != 0) {
		if (fseek(infile, -2, SEEK_CUR) != 0) {
			fprintf(stderr, "fseek() fails in ZB_startcode() (1)\n");
			exit(-1);
		}
		return 0;	// not a ZB_startcode
	}

	if ((c2 = fgetc(infile)) == EOF)
		return 0;
	if (c2 != 0) {
		if (fseek(infile, -3, SEEK_CUR) != 0) {
			fprintf(stderr, "fseek() fails in ZB_startcode() (2)\n");
			exit(-1);
		}
		return 0;	// not a ZB_startcode
	}
	if ((c3 = fgetc(infile)) == EOF)
		return 0;
	if (c3 != 0x01) {
		if (fseek(infile, -4, SEEK_CUR) != 0) {
			fprintf(stderr, "fseek() fails in ZB_startcode() (3)\n");
			exit(-1);
		}
		return 0;
	}


	// now we have found a ZB_startcode so go back in the file and return "found"

	if (fseek(infile, -4, SEEK_CUR) != 0) {
		fprintf(stderr, "fseek() fails in ZB_startcode() (5)\n");
		exit(-1);
	}
	return (1);
}


//Silx: This part of code I think is for testing the errfile (make sure that no whitespace is in it)
#ifdef OLD_ERR_PAT_FORMAT
int naluloss() {
	int c;
	do
	if ((c = getc(errfile)) == EOF) {
		if (debuglevel>1)
			printf("Error pattern file wrap-around\n");
		fseek(errfile, 0, SEEK_SET);
		c = getc(errfile);
	}
	while (isspace(c));		// Skip over whitespace
	return (c == '0');
}
#endif

void parsecmd(int ac, char *av[]) {
	int i;
	if (ac<4) {
		fprintf(stderr, "Usage: \n\n Loss <infile> <outfile> <errfile> <options>\n <infile> is the input NAL unit bitstream (Annex B, with start codes) as generated by the HM software\n <outfile> is a NAL unit bitstream file that may miss certain NAL units\n <errfile> is a textfile with error patters\n <options> check JCTVC-H0072 Doc\n");	//Silx: Added the usage description from the JCTVC-H0072 Doc
		exit(-1);
	}
	if ((infile = fopen(av[1], "rb")) == NULL) {
		fprintf(stderr, "Cannot open infile %s\n", av[1]);
		exit(-1);
	}
	if ((outfile = fopen(av[2], "wb")) == NULL) {
		fprintf(stderr, "Cannot open outfile %s\n", av[2]);
		exit(-1);
	}
	if ((errfile = fopen(av[3], "r")) == NULL) {
		fprintf(stderr, "Cannot open error file %s\n", av[3]);
		exit(-1);
	}
	for (i = 4; i<ac; i++) {
		if (av[i][0] != OPTIONCHAR) {
			fprintf(stderr, "invalid option %s\n", av[i]);
			exit(-1);
		}
		//interpret option here


		if (av[i][1] == 'a')
			// Apply Errors level.  Could use better parsing error handling			
			applyerrors = atoi(&av[i][2]); //Silx: I am interrested in this option "-a"

		else if (av[i][1] == 'd')
			// Apply debug level.  Could use better parsing error handling			
			debuglevel = atoi(&av[i][2]);

		else if (av[i][1] == 'r') {
			// for pseudo-random entry point in error pattern.  Parameter is offset in bytes
			int offset = atoi(&av[i][2]), filesize;
			fseek(errfile, 0, SEEK_END);
			filesize = ftell(errfile);
			fseek(errfile, offset%filesize, SEEK_SET);
		}
		else
			fprintf(stderr, "Unknown option %s, ignored", av[i]);
	}
}

void cleanup() {
	int i;

	fclose(infile);
	fclose(outfile);
	fclose(errfile);

	if (debuglevel>0) {
		printf("Loss terminated.  Read and processed %d NAL units\nWrote %d Nal units\nPercentage of loss: %3.2f%%\n",
			nalusread+1, naluswritten+1, 100 - (float)naluswritten*100.0 / (float)nalusread); //Silx: nalusread+1 and naluswritten+1 because the IDR Pic shares the same start prefix with the parametes set (PPS)
		for (i = 0; i<64; i++)
		if (nalutypecountread[i]>0)
			printf("Nal unit type %2d, read %5d, wrote %5d\n", i, nalutypecountread[i], nalutypecountwrite[i]);
		printf("press any key to exit...\n");
		getchar();
	}
}

#ifdef WD5_D6

// Extracts the NAL unit type accoridng to WD5-d6, JCTVC-G1103-d6.  Does not
// work with HM5.0 anchors

int nal_unit_type(unsigned char buf[]) {
	unsigned int firstbyte;
	unsigned int nalutype;

	firstbyte = buf[4];

	nalutype = firstbyte & 0x3f;	// 6 least significant bits
	// sanity check: forbidden_zero_bit is really zero
	if (firstbyte & 0x80) {
		fprintf(stderr, "forbidden_zero_bit is 1 at octet %d, exiting\n", ftell(infile));
		exit(-1);
	}

	// Sanity check: only allowed NAL unit types.  This reflects JCTVC-G1103-d6, table 7-1 on page 48
	// also statistics

	switch (nalutype) {

	case 7:		// SPS
	case 8:		// PPS
	case 13:	// Seems to be the NAL unit typoe for a picture

		// valid NAL unit types
		nalutypecount[nalutype]++;
		break;
	default:
		fprintf(stderr, "wrong NAL unit type %d 0x%x detected in first byte 0x%x, octet %d, exiting\n", nalutype, firstbyte, ftell(infile));
		getchar();
		exit(-1);

	}
	return nalutype;
}

#endif


//Silx: Edited://////////////

#ifdef HM16_17_ANCHOR
int nal_unit_type(unsigned char buf[]) {
	unsigned int firstbyte;
	unsigned int nalutype;
									//Silx: selects the first byte of the Nal Unit header
	firstbyte = buf[4];			    //Silx: note that the header in H.264/AVC is set only to one byte in contrastt to hevc it i set to 2 bytes.

	nalutype = (firstbyte & (0x7E)) >> 1;	//Silx: Mask the 6 bits responsible for the NalUnit type structure according to Annex B byte stream format (0x7E = 01111110) and the ">>" is for shiftting the resulting number to extract the exact 6 bits in the middle.
	// sanity check: forbidden_zero_bit is really zero
	if (firstbyte & 0x80) {
		fprintf(stderr, "forbidden_zero_bit is 1 at octet %d, exiting\n", ftell(infile));
		exit(-1);
	}

	// Sanity check: only allowed NAL unit types.  This reflects JCTVC-G1103-d6, table 7-1 on page 48
	// also statistics
	
	/*
	switch (nalutype) {
		//		case 1:
		//		case 4:
		//		case 5:
		//		case 6:
	case 7:	
	case 8:
		//		case 9:
		//		case 12:

		// valid NAL unit types
		break;
	case 13:
		// This seems to be the indication for a coded picture/slice in HM5.0.  Translate
		// it to 1 (coded slice of any type)
		nalutype = 1;
		break;

	default:
		fprintf(stderr, "wrong NAL unit type %d 0x%x detected in first byte 0x%x, octet %d, exiting\n", nalutype, firstbyte, ftell(infile));
		getchar();
		exit(-1);

	}*/

	///Silx:

	if (64 < nalutype || nalutype < 0){
	
	fprintf(stderr, "Wrong NAL unit type %d 0x%x detected in first byte 0x%x, octet %d, exiting\n", nalutype, firstbyte, ftell(infile));
	getchar();
	exit(-1);

	}
	return nalutype;
}
#endif
////////////////////////////////////