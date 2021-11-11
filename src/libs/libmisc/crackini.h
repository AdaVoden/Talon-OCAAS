typedef struct {
    char **list;
    int nlist;
} IniList;

extern IniList *iniRead (char *fn);
extern char *iniFind (IniList *listp, char *section, char *name);
extern void iniFree (IniList *listp);

/* For RCS Only -- Do Not Edit
 * @(#) $RCSfile: crackini.h,v $ $Date: 2003/04/15 20:48:18 $ $Revision: 1.1.1.1 $ $Name:  $
 */
