/* code to add two pterror files and produce a third.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <errno.h>

#include "P_.h"
#include "astro.h"
#include "circum.h"

#define	COMMENT	'#'		/* ignore lines beginning with this */

typedef struct {
    double ha, dec;		/* sky loc of error node */
    double dha, ddec;		/* error (target - wcs), dha is polar angle */
} MeshPoint;

static MeshPoint *mpoints0;	/* malloced list of mesh points, first file */
static int nmpoints0;
static MeshPoint *mpoints1;	/* malloced list of mesh points, second file */
static int nmpoints1;

#define	DFLTPTGRAD	degrad(10)	/* default max mesh interp radius */
static double ptgrad = DFLTPTGRAD; /* pointing interpolation radius, rads */

static int vflag;		/* verbose flag */

static void usage (char *p);
static void readMeshFile (char *fn, MeshPoint **mpp, int *nmp);
static void buildNewMesh(void);
static MeshPoint *newMeshPoint (MeshPoint **mpp, int *nmp);
static void interp1 (double ha, double dec, double *ehap, double *edecp);
static int cmpMP (const void *p1, const void *p2);
static void sortMPoints(MeshPoint *mp, int nmp);

int
main (int ac, char *av[])
{
	char *progname = av[0];

	/* crack arguments as -abc or -a -b -c */
	for (av++; --ac > 0 && **av == '-'; av++) {
	    char *str = *av;
	    char c;
	    /* in the loop ac includes the current arg */
	    while ((c = *++str) != '\0')
		switch (c) {
		case 'v':	/* more verbosity */
		    vflag++;
		    break;
		case 'r':	/* averaging radius, degrees */
		    if (ac < 2)
			usage (progname);
		     ptgrad = degrad(atof(*++av));
		    --ac;
		    break;
		default:
		    usage(progname);
		    break;
		}
	}

	/* now there are ac remaining args starting at av[0] */
	if (ac != 2)
	    usage (progname);

	/* read each file -- exit if severe trouble */
	readMeshFile (av[0], &mpoints0, &nmpoints0);
	readMeshFile (av[1], &mpoints1, &nmpoints1);

	/* sort the second so it can be rapidly searched and interpolated */
	sortMPoints(mpoints1, nmpoints1);

	/* form new mesh */
	buildNewMesh();

	return (0);
}

static void
usage (char *p)
{
	fprintf (stderr, "%s: [-options] file1 file2\n", p);
	fprintf (stderr, "Add pterrors reports file1 and file2 at file1 points.\n");
	fprintf (stderr, "  -v: verbose\n");
	fprintf (stderr, "  -r n: max interpolation radius, degrees; default = %g\n",
							raddeg(DFLTPTGRAD));

	exit(1);
}

/* read a mesh file into *mpp and set *nmp to count.
 * the mesh file is expected to have errors in arc mins, dHA a polar angle.
 * if trouble, just exit.
 */
static void
readMeshFile(char *meshfn, MeshPoint **mpp, int *nmp)
{
	char line[1024];
	int lineno;
	FILE *fp;

	/* open mesh file */
	fp = fopen (meshfn, "r");
	if (!fp) {
	    fprintf (stderr, "%s: %s\n", meshfn, strerror(errno));
	    exit(1);
	}

	/* read each line, building *mpp */
	lineno = 0;
	while (fgets(line, sizeof(line), fp)) {
	    double ha, dec, dha, ddec;
	    MeshPoint *mp;

	    lineno++;
	    if (line[0] == COMMENT)
		continue;
	    if (sscanf (line, "%lf %lf %lf %lf", &ha, &dec, &dha, &ddec) != 4)
		continue;
	    mp = newMeshPoint(mpp, nmp);
	    if (!mp) {
		fprintf (stderr, "No memory for mesh log");
		exit(1);
	    }
	    mp->ha = hrrad(ha);
	    mp->dec = degrad(dec);
	    mp->dha = degrad(dha/60.0);
	    mp->ddec = degrad(ddec/60.0);
	}

	fclose (fp);

	if (vflag)
	    fprintf (stderr, "%s: read %d mesh points\n", meshfn, *nmp);
}

