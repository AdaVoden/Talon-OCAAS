
/* Read and write config file functions */

typedef struct
{
	char linekey[32];
	char fileset[32];
	char ourset[32];
	
} EntryRec;

enum {MATCH, NOMATCH, END, ERROR};
extern int writeNewCfgFile(char *cfgFile, EntryRec *pE);
extern char * getFileValue(EntryRec *pE, char *namekey);
extern char * getOurValue(EntryRec *pE, char *namekey);
extern int readValuesFromConfig(char * cfgFile, EntryRec *pE);


