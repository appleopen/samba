#ifndef _INCLUDES_H
#define _INCLUDES_H
/* 
   Unix SMB/Netbios implementation.
   Version 1.9.
   Machine customisation and include handling
   Copyright (C) Andrew Tridgell 1994-1998
   
   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef NO_CONFIG_H /* for some tests */
#include "config.h"
#endif

#include "local.h"

#ifdef AIX
#define DEFAULT_PRINTING PRINT_AIX
#define PRINTCAP_NAME "/etc/qconfig"
#endif

#ifdef HPUX
#define DEFAULT_PRINTING PRINT_HPUX
#endif

#ifdef QNX
#define DEFAULT_PRINTING PRINT_QNX
#endif

#ifdef SUNOS4
/* on SUNOS4 termios.h conflicts with sys/ioctl.h */
#undef HAVE_TERMIOS_H
#endif

#ifdef LINUX
#ifndef DEFAULT_PRINTING
#define DEFAULT_PRINTING PRINT_BSD
#endif
#ifndef PRINTCAP_NAME
#define PRINTCAP_NAME "/etc/printcap"
#endif
#endif

/* use gcc attribute to check printf fns */
#ifdef __GNUC__
#define PRINTF_ATTRIBUTE(a1, a2) __attribute__ ((format (__printf__, a1, a2)))
#else
#define PRINTF_ATTRIBUTE(a1, a2)
#endif

#ifdef RELIANTUNIX
/*
 * <unistd.h> has to be included before any other to get
 * large file support on Reliant UNIX. Yes, it's broken :-).
 */
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#endif /* RELIANTUNIX */

#include <sys/types.h>

#ifdef TIME_WITH_SYS_TIME
#include <sys/time.h>
#include <time.h>
#else
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#else
#include <time.h>
#endif
#endif

#ifdef HAVE_SYS_RESOURCE_H
#include <sys/resource.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <stdio.h>
#include <stddef.h>

#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

#ifdef HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif

#ifdef HAVE_UNIXSOCKET
#include <sys/un.h>
#endif

#ifdef HAVE_SYS_SYSCALL_H
#include <sys/syscall.h>
#elif HAVE_SYSCALL_H
#include <syscall.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_MEMORY_H
#include <memory.h>
#endif

#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#else
#ifdef HAVE_SYS_FCNTL_H
#include <sys/fcntl.h>
#endif
#endif

#include <sys/stat.h>

#ifdef HAVE_LIMITS_H
#include <limits.h>
#endif

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#ifdef HAVE_SYS_FILIO_H
#include <sys/filio.h>
#endif

#include <signal.h>

#ifdef HAVE_SYS_WAIT_H
#include <sys/wait.h>
#endif
#ifdef HAVE_CTYPE_H
#include <ctype.h>
#endif
#ifdef HAVE_GRP_H
#include <grp.h>
#endif
#ifdef HAVE_SYS_PRIV_H
#include <sys/priv.h>
#endif
#ifdef HAVE_SYS_ID_H
#include <sys/id.h>
#endif

#include <errno.h>

#ifdef HAVE_UTIME_H
#include <utime.h>
#endif

#ifdef HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif

#ifdef HAVE_SYS_MODE_H
/* apparently AIX needs this for S_ISLNK */
#ifndef S_ISLNK
#include <sys/mode.h>
#endif
#endif

#ifdef HAVE_GLOB_H
#include <glob.h>
#endif

#include <pwd.h>

#ifdef HAVE_STDARG_H
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <syslog.h>
#include <sys/file.h>

#ifdef HAVE_NETINET_TCP_H
#include <netinet/tcp.h>
#endif

/*
 * The next three defines are needed to access the IPTOS_* options
 * on some systems.
 */

#ifdef HAVE_NETINET_IN_SYSTM_H
#include <netinet/in_systm.h>
#endif

#ifdef HAVE_NETINET_IN_IP_H
#include <netinet/in_ip.h>
#endif

#ifdef HAVE_NETINET_IP_H
#include <netinet/ip.h>
#endif

