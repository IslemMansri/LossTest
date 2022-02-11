/*Edited By Islem Mansri Silx 28/06/2020: In order to work with the Annex B byte stream format for HEVC.*/
/*Tested with Bitstreams from HM16.17*/

//#include "stdafx.h"
#include <stdio.h>
#include <stdlib.h>
#include <tchar.h>
#include <math.h>
#include <float.h>
#include <ctype.h>
#include "loss265.h"

int main(int argc, char* argv[]) 
{
	int nalusize, nalutype, nalucount = 0;		//Silx:	declare int types for the NalUnit
	int apply;

	// note: the application logic herein is using the NAL unit types of WD5-d6 (JCTVC-G1103-d6).
	// nal_unit_type() "translates" from earlier syntax, i.e. from HM_5_0_ANCHORS
	//

	parsecmd(argc, argv);	//Silx: checked in my case with mode -a (applyerrors is the resuts of parsecmd)

	while (1) 
	{
		nalusize = readnalu(nalu);	//Silx: read nalu
		nalutype = nal_unit_type(nalu);	//get nalutype
		nalucount++;

		if (debuglevel >1)
			printf("NAL unit %5d, length %5d, with type %2d ",
				nalucount, nalusize, nalutype);

		apply = 1;


		switch (applyerrors) 
		{
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


		if (!apply) // error patterns are not being applied...
		{			
			if (debuglevel>1)
				printf("... written\n");
			writenalu(nalu, nalusize);
		}
		else if (!naluloss()) // error patterns applied, but no error found in pattern file
		{ 
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