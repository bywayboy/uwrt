#ifndef _FILELOCK_H_
#define _FILELOCK_H_

	/** Whence values for lseek(); renamed by POSIX 1003.1 */
	#define L_SET       SEEK_SET
	#define L_CURR      SEEK_CUR
	#define L_INCR      SEEK_CUR
	#define L_XTND      SEEK_END

	/** Operations for flock() function */
	#define LOCK_SH     1   /** Shared lock. */
	#define LOCK_EX     2   /** Exclusive lock. */
	#define LOCK_NB     4   /** Don't block when locking. */
	#define LOCK_UN     8   /** Unlock. */

	/** Operations for access function */
	#define F_OK        0   /** does file exist */
	#define X_OK        1   /** is it executable or searchable by caller */
	#define W_OK        2   /** is it writable by caller */
	#define R_OK        4   /** is it readable by caller */

	int flock (int fd, int operation);
#endif
