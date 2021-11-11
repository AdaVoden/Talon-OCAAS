#ifndef util_H
#define util_H

#include <fstream.h>

extern unsigned int GetUINT(unsigned const char *, int bigEndian);
extern unsigned int GetULONG(unsigned const char *, int bigEndian);
extern void PutUINT(unsigned int, unsigned char *, int bigEndian);
extern void PutULONG(unsigned int, unsigned char *, int bigEndian);
extern unsigned int RoundUp4(unsigned int);

extern void PrintVersionInfo();

extern void DumpMessage(const unsigned char *data, unsigned int length);

extern const char *GetArg(int &argi, int argc, const char *const *argv);

extern int WriteAll(int fd, const unsigned char *data, unsigned int length);

extern ostream *logofs;

#endif /* util_H */
