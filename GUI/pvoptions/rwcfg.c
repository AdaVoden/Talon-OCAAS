/* Read and write values to/from the Perl config files used in PVCAM for pvoptions */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <unistd.h>
#include "telenv.h"
#include "rwcfg.h"

#define FALSE 0
#define TRUE (!FALSE)

static int readNextCfgLine(FILE * fp, EntryRec * pE, char *pLine, EntryRec **pOutEntry);


/* Read a line from the config file and compare it to set of EntryRecs.
   Fill in line buffer so it might be written out again
   Fill in EntryRec buffer if there is a match
   Return MATCH, NOMATCH, END, or ERROR
*/
static int readNextCfgLine(FILE * fp, EntryRec * pE, char *pLine, EntryRec **pOutEntry)
{
	int c;
	int found;
	EntryRec *p;			
	char name[32];
	char value[32];
	char *pName;
	char *pValue;

	// skip leading white space
	do { *pLine++ = c = getc(fp); } while(c <= 32 && c != EOF);
	// if end of file, return END
	if(c == EOF) {
		*pLine = '\0';
		return END;	
	}
	// is this a comment? if so, skip to end of line and exit
	if(c == '#') {
		do {*pLine++ = c = getc(fp); } while(c != '\n' && c != EOF);
		*pLine = '\0';
		return NOMATCH;
	}
	// gather a name
	pName = name;
	*pName++ = c;
	do {*pLine++ = *pName++ = c = getc(fp); } while(c > 32);
	--pName;
	*pName = '\0';
	// compare name to entry recs
	found = FALSE;
	p = pE;
	while(p->linekey[0])
	{
		if(!strcmp(p->linekey, name)) {
			found = TRUE;
			break;
		}
		p++;
	}	
	if(!found)	{
		do {*pLine++ = c = getc(fp); } while(c != '\n' && c != EOF);
		*pLine = '\0';
		return NOMATCH;
	}
	
	// move ahead to value portion
	do { *pLine++ = c = getc(fp); } while(c <= 32 && c != EOF);
	// if end of file, return END
	if(c == EOF) {
		*pLine = '\0';
		return END;	
	}
	
	// We expect to see an = sign here... if not, just copy the line and continue
	if(c != '=') {
		do {*pLine++ = c = getc(fp); } while(c != '\n');
		*pLine = '\0';
		return NOMATCH;
	}
	
	do { *pLine++ = c = getc(fp); } while(c <= 32 && c != EOF);
	// if end of file, return END
	if(c == EOF) {
		*pLine = '\0';
		return END;	
	}	
	
	// Copy the value up to the semicolon or end of line or end of file
	pValue = value;
	*pValue++ = c;
	do {*pLine++ = *pValue++ = c = getc(fp); } while(c != ';' && c != '\n' && c != EOF);
	--pValue;
	*pValue = '\0';
	
	// if we read the value
	if(c == ';') {
		// put it into the EntryRec
		strcpy(p->fileset,value);
		// copy rest of line after semicolon
		do{*pLine++ = c = getc(fp); } while(c != '\n' && c != EOF);
		*pLine = '\0';
		if(pOutEntry) *pOutEntry = p;
		return MATCH;
	}

	// didn't read it properly somehow...	
	*pLine = '\0';
	return ERROR;
}	

// Fill an EntryRec list with the values we find in the given config file
int readValuesFromConfig(char * cfgFile, EntryRec *pE)
{
	FILE *fp;
	int result;
	int matches;
	char cfgLine[1024];
	
	fp = telfopen(cfgFile,"r");
	if(!fp)
		return -1;
		
	matches = 0;		
	do {	
		result = readNextCfgLine(fp, pE, cfgLine,NULL);
		
		switch(result)
		{
			case MATCH:
				matches++;
				break;
			case ERROR:
				fclose(fp);				
				return -1;
			case NOMATCH:
				break;
			case END:
				break;
		}
		
	} while(result != END);

	return 0;				
}

// Get pointer to the file value string for a given key in an EntryRec list
char * getFileValue(EntryRec *pE, char *namekey)
{
	EntryRec *p = pE;
	while(p->linekey[0])
	{
		if(!strcmp(p->linekey,namekey)) {
			return p->fileset;
		}
		p++;
	}
	return NULL;
}

// Get pointer to the ourset value string for a given key in an EntryRec list
char * getOurValue(EntryRec *pE, char *namekey)
{
	EntryRec *p = pE;
	while(p->linekey[0])
	{
		if(!strcmp(p->linekey,namekey)) {
			return p->ourset;
		}
		p++;
	}
	return NULL;
}

// Write out a new config file using the values in the EntryRec
int writeNewCfgFile(char *cfgFile, EntryRec *pE)
{
#define CFGCOPY "archive/.cfgCopy"

	FILE *fpIn;
	FILE *fpOut;
	EntryRec *p;
	int result;
	int rt,done;
	char cfgLine[1024];
	char file1[512];
	char file2[512];
	
	fpIn = telfopen(cfgFile,"r");
	if(fpIn == 0)	return -1;
	
	fpOut = telfopen(CFGCOPY,"w");
	if(fpOut == 0) 	return -1;
	
	rt = 0;
	done = FALSE;
	while(!done)
	{
		result = readNextCfgLine(fpIn, pE, cfgLine, &p);
		switch(result)
		{
			case MATCH:
				fprintf(fpOut,"%s = %s;\n",p->linekey,p->ourset);
				break;
			case NOMATCH:
				fprintf(fpOut,"%s",cfgLine);	// newline already a part of this string
				break;
			case ERROR:
				rt = -1;
				// fall thru
			case END:
				done = TRUE;
				break;
		}
	}
	
	fclose(fpIn);
	fclose(fpOut);
	if(rt != -1)
	{
		telfixpath(file1,cfgFile);
		unlink(file1);
		telfixpath(file2,CFGCOPY);
		rename(file2, file1);
	}
	return rt;
}		