#if defined(HAVE_TERMIOS_H)
/* POSIX terminal handling. */
#include <termios.h>
#elif defined(HAVE_TERMIO_H)
/* Older SYSV terminal handling - don't use if we can avoid it. */
#include <termio.h>
#elif defined(HAVE_SYS_TERMIO_H)
/* Older SYSV terminal handling - don't use if we can avoid it. */
#include <sys/termio.h>
#endif

#if HAVE_DIRENT_H
# include <dirent.h>
# define NAMLEN(dirent) strlen((dirent)->d_name)
#else
# define dirent direct
# define NAMLEN(dirent) (dirent)->d_namlen
# if HAVE_SYS_NDIR_H
#  include <sys/ndir.h>
# endif
# if HAVE_SYS_DIR_H
#  include <sys/dir.h>
# endif
# if HAVE_NDIR_H
#  include <ndir.h>
# endif
#endif

#ifdef HAVE_SYS_MMAN_H
#include <sys/mman.h>
#endif

#ifdef HAVE_NET_IF_H
#include <net/if.h>
#endif


#ifdef HAVE_SYS_MOUNT_H
#include <sys/mount.h>
#endif

#ifdef HAVE_SYS_VFS_H
#include <sys/vfs.h>
#endif

#ifdef HAVE_SYS_ACL_H
#include <sys/acl.h>
#endif

#ifdef HAVE_SYS_FS_S5PARAM_H 
#include <sys/fs/s5param.h>
#endif

#if defined (HAVE_SYS_FILSYS_H) && !defined (_CRAY)
#include <sys/filsys.h> 
#endif

#ifdef HAVE_SYS_STATFS_H
# include <sys/statfs.h>
#endif

#ifdef HAVE_DUSTAT_H              
#include <sys/dustat.h>
#endif

#ifdef HAVE_SYS_STATVFS_H          
#include <sys/statvfs.h>
#endif

#ifdef HAVE_SHADOW_H
#include <shadow.h>
#endif

#ifdef HAVE_GETPWANAM
#include <sys/label.h>
#include <sys/audit.h>
#include <pwdadj.h>
#endif

#ifdef HAVE_SYS_SECURITY_H
#include <sys/security.h>
#include <prot.h>
#define PASSWORD_LENGTH 16
#endif  /* HAVE_SYS_SECURITY_H */

#ifdef HAVE_COMPAT_H
#include <compat.h>
#endif

#ifdef HAVE_STROPTS_H
#include <stropts.h>
#endif

#ifdef HAVE_POLL_H
#include <poll.h>
#endif

#ifdef HAVE_SYS_CAPABILITY_H

#if defined(BROKEN_REDHAT_7_SYSTEM_HEADERS) && !defined(_I386_STATFS_H)
#define _I386_STATFS_H
#define BROKEN_REDHAT_7_STATFS_WORKAROUND
#endif

#include <sys/capability.h>

#ifdef BROKEN_REDHAT_7_STATFS_WORKAROUND
#undef _I386_STATFS_H
#undef BROKEN_REDHAT_7_STATFS_WORKAROUND
#endif

#endif

#if defined(HAVE_RPC_RPC_H)
/*
 * Check for AUTH_ERROR define conflict with rpc/rpc.h in prot.h.
 */
#if defined(HAVE_SYS_SECURITY_H) && defined(HAVE_RPC_AUTH_ERROR_CONFLICT)
#undef AUTH_ERROR
#endif
#include <rpc/rpc.h>
#endif

#if defined(HAVE_YP_GET_DEFAULT_DOMAIN) && defined(HAVE_SETNETGRENT) && defined(HAVE_ENDNETGRENT) && defined(HAVE_GETNETGRENT)
#define HAVE_NETGROUP 1
#endif

#if defined (HAVE_NETGROUP)
#if defined(HAVE_RPCSVC_YP_PROT_H)
#include <rpcsvc/yp_prot.h>
#endif
#if defined(HAVE_RPCSVC_YPCLNT_H)
#include <rpcsvc/ypclnt.h>
#endif
#endif /* HAVE_NETGROUP */

#if defined(HAVE_SYS_IPC_H)
#include <sys/ipc.h>
#endif /* HAVE_SYS_IPC_H */

