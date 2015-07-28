/* unistd.h (emx+gcc) */

#if !defined (_UNISTD_H)
#define _UNISTD_H

#if defined (__cplusplus)
extern "C" {
#endif

#if defined (_POSIX_C_SOURCE) && !defined (_POSIX_SOURCE)
#define _POSIX_SOURCE
#endif

#if !defined (_SIZE_T)
#define _SIZE_T
typedef unsigned long size_t;
#endif

#if !defined (_SSIZE_T)
#define _SSIZE_T
typedef int ssize_t;
#endif

#if !defined (NULL)
#define NULL ((void *)0)
#endif

#if !defined (STDIN_FILENO)
#define STDIN_FILENO    0
#define STDOUT_FILENO   1
#define STDERR_FILENO   2
#endif

#if !defined (F_OK)
#define F_OK 0
#define X_OK 1
#define W_OK 2
#define R_OK 4
#endif

#if !defined (SEEK_SET)
#define SEEK_SET 0
#define SEEK_CUR 1
#define SEEK_END 2
#endif

#if !defined (_PC_LINK_MAX)
#define _PC_LINK_MAX            1
#define _PC_MAX_CANON           2
#define _PC_MAX_INPUT           3
#define _PC_NAME_MAX            4
#define _PC_PATH_MAX            5
#define _PC_PIPE_BUF            6
#define _PC_CHOWN_RESTRICTED    7
#define _PC_NO_TRUNC            8
#define _PC_VDISABLE            9
#endif

#if !defined (_SC_ARG_MAX)
#define _SC_ARG_MAX             1
#define _SC_CHILD_MAX           2
#define _SC_CLK_TCK             3
#define _SC_NGROUPS_MAX         4
#define _SC_OPEN_MAX            5
#define _SC_STREAM_MAX          6
#define _SC_TZNAME_MAX          7
#define _SC_JOB_CONTROL         8
#define _SC_SAVED_IDS           9
#define _SC_VERSION             10
#endif

#if !defined (_POSIX_VERSION)
#define _POSIX_VERSION          199009L
#endif


int access (__const__ char *name, int mode);
unsigned alarm (unsigned sec);
int chdir (__const__ char *name);
/* chown() */
int close (int handle);
/* ctermid() */
char *cuserid (char *buffer);
int dup (int handle);
int dup2 (int handle1, int handle2);
int execl (__const__ char *name, __const__ char *arg0, ...);
int execle (__const__ char *name, __const__ char *arg0, ...);
int execlp (__const__ char *name, __const__ char *arg0, ...);
int execv (__const__ char *name, char * __const__ argv[]);
int execve (__const__ char *name, char * __const__ argv[],
    char * __const__ envp[]);
int execvp (__const__ char *name, char * __const__ argv[]);
void _exit (int ret) __attribute__ ((__noreturn__));
int fork (void);
long fpathconf (int handle, int name);
char *getcwd (char *buffer, size_t size);
int getegid (void);             /* gid_t getegid (void); */
int geteuid (void);             /* uid_t geteuid (void); */
int getgid (void);              /* gid_t getgid (void); */
/* int getgroups (int gidsetsize, gid_t grouplist[]); */
int getgroups (int gidsetsize, int grouplist[]);
char *getlogin (void);
int getpgrp (void);             /* pid_t getpgrp (void); */
int getpid (void);              /* pid_t getpid (void); */
int getppid (void);             /* pid_t getppid (void); */
int getuid (void);              /* uid_t getuid (void); */
int isatty (int handle);
/* link() */
long lseek (int handle, long offset, int origin);
long pathconf (__const__ char *path, int name);
int pause (void);
int pipe (int *two_handles);
int read (int handle, void *buf, size_t nbyte);
int rmdir (__const__ char *name);
int setgid (int gid);           /* int setsid (gid_t gid); */
int setpgid (int pid, int pgid); /* int setpgid (gid_t pid, gid_t pgid); */
int setsid (void);              /* pid_t setsid (void); */
int setuid (int uid);           /* setuid (uid_t uid); */
unsigned sleep (unsigned sec);
long sysconf (int name);
int tcgetpgrp (int fd);         /* pid_t tcgetpgrp (int fd); */
int tcsetpgrp (int fd, int pgrp); /* int tcsetpgrp (int fd, pid_t pgrp) */
/* ttyname() */
int unlink (__const__ char *name);
int write (int handle, __const__ void *buf, size_t nbyte);


#if !defined (_POSIX_SOURCE)

char *getpass (__const__ char *prompt);
char *_getpass1 (__const__ char *prompt);
char *_getpass2 (__const__ char *prompt, int kbd);

#endif


#if !defined (_POSIX_SOURCE) || _POSIX_C_SOURCE >= 2

extern char *optarg;
extern int optind;
extern int opterr;
extern int optopt;

/* Note: `argv' is not const as GETOPT_ANY reorders argv[]. */

int getopt (int argc, char * argv[], __const__ char *opt_str);

#endif

#if defined (__cplusplus)
}
#endif

#endif /* !defined (_UNISTD_H) */
