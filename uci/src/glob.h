#ifndef GLOB_H_
#define GLOB_H_
typedef struct {
	size_t gl_pathc;			/* Count of total paths so far. */
	size_t gl_matchc;			/* Count of paths matching pattern. */
	size_t gl_offs;				/* Reserved at beginning of gl_pathv. */
	int gl_flags;				/* Copy of flags parameter to glob. */
	char **gl_pathv;			/* List of paths matching pattern. */
	/* Copy of errfunc parameter to glob. */
	int (*gl_errfunc) (const char *, int);
} glob_t;

/* Believed to have been introduced in 1003.2-1992 */
#define	GLOB_APPEND		0x0001	/* Append to output from previous call. */
#define	GLOB_DOOFFS		0x0002	/* Use gl_offs. */
#define	GLOB_ERR		0x0004	/* Return on error. */
#define	GLOB_MARK		0x0008	/* Append / to matching directories. */
#define	GLOB_NOCHECK	0x0010	/* Return pattern itself if nothing matches. */
#define	GLOB_NOSORT		0x0020	/* Don't sort. */

/* Error values returned by glob(3) */
#define	GLOB_NOSPACE	(-1)	/* Malloc call failed. */
#define	GLOB_ABORTED	(-2)	/* Unignored error. */
#define	GLOB_NOMATCH	(-3)	/* No match and GLOB_NOCHECK was not set. */
#define	GLOB_NOSYS		(-4)	/* Obsolete: source comptability only. */

#define	GLOB_ALTDIRFUNC	0x0040	/* Use alternately specified directory funcs. */
#define	GLOB_MAGCHAR	0x0100	/* Pattern had globbing characters. */
#define	GLOB_NOMAGIC	0x0200	/* GLOB_NOCHECK without magic chars (csh). */
#define	GLOB_QUOTE		0x0400	/* Quote special chars with \. */
#define	GLOB_LIMIT		0x1000	/* limit number of returned paths */

int glob(const char *, int, int (*)(const char *, int), glob_t *);
void globfree(glob_t *);


#endif