#if defined(HAVE_SYS_SHM_H)
#include <sys/shm.h>
#endif /* HAVE_SYS_SHM_H */

/*
 * Define VOLATILE if needed.
 */

#if defined(HAVE_VOLATILE)
#define VOLATILE volatile
#else
#define VOLATILE
#endif

/*
 * Define additional missing types
 */
#ifndef HAVE_SIG_ATOMIC_T_TYPE
typedef int sig_atomic_t;
#endif

#ifndef HAVE_SOCKLEN_T_TYPE
typedef int socklen_t;
#endif


#ifndef uchar
#define uchar unsigned char
#endif

#ifdef HAVE_UNSIGNED_CHAR
#define schar signed char
#else
#define schar char
#endif

/*
   Samba needs type definitions for int16, int32, uint16 and uint32.

   Normally these are signed and unsigned 16 and 32 bit integers, but
   they actually only need to be at least 16 and 32 bits
   respectively. Thus if your word size is 8 bytes just defining them
   as signed and unsigned int will work.
*/

#ifndef uint8
#define uint8 unsigned char
#endif

#if !defined(int16) && !defined(HAVE_INT16_FROM_RPC_RPC_H)
#if (SIZEOF_SHORT == 4)
#define int16 __ERROR___CANNOT_DETERMINE_TYPE_FOR_INT16;
#else /* SIZEOF_SHORT != 4 */
#define int16 short
#endif /* SIZEOF_SHORT != 4 */
#endif

/*
 * Note we duplicate the size tests in the unsigned 
 * case as int16 may be a typedef from rpc/rpc.h
 */

#if !defined(uint16) && !defined(HAVE_UINT16_FROM_RPC_RPC_H)
#if (SIZEOF_SHORT == 4)
#define uint16 __ERROR___CANNOT_DETERMINE_TYPE_FOR_INT16;
#else /* SIZEOF_SHORT != 4 */
#define uint16 unsigned short
#endif /* SIZEOF_SHORT != 4 */
#endif

#if !defined(int32) && !defined(HAVE_INT32_FROM_RPC_RPC_H)
#if (SIZEOF_INT == 4)
#define int32 int
#elif (SIZEOF_LONG == 4)
#define int32 long
#elif (SIZEOF_SHORT == 4)
#define int32 short
#else
/* uggh - no 32 bit type?? probably a CRAY. just hope this works ... */
#define uint32 int
#endif
#endif

/*
 * Note we duplicate the size tests in the unsigned 
 * case as int32 may be a typedef from rpc/rpc.h
 */

#if !defined(uint32) && !defined(HAVE_UINT32_FROM_RPC_RPC_H)
#if (SIZEOF_INT == 4)
#define uint32 unsigned int
#elif (SIZEOF_LONG == 4)
#define uint32 unsigned long
#elif (SIZEOF_SHORT == 4)
#define uint32 unsigned short
#else
/* uggh - no 32 bit type?? probably a CRAY. just hope this works ... */
#define uint32 unsigned
#endif
#endif

/*
 * Types for devices, inodes and offsets.
 */

#ifndef SMB_DEV_T
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_DEV64_T)
#    define SMB_DEV_T dev64_t
#  else
#    define SMB_DEV_T dev_t
#  endif
#endif

/*
 * Setup the correctly sized inode type.
 */

#ifndef SMB_INO_T
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_INO64_T)
#    define SMB_INO_T ino64_t
#  else
#    define SMB_INO_T ino_t
#  endif
#endif

#ifndef LARGE_SMB_INO_T
#  if (defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_INO64_T)) || (defined(SIZEOF_INO_T) && (SIZEOF_INO_T == 8))
#    define LARGE_SMB_INO_T 1
#  endif
#endif

#ifdef LARGE_SMB_INO_T
#define SINO_T(p, ofs, v) (SIVAL(p,ofs,(v)&0xFFFFFFFF), SIVAL(p,(ofs)+4,(v)>>32))
#else 
#define SINO_T(p, ofs, v) (SIVAL(p,ofs,v),SIVAL(p,(ofs)+4,0))
#endif

