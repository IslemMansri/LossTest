/* The copyright in this software is being made available under the BSD
* License, included below. This software may be subject to other third party
* and contributor rights, including patent rights, and no such rights are
* granted under this license.
*
* Copyright (c) 2010-2012, ITU/ISO/IEC
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
*  * Redistributions of source code must retain the above copyright notice,
*    this list of conditions and the following disclaimer.
*  * Redistributions in binary form must reproduce the above copyright notice,
*    this list of conditions and the following disclaimer in the documentation
*    and/or other materials provided with the distribution.
*  * Neither the name of the ITU/ISO/IEC nor the names of its contributors may
*    be used to endorse or promote products derived from this software without
*    specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS
* BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
* THE POSSIBILITY OF SUCH DAMAGE.
*/


//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <math.h>
#include <float.h>
#include <ctype.h>



#define OPTIONCHAR '-'
#define MAXNALUNITSIZE 1000000
#define HM5_0_ANCHOR
#define OLD_ERR_PAT_FORMAT

static FILE *errfile, *infile, *outfile;
static unsigned char nalu[MAXNALUNITSIZE];
static int nalutypecountread[64], nalutypecountwrite[64];

static int nalusread = 0, naluswritten = 0;
static int debuglevel = 1;				// default, can be overwritten by -d
static int applyerrors = 2;				// default, parameter sets make it, can be overridden by -a

void parsecmd(int ac, char *av[]);		// parses commandline and opens files, writes global statics
int readnalu(unsigned char *buf);		// returns size of read nal unit
void writenalu(unsigned char *buf, int size);		// write nalu
int nal_unit_type(unsigned char *buf);		// returns the nal_unit_type of the nal unit in buf
int naluloss();							// return 1 if nal unit is lost, 0 otherwise.
int startcode();						// reads in infile, returns 1 if current pos in file infile is a startcode,
										// and 0 otherwise.  No side effects but cleanup()and exit() when end of
										// input bitstream is reached at start code boundary.
void cleanup();							// closes files, print statistics, etc.




int main(int argc, char* argv[]) {
	int nalusize, nalutype, nalucount = 0;
	int apply;

	// note: the application logic herein is using the NAL unit types of WD5-d6 (JCTVC-G1103-d6).
	// nal_unit_type() "translates" from earlier syntax, i.e. from HM_5_0_ANCHORS
	//

	parsecmd(argc, argv);

	while (1) {
		nalusize = readnalu(nalu);
		nalutype = nal_unit_type(nalu);
		nalucount++;

		if (debuglevel >1)
			printf("NAL unit %5d, length %5d, with type %2d ",
				nalucount, nalusize, nalutype);
		apply = 1;

		switch (applyerrors) {
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
		}

		if (!apply) {			// error patterns are not being applied...
			if (debuglevel>1)
				printf("... written\n");
			writenalu(nalu, nalusize);
		}
		else if (!naluloss()) { // error patterns applied, but no error found in pattern file
			if (debuglevel>1)
				printf("... written\n");
			writenalu(nalu, nalusize);
		}
		else					//  pattern appied and pattern file says nalu is to be discarded
			if (debuglevel>1)
				printf("... discarded\n");

	}
	fprintf(stderr, "You should never see this\n");
	getchar();
}

int readnalu(unsigned char *buf) {
	int c, pos = 0;

	// Check NAL unit starts with start code
	if (!startcode()) {		// can be no startcode in file at this position or EOF
		if (feof(infile)) {
			// EOF found at the position of the first octet of the start code.
			// This is the correct position for an EOF in the bitstream, so clean up
			// and exit successfully
			cleanup();
			exit(0);
		}
		else {
			fprintf(stderr, "Startcode expected at octet offset %d expected, exiting\n", ftell(infile));
			exit(-1);
		}
	}

	// read startcode
	if (fread(&buf[pos], 1, 4, infile) != 4) {
		fprintf(stderr, "cannot read startcode at octet offset %d or thereabouts, existing\n", ftell(infile));
		exit(-1);
	}
	pos += 4;

	// read NAL unit

	while (!startcode()) {
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

int startcode() {

	int c0, c1, c2, c3;
	if ((c0 = fgetc(infile)) == EOF)
		return 0;
	if (c0 != 0) {
		if (fseek(infile, -1, SEEK_CUR) != 0) {
			fprintf(stderr, "fseek() fails in startcode() (0)\n");
			exit(-1);
		}
		return 0;	// not a startcode
	}

	if ((c1 = fgetc(infile)) == EOF)
		return 0;
	if (c1 != 0) {
		if (fseek(infile, -2, SEEK_CUR) != 0) {
			fprintf(stderr, "fseek() fails in startcode() (1)\n");
			exit(-1);
		}
		return 0;	// not a startcode
	}

	if ((c2 = fgetc(infile)) == EOF)
		return 0;
	if (c2 != 0) {
		if (fseek(infile, -3, SEEK_CUR) != 0) {
			fprintf(stderr, "fseek() fails in startcode() (2)\n");
			exit(-1);
		}
		return 0;	// not a startcode
	}
	if ((c3 = fgetc(infile)) == EOF)
		return 0;
	if (c3 != 0x01) {
		if (fseek(infile, -4, SEEK_CUR) != 0) {
			fprintf(stderr, "fseek() fails in startcode() (3)\n");
			exit(-1);
		}
		return 0;
	}


	// now we have found a startcode so go back in the file and return "found"

	if (fseek(infile, -4, SEEK_CUR) != 0) {
		fprintf(stderr, "fseek() fails in startcode() (5)\n");
		exit(-1);
	}
	return (1);
}

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
		fprintf(stderr, "Usage: xxx\n");
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
			applyerrors = atoi(&av[i][2]);

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
			nalusread, naluswritten, 100 - (float)naluswritten*100.0 / (float)nalusread);
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

#ifdef HM5_0_ANCHOR
int nal_unit_type(unsigned char buf[]) {
	unsigned int firstbyte;
	unsigned int nalutype;

	firstbyte = buf[4];

	nalutype = firstbyte & 0x1f;	// 5 least significant bits
									// sanity check: forbidden_zero_bit is really zero
	if (firstbyte & 0x80) {
		fprintf(stderr, "forbidden_zero_bit is 1 at octet %d, exiting\n", ftell(infile));
		exit(-1);
	}

	// Sanity check: only allowed NAL unit types.  This reflects JCTVC-G1103-d6, table 7-1 on page 48
	// also statistics

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

	}
	return nalutype;
}
#endif