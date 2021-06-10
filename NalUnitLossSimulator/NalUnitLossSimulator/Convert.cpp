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
/*

//#include "stdafx.h""
#include <stdio.h>
#include <stdlib.h>
#include <windows.h>		// for sleep()


int good, lost, lineno;

void cleanup() {
	fclose(stdout);
	fprintf(stderr, "Lines in srcefile (inc. header) %d, good %d, lost %d, lossrate %2.2f%%\n",
		lineno, good, lost, 100.0*(float)lost / (float)(lost + good));
	fprintf(stderr, "Terminating in 20 sec\n");
	Sleep(20000);		// note: in windows, this is in millisec, not sec
	exit(0);
}

int main(int argc, char argv[]) {
	int i, c, inlinecnt;

	// skip over error file header; first 7 lines
	for (i = 0; i<7; i++)
	while ((c = getchar()) != EOF)
	if (c == '\n')
		break;
	lineno = 8;
	good = lost = inlinecnt = 0;

	while (1) {
		// skip over the first number, until comma
		while ((c = getchar()) != EOF)
		if (c == ',')
			break;
		if (c == EOF)
			cleanup();

		//skip over second number, until comma
		while ((c = getchar()) != EOF)
		if (c == ',')
			break;
		if (c == EOF)
			cleanup();
		//interprete character after comma, '1' means lost, '0' means good
		if ((c = getchar()) == EOF)
			cleanup();
		else if (c == '1') {
			printf("0");
			lost++;
		}
		else {
			printf("1");
			good++;
		}
		lineno++;
		inlinecnt++;
		if (inlinecnt >= 64) {
			printf("\n");
			inlinecnt = 0;
		}
	}
}
*/