#ifndef SMB_OFF_T
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OFF64_T)
#    define SMB_OFF_T off64_t
#  else
#    define SMB_OFF_T off_t
#  endif
#endif

/* this should really be a 64 bit type if possible */
#define br_off SMB_BIG_UINT

#define SMB_OFF_T_BITS (sizeof(SMB_OFF_T)*8)

/*
 * Set the define that tells us if we can do 64 bit
 * NT SMB calls.
 */

#ifndef LARGE_SMB_OFF_T
#  if (defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_OFF64_T)) || (defined(SIZEOF_OFF_T) && (SIZEOF_OFF_T == 8))
#    define LARGE_SMB_OFF_T 1
#  endif
#endif

#ifdef LARGE_SMB_OFF_T
#define SOFF_T(p, ofs, v) (SIVAL(p,ofs,(v)&0xFFFFFFFF), SIVAL(p,(ofs)+4,(v)>>32))
#define SOFF_T_R(p, ofs, v) (SIVAL(p,(ofs)+4,(v)&0xFFFFFFFF), SIVAL(p,ofs,(v)>>32))
#else 
#define SOFF_T(p, ofs, v) (SIVAL(p,ofs,v),SIVAL(p,(ofs)+4,0))
#define SOFF_T_R(p, ofs, v) (SIVAL(p,(ofs)+4,v),SIVAL(p,ofs,0))
#endif

/*
 * Type for stat structure.
 */

#ifndef SMB_STRUCT_STAT
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STAT64) && defined(HAVE_OFF64_T)
#    define SMB_STRUCT_STAT struct stat64
#  else
#    define SMB_STRUCT_STAT struct stat
#  endif
#endif

/*
 * Type for dirent structure.
 */

#ifndef SMB_STRUCT_DIRENT
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STRUCT_DIRENT64)
#    define SMB_STRUCT_DIRENT struct dirent64
#  else
#    define SMB_STRUCT_DIRENT struct dirent
#  endif
#endif

/*
 * Defines for 64 bit fcntl locks.
 */

#ifndef SMB_STRUCT_FLOCK
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STRUCT_FLOCK64) && defined(HAVE_OFF64_T)
#    define SMB_STRUCT_FLOCK struct flock64
#  else
#    define SMB_STRUCT_FLOCK struct flock
#  endif
#endif

#ifndef SMB_F_SETLKW
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STRUCT_FLOCK64) && defined(HAVE_OFF64_T)
#    define SMB_F_SETLKW F_SETLKW64
#  else
#    define SMB_F_SETLKW F_SETLKW
#  endif
#endif

#ifndef SMB_F_SETLK
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STRUCT_FLOCK64) && defined(HAVE_OFF64_T)
#    define SMB_F_SETLK F_SETLK64
#  else
#    define SMB_F_SETLK F_SETLK
#  endif
#endif

#ifndef SMB_F_GETLK
#  if defined(HAVE_EXPLICIT_LARGEFILE_SUPPORT) && defined(HAVE_STRUCT_FLOCK64) && defined(HAVE_OFF64_T)
#    define SMB_F_GETLK F_GETLK64
#  else
#    define SMB_F_GETLK F_GETLK
#  endif
#endif

#if defined(HAVE_LONGLONG)
#define SMB_BIG_UINT unsigned long long
#define SMB_BIG_INT long long
#define SBIG_UINT(p, ofs, v) (SIVAL(p,ofs,(v)&0xFFFFFFFF), SIVAL(p,(ofs)+4,(v)>>32))
#else
#define SMB_BIG_UINT unsigned long
#define SMB_BIG_INT long
#define SBIG_UINT(p, ofs, v) (SIVAL(p,ofs,v),SIVAL(p,(ofs)+4,0))
#endif

#define SMB_BIG_UINT_BITS (sizeof(SMB_BIG_UINT)*8)

#ifndef MIN
#define MIN(a,b) ((a)<(b)?(a):(b))
#endif

#ifndef MAX
#define MAX(a,b) ((a)>(b)?(a):(b))
#endif

#ifndef HAVE_STRERROR
extern char *sys_errlist[];
#define strerror(i) sys_errlist[i]
#endif