/* add mpoints1 to mpoints0 and print the sum */
static void
buildNewMesh()
{
	int i;

	for (i = 0; i < nmpoints0; i++) {
	    MeshPoint *mp = &mpoints0[i];
	    double ha, dec, dha, ddec;

	    ha = mp->ha;
	    dec = mp->dec;
	    interp1 (ha, dec, &dha, &ddec);
	    printf ("%10.5f %10.5f %7.2f %7.2f\n", radhr(ha), raddeg(dec),
			raddeg(mp->dha + dha)*60, raddeg(mp->ddec + ddec)*60);
	}
}

/* given a target location and the mesh points in mpoints1[], interpolate
 *   to find the error there.
 * use a weighted average based on distance if find two or more mesh points 
 *   within ptgrad. the weight is the inverse of the distance away from the
 *   target. If don't find at least two then just return 0 error.
 * N.B. we assume mpoints1[] is sorted by increasing tdec.
 */
static void
interp1 (double ha, double dec, double *ehap, double *edecp)
{
	double cdec = cos(dec), sdec = sin(dec);
	double cptgrad = cos(ptgrad);
	double swh, swd, sw;
	int nfound;
	int l, u, m;
	int i;

	/* binary search for closest tdec */
	l = 0;
	u = nmpoints1-1;
	do {
	    m = (l+u)/2;
	    if (dec < mpoints1[m].dec)
		u = m-1;
	    else
		l = m+1;
	} while (l <= u);

	/* look either side of m within ptgrad */
	for (u = m; u < nmpoints1 && fabs(dec - mpoints1[u].dec) <= ptgrad; u++)
	    continue;
	for (l = m; l >= 0 && fabs(dec - mpoints1[l].dec) <= ptgrad; --l)
	    continue;

	swh = swd = sw = 0.0;
	nfound = 0;
	for (i = l+1; i < u; i++) {
	    MeshPoint *rp = &mpoints1[i];
	    double cosr, w;	/* cos dist, weight */
	    double edec;	/* dec error */
	    double eha;		/* ha error */

	    /* distance to this mesh point -- reject immediately if > ptgrad */
	    cosr = sdec*sin(rp->dec) + cdec*cos(rp->dec)*cos(ha - rp->ha);
	    if (cosr < cptgrad)
		continue;

	    /* weight varies linearly from 1 if right on a mesh point to 0
	     * at ptgrad.
	     */
	    w = (ptgrad - acos(cosr))/ptgrad;

	    eha = rp->dha;
	    edec = rp->ddec;

	    swh += w*eha;
	    swd += w*edec;
	    sw += w;

	    nfound++;
	}

	/* if found at least two, use average.
	 * else return 0 errors.
	 */
	if (nfound >= 2) {
	    *ehap = swh/sw;
	    *edecp = swd/sw;
	} else {
	    *ehap = 0.0;
	    *edecp = 0.0;
	}
}

/* add room for one more in (*mpp)[] and return pointer to the new one.
 * return NULL if no more room.
 */
static MeshPoint *
newMeshPoint (MeshPoint **mpp, int *nmp)
{
	char *new;

	new = (*mpp) ? realloc ((void*)(*mpp), ((*nmp)+1)*sizeof(MeshPoint))
						: malloc (sizeof(MeshPoint));
	if (!new)
	    return (NULL);

	*mpp = (MeshPoint *) new;
	return (&(*mpp)[(*nmp)++]);
}

/* qsort-style function to compare 2 MeshPoints by increasing tdec */
static int
cmpMP (const void *p1, const void *p2)
{
	MeshPoint *m1 = (MeshPoint *)p1;
	MeshPoint *m2 = (MeshPoint *)p2;
	double ddec = m1->dec - m2->dec;

	if (ddec < 0)
	    return (-1);
	if (ddec > 0)
	    return (1);
	return (0);
}

/* sort the given mpoints array by increasing dec */
static void
sortMPoints(MeshPoint *mp, int nmp)
{
	qsort ((void *)mp, nmp, sizeof(MeshPoint), cmpMP);
}