#ifndef HAVE_STRCHR
# define strchr                 index
# define strrchr                rindex
#endif

#ifndef HAVE_ERRNO_DECL
extern int errno;
#endif

#ifdef HAVE_BROKEN_GETGROUPS
#define GID_T int
#else
#define GID_T gid_t
#endif

#ifndef NGROUPS_MAX
#define NGROUPS_MAX 32 /* Guess... */
#endif

/* Lists, trees, caching, database... */
#include "ubi_sLinkList.h"
#include "ubi_dLinkList.h"
#include "dlinklist.h"
#include "../tdb/tdb.h"
#include "../tdb/spinlock.h"
#include "talloc.h"
#include "interfaces.h"
#include "hash.h"
#include "trans2.h"
#include "nterr.h"
#include "secrets.h"
#include "messages.h"
#include "util_list.h"

#include "util_getent.h"

#ifndef UBI_BINTREE_H
#include "ubi_Cache.h"
#endif /* UBI_BINTREE_H */

#include "debugparse.h"

#include "version.h"
#include "smb.h"
#include "smbw.h"
#include "nameserv.h"

#include "byteorder.h"

#include "kanji.h"
#include "charset.h"

#include "ntdomain.h"

#include "msdfs.h"

#include "profile.h"

#include "mapping.h"

#include "rap.h"

#ifndef MAXCODEPAGELINES
#define MAXCODEPAGELINES 256
#endif

/*
 * Type for wide character dirent structure.
 * Only d_name is defined by POSIX.
 */

typedef struct smb_wdirent {
	wpstring        d_name;
} SMB_STRUCT_WDIRENT;

/*
 * Type for wide character passwd structure.
 */

typedef struct smb_wpasswd {
	wfstring       pw_name;
	char           *pw_passwd;
	uid_t          pw_uid;
	gid_t          pw_gid;
	wpstring       pw_gecos;
	wpstring       pw_dir;
	wpstring       pw_shell;
} SMB_STRUCT_WPASSWD;

/* Defines for wisXXX functions. */
#define UNI_UPPER    0x1
#define UNI_LOWER    0x2
#define UNI_DIGIT    0x4
#define UNI_XDIGIT   0x8
#define UNI_SPACE    0x10

#include "nsswitch/nss.h"

/***** automatically generated prototypes *****/
#include "proto.h"

/* String routines */

#include "safe_string.h"

#ifdef __COMPAR_FN_T
#define QSORT_CAST (__compar_fn_t)
#endif

#ifndef QSORT_CAST
#define QSORT_CAST (int (*)(const void *, const void *))
#endif

/* this guess needs to be improved (tridge) */
#if (defined(STAT_STATVFS) || defined(STAT_STATVFS64)) && !defined(SYSV)
#define SYSV 1
#endif

#ifndef DEFAULT_PRINTING
#ifdef HAVE_CUPS
#define DEFAULT_PRINTING PRINT_CUPS
#define PRINTCAP_NAME "cups"
#elif defined(SYSV)
#define DEFAULT_PRINTING PRINT_SYSV
#define PRINTCAP_NAME "lpstat"
#else
#define DEFAULT_PRINTING PRINT_BSD
#define PRINTCAP_NAME "/etc/printcap"
#endif
#endif

#ifndef PRINTCAP_NAME
#define PRINTCAP_NAME "/etc/printcap"
#endif

#ifndef SIGCLD
#define SIGCLD SIGCHLD
#endif

#ifndef MAP_FILE
#define MAP_FILE 0
#endif

#if (!defined(WITH_NISPLUS) && !defined(WITH_LDAP) && !defined(WITH_TDB_SAM))
#define USE_SMBPASS_DB 1
#endif

#if defined(HAVE_PUTPRPWNAM) && defined(AUTH_CLEARTEXT_SEG_CHARS)
#define OSF1_ENH_SEC 1
#endif

#ifndef ALLOW_CHANGE_PASSWORD
#if (defined(HAVE_TERMIOS_H) && defined(HAVE_DUP2) && defined(HAVE_SETSID))
#define ALLOW_CHANGE_PASSWORD 1
#endif
#endif

/* what is the longest significant password available on your system? 
 Knowing this speeds up password searches a lot */
#ifndef PASSWORD_LENGTH
#define PASSWORD_LENGTH 8
#endif

#ifdef REPLACE_INET_NTOA
#define inet_ntoa rep_inet_ntoa
#endif

#ifndef HAVE_PIPE
#define SYNC_DNS 1
#endif

#ifndef MAXPATHLEN
#define MAXPATHLEN 256
#endif

#ifndef SEEK_SET
#define SEEK_SET 0
#endif

#ifndef INADDR_LOOPBACK
#define INADDR_LOOPBACK 0x7f000001
#endif

#ifndef INADDR_NONE
#define INADDR_NONE 0xffffffff
#endif

#ifndef HAVE_CRYPT
#define crypt ufc_crypt
#endif

#ifndef O_ACCMODE
#define O_ACCMODE (O_RDONLY | O_WRONLY | O_RDWR)
#endif

#if defined(HAVE_CRYPT16) && defined(HAVE_GETAUTHUID)
#define ULTRIX_AUTH 1
#endif

#ifdef HAVE_LIBREADLINE
#  ifdef HAVE_READLINE_READLINE_H
#    include <readline/readline.h>
#    ifdef HAVE_READLINE_HISTORY_H
#      include <readline/history.h>
#    endif
#  else
#    ifdef HAVE_READLINE_H
#      include <readline.h>
#      ifdef HAVE_HISTORY_H
#        include <history.h>
#      endif
#    else
#      undef HAVE_LIBREADLINE
#    endif
#  endif
#endif

#ifndef HAVE_STRDUP
char *strdup(const char *s);
#endif

#ifndef HAVE_MEMMOVE
void *memmove(void *dest,const void *src,int size);
#endif

#ifndef HAVE_INITGROUPS
int initgroups(char *name,gid_t id);
#endif

#ifndef HAVE_RENAME
int rename(const char *zfrom, const char *zto);
#endif

#ifndef HAVE_MKTIME
time_t mktime(struct tm *t);
#endif

#ifndef HAVE_FTRUNCATE
int ftruncate(int f,long l);
#endif

#ifndef HAVE_STRTOUL
unsigned long strtoul(const char *nptr, char **endptr, int base);
#endif

#if (defined(USE_SETRESUID) && !defined(HAVE_SETRESUID_DECL))
/* stupid glibc */
int setresuid(uid_t ruid, uid_t euid, uid_t suid);
#endif
#if (defined(USE_SETRESUID) && !defined(HAVE_SETRESGID_DECL))
int setresgid(gid_t rgid, gid_t egid, gid_t sgid);
#endif
#ifndef HAVE_VASPRINTF_DECL
int vasprintf(char **ptr, const char *format, va_list ap);
#endif

#if !defined(HAVE_BZERO) && defined(HAVE_MEMSET)
#define bzero(a,b) memset((a),'\0',(b))
#endif

#ifdef REPLACE_GETPASS
#define getpass(prompt) getsmbpass((prompt))
#endif

/*
 * Some older systems seem not to have MAXHOSTNAMELEN
 * defined.
 */
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 254
#endif

/* yuck, I'd like a better way of doing this */
#define DIRP_SIZE (256 + 32)

/*
 * glibc on linux doesn't seem to have MSG_WAITALL
 * defined. I think the kernel has it though..
 */

#ifndef MSG_WAITALL
#define MSG_WAITALL 0
#endif

/* default socket options. Dave Miller thinks we should default to TCP_NODELAY
   given the socket IO pattern that Samba uses */
#ifdef TCP_NODELAY
#define DEFAULT_SOCKET_OPTIONS "TCP_NODELAY"
#else
#define DEFAULT_SOCKET_OPTIONS ""
#endif

/* Load header file for libdl stuff */

#ifdef HAVE_LIBDL
#include <dlfcn.h>
#endif

/* Some POSIX definitions for those without */
 
#ifndef S_IFDIR
#define S_IFDIR         0x4000
#endif
#ifndef S_ISDIR
#define S_ISDIR(mode)   ((mode & 0xF000) == S_IFDIR)
#endif
#ifndef S_IRWXU
#define S_IRWXU 00700           /* read, write, execute: owner */
#endif
#ifndef S_IRUSR
#define S_IRUSR 00400           /* read permission: owner */
#endif
#ifndef S_IWUSR
#define S_IWUSR 00200           /* write permission: owner */
#endif
#ifndef S_IXUSR
#define S_IXUSR 00100           /* execute permission: owner */
#endif
#ifndef S_IRWXG
#define S_IRWXG 00070           /* read, write, execute: group */
#endif
#ifndef S_IRGRP
#define S_IRGRP 00040           /* read permission: group */
#endif
#ifndef S_IWGRP
#define S_IWGRP 00020           /* write permission: group */
#endif
#ifndef S_IXGRP
#define S_IXGRP 00010           /* execute permission: group */
#endif
#ifndef S_IRWXO
#define S_IRWXO 00007           /* read, write, execute: other */
#endif
#ifndef S_IROTH
#define S_IROTH 00004           /* read permission: other */
#endif
#ifndef S_IWOTH
#define S_IWOTH 00002           /* write permission: other */
#endif
#ifndef S_IXOTH
#define S_IXOTH 00001           /* execute permission: other */
#endif

/* NetBSD doesn't have these */
#ifndef SHM_R
#define SHM_R 0400
#endif

#ifndef SHM_W
#define SHM_W 0200
#endif

#if HAVE_KERNEL_SHARE_MODES
#ifndef LOCK_MAND 
#define LOCK_MAND	32	/* This is a mandatory flock */
#define LOCK_READ	64	/* ... Which allows concurrent read operations */
#define LOCK_WRITE	128	/* ... Which allows concurrent write operations */
#define LOCK_RW		192	/* ... Which allows concurrent read & write ops */
#endif
#endif

extern int DEBUGLEVEL;

#define MAX_SEC_CTX_DEPTH 8    /* Maximum number of security contexts */


#ifdef GLIBC_HACK_FCNTL64
/* this is a gross hack. 64 bit locking is completely screwed up on
   i386 Linux in glibc 2.1.95 (which ships with RedHat 7.0). This hack
   "fixes" the problem with the current 2.4.0test kernels 
*/
#define fcntl fcntl64
#undef F_SETLKW 
#undef F_SETLK 
#define F_SETLK 13
#define F_SETLKW 14
#endif

/* Needed for sys_dlopen/sys_dlsym/sys_dlclose */
#ifndef RTLD_GLOBAL
#define RTLD_GLOBAL 0
#endif

#ifndef RTLD_LAZY
#define RTLD_LAZY 0
#endif

#ifndef RTLD_NOW
#define RTLD_NOW 0
#endif

/* add varargs prototypes with printf checking */
int fdprintf(int , char *, ...) PRINTF_ATTRIBUTE(2,3);
#ifndef HAVE_SNPRINTF_DECL
int snprintf(char *,size_t ,const char *, ...) PRINTF_ATTRIBUTE(3,4);
#endif
#ifndef HAVE_ASPRINTF_DECL
int asprintf(char **,const char *, ...) PRINTF_ATTRIBUTE(2,3);
#endif

/* we used to use these fns, but now we have good replacements
   for snprintf and vsnprintf */
#define slprintf snprintf
#define vslprintf vsnprintf

/* MacOS X DirectoryService */
#ifdef DIRECTORY_SERVICE_X
#include <DirectoryService/DirServices.h>
#include <DirectoryService/DirServicesConst.h>
#include <DirectoryService/DirServicesUtils.h>
#include <Security/checkpw.h>
#include <CoreFoundation/CFString.h>

tDirNodeReference getusernode(tDirReference dirRef, char *userName);
BOOL pass_check_directoryservice(char *user, char *challenge, char *password);
BOOL DirServicesAuthUser(tDirReference dirRef, tDirNodeReference userNode, char *user, char *challenge, char *password, char *inAuthMethod);
#endif

/* MacOS X CFString */
#ifdef USES_CFSTRING
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CFStringEncodingExt.h>
#include <CoreFoundation/CFStringEncodingConverter.h>
#endif

#endif /* _INCLUDES_H */

