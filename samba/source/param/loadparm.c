/* 
   Unix SMB/CIFS implementation.
   Parameter loading functions
   Copyright (C) Karl Auer 1993-1998

   Largely re-written by Andrew Tridgell, September 1994

   Copyright (C) Simo Sorce 2001
   Copyright (C) Alexander Bokovoy 2002
   Copyright (C) Stefan (metze) Metzmacher 2002
   
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

/*
 *  Load parameters.
 *
 *  This module provides suitable callback functions for the params
 *  module. It builds the internal table of service details which is
 *  then used by the rest of the server.
 *
 * To add a parameter:
 *
 * 1) add it to the global or service structure definition
 * 2) add it to the parm_table
 * 3) add it to the list of available functions (eg: using FN_GLOBAL_STRING())
 * 4) If it's a global then initialise it in init_globals. If a local
 *    (ie. service) parameter then initialise it in the sDefault structure
 *  
 *
 * Notes:
 *   The configuration file is processed sequentially for speed. It is NOT
 *   accessed randomly as happens in 'real' Windows. For this reason, there
 *   is a fair bit of sequence-dependent code here - ie., code which assumes
 *   that certain things happen before others. In particular, the code which
 *   happens at the boundary between sections is delicately poised, so be
 *   careful!
 *
 */

#include "includes.h"

BOOL in_client = False;		/* Not in the client by default */
BOOL bLoaded = False;

extern userdom_struct current_user_info;
extern pstring user_socket_options;

#ifndef GLOBAL_NAME
#define GLOBAL_NAME "global"
#endif

#ifndef PRINTERS_NAME
#define PRINTERS_NAME "printers"
#endif

#ifndef HOMES_NAME
#define HOMES_NAME "homes"
#endif

/* some helpful bits */
#define LP_SNUM_OK(i) (((i) >= 0) && ((i) < iNumServices) && ServicePtrs[(i)]->valid)
#define VALID(i) ServicePtrs[i]->valid

int keepalive = DEFAULT_KEEPALIVE;
BOOL use_getwd_cache = True;

extern int extra_time_offset;

static BOOL defaults_saved = False;

/* 
 * This structure describes global (ie., server-wide) parameters.
 */
typedef struct
{
	char *smb_ports;
	char *dos_charset;
	char *unix_charset;
	char *display_charset;
	char *szPrintcapname;
	char *szEnumPortsCommand;
	char *szAddPrinterCommand;
	char *szDeletePrinterCommand;
	char *szOs2DriverMap;
	char *szLockDir;
	char *szPidDir;
	char *szRootdir;
	char *szDefaultService;
	char *szDfree;
	char *szMsgCommand;
	char *szHostsEquiv;
	char *szServerString;
	char *szAutoServices;
	char *szPasswdProgram;
	char *szPasswdChat;
	char *szLogFile;
	char *szConfigFile;
	char *szSMBPasswdFile;
	char *szPrivateDir;
	char **szPassdbBackend;
	char *szPasswordServer;
	char *szSocketOptions;
	char *szRealm;
	char *szADSserver;
	char *szUsernameMap;
	char *szLogonScript;
	char *szLogonPath;
	char *szLogonDrive;
	char *szLogonHome;
	char **szWINSservers;
	char **szInterfaces;
	char *szRemoteAnnounce;
	char *szRemoteBrowseSync;
	char *szSocketAddress;
	char *szNISHomeMapName;
	char *szAnnounceVersion;	/* This is initialised in init_globals */
	char *szWorkgroup;
	char *szNetbiosName;
	char **szNetbiosAliases;
	char *szNetbiosScope;
	char *szDomainOtherSIDs;
	char *szNameResolveOrder;
	char *szPanicAction;
	char *szAddUserScript;
	char *szDelUserScript;
	char *szAddGroupScript;
	char *szDelGroupScript;
	char *szAddUserToGroupScript;
	char *szDelUserFromGroupScript;
	char *szSetPrimaryGroupScript;
	char *szAddMachineScript;
	char *szShutdownScript;
	char *szAbortShutdownScript;
	char *szWINSHook;
	char *szWINSPartners;
#ifdef WITH_UTMP
	char *szUtmpDir;
	char *szWtmpDir;
	BOOL bUtmp;
#endif
	char *szSourceEnv;
	char *szWinbindUID;
	char *szWinbindGID;
	char *szNonUnixAccountRange;
	BOOL bAlgorithmicRidBase;
	char *szTemplateHomedir;
	char *szTemplateShell;
	char *szWinbindSeparator;
	BOOL bWinbindEnumUsers;
	BOOL bWinbindEnumGroups;
	BOOL bWinbindUseDefaultDomain;
	char *szAddShareCommand;
	char *szChangeShareCommand;
	char *szDeleteShareCommand;
	char *szGuestaccount;
	char *szManglingMethod;
	int mangle_prefix;
	int max_log_size;
	char *szLogLevel;
	int mangled_stack;
	int max_xmit;
	int max_mux;
	int max_open_files;
	int pwordlevel;
	int unamelevel;
	int deadtime;
	int maxprotocol;
	int minprotocol;
	int security;
	char **AuthMethods;
	BOOL paranoid_server_security;
	int maxdisksize;
	int lpqcachetime;
	int iMaxSmbdProcesses;
	BOOL bDisableSpoolss;
	int iTotalPrintJobs;
	int syslog;
	int os_level;
	int enhanced_browsing;
	int max_ttl;
	int max_wins_ttl;
	int min_wins_ttl;
	int ReadSize;
	int lm_announce;
	int lm_interval;
	int announce_as;	/* This is initialised in init_globals */
	int machine_password_timeout;
	int change_notify_timeout;
	int stat_cache_size;
	int map_to_guest;
	int min_passwd_length;
	int oplock_break_wait_time;
	int winbind_cache_time;
	int iLockSpinCount;
	int iLockSpinTime;
	char *szLdapMachineSuffix;
	char *szLdapUserSuffix;
#ifdef WITH_LDAP_SAMCONFIG
	int ldap_port;
	char *szLdapServer;
#endif
	int ldap_ssl;
	char *szLdapSuffix;
	char *szLdapFilter;
	char *szLdapAdminDn;
	BOOL ldap_trust_ids;
	char *szAclCompat;
	int ldap_passwd_sync; 
	BOOL bMsAddPrinterWizard;
	BOOL bDNSproxy;
	BOOL bWINSsupport;
	BOOL bWINSproxy;
	BOOL bLocalMaster;
	BOOL bPreferredMaster;
	BOOL bDomainMaster;
	BOOL bDomainLogons;
	BOOL bEncryptPasswords;
	BOOL bUpdateEncrypt;
	BOOL bStripDot;
	BOOL bNullPasswords;
	BOOL bObeyPamRestrictions;
	BOOL bLoadPrinters;
	BOOL bLargeReadwrite;
	BOOL bReadRaw;
	BOOL bWriteRaw;
	BOOL bReadPrediction;
	BOOL bReadbmpx;
	BOOL bSyslogOnly;
	BOOL bBrowseList;
	BOOL bNISHomeMap;
	BOOL bTimeServer;
	BOOL bBindInterfacesOnly;
	BOOL bPamPasswordChange;
	BOOL bUnixPasswdSync;
	BOOL bPasswdChatDebug;
	BOOL bTimestampLogs;
	BOOL bNTSmbSupport;
	BOOL bNTPipeSupport;
	BOOL bNTStatusSupport;
	BOOL bStatCache;
	BOOL bKernelOplocks;
	BOOL bAllowTrustedDomains;
	BOOL bLanmanAuth;
	BOOL bNTLMAuth;
	BOOL bUseSpnego;
	BOOL bClientLanManAuth;
	BOOL bClientNTLMv2Auth;
	BOOL bClientUseSpnego;
	BOOL bDebugHiresTimestamp;
	BOOL bDebugPid;
	BOOL bDebugUid;
	BOOL bHostMSDfs;
	BOOL bHideLocalUsers;
	BOOL bUnicode;
	BOOL bUseMmap;
	BOOL bHostnameLookups;
	BOOL bUnixExtensions;
	BOOL bDisableNetbios;
	BOOL bKernelChangeNotify;
	int restrict_anonymous;
	int name_cache_timeout;
	BOOL client_signing;
#ifdef WITH_OPENDIRECTORY
	BOOL bOpenDirectory;
#endif
}
global;

static global Globals;

/* 
 * This structure describes a single service. 
 */
typedef struct
{
	BOOL valid;
	BOOL autoloaded;
	char *szService;
	char *szPath;
	char *szUsername;
	char **szInvalidUsers;
	char **szValidUsers;
	char **szAdminUsers;
	char *szCopy;
	char *szInclude;
	char *szPreExec;
	char *szPostExec;
	char *szRootPreExec;
	char *szRootPostExec;
	char *szPrintcommand;
	char *szLpqcommand;
	char *szLprmcommand;
	char *szLppausecommand;
	char *szLpresumecommand;
	char *szQueuepausecommand;
	char *szQueueresumecommand;
	char *szPrintername;
	char *szDontdescend;
	char **szHostsallow;
	char **szHostsdeny;
	char *szMagicScript;
	char *szMagicOutput;
	char *szMangledMap;
	char *szVetoFiles;
	char *szHideFiles;
	char *szVetoOplockFiles;
	char *comment;
	char *force_user;
	char *force_group;
	char **readlist;
	char **writelist;
	char **printer_admin;
	char *volume;
	char *fstype;
	char *szVfsObjectFile;
	char *szVfsOptions;
	char *szVfsPath;
	char *szMSDfsProxy;
	int iMinPrintSpace;
	int iMaxPrintJobs;
	int iMaxReportedPrintJobs;
	int iWriteCacheSize;
	int iCreate_mask;
	int iCreate_force_mode;
	int iSecurity_mask;
	int iSecurity_force_mode;
	int iDir_mask;
	int iDir_force_mode;
	int iDir_Security_mask;
	int iDir_Security_force_mode;
	int iMaxConnections;
	int iDefaultCase;
	int iPrinting;
	int iOplockContentionLimit;
	int iCSCPolicy;
	int iBlock_size;
	BOOL bPreexecClose;
	BOOL bRootpreexecClose;
	BOOL bCaseSensitive;
	BOOL bCasePreserve;
	BOOL bShortCasePreserve;
	BOOL bCaseMangle;
	BOOL bHideDotFiles;
	BOOL bHideSpecialFiles;
	BOOL bHideUnReadable;
	BOOL bHideUnWriteableFiles;
	BOOL bBrowseable;
	BOOL bAvailable;
	BOOL bRead_only;
	BOOL bNo_set_dir;
	BOOL bGuest_only;
	BOOL bGuest_ok;
	BOOL bPrint_ok;
	BOOL bMap_system;
	BOOL bMap_hidden;
	BOOL bMap_archive;
	BOOL bLocking;
	BOOL bStrictLocking;
	BOOL bPosixLocking;
	BOOL bShareModes;
	BOOL bOpLocks;
	BOOL bLevel2OpLocks;
	BOOL bOnlyUser;
	BOOL bMangledNames;
	BOOL bWidelinks;
	BOOL bSymlinks;
	BOOL bSyncAlways;
	BOOL bStrictAllocate;
	BOOL bStrictSync;
	char magic_char;
	BOOL *copymap;
	BOOL bDeleteReadonly;
	BOOL bFakeOplocks;
	BOOL bDeleteVetoFiles;
	BOOL bDosFilemode;
	BOOL bDosFiletimes;
	BOOL bDosFiletimeResolution;
	BOOL bFakeDirCreateTimes;
	BOOL bBlockingLocks;
	BOOL bInheritPerms;
	BOOL bInheritACLS;
	BOOL bMSDfsRoot;
	BOOL bUseClientDriver;
	BOOL bDefaultDevmode;
	BOOL bNTAclSupport;
	BOOL bUseSendfile;
	BOOL bProfileAcls;

	char dummy[3];		/* for alignment */
}
service;


/* This is a default service used to prime a services structure */
static service sDefault = {
	True,			/* valid */
	False,			/* not autoloaded */
	NULL,			/* szService */
	NULL,			/* szPath */
	NULL,			/* szUsername */
	NULL,			/* szInvalidUsers */
	NULL,			/* szValidUsers */
	NULL,			/* szAdminUsers */
	NULL,			/* szCopy */
	NULL,			/* szInclude */
	NULL,			/* szPreExec */
	NULL,			/* szPostExec */
	NULL,			/* szRootPreExec */
	NULL,			/* szRootPostExec */
	NULL,			/* szPrintcommand */
	NULL,			/* szLpqcommand */
	NULL,			/* szLprmcommand */
	NULL,			/* szLppausecommand */
	NULL,			/* szLpresumecommand */
	NULL,			/* szQueuepausecommand */
	NULL,			/* szQueueresumecommand */
	NULL,			/* szPrintername */
	NULL,			/* szDontdescend */
	NULL,			/* szHostsallow */
	NULL,			/* szHostsdeny */
	NULL,			/* szMagicScript */
	NULL,			/* szMagicOutput */
	NULL,			/* szMangledMap */
	NULL,			/* szVetoFiles */
	NULL,			/* szHideFiles */
	NULL,			/* szVetoOplockFiles */
	NULL,			/* comment */
	NULL,			/* force user */
	NULL,			/* force group */
	NULL,			/* readlist */
	NULL,			/* writelist */
	NULL,			/* printer admin */
	NULL,			/* volume */
	NULL,			/* fstype */
	NULL,			/* vfs object */
	NULL,			/* vfs options */
	NULL,			/* vfs path */
	NULL,                   /* szMSDfsProxy */
	0,			/* iMinPrintSpace */
	1000,			/* iMaxPrintJobs */
	0,			/* iMaxReportedPrintJobs */
	0,			/* iWriteCacheSize */
	0744,			/* iCreate_mask */
	0000,			/* iCreate_force_mode */
	0777,			/* iSecurity_mask */
	0,			/* iSecurity_force_mode */
	0755,			/* iDir_mask */
	0000,			/* iDir_force_mode */
	0777,			/* iDir_Security_mask */
	0,			/* iDir_Security_force_mode */
	0,			/* iMaxConnections */
	CASE_LOWER,		/* iDefaultCase */
	DEFAULT_PRINTING,	/* iPrinting */
	2,			/* iOplockContentionLimit */
	0,			/* iCSCPolicy */
	1024,           /* iBlock_size */
	False,			/* bPreexecClose */
	False,			/* bRootpreexecClose */
	False,			/* case sensitive */
	True,			/* case preserve */
	True,			/* short case preserve */
	False,			/* case mangle */
	True,			/* bHideDotFiles */
	False,			/* bHideSpecialFiles */
	False,			/* bHideUnReadable */
	False,			/* bHideUnWriteableFiles */
	True,			/* bBrowseable */
	True,			/* bAvailable */
	True,			/* bRead_only */
	True,			/* bNo_set_dir */
	False,			/* bGuest_only */
	False,			/* bGuest_ok */
	False,			/* bPrint_ok */
	False,			/* bMap_system */
	False,			/* bMap_hidden */
	True,			/* bMap_archive */
	True,			/* bLocking */
	True,			/* bStrictLocking */
	True,			/* bPosixLocking */
	True,			/* bShareModes */
	False,			/* bOpLocks */
	True,			/* bLevel2OpLocks */
	False,			/* bOnlyUser */
	True,			/* bMangledNames */
	True,			/* bWidelinks */
	True,			/* bSymlinks */
	False,			/* bSyncAlways */
	False,			/* bStrictAllocate */
	False,			/* bStrictSync */
	'~',			/* magic char */
	NULL,			/* copymap */
	False,			/* bDeleteReadonly */
	False,			/* bFakeOplocks */
	False,			/* bDeleteVetoFiles */
	False,			/* bDosFilemode */
	False,			/* bDosFiletimes */
	False,			/* bDosFiletimeResolution */
	False,			/* bFakeDirCreateTimes */
	True,			/* bBlockingLocks */
	False,			/* bInheritPerms */
	False,			/* bInheritACLS */
	False,			/* bMSDfsRoot */
	False,			/* bUseClientDriver */
	False,			/* bDefaultDevmode */
	True,			/* bNTAclSupport */
	False,			/* bUseSendfile */
	False,			/* bProfileAcls */

	""			/* dummy */
};

/* local variables */
static service **ServicePtrs = NULL;
static int iNumServices = 0;
static int iServiceIndex = 0;
static BOOL bInGlobalSection = True;
static BOOL bGlobalOnly = False;
static int server_role;
static int default_server_announce;

#define NUMPARAMETERS (sizeof(parm_table) / sizeof(struct parm_struct))

/* prototypes for the special type handlers */
static BOOL handle_include(const char *pszParmValue, char **ptr);
static BOOL handle_copy(const char *pszParmValue, char **ptr);
static BOOL handle_vfs_object(const char *pszParmValue, char **ptr);
static BOOL handle_source_env(const char *pszParmValue, char **ptr);
static BOOL handle_netbios_name(const char *pszParmValue, char **ptr);
static BOOL handle_winbind_uid(const char *pszParmValue, char **ptr);
static BOOL handle_winbind_gid(const char *pszParmValue, char **ptr);
static BOOL handle_non_unix_account_range(const char *pszParmValue, char **ptr);
static BOOL handle_debug_list( const char *pszParmValue, char **ptr );
static BOOL handle_workgroup( const char *pszParmValue, char **ptr );
static BOOL handle_netbios_aliases( const char *pszParmValue, char **ptr );
static BOOL handle_netbios_scope( const char *pszParmValue, char **ptr );

static BOOL handle_ldap_machine_suffix ( const char *pszParmValue, char **ptr );
static BOOL handle_ldap_user_suffix ( const char *pszParmValue, char **ptr );
static BOOL handle_ldap_suffix ( const char *pszParmValue, char **ptr );

static BOOL handle_acl_compatibility(const char *pszParmValue, char **ptr);

static void set_server_role(void);
static void set_default_server_announce_type(void);

static const struct enum_list enum_protocol[] = {
	{PROTOCOL_NT1, "NT1"},
	{PROTOCOL_LANMAN2, "LANMAN2"},
	{PROTOCOL_LANMAN1, "LANMAN1"},
	{PROTOCOL_CORE, "CORE"},
	{PROTOCOL_COREPLUS, "COREPLUS"},
	{PROTOCOL_COREPLUS, "CORE+"},
	{-1, NULL}
};

static const struct enum_list enum_security[] = {
	{SEC_SHARE, "SHARE"},
	{SEC_USER, "USER"},
	{SEC_SERVER, "SERVER"},
	{SEC_DOMAIN, "DOMAIN"},
#ifdef HAVE_ADS
	{SEC_ADS, "ADS"},
#endif
	{-1, NULL}
};

static const struct enum_list enum_printing[] = {
	{PRINT_SYSV, "sysv"},
	{PRINT_AIX, "aix"},
	{PRINT_HPUX, "hpux"},
	{PRINT_BSD, "bsd"},
	{PRINT_QNX, "qnx"},
	{PRINT_PLP, "plp"},
	{PRINT_LPRNG, "lprng"},
	{PRINT_SOFTQ, "softq"},
	{PRINT_CUPS, "cups"},
	{PRINT_LPRNT, "nt"},
	{PRINT_LPROS2, "os2"},
#ifdef DEVELOPER
	{PRINT_TEST, "test"},
	{PRINT_VLP, "vlp"},
#endif /* DEVELOPER */
	{-1, NULL}
};

static const struct enum_list enum_ldap_ssl[] = {
#ifdef WITH_LDAP_SAMCONFIG
	{LDAP_SSL_ON, "Yes"},
	{LDAP_SSL_ON, "yes"},
	{LDAP_SSL_ON, "on"},
	{LDAP_SSL_ON, "On"},
#endif
	{LDAP_SSL_OFF, "no"},
	{LDAP_SSL_OFF, "No"},
	{LDAP_SSL_OFF, "off"},
	{LDAP_SSL_OFF, "Off"},
	{LDAP_SSL_START_TLS, "start tls"},
	{LDAP_SSL_START_TLS, "Start_tls"},
	{-1, NULL}
};

static const struct enum_list enum_ldap_passwd_sync[] = {
	{LDAP_PASSWD_SYNC_ON, "Yes"},
	{LDAP_PASSWD_SYNC_ON, "yes"},
	{LDAP_PASSWD_SYNC_ON, "on"},
	{LDAP_PASSWD_SYNC_ON, "On"},
	{LDAP_PASSWD_SYNC_OFF, "no"},
	{LDAP_PASSWD_SYNC_OFF, "No"},
	{LDAP_PASSWD_SYNC_OFF, "off"},
	{LDAP_PASSWD_SYNC_OFF, "Off"},
#ifdef LDAP_EXOP_X_MODIFY_PASSWD	
	{LDAP_PASSWD_SYNC_ONLY, "Only"},
	{LDAP_PASSWD_SYNC_ONLY, "only"},
#endif /* LDAP_EXOP_X_MODIFY_PASSWD */	
	{-1, NULL}
};

/* Types of machine we can announce as. */
#define ANNOUNCE_AS_NT_SERVER 1
#define ANNOUNCE_AS_WIN95 2
#define ANNOUNCE_AS_WFW 3
#define ANNOUNCE_AS_NT_WORKSTATION 4

static const struct enum_list enum_announce_as[] = {
	{ANNOUNCE_AS_NT_SERVER, "NT"},
	{ANNOUNCE_AS_NT_SERVER, "NT Server"},
	{ANNOUNCE_AS_NT_WORKSTATION, "NT Workstation"},
	{ANNOUNCE_AS_WIN95, "win95"},
	{ANNOUNCE_AS_WFW, "WfW"},
	{-1, NULL}
};

static const struct enum_list enum_case[] = {
	{CASE_LOWER, "lower"},
	{CASE_UPPER, "upper"},
	{-1, NULL}
};

static const struct enum_list enum_bool_auto[] = {
	{False, "No"},
	{False, "False"},
	{False, "0"},
	{True, "Yes"},
	{True, "True"},
	{True, "1"},
	{Auto, "Auto"},
	{-1, NULL}
};

/* Client-side offline caching policy types */
#define CSC_POLICY_MANUAL 0
#define CSC_POLICY_DOCUMENTS 1
#define CSC_POLICY_PROGRAMS 2
#define CSC_POLICY_DISABLE 3

static const struct enum_list enum_csc_policy[] = {
	{CSC_POLICY_MANUAL, "manual"},
	{CSC_POLICY_DOCUMENTS, "documents"},
	{CSC_POLICY_PROGRAMS, "programs"},
	{CSC_POLICY_DISABLE, "disable"},
	{-1, NULL}
};

/* 
   Do you want session setups at user level security with a invalid
   password to be rejected or allowed in as guest? WinNT rejects them
   but it can be a pain as it means "net view" needs to use a password

   You have 3 choices in the setting of map_to_guest:

   "Never" means session setups with an invalid password
   are rejected. This is the default.

   "Bad User" means session setups with an invalid password
   are rejected, unless the username does not exist, in which case it
   is treated as a guest login

   "Bad Password" means session setups with an invalid password
   are treated as a guest login

   Note that map_to_guest only has an effect in user or server
   level security.
*/

static const struct enum_list enum_map_to_guest[] = {
	{NEVER_MAP_TO_GUEST, "Never"},
	{MAP_TO_GUEST_ON_BAD_USER, "Bad User"},
	{MAP_TO_GUEST_ON_BAD_PASSWORD, "Bad Password"},
	{-1, NULL}
};

/* Note: We do not initialise the defaults union - it is not allowed in ANSI C
 *
 * Note: We have a flag called FLAG_DEVELOPER but is not used at this time, it
 * is implied in current control logic. This may change at some later time. A
 * flag value of 0 means - show as development option only.
 *
 * The FLAG_HIDE is explicit. Paramters set this way do NOT appear in any edit
 * screen in SWAT. This is used to exclude parameters as well as to squash all
 * parameters that have been duplicated by pseudonyms.
 */
static struct parm_struct parm_table[] = {
	{"Base Options", P_SEP, P_SEPARATOR},

	{"dos charset", P_STRING, P_GLOBAL, &Globals.dos_charset, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"unix charset", P_STRING, P_GLOBAL, &Globals.unix_charset, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"display charset", P_STRING, P_GLOBAL, &Globals.display_charset, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"comment", P_STRING, P_LOCAL, &sDefault.comment, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT | FLAG_DEVELOPER},
	{"path", P_STRING, P_LOCAL, &sDefault.szPath, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT | FLAG_DEVELOPER},
	{"directory", P_STRING, P_LOCAL, &sDefault.szPath, NULL, NULL, FLAG_HIDE},
	{"workgroup", P_USTRING, P_GLOBAL, &Globals.szWorkgroup, handle_workgroup, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"realm", P_USTRING, P_GLOBAL, &Globals.szRealm, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"ADS server", P_STRING, P_GLOBAL, &Globals.szADSserver, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"netbios name", P_USTRING, P_GLOBAL, &Globals.szNetbiosName, handle_netbios_name, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"netbios aliases", P_LIST, P_GLOBAL, &Globals.szNetbiosAliases, handle_netbios_aliases, NULL, FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"netbios scope", P_USTRING, P_GLOBAL, &Globals.szNetbiosScope, handle_netbios_scope, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"server string", P_STRING, P_GLOBAL, &Globals.szServerString, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED  | FLAG_DEVELOPER},
	{"interfaces", P_LIST, P_GLOBAL, &Globals.szInterfaces, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"bind interfaces only", P_BOOL, P_GLOBAL, &Globals.bBindInterfacesOnly, NULL, NULL, FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},

	{"Security Options", P_SEP, P_SEPARATOR},
	
	{"security", P_ENUM, P_GLOBAL, &Globals.security, NULL, enum_security, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"auth methods", P_LIST, P_GLOBAL, &Globals.AuthMethods, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"encrypt passwords", P_BOOL, P_GLOBAL, &Globals.bEncryptPasswords, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"update encrypted", P_BOOL, P_GLOBAL, &Globals.bUpdateEncrypt, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_DEVELOPER},
	{"allow trusted domains", P_BOOL, P_GLOBAL, &Globals.bAllowTrustedDomains, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"hosts equiv", P_STRING, P_GLOBAL, &Globals.szHostsEquiv, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"min passwd length", P_INTEGER, P_GLOBAL, &Globals.min_passwd_length, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"min password length", P_INTEGER, P_GLOBAL, &Globals.min_passwd_length, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"map to guest", P_ENUM, P_GLOBAL, &Globals.map_to_guest, NULL, enum_map_to_guest, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"null passwords", P_BOOL, P_GLOBAL, &Globals.bNullPasswords, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"obey pam restrictions", P_BOOL, P_GLOBAL, &Globals.bObeyPamRestrictions, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"password server", P_STRING, P_GLOBAL, &Globals.szPasswordServer, NULL, NULL, FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"smb passwd file", P_STRING, P_GLOBAL, &Globals.szSMBPasswdFile, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"private dir", P_STRING, P_GLOBAL, &Globals.szPrivateDir, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"passdb backend", P_LIST, P_GLOBAL, &Globals.szPassdbBackend, NULL, NULL, FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"non unix account range", P_STRING, P_GLOBAL, &Globals.szNonUnixAccountRange, handle_non_unix_account_range, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"algorithmic rid base", P_INTEGER, P_GLOBAL, &Globals.bAlgorithmicRidBase, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"root directory", P_STRING, P_GLOBAL, &Globals.szRootdir, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"root dir", P_STRING, P_GLOBAL, &Globals.szRootdir, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"root", P_STRING, P_GLOBAL, &Globals.szRootdir, NULL, NULL, FLAG_HIDE | FLAG_DEVELOPER},
	{"guest account", P_STRING, P_GLOBAL, &Globals.szGuestaccount, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_DEVELOPER},
	
	{"pam password change", P_BOOL, P_GLOBAL, &Globals.bPamPasswordChange, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"passwd program", P_STRING, P_GLOBAL, &Globals.szPasswdProgram, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"passwd chat", P_STRING, P_GLOBAL, &Globals.szPasswdChat, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"passwd chat debug", P_BOOL, P_GLOBAL, &Globals.bPasswdChatDebug, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"username map", P_STRING, P_GLOBAL, &Globals.szUsernameMap, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER | FLAG_DEVELOPER},
	{"password level", P_INTEGER, P_GLOBAL, &Globals.pwordlevel, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"username level", P_INTEGER, P_GLOBAL, &Globals.unamelevel, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"unix password sync", P_BOOL, P_GLOBAL, &Globals.bUnixPasswdSync, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"restrict anonymous", P_INTEGER, P_GLOBAL, &Globals.restrict_anonymous, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"lanman auth", P_BOOL, P_GLOBAL, &Globals.bLanmanAuth, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"ntlm auth", P_BOOL, P_GLOBAL, &Globals.bNTLMAuth, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"client NTLMv2 auth", P_BOOL, P_GLOBAL, &Globals.bClientNTLMv2Auth, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"client lanman auth", P_BOOL, P_GLOBAL, &Globals.bClientLanManAuth, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	
	{"username", P_STRING, P_LOCAL, &sDefault.szUsername, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"user", P_STRING, P_LOCAL, &sDefault.szUsername, NULL, NULL, FLAG_HIDE},
	{"users", P_STRING, P_LOCAL, &sDefault.szUsername, NULL, NULL, FLAG_HIDE},
	
	{"invalid users", P_LIST, P_LOCAL, &sDefault.szInvalidUsers, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"valid users", P_LIST, P_LOCAL, &sDefault.szValidUsers, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"admin users", P_LIST, P_LOCAL, &sDefault.szAdminUsers, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"read list", P_LIST, P_LOCAL, &sDefault.readlist, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"write list", P_LIST, P_LOCAL, &sDefault.writelist, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"printer admin", P_LIST, P_LOCAL, &sDefault.printer_admin, NULL, NULL, FLAG_GLOBAL | FLAG_PRINT},
	{"force user", P_STRING, P_LOCAL, &sDefault.force_user, NULL, NULL, FLAG_SHARE},
	{"force group", P_STRING, P_LOCAL, &sDefault.force_group, NULL, NULL, FLAG_SHARE},
	{"group", P_STRING, P_LOCAL, &sDefault.force_group, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	
	{"read only", P_BOOL, P_LOCAL, &sDefault.bRead_only, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE},
	{"write ok", P_BOOLREV, P_LOCAL, &sDefault.bRead_only, NULL, NULL, FLAG_HIDE},
	{"writeable", P_BOOLREV, P_LOCAL, &sDefault.bRead_only, NULL, NULL, FLAG_HIDE},
	{"writable", P_BOOLREV, P_LOCAL, &sDefault.bRead_only, NULL, NULL, FLAG_HIDE},
	
	{"create mask", P_OCTAL, P_LOCAL, &sDefault.iCreate_mask, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"create mode", P_OCTAL, P_LOCAL, &sDefault.iCreate_mask, NULL, NULL, FLAG_GLOBAL},
	{"force create mode", P_OCTAL, P_LOCAL, &sDefault.iCreate_force_mode, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"security mask", P_OCTAL, P_LOCAL, &sDefault.iSecurity_mask, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"force security mode", P_OCTAL, P_LOCAL, &sDefault.iSecurity_force_mode, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"directory mask", P_OCTAL, P_LOCAL, &sDefault.iDir_mask, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"directory mode", P_OCTAL, P_LOCAL, &sDefault.iDir_mask, NULL, NULL, FLAG_GLOBAL},
	{"force directory mode", P_OCTAL, P_LOCAL, &sDefault.iDir_force_mode, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"directory security mask", P_OCTAL, P_LOCAL, &sDefault.iDir_Security_mask, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"force directory security mode", P_OCTAL, P_LOCAL, &sDefault.iDir_Security_force_mode, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE},
	{"inherit permissions", P_BOOL, P_LOCAL, &sDefault.bInheritPerms, NULL, NULL, FLAG_SHARE},
	{"inherit acls", P_BOOL, P_LOCAL, &sDefault.bInheritACLS, NULL, NULL, FLAG_SHARE},
	{"guest only", P_BOOL, P_LOCAL, &sDefault.bGuest_only, NULL, NULL, FLAG_SHARE},
	{"only guest", P_BOOL, P_LOCAL, &sDefault.bGuest_only, NULL, NULL, FLAG_HIDE},

	{"guest ok", P_BOOL, P_LOCAL, &sDefault.bGuest_ok, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT | FLAG_DEVELOPER},
	{"public", P_BOOL, P_LOCAL, &sDefault.bGuest_ok, NULL, NULL, FLAG_HIDE},
	
	{"only user", P_BOOL, P_LOCAL, &sDefault.bOnlyUser, NULL, NULL, FLAG_SHARE},
	{"hosts allow", P_LIST, P_LOCAL, &sDefault.szHostsallow, NULL, NULL, FLAG_GLOBAL | FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT | FLAG_DEVELOPER},
	{"allow hosts", P_LIST, P_LOCAL, &sDefault.szHostsallow, NULL, NULL, FLAG_HIDE},
	{"hosts deny", P_LIST, P_LOCAL, &sDefault.szHostsdeny, NULL, NULL, FLAG_GLOBAL | FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT | FLAG_DEVELOPER},
	{"deny hosts", P_LIST, P_LOCAL, &sDefault.szHostsdeny, NULL, NULL, FLAG_HIDE},

	{"Logging Options", P_SEP, P_SEPARATOR},

	{"log level", P_STRING, P_GLOBAL, &Globals.szLogLevel, handle_debug_list, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"debuglevel", P_STRING, P_GLOBAL, &Globals.szLogLevel, handle_debug_list, NULL, FLAG_HIDE},
	{"syslog", P_INTEGER, P_GLOBAL, &Globals.syslog, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"syslog only", P_BOOL, P_GLOBAL, &Globals.bSyslogOnly, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"log file", P_STRING, P_GLOBAL, &Globals.szLogFile, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	
	{"max log size", P_INTEGER, P_GLOBAL, &Globals.max_log_size, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"timestamp logs", P_BOOL, P_GLOBAL, &Globals.bTimestampLogs, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"debug timestamp", P_BOOL, P_GLOBAL, &Globals.bTimestampLogs, NULL, NULL, FLAG_DEVELOPER},
	{"debug hires timestamp", P_BOOL, P_GLOBAL, &Globals.bDebugHiresTimestamp, NULL, NULL, FLAG_DEVELOPER},
	{"debug pid", P_BOOL, P_GLOBAL, &Globals.bDebugPid, NULL, NULL, FLAG_DEVELOPER},
	{"debug uid", P_BOOL, P_GLOBAL, &Globals.bDebugUid, NULL, NULL, FLAG_DEVELOPER},
	
	{"Protocol Options", P_SEP, P_SEPARATOR},
	
	{"smb ports", P_STRING, P_GLOBAL, &Globals.smb_ports, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"protocol", P_ENUM, P_GLOBAL, &Globals.maxprotocol, NULL, enum_protocol, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"large readwrite", P_BOOL, P_GLOBAL, &Globals.bLargeReadwrite, NULL, NULL, FLAG_DEVELOPER},
	{"max protocol", P_ENUM, P_GLOBAL, &Globals.maxprotocol, NULL, enum_protocol, FLAG_DEVELOPER},
	{"min protocol", P_ENUM, P_GLOBAL, &Globals.minprotocol, NULL, enum_protocol, FLAG_DEVELOPER},
	{"unicode", P_BOOL, P_GLOBAL, &Globals.bUnicode, NULL, NULL, FLAG_DEVELOPER},
	{"read bmpx", P_BOOL, P_GLOBAL, &Globals.bReadbmpx, NULL, NULL, FLAG_DEVELOPER},
	{"read raw", P_BOOL, P_GLOBAL, &Globals.bReadRaw, NULL, NULL, FLAG_DEVELOPER},
	{"write raw", P_BOOL, P_GLOBAL, &Globals.bWriteRaw, NULL, NULL, FLAG_DEVELOPER},
	{"disable netbios", P_BOOL, P_GLOBAL, &Globals.bDisableNetbios, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	
	{"acl compatibility", P_STRING, P_GLOBAL, &Globals.szAclCompat, handle_acl_compatibility, NULL, FLAG_SHARE | FLAG_GLOBAL | FLAG_ADVANCED},
	{"nt acl support", P_BOOL,  P_LOCAL, &sDefault.bNTAclSupport, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE | FLAG_ADVANCED | FLAG_WIZARD},
	{"nt pipe support", P_BOOL, P_GLOBAL, &Globals.bNTPipeSupport, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"nt status support", P_BOOL, P_GLOBAL, &Globals.bNTStatusSupport, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"profile acls", P_BOOL,  P_LOCAL, &sDefault.bProfileAcls, NULL, NULL, FLAG_GLOBAL | FLAG_SHARE  | FLAG_ADVANCED | FLAG_WIZARD},
	
	{"announce version", P_STRING, P_GLOBAL, &Globals.szAnnounceVersion, NULL, NULL, FLAG_DEVELOPER},
	{"announce as", P_ENUM, P_GLOBAL, &Globals.announce_as, NULL, enum_announce_as, FLAG_DEVELOPER},
	{"max mux", P_INTEGER, P_GLOBAL, &Globals.max_mux, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"max xmit", P_INTEGER, P_GLOBAL, &Globals.max_xmit, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	
	{"name resolve order", P_STRING, P_GLOBAL, &Globals.szNameResolveOrder, NULL, NULL, FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"max ttl", P_INTEGER, P_GLOBAL, &Globals.max_ttl, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER}, 
	{"max wins ttl", P_INTEGER, P_GLOBAL, &Globals.max_wins_ttl, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"min wins ttl", P_INTEGER, P_GLOBAL, &Globals.min_wins_ttl, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"time server", P_BOOL, P_GLOBAL, &Globals.bTimeServer, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"unix extensions", P_BOOL, P_GLOBAL, &Globals.bUnixExtensions, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"use spnego", P_BOOL, P_GLOBAL, &Globals.bUseSpnego, NULL, NULL, FLAG_DEVELOPER},
	{"client signing", P_BOOL, P_GLOBAL, &Globals.client_signing, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"client use spnego", P_BOOL, P_GLOBAL, &Globals.bClientUseSpnego, NULL, NULL, FLAG_DEVELOPER},

	{"Tuning Options", P_SEP, P_SEPARATOR},
	
	{"block size", P_INTEGER, P_LOCAL, &sDefault.iBlock_size, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"change notify timeout", P_INTEGER, P_GLOBAL, &Globals.change_notify_timeout, NULL, NULL, FLAG_DEVELOPER},
	{"deadtime", P_INTEGER, P_GLOBAL, &Globals.deadtime, NULL, NULL, FLAG_DEVELOPER},
	{"getwd cache", P_BOOL, P_GLOBAL, &use_getwd_cache, NULL, NULL, FLAG_DEVELOPER},
	{"keepalive", P_INTEGER, P_GLOBAL, &keepalive, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"kernel change notify", P_BOOL, P_GLOBAL, &Globals.bKernelChangeNotify, NULL, NULL, FLAG_DEVELOPER},
	
	{"lpq cache time", P_INTEGER, P_GLOBAL, &Globals.lpqcachetime, NULL, NULL, FLAG_DEVELOPER},
	{"max smbd processes", P_INTEGER, P_GLOBAL, &Globals.iMaxSmbdProcesses, NULL, NULL, FLAG_DEVELOPER},
	{"max connections", P_INTEGER, P_LOCAL, &sDefault.iMaxConnections, NULL, NULL, FLAG_SHARE},
	{"paranoid server security", P_BOOL, P_GLOBAL, &Globals.paranoid_server_security, NULL, NULL, FLAG_DEVELOPER},
	{"max disk size", P_INTEGER, P_GLOBAL, &Globals.maxdisksize, NULL, NULL, FLAG_DEVELOPER},
	{"max open files", P_INTEGER, P_GLOBAL, &Globals.max_open_files, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"min print space", P_INTEGER, P_LOCAL, &sDefault.iMinPrintSpace, NULL, NULL, FLAG_PRINT},
	{"read size", P_INTEGER, P_GLOBAL, &Globals.ReadSize, NULL, NULL, FLAG_DEVELOPER},
	
	{"socket options", P_GSTRING, P_GLOBAL, user_socket_options, NULL, NULL, FLAG_DEVELOPER},
	{"stat cache size", P_INTEGER, P_GLOBAL, &Globals.stat_cache_size, NULL, NULL, FLAG_DEVELOPER},
	{"strict allocate", P_BOOL, P_LOCAL, &sDefault.bStrictAllocate, NULL, NULL, FLAG_SHARE},
	{"strict sync", P_BOOL, P_LOCAL, &sDefault.bStrictSync, NULL, NULL, FLAG_SHARE},
	{"sync always", P_BOOL, P_LOCAL, &sDefault.bSyncAlways, NULL, NULL, FLAG_SHARE},
	{"use mmap", P_BOOL, P_GLOBAL, &Globals.bUseMmap, NULL, NULL, FLAG_DEVELOPER},
	{"use sendfile", P_BOOL, P_LOCAL, &sDefault.bUseSendfile, NULL, NULL, FLAG_SHARE},
	{"hostname lookups", P_BOOL, P_GLOBAL, &Globals.bHostnameLookups, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"write cache size", P_INTEGER, P_LOCAL, &sDefault.iWriteCacheSize, NULL, NULL, FLAG_SHARE},

	{"name cache timeout", P_INTEGER, P_GLOBAL, &Globals.name_cache_timeout, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},

	{"Printing Options", P_SEP, P_SEPARATOR},
	
	{"total print jobs", P_INTEGER, P_GLOBAL, &Globals.iTotalPrintJobs, NULL, NULL, FLAG_PRINT},
	{"max reported print jobs", P_INTEGER, P_LOCAL, &sDefault.iMaxReportedPrintJobs, NULL, NULL, FLAG_PRINT},
	{"max print jobs", P_INTEGER, P_LOCAL, &sDefault.iMaxPrintJobs, NULL, NULL, FLAG_PRINT},
	{"load printers", P_BOOL, P_GLOBAL, &Globals.bLoadPrinters, NULL, NULL, FLAG_PRINT},
	{"printcap name", P_STRING, P_GLOBAL, &Globals.szPrintcapname, NULL, NULL, FLAG_PRINT | FLAG_DEVELOPER},
	{"printcap", P_STRING, P_GLOBAL, &Globals.szPrintcapname, NULL, NULL, FLAG_HIDE},
	{"printable", P_BOOL, P_LOCAL, &sDefault.bPrint_ok, NULL, NULL, FLAG_PRINT},
	{"print ok", P_BOOL, P_LOCAL, &sDefault.bPrint_ok, NULL, NULL, FLAG_HIDE},
	{"printing", P_ENUM, P_LOCAL, &sDefault.iPrinting, NULL, enum_printing, FLAG_PRINT | FLAG_GLOBAL},
	{"print command", P_STRING, P_LOCAL, &sDefault.szPrintcommand, NULL, NULL, FLAG_PRINT | FLAG_GLOBAL},
	{"disable spoolss", P_BOOL, P_GLOBAL, &Globals.bDisableSpoolss, NULL, NULL, FLAG_PRINT | FLAG_GLOBAL},
	{"lpq command", P_STRING, P_LOCAL, &sDefault.szLpqcommand, NULL, NULL, FLAG_PRINT | FLAG_GLOBAL},
	{"lprm command", P_STRING, P_LOCAL, &sDefault.szLprmcommand, NULL, NULL, FLAG_PRINT | FLAG_GLOBAL},
	{"lppause command", P_STRING, P_LOCAL, &sDefault.szLppausecommand, NULL, NULL, FLAG_PRINT | FLAG_GLOBAL},
	{"lpresume command", P_STRING, P_LOCAL, &sDefault.szLpresumecommand, NULL, NULL, FLAG_PRINT | FLAG_GLOBAL},
	{"queuepause command", P_STRING, P_LOCAL, &sDefault.szQueuepausecommand, NULL, NULL, FLAG_PRINT | FLAG_GLOBAL},
	{"queueresume command", P_STRING, P_LOCAL, &sDefault.szQueueresumecommand, NULL, NULL, FLAG_PRINT | FLAG_GLOBAL},

	{"enumports command", P_STRING, P_GLOBAL, &Globals.szEnumPortsCommand, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"addprinter command", P_STRING, P_GLOBAL, &Globals.szAddPrinterCommand, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"deleteprinter command", P_STRING, P_GLOBAL, &Globals.szDeletePrinterCommand, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"show add printer wizard", P_BOOL, P_GLOBAL, &Globals.bMsAddPrinterWizard, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"os2 driver map", P_STRING, P_GLOBAL, &Globals.szOs2DriverMap, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	
	{"printer name", P_STRING, P_LOCAL, &sDefault.szPrintername, NULL, NULL, FLAG_PRINT},
	{"printer", P_STRING, P_LOCAL, &sDefault.szPrintername, NULL, NULL, FLAG_HIDE},
	{"use client driver", P_BOOL, P_LOCAL, &sDefault.bUseClientDriver, NULL, NULL, FLAG_PRINT},
	{"default devmode", P_BOOL, P_LOCAL, &sDefault.bDefaultDevmode, NULL, NULL, FLAG_PRINT},

	{"Filename Handling", P_SEP, P_SEPARATOR},
	{"strip dot", P_BOOL, P_GLOBAL, &Globals.bStripDot, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"mangling method", P_STRING, P_GLOBAL, &Globals.szManglingMethod, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"mangle prefix", P_INTEGER, P_GLOBAL, &Globals.mangle_prefix, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	
	{"mangled stack", P_INTEGER, P_GLOBAL, &Globals.mangled_stack, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"default case", P_ENUM, P_LOCAL, &sDefault.iDefaultCase, NULL, enum_case, FLAG_SHARE},
	{"case sensitive", P_BOOL, P_LOCAL, &sDefault.bCaseSensitive, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"casesignames", P_BOOL, P_LOCAL, &sDefault.bCaseSensitive, NULL, NULL, FLAG_HIDE},
	{"preserve case", P_BOOL, P_LOCAL, &sDefault.bCasePreserve, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"short preserve case", P_BOOL, P_LOCAL, &sDefault.bShortCasePreserve, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"mangle case", P_BOOL, P_LOCAL, &sDefault.bCaseMangle, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"mangling char", P_CHAR, P_LOCAL, &sDefault.magic_char, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"hide dot files", P_BOOL, P_LOCAL, &sDefault.bHideDotFiles, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"hide special files", P_BOOL, P_LOCAL, &sDefault.bHideSpecialFiles, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"hide unreadable", P_BOOL, P_LOCAL, &sDefault.bHideUnReadable, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"hide unwriteable files", P_BOOL, P_LOCAL, &sDefault.bHideUnWriteableFiles, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"delete veto files", P_BOOL, P_LOCAL, &sDefault.bDeleteVetoFiles, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"veto files", P_STRING, P_LOCAL, &sDefault.szVetoFiles, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL },
	{"hide files", P_STRING, P_LOCAL, &sDefault.szHideFiles, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL },
	{"veto oplock files", P_STRING, P_LOCAL, &sDefault.szVetoOplockFiles, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL },
	{"map system", P_BOOL, P_LOCAL, &sDefault.bMap_system, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"map hidden", P_BOOL, P_LOCAL, &sDefault.bMap_hidden, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"map archive", P_BOOL, P_LOCAL, &sDefault.bMap_archive, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"mangled names", P_BOOL, P_LOCAL, &sDefault.bMangledNames, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"mangled map", P_STRING, P_LOCAL, &sDefault.szMangledMap, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"stat cache", P_BOOL, P_GLOBAL, &Globals.bStatCache, NULL, NULL, FLAG_DEVELOPER},

	{"Domain Options", P_SEP, P_SEPARATOR},
	
	{"machine password timeout", P_INTEGER, P_GLOBAL, &Globals.machine_password_timeout, NULL, NULL, FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},

	{"Logon Options", P_SEP, P_SEPARATOR},

	{"add user script", P_STRING, P_GLOBAL, &Globals.szAddUserScript, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"delete user script", P_STRING, P_GLOBAL, &Globals.szDelUserScript, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"add group script", P_STRING, P_GLOBAL, &Globals.szAddGroupScript, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"delete group script", P_STRING, P_GLOBAL, &Globals.szDelGroupScript, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"add user to group script", P_STRING, P_GLOBAL, &Globals.szAddUserToGroupScript, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"delete user from group script", P_STRING, P_GLOBAL, &Globals.szDelUserFromGroupScript, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"set primary group script", P_STRING, P_GLOBAL, &Globals.szSetPrimaryGroupScript, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"add machine script", P_STRING, P_GLOBAL, &Globals.szAddMachineScript, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"shutdown script", P_STRING, P_GLOBAL, &Globals.szShutdownScript, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"abort shutdown script", P_STRING, P_GLOBAL, &Globals.szAbortShutdownScript, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},

	{"logon script", P_STRING, P_GLOBAL, &Globals.szLogonScript, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"logon path", P_STRING, P_GLOBAL, &Globals.szLogonPath, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"logon drive", P_STRING, P_GLOBAL, &Globals.szLogonDrive, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"logon home", P_STRING, P_GLOBAL, &Globals.szLogonHome, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"domain logons", P_BOOL, P_GLOBAL, &Globals.bDomainLogons, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},

	{"Browse Options", P_SEP, P_SEPARATOR},
	
	{"os level", P_INTEGER, P_GLOBAL, &Globals.os_level, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_DEVELOPER},
	{"lm announce", P_ENUM, P_GLOBAL, &Globals.lm_announce, NULL, enum_bool_auto, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"lm interval", P_INTEGER, P_GLOBAL, &Globals.lm_interval, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"preferred master", P_ENUM, P_GLOBAL, &Globals.bPreferredMaster, NULL, enum_bool_auto, FLAG_BASIC | FLAG_ADVANCED | FLAG_DEVELOPER},
	{"prefered master", P_ENUM, P_GLOBAL, &Globals.bPreferredMaster, NULL, enum_bool_auto, FLAG_HIDE},
	{"local master", P_BOOL, P_GLOBAL, &Globals.bLocalMaster, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_DEVELOPER},
	{"domain master", P_ENUM, P_GLOBAL, &Globals.bDomainMaster, NULL, enum_bool_auto, FLAG_BASIC | FLAG_ADVANCED | FLAG_DEVELOPER},
	{"browse list", P_BOOL, P_GLOBAL, &Globals.bBrowseList, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"browseable", P_BOOL, P_LOCAL, &sDefault.bBrowseable, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT | FLAG_DEVELOPER},
	{"browsable", P_BOOL, P_LOCAL, &sDefault.bBrowseable, NULL, NULL, FLAG_HIDE},
	{"enhanced browsing", P_BOOL, P_GLOBAL, &Globals.enhanced_browsing, NULL, NULL, FLAG_DEVELOPER | FLAG_ADVANCED},

	{"WINS Options", P_SEP, P_SEPARATOR},
	{"dns proxy", P_BOOL, P_GLOBAL, &Globals.bDNSproxy, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"wins proxy", P_BOOL, P_GLOBAL, &Globals.bWINSproxy, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	
	{"wins server", P_LIST, P_GLOBAL, &Globals.szWINSservers, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"wins support", P_BOOL, P_GLOBAL, &Globals.bWINSsupport, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},
	{"wins hook", P_STRING, P_GLOBAL, &Globals.szWINSHook, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"wins partners", P_STRING, P_GLOBAL, &Globals.szWINSPartners, NULL, NULL, FLAG_ADVANCED | FLAG_WIZARD | FLAG_DEVELOPER},

	{"Locking Options", P_SEP, P_SEPARATOR},
	
	{"blocking locks", P_BOOL, P_LOCAL, &sDefault.bBlockingLocks, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"csc policy", P_ENUM, P_LOCAL, &sDefault.iCSCPolicy, NULL, enum_csc_policy, FLAG_SHARE | FLAG_GLOBAL},
	{"fake oplocks", P_BOOL, P_LOCAL, &sDefault.bFakeOplocks, NULL, NULL, FLAG_SHARE},
	{"kernel oplocks", P_BOOL, P_GLOBAL, &Globals.bKernelOplocks, NULL, NULL, FLAG_GLOBAL},
	{"locking", P_BOOL, P_LOCAL, &sDefault.bLocking, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"lock spin count", P_INTEGER, P_GLOBAL, &Globals.iLockSpinCount, NULL, NULL, FLAG_GLOBAL},
	{"lock spin time", P_INTEGER, P_GLOBAL, &Globals.iLockSpinTime, NULL, NULL, FLAG_GLOBAL},
	
	{"oplocks", P_BOOL, P_LOCAL, &sDefault.bOpLocks, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"level2 oplocks", P_BOOL, P_LOCAL, &sDefault.bLevel2OpLocks, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"oplock break wait time", P_INTEGER, P_GLOBAL, &Globals.oplock_break_wait_time, NULL, NULL, FLAG_GLOBAL},
	{"oplock contention limit", P_INTEGER, P_LOCAL, &sDefault.iOplockContentionLimit, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"posix locking", P_BOOL, P_LOCAL, &sDefault.bPosixLocking, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"strict locking", P_BOOL, P_LOCAL, &sDefault.bStrictLocking, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"share modes", P_BOOL, P_LOCAL,  &sDefault.bShareModes, NULL, NULL, FLAG_SHARE|FLAG_GLOBAL},

	{"Ldap Options", P_SEP, P_SEPARATOR},
	
#ifdef WITH_LDAP_SAMCONFIG
	{"ldap server", P_STRING, P_GLOBAL, &Globals.szLdapServer, NULL, NULL, 0},
	{"ldap port", P_INTEGER, P_GLOBAL, &Globals.ldap_port, NULL, NULL, 0}, 
#endif
	{"ldap suffix", P_STRING, P_GLOBAL, &Globals.szLdapSuffix, handle_ldap_suffix, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"ldap machine suffix", P_STRING, P_GLOBAL, &Globals.szLdapMachineSuffix, handle_ldap_machine_suffix, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"ldap user suffix", P_STRING, P_GLOBAL, &Globals.szLdapUserSuffix, handle_ldap_user_suffix, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"ldap filter", P_STRING, P_GLOBAL, &Globals.szLdapFilter, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"ldap admin dn", P_STRING, P_GLOBAL, &Globals.szLdapAdminDn, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"ldap ssl", P_ENUM, P_GLOBAL, &Globals.ldap_ssl, NULL, enum_ldap_ssl, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"ldap passwd sync", P_ENUM, P_GLOBAL, &Globals.ldap_passwd_sync, NULL, enum_ldap_passwd_sync, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"ldap trust ids", P_BOOL, P_GLOBAL, &Globals.ldap_trust_ids, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},

	{"Miscellaneous Options", P_SEP, P_SEPARATOR},
	{"add share command", P_STRING, P_GLOBAL, &Globals.szAddShareCommand, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"change share command", P_STRING, P_GLOBAL, &Globals.szChangeShareCommand, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"delete share command", P_STRING, P_GLOBAL, &Globals.szDeleteShareCommand, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	
	{"config file", P_STRING, P_GLOBAL, &Globals.szConfigFile, NULL, NULL, FLAG_HIDE},
	{"preload", P_STRING, P_GLOBAL, &Globals.szAutoServices, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"auto services", P_STRING, P_GLOBAL, &Globals.szAutoServices, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"lock dir", P_STRING, P_GLOBAL, &Globals.szLockDir, NULL, NULL, FLAG_HIDE}, 
	{"lock directory", P_STRING, P_GLOBAL, &Globals.szLockDir, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"pid directory", P_STRING, P_GLOBAL, &Globals.szPidDir, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER}, 
#ifdef WITH_UTMP
	{"utmp directory", P_STRING, P_GLOBAL, &Globals.szUtmpDir, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"wtmp directory", P_STRING, P_GLOBAL, &Globals.szWtmpDir, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"utmp",          P_BOOL, P_GLOBAL, &Globals.bUtmp, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
#endif
	
	{"default service", P_STRING, P_GLOBAL, &Globals.szDefaultService, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"default", P_STRING, P_GLOBAL, &Globals.szDefaultService, NULL, NULL,  FLAG_DEVELOPER},
	{"message command", P_STRING, P_GLOBAL, &Globals.szMsgCommand, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"dfree command", P_STRING, P_GLOBAL, &Globals.szDfree, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"remote announce", P_STRING, P_GLOBAL, &Globals.szRemoteAnnounce, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"remote browse sync", P_STRING, P_GLOBAL, &Globals.szRemoteBrowseSync, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"socket address", P_STRING, P_GLOBAL, &Globals.szSocketAddress, NULL, NULL, FLAG_DEVELOPER},
	{"homedir map", P_STRING, P_GLOBAL, &Globals.szNISHomeMapName, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"time offset", P_INTEGER, P_GLOBAL, &extra_time_offset, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"NIS homedir", P_BOOL, P_GLOBAL, &Globals.bNISHomeMap, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"-valid", P_BOOL, P_LOCAL, &sDefault.valid, NULL, NULL, FLAG_HIDE},
	
	{"copy", P_STRING, P_LOCAL, &sDefault.szCopy, handle_copy, NULL, FLAG_HIDE},
	{"include", P_STRING, P_LOCAL, &sDefault.szInclude, handle_include, NULL, FLAG_HIDE},
	{"exec", P_STRING, P_LOCAL, &sDefault.szPreExec, NULL, NULL, FLAG_SHARE | FLAG_PRINT},
	{"preexec", P_STRING, P_LOCAL, &sDefault.szPreExec, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	
	{"preexec close", P_BOOL, P_LOCAL, &sDefault.bPreexecClose, NULL, NULL, FLAG_SHARE},
	{"postexec", P_STRING, P_LOCAL, &sDefault.szPostExec, NULL, NULL, FLAG_SHARE | FLAG_PRINT},
	{"root preexec", P_STRING, P_LOCAL, &sDefault.szRootPreExec, NULL, NULL, FLAG_SHARE | FLAG_PRINT},
	{"root preexec close", P_BOOL, P_LOCAL, &sDefault.bRootpreexecClose, NULL, NULL, FLAG_SHARE},
	{"root postexec", P_STRING, P_LOCAL, &sDefault.szRootPostExec, NULL, NULL, FLAG_SHARE | FLAG_PRINT},
	{"available", P_BOOL, P_LOCAL, &sDefault.bAvailable, NULL, NULL, FLAG_BASIC | FLAG_ADVANCED | FLAG_SHARE | FLAG_PRINT},
	{"volume", P_STRING, P_LOCAL, &sDefault.volume, NULL, NULL, FLAG_SHARE },
	{"fstype", P_STRING, P_LOCAL, &sDefault.fstype, NULL, NULL, FLAG_SHARE},
	{"set directory", P_BOOLREV, P_LOCAL, &sDefault.bNo_set_dir, NULL, NULL, FLAG_SHARE},
	{"source environment", P_STRING, P_GLOBAL, &Globals.szSourceEnv, handle_source_env, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"wide links", P_BOOL, P_LOCAL, &sDefault.bWidelinks, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"follow symlinks", P_BOOL, P_LOCAL, &sDefault.bSymlinks, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"dont descend", P_STRING, P_LOCAL, &sDefault.szDontdescend, NULL, NULL, FLAG_SHARE},
	{"magic script", P_STRING, P_LOCAL, &sDefault.szMagicScript, NULL, NULL, FLAG_SHARE},
	{"magic output", P_STRING, P_LOCAL, &sDefault.szMagicOutput, NULL, NULL, FLAG_SHARE},
	{"delete readonly", P_BOOL, P_LOCAL, &sDefault.bDeleteReadonly, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"dos filemode", P_BOOL, P_LOCAL, &sDefault.bDosFilemode, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"dos filetimes", P_BOOL, P_LOCAL, &sDefault.bDosFiletimes, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"dos filetime resolution", P_BOOL, P_LOCAL, &sDefault.bDosFiletimeResolution, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},

	{"fake directory create times", P_BOOL, P_LOCAL, &sDefault.bFakeDirCreateTimes, NULL, NULL, FLAG_SHARE | FLAG_GLOBAL},
	{"panic action", P_STRING, P_GLOBAL, &Globals.szPanicAction, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"hide local users", P_BOOL, P_GLOBAL, &Globals.bHideLocalUsers, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},

	{"VFS module options", P_SEP, P_SEPARATOR},
	
	{"vfs object", P_STRING, P_LOCAL, &sDefault.szVfsObjectFile, handle_vfs_object, NULL, FLAG_SHARE},
	{"vfs options", P_STRING, P_LOCAL, &sDefault.szVfsOptions, NULL, NULL, FLAG_SHARE},
	{"vfs path", P_STRING, P_LOCAL, &sDefault.szVfsPath, NULL, NULL, FLAG_SHARE},

	
	{"msdfs root", P_BOOL, P_LOCAL, &sDefault.bMSDfsRoot, NULL, NULL, FLAG_SHARE},
	{"msdfs proxy", P_STRING, P_LOCAL, &sDefault.szMSDfsProxy, NULL, NULL, FLAG_SHARE},
	{"host msdfs", P_BOOL, P_GLOBAL, &Globals.bHostMSDfs, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},

	{"Winbind options", P_SEP, P_SEPARATOR},

	{"winbind uid", P_STRING, P_GLOBAL, &Globals.szWinbindUID, handle_winbind_uid, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"winbind gid", P_STRING, P_GLOBAL, &Globals.szWinbindGID, handle_winbind_gid, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"template homedir", P_STRING, P_GLOBAL, &Globals.szTemplateHomedir, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"template shell", P_STRING, P_GLOBAL, &Globals.szTemplateShell, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"winbind separator", P_STRING, P_GLOBAL, &Globals.szWinbindSeparator, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"winbind cache time", P_INTEGER, P_GLOBAL, &Globals.winbind_cache_time, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"winbind enum users", P_BOOL, P_GLOBAL, &Globals.bWinbindEnumUsers, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"winbind enum groups", P_BOOL, P_GLOBAL, &Globals.bWinbindEnumGroups, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},
	{"winbind use default domain", P_BOOL, P_GLOBAL, &Globals.bWinbindUseDefaultDomain, NULL, NULL, FLAG_ADVANCED | FLAG_DEVELOPER},

	{NULL, P_BOOL, P_NONE, NULL, NULL, NULL, 0}
};

/***************************************************************************
 Initialise the sDefault parameter structure for the printer values.
***************************************************************************/

static void init_printer_values(void)
{
	/* choose defaults depending on the type of printing */
	switch (sDefault.iPrinting) {
		case PRINT_BSD:
		case PRINT_AIX:
		case PRINT_LPRNT:
		case PRINT_LPROS2:
			string_set(&sDefault.szLpqcommand, "lpq -P'%p'");
			string_set(&sDefault.szLprmcommand, "lprm -P'%p' %j");
			string_set(&sDefault.szPrintcommand,
				   "lpr -r -P'%p' %s");
			break;

		case PRINT_LPRNG:
		case PRINT_PLP:
			string_set(&sDefault.szLpqcommand, "lpq -P'%p'");
			string_set(&sDefault.szLprmcommand, "lprm -P'%p' %j");
			string_set(&sDefault.szPrintcommand,
				   "lpr -r -P'%p' %s");
			string_set(&sDefault.szQueuepausecommand,
				   "lpc stop '%p'");
			string_set(&sDefault.szQueueresumecommand,
				   "lpc start '%p'");
			string_set(&sDefault.szLppausecommand,
				   "lpc hold '%p' %j");
			string_set(&sDefault.szLpresumecommand,
				   "lpc release '%p' %j");
			break;

		case PRINT_CUPS:
#ifdef HAVE_CUPS
			string_set(&sDefault.szLpqcommand, "");
			string_set(&sDefault.szLprmcommand, "");
			string_set(&sDefault.szPrintcommand, "");
			string_set(&sDefault.szLppausecommand, "");
			string_set(&sDefault.szLpresumecommand, "");
			string_set(&sDefault.szQueuepausecommand, "");
			string_set(&sDefault.szQueueresumecommand, "");

	                string_set(&Globals.szPrintcapname, "cups");
#else
			string_set(&sDefault.szLpqcommand,
			           "/usr/bin/lpstat -o '%p'");
			string_set(&sDefault.szLprmcommand,
			           "/usr/bin/cancel '%p-%j'");
			string_set(&sDefault.szPrintcommand,
			           "/usr/bin/lp -d '%p' %s; rm %s");
			string_set(&sDefault.szLppausecommand,
				   "lp -i '%p-%j' -H hold");
			string_set(&sDefault.szLpresumecommand,
				   "lp -i '%p-%j' -H resume");
			string_set(&sDefault.szQueuepausecommand,
			           "/usr/bin/disable '%p'");
			string_set(&sDefault.szQueueresumecommand,
			           "/usr/bin/enable '%p'");
			string_set(&Globals.szPrintcapname, "lpstat");
#endif /* HAVE_CUPS */
			break;

		case PRINT_SYSV:
		case PRINT_HPUX:
			string_set(&sDefault.szLpqcommand, "lpstat -o%p");
			string_set(&sDefault.szLprmcommand, "cancel %p-%j");
			string_set(&sDefault.szPrintcommand,
				   "lp -c -d%p %s; rm %s");
			string_set(&sDefault.szQueuepausecommand,
				   "disable %p");
			string_set(&sDefault.szQueueresumecommand,
				   "enable %p");
#ifndef HPUX
			string_set(&sDefault.szLppausecommand,
				   "lp -i %p-%j -H hold");
			string_set(&sDefault.szLpresumecommand,
				   "lp -i %p-%j -H resume");
#endif /* HPUX */
			break;

		case PRINT_QNX:
			string_set(&sDefault.szLpqcommand, "lpq -P%p");
			string_set(&sDefault.szLprmcommand, "lprm -P%p %j");
			string_set(&sDefault.szPrintcommand, "lp -r -P%p %s");
			break;

		case PRINT_SOFTQ:
			string_set(&sDefault.szLpqcommand, "qstat -l -d%p");
			string_set(&sDefault.szLprmcommand,
				   "qstat -s -j%j -c");
			string_set(&sDefault.szPrintcommand,
				   "lp -d%p -s %s; rm %s");
			string_set(&sDefault.szLppausecommand,
				   "qstat -s -j%j -h");
			string_set(&sDefault.szLpresumecommand,
				   "qstat -s -j%j -r");
			break;
#ifdef DEVELOPER
	case PRINT_TEST:
	case PRINT_VLP:
		string_set(&sDefault.szPrintcommand, "vlp print %p %s");
		string_set(&sDefault.szLpqcommand, "vlp lpq %p");
		string_set(&sDefault.szLprmcommand, "vlp lprm %p %j");
		string_set(&sDefault.szLppausecommand, "vlp lppause %p %j");
		string_set(&sDefault.szLpresumecommand, "vlp lpresum %p %j");
		string_set(&sDefault.szQueuepausecommand, "vlp queuepause %p");
		string_set(&sDefault.szQueueresumecommand, "vlp queueresume %p");
		break;
#endif /* DEVELOPER */

	}
}

/***************************************************************************
 Initialise the global parameter structure.
***************************************************************************/

static void init_globals(void)
{
	static BOOL done_init = False;
	pstring s;

	if (!done_init) {
		int i;
		memset((void *)&Globals, '\0', sizeof(Globals));

		for (i = 0; parm_table[i].label; i++)
			if ((parm_table[i].type == P_STRING ||
			     parm_table[i].type == P_USTRING) &&
			    parm_table[i].ptr)
				string_set(parm_table[i].ptr, "");

		string_set(&sDefault.fstype, FSTYPE_STRING);

		init_printer_values();

		done_init = True;
	}


	DEBUG(3, ("Initialising global parameters\n"));

	string_set(&Globals.szSMBPasswdFile, dyn_SMB_PASSWD_FILE);
	string_set(&Globals.szPrivateDir, dyn_PRIVATE_DIR);

	/* use the new 'hash2' method by default, with a prefix of 1 */
	string_set(&Globals.szManglingMethod, "hash2");
	Globals.mangle_prefix = 1;

	string_set(&Globals.szGuestaccount, GUEST_ACCOUNT);

	/* using UTF8 by default allows us to support all chars */
	string_set(&Globals.unix_charset, "UTF8");

	/* Use codepage 850 as a default for the dos character set */
	string_set(&Globals.dos_charset, "CP850");

	/*
	 * Allow the default PASSWD_CHAT to be overridden in local.h.
	 */
	string_set(&Globals.szPasswdChat, DEFAULT_PASSWD_CHAT);
	
	set_global_myname(myhostname());
	string_set(&Globals.szNetbiosName,global_myname());

	set_global_myworkgroup(WORKGROUP);
	string_set(&Globals.szWorkgroup, lp_workgroup());
	
	string_set(&Globals.szPasswdProgram, "");
	string_set(&Globals.szPrintcapname, PRINTCAP_NAME);
	string_set(&Globals.szPidDir, dyn_PIDDIR);
	string_set(&Globals.szLockDir, dyn_LOCKDIR);
	string_set(&Globals.szSocketAddress, "0.0.0.0");
	pstrcpy(s, "Samba ");
	pstrcat(s, VERSION);
	string_set(&Globals.szServerString, s);
	slprintf(s, sizeof(s) - 1, "%d.%d", DEFAULT_MAJOR_VERSION,
		 DEFAULT_MINOR_VERSION);
	string_set(&Globals.szAnnounceVersion, s);

	pstrcpy(user_socket_options, DEFAULT_SOCKET_OPTIONS);

	string_set(&Globals.szLogonDrive, "");
	/* %N is the NIS auto.home server if -DAUTOHOME is used, else same as %L */
	string_set(&Globals.szLogonHome, "\\\\%N\\%U");
	string_set(&Globals.szLogonPath, "\\\\%N\\%U\\profile");

	string_set(&Globals.szNameResolveOrder, "lmhosts wins host bcast");
	string_set(&Globals.szPasswordServer, "*");

	Globals.bAlgorithmicRidBase = BASE_RID;

	Globals.bLoadPrinters = True;
	Globals.mangled_stack = 50;
	/* Was 65535 (0xFFFF). 0x4101 matches W2K and causes major speed improvements... */
	/* Discovered by 2 days of pain by Don McCall @ HP :-). */
	Globals.max_xmit = 0x4104;
	Globals.max_mux = 50;	/* This is *needed* for profile support. */
	Globals.lpqcachetime = 10;
	Globals.bDisableSpoolss = False;
	Globals.iMaxSmbdProcesses = 0;/* no limit specified */
	Globals.iTotalPrintJobs = 0;  /* no limit specified */
	Globals.pwordlevel = 0;
	Globals.unamelevel = 0;
	Globals.deadtime = 0;
	Globals.bLargeReadwrite = True;
	Globals.max_log_size = 5000;
	Globals.max_open_files = MAX_OPEN_FILES;
	Globals.maxprotocol = PROTOCOL_NT1;
	Globals.minprotocol = PROTOCOL_CORE;
	Globals.security = SEC_USER;
	Globals.paranoid_server_security = True;
	Globals.bEncryptPasswords = True;
	Globals.bUpdateEncrypt = False;
	Globals.bReadRaw = True;
	Globals.bWriteRaw = True;
	Globals.bReadPrediction = False;
	Globals.bReadbmpx = False;
	Globals.bNullPasswords = False;
	Globals.bObeyPamRestrictions = False;
	Globals.bStripDot = False;
	Globals.syslog = 1;
	Globals.bSyslogOnly = False;
	Globals.bTimestampLogs = True;
	string_set(&Globals.szLogLevel, "0");
	Globals.bDebugHiresTimestamp = False;
	Globals.bDebugPid = False;
	Globals.bDebugUid = False;
	Globals.max_ttl = 60 * 60 * 24 * 3;	/* 3 days default. */
	Globals.max_wins_ttl = 60 * 60 * 24 * 6;	/* 6 days default. */
	Globals.min_wins_ttl = 60 * 60 * 6;	/* 6 hours default. */
	Globals.machine_password_timeout = 60 * 60 * 24 * 7;	/* 7 days default. */
	Globals.change_notify_timeout = 60;	/* 1 minute default. */
	Globals.bKernelChangeNotify = True;	/* On if we have it. */
	Globals.ReadSize = 16 * 1024;
	Globals.lm_announce = 2;	/* = Auto: send only if LM clients found */
	Globals.lm_interval = 60;
	Globals.stat_cache_size = 50;	/* Number of stat translations we'll keep */
	Globals.announce_as = ANNOUNCE_AS_NT_SERVER;
#if (defined(HAVE_NETGROUP) && defined(WITH_AUTOMOUNT))
	Globals.bNISHomeMap = False;
#ifdef WITH_NISPLUS_HOME
	string_set(&Globals.szNISHomeMapName, "auto_home.org_dir");
#else
	string_set(&Globals.szNISHomeMapName, "auto.home");
#endif
#endif
	Globals.bTimeServer = False;
	Globals.bBindInterfacesOnly = False;
	Globals.bUnixPasswdSync = False;
	Globals.bPamPasswordChange = False;
	Globals.bPasswdChatDebug = False;
	Globals.bUnicode = True;	/* Do unicode on the wire by default */
	Globals.bNTPipeSupport = True;	/* Do NT pipes by default. */
	Globals.bNTStatusSupport = True; /* Use NT status by default. */
	Globals.bStatCache = True;	/* use stat cache by default */
	Globals.restrict_anonymous = 0;
	Globals.bClientLanManAuth = True;	/* Do use the LanMan hash if it is available */
	Globals.bLanmanAuth = True;	/* Do use the LanMan hash if it is available */
	Globals.bNTLMAuth = True;	/* Do use NTLMv1 if it is available (otherwise NTLMv2) */
	
	Globals.map_to_guest = 0;	/* By Default, "Never" */
	Globals.min_passwd_length = MINPASSWDLENGTH;	/* By Default, 5. */
	Globals.oplock_break_wait_time = 0;	/* By Default, 0 msecs. */
	Globals.enhanced_browsing = True; 
	Globals.iLockSpinCount = 3; /* Try 2 times. */
	Globals.iLockSpinTime = 10; /* usec. */
#ifdef MMAP_BLACKLIST
	Globals.bUseMmap = False;
#else
	Globals.bUseMmap = True;
#endif
	Globals.bUnixExtensions = False;

	/* hostname lookups can be very expensive and are broken on
	   a large number of sites (tridge) */
	Globals.bHostnameLookups = False;

#ifdef WITH_LDAP_SAMCONFIG
	string_set(&Globals.szLdapServer, "localhost");
	Globals.ldap_port = 636;
	Globals.szPassdbBackend = str_list_make("ldapsam unixsam", NULL);
#else
	Globals.szPassdbBackend = str_list_make("smbpasswd unixsam", NULL);
#endif /* WITH_LDAP_SAMCONFIG */

	string_set(&Globals.szLdapSuffix, "");
	string_set(&Globals.szLdapMachineSuffix, "");
	string_set(&Globals.szLdapUserSuffix, "");

	string_set(&Globals.szLdapFilter, "(&(uid=%u)(objectclass=sambaAccount))");
	string_set(&Globals.szLdapAdminDn, "");
	Globals.ldap_ssl = LDAP_SSL_ON;
	Globals.ldap_passwd_sync = LDAP_PASSWD_SYNC_OFF;

/* these parameters are set to defaults that are more appropriate
   for the increasing samba install base:

   as a member of the workgroup, that will possibly become a
   _local_ master browser (lm = True).  this is opposed to a forced
   local master browser startup (pm = True).

   doesn't provide WINS server service by default (wsupp = False),
   and doesn't provide domain master browser services by default, either.

*/

	Globals.bMsAddPrinterWizard = True;
	Globals.bPreferredMaster = Auto;	/* depending on bDomainMaster */
	Globals.os_level = 20;
	Globals.bLocalMaster = True;
	Globals.bDomainMaster = Auto;	/* depending on bDomainLogons */
	Globals.bDomainLogons = False;
	Globals.bBrowseList = True;
	Globals.bWINSsupport = False;
	Globals.bWINSproxy = False;

	Globals.bDNSproxy = True;

	/* this just means to use them if they exist */
	Globals.bKernelOplocks = True;

	Globals.bAllowTrustedDomains = True;

	string_set(&Globals.szTemplateShell, "/bin/false");
	string_set(&Globals.szTemplateHomedir, "/home/%D/%U");
	string_set(&Globals.szWinbindSeparator, "\\");
	string_set(&Globals.szAclCompat, "");

	Globals.winbind_cache_time = 15;
	Globals.bWinbindEnumUsers = True;
	Globals.bWinbindEnumGroups = True;
	Globals.bWinbindUseDefaultDomain = False;

	Globals.name_cache_timeout = 660; /* In seconds */

	Globals.bUseSpnego = True;
	Globals.bClientUseSpnego = True;

	string_set(&Globals.smb_ports, SMB_PORTS);
#ifdef WITH_OPENDIRECTORY
	Globals.bOpenDirectory = True;
#endif
	
}

static TALLOC_CTX *lp_talloc;

/******************************************************************* a
 Free up temporary memory - called from the main loop.
********************************************************************/

void lp_talloc_free(void)
{
	if (!lp_talloc)
		return;
	talloc_destroy(lp_talloc);
	lp_talloc = NULL;
}

/*******************************************************************
 Convenience routine to grab string parameters into temporary memory
 and run standard_sub_basic on them. The buffers can be written to by
 callers without affecting the source string.
********************************************************************/

static char *lp_string(const char *s)
{
	size_t len = s ? strlen(s) : 0;
	char *ret;

	/* The follow debug is useful for tracking down memory problems
	   especially if you have an inner loop that is calling a lp_*()
	   function that returns a string.  Perhaps this debug should be
	   present all the time? */

#if 0
	DEBUG(10, ("lp_string(%s)\n", s));
#endif

	if (!lp_talloc)
		lp_talloc = talloc_init("lp_talloc");

	ret = (char *)talloc(lp_talloc, len + 100);	/* leave room for substitution */

	if (!ret)
		return NULL;

	if (!s)
		*ret = 0;
	else
		StrnCpy(ret, s, len);

	if (trim_string(ret, "\"", "\"")) {
		if (strchr(ret,'"') != NULL)
			StrnCpy(ret, s, len);
	}

	standard_sub_basic(current_user_info.smb_name,ret,len+100);
	return (ret);
}

/*
   In this section all the functions that are used to access the 
   parameters from the rest of the program are defined 
*/

#define FN_GLOBAL_STRING(fn_name,ptr) \
 char *fn_name(void) {return(lp_string(*(char **)(ptr) ? *(char **)(ptr) : ""));}
#define FN_GLOBAL_CONST_STRING(fn_name,ptr) \
 const char *fn_name(void) {return(*(const char **)(ptr) ? *(const char **)(ptr) : "");}
#define FN_GLOBAL_LIST(fn_name,ptr) \
 const char **fn_name(void) {return(*(const char ***)(ptr));}
#define FN_GLOBAL_BOOL(fn_name,ptr) \
 BOOL fn_name(void) {return(*(BOOL *)(ptr));}
#define FN_GLOBAL_CHAR(fn_name,ptr) \
 char fn_name(void) {return(*(char *)(ptr));}
#define FN_GLOBAL_INTEGER(fn_name,ptr) \
 int fn_name(void) {return(*(int *)(ptr));}

#define FN_LOCAL_STRING(fn_name,val) \
 char *fn_name(int i) {return(lp_string((LP_SNUM_OK(i) && ServicePtrs[(i)]->val) ? ServicePtrs[(i)]->val : sDefault.val));}
#define FN_LOCAL_CONST_STRING(fn_name,val) \
 const char *fn_name(int i) {return (const char *)((LP_SNUM_OK(i) && ServicePtrs[(i)]->val) ? ServicePtrs[(i)]->val : sDefault.val);}
#define FN_LOCAL_LIST(fn_name,val) \
 const char **fn_name(int i) {return(const char **)(LP_SNUM_OK(i)? ServicePtrs[(i)]->val : sDefault.val);}
#define FN_LOCAL_BOOL(fn_name,val) \
 BOOL fn_name(int i) {return(LP_SNUM_OK(i)? ServicePtrs[(i)]->val : sDefault.val);}
#define FN_LOCAL_CHAR(fn_name,val) \
 char fn_name(int i) {return(LP_SNUM_OK(i)? ServicePtrs[(i)]->val : sDefault.val);}
#define FN_LOCAL_INTEGER(fn_name,val) \
 int fn_name(int i) {return(LP_SNUM_OK(i)? ServicePtrs[(i)]->val : sDefault.val);}

FN_GLOBAL_STRING(lp_smb_ports, &Globals.smb_ports)
FN_GLOBAL_STRING(lp_dos_charset, &Globals.dos_charset)
FN_GLOBAL_STRING(lp_unix_charset, &Globals.unix_charset)
FN_GLOBAL_STRING(lp_display_charset, &Globals.display_charset)
FN_GLOBAL_STRING(lp_logfile, &Globals.szLogFile)
FN_GLOBAL_STRING(lp_configfile, &Globals.szConfigFile)
FN_GLOBAL_STRING(lp_smb_passwd_file, &Globals.szSMBPasswdFile)
FN_GLOBAL_STRING(lp_private_dir, &Globals.szPrivateDir)
FN_GLOBAL_STRING(lp_serverstring, &Globals.szServerString)
FN_GLOBAL_STRING(lp_printcapname, &Globals.szPrintcapname)
FN_GLOBAL_STRING(lp_enumports_cmd, &Globals.szEnumPortsCommand)
FN_GLOBAL_STRING(lp_addprinter_cmd, &Globals.szAddPrinterCommand)
FN_GLOBAL_STRING(lp_deleteprinter_cmd, &Globals.szDeletePrinterCommand)
FN_GLOBAL_STRING(lp_os2_driver_map, &Globals.szOs2DriverMap)
FN_GLOBAL_STRING(lp_lockdir, &Globals.szLockDir)
FN_GLOBAL_STRING(lp_piddir, &Globals.szPidDir)
FN_GLOBAL_STRING(lp_mangling_method, &Globals.szManglingMethod)
FN_GLOBAL_INTEGER(lp_mangle_prefix, &Globals.mangle_prefix)
#ifdef WITH_UTMP
FN_GLOBAL_STRING(lp_utmpdir, &Globals.szUtmpDir)
FN_GLOBAL_STRING(lp_wtmpdir, &Globals.szWtmpDir)
FN_GLOBAL_BOOL(lp_utmp, &Globals.bUtmp)
#endif
FN_GLOBAL_STRING(lp_rootdir, &Globals.szRootdir)
FN_GLOBAL_STRING(lp_source_environment, &Globals.szSourceEnv)
FN_GLOBAL_STRING(lp_defaultservice, &Globals.szDefaultService)
FN_GLOBAL_STRING(lp_msg_command, &Globals.szMsgCommand)
FN_GLOBAL_STRING(lp_dfree_command, &Globals.szDfree)
FN_GLOBAL_STRING(lp_hosts_equiv, &Globals.szHostsEquiv)
FN_GLOBAL_STRING(lp_auto_services, &Globals.szAutoServices)
FN_GLOBAL_STRING(lp_passwd_program, &Globals.szPasswdProgram)
FN_GLOBAL_STRING(lp_passwd_chat, &Globals.szPasswdChat)
FN_GLOBAL_STRING(lp_passwordserver, &Globals.szPasswordServer)
FN_GLOBAL_STRING(lp_name_resolve_order, &Globals.szNameResolveOrder)
FN_GLOBAL_STRING(lp_realm, &Globals.szRealm)
FN_GLOBAL_STRING(lp_ads_server, &Globals.szADSserver)
FN_GLOBAL_STRING(lp_username_map, &Globals.szUsernameMap)
FN_GLOBAL_CONST_STRING(lp_logon_script, &Globals.szLogonScript)
FN_GLOBAL_CONST_STRING(lp_logon_path, &Globals.szLogonPath)
FN_GLOBAL_CONST_STRING(lp_logon_drive, &Globals.szLogonDrive)
FN_GLOBAL_CONST_STRING(lp_logon_home, &Globals.szLogonHome)
FN_GLOBAL_STRING(lp_remote_announce, &Globals.szRemoteAnnounce)
FN_GLOBAL_STRING(lp_remote_browse_sync, &Globals.szRemoteBrowseSync)
FN_GLOBAL_LIST(lp_wins_server_list, &Globals.szWINSservers)
FN_GLOBAL_LIST(lp_interfaces, &Globals.szInterfaces)
FN_GLOBAL_STRING(lp_socket_address, &Globals.szSocketAddress)
FN_GLOBAL_STRING(lp_nis_home_map_name, &Globals.szNISHomeMapName)
static FN_GLOBAL_STRING(lp_announce_version, &Globals.szAnnounceVersion)
FN_GLOBAL_LIST(lp_netbios_aliases, &Globals.szNetbiosAliases)
FN_GLOBAL_LIST(lp_passdb_backend, &Globals.szPassdbBackend)
FN_GLOBAL_STRING(lp_panic_action, &Globals.szPanicAction)
FN_GLOBAL_STRING(lp_adduser_script, &Globals.szAddUserScript)
FN_GLOBAL_STRING(lp_deluser_script, &Globals.szDelUserScript)

FN_GLOBAL_CONST_STRING(lp_guestaccount, &Globals.szGuestaccount)
FN_GLOBAL_STRING(lp_addgroup_script, &Globals.szAddGroupScript)
FN_GLOBAL_STRING(lp_delgroup_script, &Globals.szDelGroupScript)
FN_GLOBAL_STRING(lp_addusertogroup_script, &Globals.szAddUserToGroupScript)
FN_GLOBAL_STRING(lp_deluserfromgroup_script, &Globals.szDelUserFromGroupScript)
FN_GLOBAL_STRING(lp_setprimarygroup_script, &Globals.szSetPrimaryGroupScript)

FN_GLOBAL_STRING(lp_addmachine_script, &Globals.szAddMachineScript)

FN_GLOBAL_STRING(lp_shutdown_script, &Globals.szShutdownScript)
FN_GLOBAL_STRING(lp_abort_shutdown_script, &Globals.szAbortShutdownScript)

FN_GLOBAL_STRING(lp_wins_hook, &Globals.szWINSHook)
FN_GLOBAL_STRING(lp_wins_partners, &Globals.szWINSPartners)
FN_GLOBAL_STRING(lp_template_homedir, &Globals.szTemplateHomedir)
FN_GLOBAL_STRING(lp_template_shell, &Globals.szTemplateShell)
FN_GLOBAL_CONST_STRING(lp_winbind_separator, &Globals.szWinbindSeparator)
FN_GLOBAL_STRING(lp_acl_compatibility, &Globals.szAclCompat)
FN_GLOBAL_BOOL(lp_winbind_enum_users, &Globals.bWinbindEnumUsers)
FN_GLOBAL_BOOL(lp_winbind_enum_groups, &Globals.bWinbindEnumGroups)
FN_GLOBAL_BOOL(lp_winbind_use_default_domain, &Globals.bWinbindUseDefaultDomain)

#ifdef WITH_LDAP_SAMCONFIG
FN_GLOBAL_STRING(lp_ldap_server, &Globals.szLdapServer)
FN_GLOBAL_INTEGER(lp_ldap_port, &Globals.ldap_port)
#endif
FN_GLOBAL_STRING(lp_ldap_suffix, &Globals.szLdapSuffix)
FN_GLOBAL_STRING(lp_ldap_machine_suffix, &Globals.szLdapMachineSuffix)
FN_GLOBAL_STRING(lp_ldap_user_suffix, &Globals.szLdapUserSuffix)
FN_GLOBAL_STRING(lp_ldap_filter, &Globals.szLdapFilter)
FN_GLOBAL_STRING(lp_ldap_admin_dn, &Globals.szLdapAdminDn)
FN_GLOBAL_INTEGER(lp_ldap_ssl, &Globals.ldap_ssl)
FN_GLOBAL_INTEGER(lp_ldap_passwd_sync, &Globals.ldap_passwd_sync)
FN_GLOBAL_BOOL(lp_ldap_trust_ids, &Globals.ldap_trust_ids)
FN_GLOBAL_STRING(lp_add_share_cmd, &Globals.szAddShareCommand)
FN_GLOBAL_STRING(lp_change_share_cmd, &Globals.szChangeShareCommand)
FN_GLOBAL_STRING(lp_delete_share_cmd, &Globals.szDeleteShareCommand)

FN_GLOBAL_BOOL(lp_disable_netbios, &Globals.bDisableNetbios)
FN_GLOBAL_BOOL(lp_ms_add_printer_wizard, &Globals.bMsAddPrinterWizard)
FN_GLOBAL_BOOL(lp_dns_proxy, &Globals.bDNSproxy)
FN_GLOBAL_BOOL(lp_wins_support, &Globals.bWINSsupport)
FN_GLOBAL_BOOL(lp_we_are_a_wins_server, &Globals.bWINSsupport)
FN_GLOBAL_BOOL(lp_wins_proxy, &Globals.bWINSproxy)
FN_GLOBAL_BOOL(lp_local_master, &Globals.bLocalMaster)
FN_GLOBAL_BOOL(lp_domain_logons, &Globals.bDomainLogons)
FN_GLOBAL_BOOL(lp_load_printers, &Globals.bLoadPrinters)
FN_GLOBAL_BOOL(lp_readprediction, &Globals.bReadPrediction)
FN_GLOBAL_BOOL(lp_readbmpx, &Globals.bReadbmpx)
FN_GLOBAL_BOOL(lp_readraw, &Globals.bReadRaw)
FN_GLOBAL_BOOL(lp_large_readwrite, &Globals.bLargeReadwrite)
FN_GLOBAL_BOOL(lp_writeraw, &Globals.bWriteRaw)
FN_GLOBAL_BOOL(lp_null_passwords, &Globals.bNullPasswords)
FN_GLOBAL_BOOL(lp_obey_pam_restrictions, &Globals.bObeyPamRestrictions)
FN_GLOBAL_BOOL(lp_strip_dot, &Globals.bStripDot)
FN_GLOBAL_BOOL(lp_encrypted_passwords, &Globals.bEncryptPasswords)
FN_GLOBAL_BOOL(lp_update_encrypted, &Globals.bUpdateEncrypt)
FN_GLOBAL_BOOL(lp_syslog_only, &Globals.bSyslogOnly)
FN_GLOBAL_BOOL(lp_timestamp_logs, &Globals.bTimestampLogs)
FN_GLOBAL_BOOL(lp_debug_hires_timestamp, &Globals.bDebugHiresTimestamp)
FN_GLOBAL_BOOL(lp_debug_pid, &Globals.bDebugPid)
FN_GLOBAL_BOOL(lp_debug_uid, &Globals.bDebugUid)
FN_GLOBAL_BOOL(lp_browse_list, &Globals.bBrowseList)
FN_GLOBAL_BOOL(lp_nis_home_map, &Globals.bNISHomeMap)
static FN_GLOBAL_BOOL(lp_time_server, &Globals.bTimeServer)
FN_GLOBAL_BOOL(lp_bind_interfaces_only, &Globals.bBindInterfacesOnly)
FN_GLOBAL_BOOL(lp_pam_password_change, &Globals.bPamPasswordChange)
FN_GLOBAL_BOOL(lp_unix_password_sync, &Globals.bUnixPasswdSync)
FN_GLOBAL_BOOL(lp_passwd_chat_debug, &Globals.bPasswdChatDebug)
FN_GLOBAL_BOOL(lp_unicode, &Globals.bUnicode)
FN_GLOBAL_BOOL(lp_nt_pipe_support, &Globals.bNTPipeSupport)
FN_GLOBAL_BOOL(lp_nt_status_support, &Globals.bNTStatusSupport)
FN_GLOBAL_BOOL(lp_stat_cache, &Globals.bStatCache)
FN_GLOBAL_BOOL(lp_allow_trusted_domains, &Globals.bAllowTrustedDomains)
FN_GLOBAL_INTEGER(lp_restrict_anonymous, &Globals.restrict_anonymous)
FN_GLOBAL_BOOL(lp_lanman_auth, &Globals.bLanmanAuth)
FN_GLOBAL_BOOL(lp_ntlm_auth, &Globals.bNTLMAuth)
FN_GLOBAL_BOOL(lp_client_lanman_auth, &Globals.bClientLanManAuth)
FN_GLOBAL_BOOL(lp_client_ntlmv2_auth, &Globals.bClientNTLMv2Auth)
FN_GLOBAL_BOOL(lp_host_msdfs, &Globals.bHostMSDfs)
FN_GLOBAL_BOOL(lp_kernel_oplocks, &Globals.bKernelOplocks)
FN_GLOBAL_BOOL(lp_enhanced_browsing, &Globals.enhanced_browsing)
FN_GLOBAL_BOOL(lp_use_mmap, &Globals.bUseMmap)
FN_GLOBAL_BOOL(lp_unix_extensions, &Globals.bUnixExtensions)
FN_GLOBAL_BOOL(lp_use_spnego, &Globals.bUseSpnego)
FN_GLOBAL_BOOL(lp_client_use_spnego, &Globals.bClientUseSpnego)
FN_GLOBAL_BOOL(lp_hostname_lookups, &Globals.bHostnameLookups)
FN_GLOBAL_BOOL(lp_kernel_change_notify, &Globals.bKernelChangeNotify)
FN_GLOBAL_INTEGER(lp_os_level, &Globals.os_level)
FN_GLOBAL_INTEGER(lp_max_ttl, &Globals.max_ttl)
FN_GLOBAL_INTEGER(lp_max_wins_ttl, &Globals.max_wins_ttl)
FN_GLOBAL_INTEGER(lp_min_wins_ttl, &Globals.min_wins_ttl)
FN_GLOBAL_INTEGER(lp_max_log_size, &Globals.max_log_size)
FN_GLOBAL_INTEGER(lp_max_open_files, &Globals.max_open_files)
FN_GLOBAL_INTEGER(lp_maxxmit, &Globals.max_xmit)
FN_GLOBAL_INTEGER(lp_maxmux, &Globals.max_mux)
FN_GLOBAL_INTEGER(lp_passwordlevel, &Globals.pwordlevel)
FN_GLOBAL_INTEGER(lp_usernamelevel, &Globals.unamelevel)
FN_GLOBAL_INTEGER(lp_readsize, &Globals.ReadSize)
FN_GLOBAL_INTEGER(lp_deadtime, &Globals.deadtime)
FN_GLOBAL_INTEGER(lp_maxprotocol, &Globals.maxprotocol)
FN_GLOBAL_INTEGER(lp_minprotocol, &Globals.minprotocol)
FN_GLOBAL_INTEGER(lp_security, &Globals.security)
FN_GLOBAL_LIST(lp_auth_methods, &Globals.AuthMethods)
FN_GLOBAL_BOOL(lp_paranoid_server_security, &Globals.paranoid_server_security)
FN_GLOBAL_INTEGER(lp_maxdisksize, &Globals.maxdisksize)
FN_GLOBAL_INTEGER(lp_lpqcachetime, &Globals.lpqcachetime)
FN_GLOBAL_INTEGER(lp_max_smbd_processes, &Globals.iMaxSmbdProcesses)
FN_GLOBAL_INTEGER(lp_disable_spoolss, &Globals.bDisableSpoolss)
FN_GLOBAL_INTEGER(lp_totalprintjobs, &Globals.iTotalPrintJobs)
FN_GLOBAL_INTEGER(lp_syslog, &Globals.syslog)
static FN_GLOBAL_INTEGER(lp_announce_as, &Globals.announce_as)
FN_GLOBAL_INTEGER(lp_lm_announce, &Globals.lm_announce)
FN_GLOBAL_INTEGER(lp_lm_interval, &Globals.lm_interval)
FN_GLOBAL_INTEGER(lp_machine_password_timeout, &Globals.machine_password_timeout)
FN_GLOBAL_INTEGER(lp_change_notify_timeout, &Globals.change_notify_timeout)
FN_GLOBAL_INTEGER(lp_stat_cache_size, &Globals.stat_cache_size)
FN_GLOBAL_INTEGER(lp_map_to_guest, &Globals.map_to_guest)
FN_GLOBAL_INTEGER(lp_min_passwd_length, &Globals.min_passwd_length)
FN_GLOBAL_INTEGER(lp_oplock_break_wait_time, &Globals.oplock_break_wait_time)
FN_GLOBAL_INTEGER(lp_lock_spin_count, &Globals.iLockSpinCount)
FN_GLOBAL_INTEGER(lp_lock_sleep_time, &Globals.iLockSpinTime)
FN_LOCAL_STRING(lp_preexec, szPreExec)
FN_LOCAL_STRING(lp_postexec, szPostExec)
FN_LOCAL_STRING(lp_rootpreexec, szRootPreExec)
FN_LOCAL_STRING(lp_rootpostexec, szRootPostExec)
FN_LOCAL_STRING(lp_servicename, szService)
FN_LOCAL_CONST_STRING(lp_const_servicename, szService)
FN_LOCAL_STRING(lp_pathname, szPath)
FN_LOCAL_STRING(lp_dontdescend, szDontdescend)
FN_LOCAL_STRING(lp_username, szUsername)
FN_LOCAL_LIST(lp_invalid_users, szInvalidUsers)
FN_LOCAL_LIST(lp_valid_users, szValidUsers)
FN_LOCAL_LIST(lp_admin_users, szAdminUsers)
FN_LOCAL_STRING(lp_printcommand, szPrintcommand)
FN_LOCAL_STRING(lp_lpqcommand, szLpqcommand)
FN_LOCAL_STRING(lp_lprmcommand, szLprmcommand)
FN_LOCAL_STRING(lp_lppausecommand, szLppausecommand)
FN_LOCAL_STRING(lp_lpresumecommand, szLpresumecommand)
FN_LOCAL_STRING(lp_queuepausecommand, szQueuepausecommand)
FN_LOCAL_STRING(lp_queueresumecommand, szQueueresumecommand)
static FN_LOCAL_STRING(_lp_printername, szPrintername)
FN_LOCAL_LIST(lp_hostsallow, szHostsallow)
FN_LOCAL_LIST(lp_hostsdeny, szHostsdeny)
FN_LOCAL_STRING(lp_magicscript, szMagicScript)
FN_LOCAL_STRING(lp_magicoutput, szMagicOutput)
FN_LOCAL_STRING(lp_comment, comment)
FN_LOCAL_STRING(lp_force_user, force_user)
FN_LOCAL_STRING(lp_force_group, force_group)
FN_LOCAL_LIST(lp_readlist, readlist)
FN_LOCAL_LIST(lp_writelist, writelist)
FN_LOCAL_LIST(lp_printer_admin, printer_admin)
FN_LOCAL_STRING(lp_fstype, fstype)
FN_LOCAL_STRING(lp_vfsobj, szVfsObjectFile)
FN_LOCAL_STRING(lp_vfs_options, szVfsOptions)
FN_LOCAL_STRING(lp_vfs_path, szVfsPath)
FN_LOCAL_STRING(lp_msdfs_proxy, szMSDfsProxy)
static FN_LOCAL_STRING(lp_volume, volume)
FN_LOCAL_STRING(lp_mangled_map, szMangledMap)
FN_LOCAL_STRING(lp_veto_files, szVetoFiles)
FN_LOCAL_STRING(lp_hide_files, szHideFiles)
FN_LOCAL_STRING(lp_veto_oplocks, szVetoOplockFiles)
FN_LOCAL_BOOL(lp_msdfs_root, bMSDfsRoot)
FN_LOCAL_BOOL(lp_autoloaded, autoloaded)
FN_LOCAL_BOOL(lp_preexec_close, bPreexecClose)
FN_LOCAL_BOOL(lp_rootpreexec_close, bRootpreexecClose)
FN_LOCAL_BOOL(lp_casesensitive, bCaseSensitive)
FN_LOCAL_BOOL(lp_preservecase, bCasePreserve)
FN_LOCAL_BOOL(lp_shortpreservecase, bShortCasePreserve)
FN_LOCAL_BOOL(lp_casemangle, bCaseMangle)
FN_LOCAL_BOOL(lp_hide_dot_files, bHideDotFiles)
FN_LOCAL_BOOL(lp_hide_special_files, bHideSpecialFiles)
FN_LOCAL_BOOL(lp_hideunreadable, bHideUnReadable)
FN_LOCAL_BOOL(lp_hideunwriteable_files, bHideUnWriteableFiles)
FN_LOCAL_BOOL(lp_browseable, bBrowseable)
FN_LOCAL_BOOL(lp_readonly, bRead_only)
FN_LOCAL_BOOL(lp_no_set_dir, bNo_set_dir)
FN_LOCAL_BOOL(lp_guest_ok, bGuest_ok)
FN_LOCAL_BOOL(lp_guest_only, bGuest_only)
FN_LOCAL_BOOL(lp_print_ok, bPrint_ok)
FN_LOCAL_BOOL(lp_map_hidden, bMap_hidden)
FN_LOCAL_BOOL(lp_map_archive, bMap_archive)
FN_LOCAL_BOOL(lp_locking, bLocking)
FN_LOCAL_BOOL(lp_strict_locking, bStrictLocking)
FN_LOCAL_BOOL(lp_posix_locking, bPosixLocking)
FN_LOCAL_BOOL(lp_share_modes, bShareModes)
FN_LOCAL_BOOL(lp_oplocks, bOpLocks)
FN_LOCAL_BOOL(lp_level2_oplocks, bLevel2OpLocks)
FN_LOCAL_BOOL(lp_onlyuser, bOnlyUser)
FN_LOCAL_BOOL(lp_manglednames, bMangledNames)
FN_LOCAL_BOOL(lp_widelinks, bWidelinks)
FN_LOCAL_BOOL(lp_symlinks, bSymlinks)
FN_LOCAL_BOOL(lp_syncalways, bSyncAlways)
FN_LOCAL_BOOL(lp_strict_allocate, bStrictAllocate)
FN_LOCAL_BOOL(lp_strict_sync, bStrictSync)
FN_LOCAL_BOOL(lp_map_system, bMap_system)
FN_LOCAL_BOOL(lp_delete_readonly, bDeleteReadonly)
FN_LOCAL_BOOL(lp_fake_oplocks, bFakeOplocks)
FN_LOCAL_BOOL(lp_recursive_veto_delete, bDeleteVetoFiles)
FN_LOCAL_BOOL(lp_dos_filemode, bDosFilemode)
FN_LOCAL_BOOL(lp_dos_filetimes, bDosFiletimes)
FN_LOCAL_BOOL(lp_dos_filetime_resolution, bDosFiletimeResolution)
FN_LOCAL_BOOL(lp_fake_dir_create_times, bFakeDirCreateTimes)
FN_LOCAL_BOOL(lp_blocking_locks, bBlockingLocks)
FN_LOCAL_BOOL(lp_inherit_perms, bInheritPerms)
FN_LOCAL_BOOL(lp_inherit_acls, bInheritACLS)
FN_LOCAL_BOOL(lp_use_client_driver, bUseClientDriver)
FN_LOCAL_BOOL(lp_default_devmode, bDefaultDevmode)
FN_LOCAL_BOOL(lp_nt_acl_support, bNTAclSupport)
FN_LOCAL_BOOL(lp_use_sendfile, bUseSendfile)
FN_LOCAL_BOOL(lp_profile_acls, bProfileAcls)
FN_LOCAL_INTEGER(lp_create_mask, iCreate_mask)
FN_LOCAL_INTEGER(lp_force_create_mode, iCreate_force_mode)
FN_LOCAL_INTEGER(lp_security_mask, iSecurity_mask)
FN_LOCAL_INTEGER(lp_force_security_mode, iSecurity_force_mode)
FN_LOCAL_INTEGER(lp_dir_mask, iDir_mask)
FN_LOCAL_INTEGER(lp_force_dir_mode, iDir_force_mode)
FN_LOCAL_INTEGER(lp_dir_security_mask, iDir_Security_mask)
FN_LOCAL_INTEGER(lp_force_dir_security_mode, iDir_Security_force_mode)
FN_LOCAL_INTEGER(lp_max_connections, iMaxConnections)
FN_LOCAL_INTEGER(lp_defaultcase, iDefaultCase)
FN_LOCAL_INTEGER(lp_minprintspace, iMinPrintSpace)
FN_LOCAL_INTEGER(lp_printing, iPrinting)
FN_LOCAL_INTEGER(lp_max_reported_jobs, iMaxReportedPrintJobs)
FN_LOCAL_INTEGER(lp_oplock_contention_limit, iOplockContentionLimit)
FN_LOCAL_INTEGER(lp_csc_policy, iCSCPolicy)
FN_LOCAL_INTEGER(lp_write_cache_size, iWriteCacheSize)
FN_LOCAL_INTEGER(lp_block_size, iBlock_size)
FN_LOCAL_CHAR(lp_magicchar, magic_char)
FN_GLOBAL_INTEGER(lp_winbind_cache_time, &Globals.winbind_cache_time)
FN_GLOBAL_BOOL(lp_hide_local_users, &Globals.bHideLocalUsers)
FN_GLOBAL_BOOL(lp_algorithmic_rid_base, &Globals.bAlgorithmicRidBase)
FN_GLOBAL_INTEGER(lp_name_cache_timeout, &Globals.name_cache_timeout)
FN_GLOBAL_BOOL(lp_client_signing, &Globals.client_signing)
#ifdef WITH_OPENDIRECTORY
FN_GLOBAL_BOOL(lp_opendirectory, &Globals.bOpenDirectory)
#endif
typedef struct _param_opt_struct param_opt_struct;
struct _param_opt_struct {
	char *key;
	char *value;
	param_opt_struct *prev, *next;
};

static param_opt_struct *param_opt = NULL;

/* Return parametric option from given service. Type is a part of option before ':' */
/* Parametric option has following syntax: 'Type: option = value' */
/* Returned value is allocated in 'lp_talloc' context */

char *lp_parm_string(const char *servicename, const char *type, const char *option)
{
    param_opt_struct *data;
    pstring vfskey;
    
    if (param_opt != NULL) {
	    ZERO_STRUCT(vfskey);
	    pstr_sprintf(vfskey, "%s:%s:%s", (servicename==NULL) ? "global" : servicename,
		    type, option);
	    data = param_opt;
	    while (data) {
		    if (strcmp(data->key, vfskey) == 0) {
			    return lp_string(data->value);
		    }
		    data = data->next;
	    }
	    /* Try to fetch the same option but from globals */
	    pstr_sprintf(vfskey, "global:%s:%s", type, option);
	    data = param_opt;
	    while (data) {
		    if (strcmp(data->key, vfskey) == 0) {
			    return lp_string(data->value);
		    }
		    data = data->next;
	    }
	
    }
    return NULL;
}

/* local prototypes */

static int map_parameter(const char *pszParmName);
static BOOL set_boolean(BOOL *pb, const char *pszParmValue);
static int getservicebyname(const char *pszServiceName,
			    service * pserviceDest);
static void copy_service(service * pserviceDest,
			 service * pserviceSource, BOOL *pcopymapDest);
static BOOL service_ok(int iService);
static BOOL do_parameter(const char *pszParmName, const char *pszParmValue);
static BOOL do_section(const char *pszSectionName);
static void init_copymap(service * pservice);


/***************************************************************************
 Initialise a service to the defaults.
***************************************************************************/

static void init_service(service * pservice)
{
	memset((char *)pservice, '\0', sizeof(service));
	copy_service(pservice, &sDefault, NULL);
}

/***************************************************************************
 Free the dynamically allocated parts of a service struct.
***************************************************************************/

static void free_service(service *pservice)
{
	int i;
	if (!pservice)
		return;

	if (pservice->szService)
		DEBUG(5, ("free_service: Freeing service %s\n",
		       pservice->szService));

	string_free(&pservice->szService);
	SAFE_FREE(pservice->copymap);

	for (i = 0; parm_table[i].label; i++) {
		if ((parm_table[i].type == P_STRING ||
		     parm_table[i].type == P_USTRING) &&
		    parm_table[i].class == P_LOCAL)
			string_free((char **)
				    (((char *)pservice) +
				     PTR_DIFF(parm_table[i].ptr, &sDefault)));
		else if (parm_table[i].type == P_LIST &&
			 parm_table[i].class == P_LOCAL)
			     str_list_free((char ***)
			     		    (((char *)pservice) +
					     PTR_DIFF(parm_table[i].ptr, &sDefault)));
	}
				

	ZERO_STRUCTP(pservice);
}

/***************************************************************************
 Add a new service to the services array initialising it with the given 
 service. 
***************************************************************************/

static int add_a_service(const service *pservice, const char *name)
{
	int i;
	service tservice;
	int num_to_alloc = iNumServices + 1;

	tservice = *pservice;

	/* it might already exist */
	if (name) {
		i = getservicebyname(name, NULL);
		if (i >= 0)
			return (i);
	}

	/* find an invalid one */
	for (i = 0; i < iNumServices; i++)
		if (!ServicePtrs[i]->valid)
			break;

	/* if not, then create one */
	if (i == iNumServices) {
		service **tsp;
		
		tsp = (service **) Realloc(ServicePtrs,
					   sizeof(service *) *
					   num_to_alloc);
					   
		if (!tsp) {
			DEBUG(0,("add_a_service: failed to enlarge ServicePtrs!\n"));
			return (-1);
		}
		else {
			ServicePtrs = tsp;
			ServicePtrs[iNumServices] =
				(service *) malloc(sizeof(service));
		}
		if (!ServicePtrs[iNumServices]) {
			DEBUG(0,("add_a_service: out of memory!\n"));
			return (-1);
		}

		iNumServices++;
	} else
		free_service(ServicePtrs[i]);

	ServicePtrs[i]->valid = True;

	init_service(ServicePtrs[i]);
	copy_service(ServicePtrs[i], &tservice, NULL);
	if (name)
		string_set(&ServicePtrs[i]->szService, name);
	return (i);
}

/***************************************************************************
 Add a new home service, with the specified home directory, defaults coming 
 from service ifrom.
***************************************************************************/

BOOL lp_add_home(const char *pszHomename, int iDefaultService, 
		 const char *user, const char *pszHomedir)
{
	int i;
	pstring newHomedir;

	i = add_a_service(ServicePtrs[iDefaultService], pszHomename);

	if (i < 0)
		return (False);

	if (!(*(ServicePtrs[iDefaultService]->szPath))
	    || strequal(ServicePtrs[iDefaultService]->szPath, lp_pathname(-1))) {
		pstrcpy(newHomedir, pszHomedir);
	} else {
		pstrcpy(newHomedir, lp_pathname(iDefaultService));
		string_sub(newHomedir,"%H", pszHomedir, sizeof(newHomedir)); 
	}

	string_set(&ServicePtrs[i]->szPath, newHomedir);

	if (!(*(ServicePtrs[i]->comment))) {
		pstring comment;
		slprintf(comment, sizeof(comment) - 1,
			 "Home directory of %s", user);
		string_set(&ServicePtrs[i]->comment, comment);
	}
	ServicePtrs[i]->bAvailable = sDefault.bAvailable;
	ServicePtrs[i]->bBrowseable = sDefault.bBrowseable;

	DEBUG(3, ("adding home's share [%s] for user '%s' at '%s'\n", pszHomename, 
	       user, newHomedir));
	
	return (True);
}

/***************************************************************************
 Add a new service, based on an old one.
***************************************************************************/

int lp_add_service(const char *pszService, int iDefaultService)
{
	return (add_a_service(ServicePtrs[iDefaultService], pszService));
}

/***************************************************************************
 Add the IPC service.
***************************************************************************/

static BOOL lp_add_ipc(const char *ipc_name, BOOL guest_ok)
{
	pstring comment;
	int i = add_a_service(&sDefault, ipc_name);

	if (i < 0)
		return (False);

	slprintf(comment, sizeof(comment) - 1,
		 "IPC Service (%s)", Globals.szServerString);

	string_set(&ServicePtrs[i]->szPath, tmpdir());
	string_set(&ServicePtrs[i]->szUsername, "");
	string_set(&ServicePtrs[i]->comment, comment);
	string_set(&ServicePtrs[i]->fstype, "IPC");
	ServicePtrs[i]->iMaxConnections = 0;
	ServicePtrs[i]->bAvailable = True;
	ServicePtrs[i]->bRead_only = True;
	ServicePtrs[i]->bGuest_only = False;
	ServicePtrs[i]->bGuest_ok = guest_ok;
	ServicePtrs[i]->bPrint_ok = False;
	ServicePtrs[i]->bBrowseable = sDefault.bBrowseable;

	DEBUG(3, ("adding IPC service\n"));

	return (True);
}

/***************************************************************************
 Add a new printer service, with defaults coming from service iFrom.
***************************************************************************/

BOOL lp_add_printer(const char *pszPrintername, int iDefaultService)
{
	const char *comment = "From Printcap";
	int i = add_a_service(ServicePtrs[iDefaultService], pszPrintername);

	if (i < 0)
		return (False);

	/* note that we do NOT default the availability flag to True - */
	/* we take it from the default service passed. This allows all */
	/* dynamic printers to be disabled by disabling the [printers] */
	/* entry (if/when the 'available' keyword is implemented!).    */

	/* the printer name is set to the service name. */
	string_set(&ServicePtrs[i]->szPrintername, pszPrintername);
	string_set(&ServicePtrs[i]->comment, comment);
	ServicePtrs[i]->bBrowseable = sDefault.bBrowseable;
	/* Printers cannot be read_only. */
	ServicePtrs[i]->bRead_only = False;
	/* No share modes on printer services. */
	ServicePtrs[i]->bShareModes = False;
	/* No oplocks on printer services. */
	ServicePtrs[i]->bOpLocks = False;
	/* Printer services must be printable. */
	ServicePtrs[i]->bPrint_ok = True;

	DEBUG(3, ("adding printer service %s\n", pszPrintername));

	update_server_announce_as_printserver();

	return (True);
}

/***************************************************************************
 Map a parameter's string representation to something we can use. 
 Returns False if the parameter string is not recognised, else TRUE.
***************************************************************************/

static int map_parameter(const char *pszParmName)
{
	int iIndex;

	if (*pszParmName == '-')
		return (-1);

	for (iIndex = 0; parm_table[iIndex].label; iIndex++)
		if (strwicmp(parm_table[iIndex].label, pszParmName) == 0)
			return (iIndex);

	/* Warn only if it isn't parametric option */
	if (strchr(pszParmName, ':') == NULL)
		DEBUG(0, ("Unknown parameter encountered: \"%s\"\n", pszParmName));
	/* We do return 'fail' for parametric options as well because they are
	   stored in different storage
	 */
	return (-1);
}

/***************************************************************************
 Set a boolean variable from the text value stored in the passed string.
 Returns True in success, False if the passed string does not correctly 
 represent a boolean.
***************************************************************************/

static BOOL set_boolean(BOOL *pb, const char *pszParmValue)
{
	BOOL bRetval;

	bRetval = True;
	if (strwicmp(pszParmValue, "yes") == 0 ||
	    strwicmp(pszParmValue, "true") == 0 ||
	    strwicmp(pszParmValue, "1") == 0)
		*pb = True;
	else if (strwicmp(pszParmValue, "no") == 0 ||
		    strwicmp(pszParmValue, "False") == 0 ||
		    strwicmp(pszParmValue, "0") == 0)
		*pb = False;
	else {
		DEBUG(0,
		      ("ERROR: Badly formed boolean in configuration file: \"%s\".\n",
		       pszParmValue));
		bRetval = False;
	}
	return (bRetval);
}

/***************************************************************************
Find a service by name. Otherwise works like get_service.
***************************************************************************/

static int getservicebyname(const char *pszServiceName, service * pserviceDest)
{
	int iService;

	for (iService = iNumServices - 1; iService >= 0; iService--)
		if (VALID(iService) &&
		    strwicmp(ServicePtrs[iService]->szService, pszServiceName) == 0) {
			if (pserviceDest != NULL)
				copy_service(pserviceDest, ServicePtrs[iService], NULL);
			break;
		}

	return (iService);
}

/***************************************************************************
 Copy a service structure to another.
 If pcopymapDest is NULL then copy all fields
***************************************************************************/

static void copy_service(service * pserviceDest, service * pserviceSource, BOOL *pcopymapDest)
{
	int i;
	BOOL bcopyall = (pcopymapDest == NULL);

	for (i = 0; parm_table[i].label; i++)
		if (parm_table[i].ptr && parm_table[i].class == P_LOCAL &&
		    (bcopyall || pcopymapDest[i])) {
			void *def_ptr = parm_table[i].ptr;
			void *src_ptr =
				((char *)pserviceSource) + PTR_DIFF(def_ptr,
								    &sDefault);
			void *dest_ptr =
				((char *)pserviceDest) + PTR_DIFF(def_ptr,
								  &sDefault);

			switch (parm_table[i].type) {
				case P_BOOL:
				case P_BOOLREV:
					*(BOOL *)dest_ptr = *(BOOL *)src_ptr;
					break;

				case P_INTEGER:
				case P_ENUM:
				case P_OCTAL:
					*(int *)dest_ptr = *(int *)src_ptr;
					break;

				case P_CHAR:
					*(char *)dest_ptr = *(char *)src_ptr;
					break;

				case P_STRING:
					string_set(dest_ptr,
						   *(char **)src_ptr);
					break;

				case P_USTRING:
					string_set(dest_ptr,
						   *(char **)src_ptr);
					strupper(*(char **)dest_ptr);
					break;
				case P_LIST:
					str_list_copy((char ***)dest_ptr, *(const char ***)src_ptr);
					break;
				default:
					break;
			}
		}

	if (bcopyall) {
		init_copymap(pserviceDest);
		if (pserviceSource->copymap)
			memcpy((void *)pserviceDest->copymap,
			       (void *)pserviceSource->copymap,
			       sizeof(BOOL) * NUMPARAMETERS);
	}
}

/***************************************************************************
Check a service for consistency. Return False if the service is in any way
incomplete or faulty, else True.
***************************************************************************/

static BOOL service_ok(int iService)
{
	BOOL bRetval;

	bRetval = True;
	if (ServicePtrs[iService]->szService[0] == '\0') {
		DEBUG(0, ("The following message indicates an internal error:\n"));
		DEBUG(0, ("No service name in service entry.\n"));
		bRetval = False;
	}

	/* The [printers] entry MUST be printable. I'm all for flexibility, but */
	/* I can't see why you'd want a non-printable printer service...        */
	if (strwicmp(ServicePtrs[iService]->szService, PRINTERS_NAME) == 0) {
		if (!ServicePtrs[iService]->bPrint_ok) {
			DEBUG(0, ("WARNING: [%s] service MUST be printable!\n",
			       ServicePtrs[iService]->szService));
			ServicePtrs[iService]->bPrint_ok = True;
		}
		/* [printers] service must also be non-browsable. */
		if (ServicePtrs[iService]->bBrowseable)
			ServicePtrs[iService]->bBrowseable = False;
	}

	if (ServicePtrs[iService]->szPath[0] == '\0' &&
	    strwicmp(ServicePtrs[iService]->szService, HOMES_NAME) != 0) {
		DEBUG(0, ("No path in service %s - using %s\n",
		       ServicePtrs[iService]->szService, tmpdir()));
		string_set(&ServicePtrs[iService]->szPath, tmpdir());
	}

	/* If a service is flagged unavailable, log the fact at level 0. */
	if (!ServicePtrs[iService]->bAvailable)
		DEBUG(1, ("NOTE: Service %s is flagged unavailable.\n",
			  ServicePtrs[iService]->szService));

	return (bRetval);
}

static struct file_lists {
	struct file_lists *next;
	char *name;
	char *subfname;
	time_t modtime;
} *file_lists = NULL;

/*******************************************************************
 Keep a linked list of all config files so we know when one has changed 
 it's date and needs to be reloaded.
********************************************************************/

static void add_to_file_list(const char *fname, const char *subfname)
{
	struct file_lists *f = file_lists;

	while (f) {
		if (f->name && !strcmp(f->name, fname))
			break;
		f = f->next;
	}

	if (!f) {
		f = (struct file_lists *)malloc(sizeof(file_lists[0]));
		if (!f)
			return;
		f->next = file_lists;
		f->name = strdup(fname);
		if (!f->name) {
			SAFE_FREE(f);
			return;
		}
		f->subfname = strdup(subfname);
		if (!f->subfname) {
			SAFE_FREE(f);
			return;
		}
		file_lists = f;
		f->modtime = file_modtime(subfname);
	} else {
		time_t t = file_modtime(subfname);
		if (t)
			f->modtime = t;
	}
}

/*******************************************************************
 Check if a config file has changed date.
********************************************************************/

BOOL lp_file_list_changed(void)
{
	struct file_lists *f = file_lists;
	DEBUG(6, ("lp_file_list_changed()\n"));

	while (f) {
		pstring n2;
		time_t mod_time;

		pstrcpy(n2, f->name);
		standard_sub_basic(current_user_info.smb_name, n2,sizeof(n2));

		DEBUGADD(6, ("file %s -> %s  last mod_time: %s\n",
			     f->name, n2, ctime(&f->modtime)));

		mod_time = file_modtime(n2);

		if (mod_time && ((f->modtime != mod_time) || (f->subfname == NULL) || (strcmp(n2, f->subfname) != 0))) {
			DEBUGADD(6,
				 ("file %s modified: %s\n", n2,
				  ctime(&mod_time)));
			f->modtime = mod_time;
			SAFE_FREE(f->subfname);
			f->subfname = strdup(n2);
			return (True);
		}
		f = f->next;
	}
	return (False);
}

/***************************************************************************
 Run standard_sub_basic on netbios name... needed because global_myname
 is not accessed through any lp_ macro.
 Note: We must *NOT* use string_set() here as ptr points to global_myname.
***************************************************************************/

static BOOL handle_netbios_name(const char *pszParmValue, char **ptr)
{
	BOOL ret;
	pstring netbios_name;

	pstrcpy(netbios_name, pszParmValue);

	standard_sub_basic(current_user_info.smb_name, netbios_name,sizeof(netbios_name));


	ret = set_global_myname(netbios_name);
	string_set(&Globals.szNetbiosName,global_myname());
	
	DEBUG(4, ("handle_netbios_name: set global_myname to: %s\n",
	       global_myname()));

	return ret;
}

static BOOL handle_workgroup(const char *pszParmValue, char **ptr)
{
	BOOL ret;
	
	ret = set_global_myworkgroup(pszParmValue);
	string_set(&Globals.szWorkgroup,lp_workgroup());
	
	return ret;
}

static BOOL handle_netbios_scope(const char *pszParmValue, char **ptr)
{
	BOOL ret;
	
	ret = set_global_scope(pszParmValue);
	string_set(&Globals.szNetbiosScope,global_scope());

	return ret;
}

static BOOL handle_netbios_aliases(const char *pszParmValue, char **ptr)
{
	Globals.szNetbiosAliases = str_list_make(pszParmValue, NULL);
	return set_netbios_aliases((const char **)Globals.szNetbiosAliases);
}

/***************************************************************************
 Do the work of sourcing in environment variable/value pairs.
***************************************************************************/

static BOOL source_env(char **lines)
{
	char *varval;
	size_t len;
	int i;
	char *p;

	for (i = 0; lines[i]; i++) {
		char *line = lines[i];

		if ((len = strlen(line)) == 0)
			continue;

		if (line[len - 1] == '\n')
			line[--len] = '\0';

		if ((varval = malloc(len + 1)) == NULL) {
			DEBUG(0, ("source_env: Not enough memory!\n"));
			return (False);
		}

		DEBUG(4, ("source_env: Adding to environment: %s\n", line));
		strncpy(varval, line, len);
		varval[len] = '\0';

		p = strchr_m(line, (int)'=');
		if (p == NULL) {
			DEBUG(4, ("source_env: missing '=': %s\n", line));
			continue;
		}

		if (putenv(varval)) {
			DEBUG(0, ("source_env: Failed to put environment variable %s\n",
			       varval));
			continue;
		}

		*p = '\0';
		p++;
		DEBUG(4, ("source_env: getting var %s = %s\n", line, getenv(line)));
	}

	DEBUG(4, ("source_env: returning successfully\n"));
	return (True);
}

/***************************************************************************
 Handle the source environment operation.
***************************************************************************/

static BOOL handle_source_env(const char *pszParmValue, char **ptr)
{
	pstring fname;
	char *p = fname;
	BOOL result;
	char **lines;

	pstrcpy(fname, pszParmValue);

	standard_sub_basic(current_user_info.smb_name, fname,sizeof(fname));

	string_set(ptr, pszParmValue);

	DEBUG(4, ("handle_source_env: checking env type\n"));

	/*
	 * Filename starting with '|' means popen and read from stdin.
	 */

	if (*p == '|')
		lines = file_lines_pload(p + 1, NULL);
	else
		lines = file_lines_load(fname, NULL);

	if (!lines) {
		DEBUG(0, ("handle_source_env: Failed to open file %s, Error was %s\n",
		       fname, strerror(errno)));
		return (False);
	}

	result = source_env(lines);
	file_lines_free(lines);

	return (result);
}

/***************************************************************************
 Handle the interpretation of the vfs object parameter.
*************************************************************************/

static BOOL handle_vfs_object(const char *pszParmValue, char **ptr)
{
	/* Set string value */

	string_set(ptr, pszParmValue);

	/* Do any other initialisation required for vfs.  Note that
	   anything done here may have linking repercussions in nmbd. */

	return True;
}

/***************************************************************************
 Handle the include operation.
***************************************************************************/

static BOOL handle_include(const char *pszParmValue, char **ptr)
{
	pstring fname;
	pstrcpy(fname, pszParmValue);

	standard_sub_basic(current_user_info.smb_name, fname,sizeof(fname));

	add_to_file_list(pszParmValue, fname);

	string_set(ptr, fname);

	if (file_exist(fname, NULL))
		return (pm_process(fname, do_section, do_parameter));

	DEBUG(2, ("Can't find include file %s\n", fname));

	return (False);
}

/***************************************************************************
 Handle the interpretation of the copy parameter.
***************************************************************************/

static BOOL handle_copy(const char *pszParmValue, char **ptr)
{
	BOOL bRetval;
	int iTemp;
	service serviceTemp;

	string_set(ptr, pszParmValue);

	init_service(&serviceTemp);

	bRetval = False;

	DEBUG(3, ("Copying service from service %s\n", pszParmValue));

	if ((iTemp = getservicebyname(pszParmValue, &serviceTemp)) >= 0) {
		if (iTemp == iServiceIndex) {
			DEBUG(0, ("Can't copy service %s - unable to copy self!\n", pszParmValue));
		} else {
			copy_service(ServicePtrs[iServiceIndex],
				     &serviceTemp,
				     ServicePtrs[iServiceIndex]->copymap);
			bRetval = True;
		}
	} else {
		DEBUG(0, ("Unable to copy service - source not found: %s\n", pszParmValue));
		bRetval = False;
	}

	free_service(&serviceTemp);
	return (bRetval);
}

/***************************************************************************
 Handle winbind/non unix account uid and gid allocation parameters.  The format of these
 parameters is:

 [global]

        winbind uid = 1000-1999
        winbind gid = 700-899

 We only do simple parsing checks here.  The strings are parsed into useful
 structures in the winbind daemon code.

***************************************************************************/

/* Some lp_ routines to return winbind [ug]id information */

static uid_t winbind_uid_low, winbind_uid_high;
static gid_t winbind_gid_low, winbind_gid_high;
static uint32 non_unix_account_low, non_unix_account_high;

BOOL lp_winbind_uid(uid_t *low, uid_t *high)
{
        if (winbind_uid_low == 0 || winbind_uid_high == 0)
                return False;

        if (low)
                *low = winbind_uid_low;

        if (high)
                *high = winbind_uid_high;

        return True;
}

BOOL lp_winbind_gid(gid_t *low, gid_t *high)
{
        if (winbind_gid_low == 0 || winbind_gid_high == 0)
                return False;

        if (low)
                *low = winbind_gid_low;

        if (high)
                *high = winbind_gid_high;

        return True;
}

BOOL lp_non_unix_account_range(uint32 *low, uint32 *high)
{
        if (non_unix_account_low == 0 || non_unix_account_high == 0)
                return False;

        if (low)
                *low = non_unix_account_low;

        if (high)
                *high = non_unix_account_high;

        return True;
}

/* Do some simple checks on "winbind [ug]id" parameter values */

static BOOL handle_winbind_uid(const char *pszParmValue, char **ptr)
{
	uint32 low, high;

	if (sscanf(pszParmValue, "%u-%u", &low, &high) != 2 || high < low)
		return False;

	/* Parse OK */

	string_set(ptr, pszParmValue);

        winbind_uid_low = low;
        winbind_uid_high = high;

	return True;
}

static BOOL handle_winbind_gid(const char *pszParmValue, char **ptr)
{
	uint32 low, high;

	if (sscanf(pszParmValue, "%u-%u", &low, &high) != 2 || high < low)
		return False;

	/* Parse OK */

	string_set(ptr, pszParmValue);

        winbind_gid_low = low;
        winbind_gid_high = high;

	return True;
}

/***************************************************************************
 Do some simple checks on "non unix account range" parameter values.
***************************************************************************/

static BOOL handle_non_unix_account_range(const char *pszParmValue, char **ptr)
{
	uint32 low, high;

	if (sscanf(pszParmValue, "%u-%u", &low, &high) != 2 || high < low)
		return False;

	/* Parse OK */

	string_set(ptr, pszParmValue);

        non_unix_account_low = low;
        non_unix_account_high = high;

	return True;
}

/***************************************************************************
 Handle the DEBUG level list.
***************************************************************************/

static BOOL handle_debug_list( const char *pszParmValueIn, char **ptr )
{
	pstring pszParmValue;

	pstrcpy(pszParmValue, pszParmValueIn);
	string_set(ptr, pszParmValueIn);
	return debug_parse_levels( pszParmValue );
}

/***************************************************************************
 Handle the ldap machine suffix option.
***************************************************************************/

static BOOL handle_ldap_machine_suffix( const char *pszParmValue, char **ptr)
{
       pstring suffix;
       
       pstrcpy(suffix, pszParmValue);

       if (! *Globals.szLdapSuffix ) {
               string_set( ptr, suffix );
               return True;
       }

       if (! strstr(suffix, Globals.szLdapSuffix) ) {
               if ( *pszParmValue )
                       pstrcat(suffix, ",");
               pstrcat(suffix, Globals.szLdapSuffix);
       }
       string_set( ptr, suffix );
       return True;
}

/***************************************************************************
 Handle the ldap user suffix option.
***************************************************************************/

static BOOL handle_ldap_user_suffix( const char *pszParmValue, char **ptr)
{
       pstring suffix;
       
       pstrcpy(suffix, pszParmValue);

       if (! *Globals.szLdapSuffix ) {
               string_set( ptr, suffix );
               return True;
       }
       
       if (! strstr(suffix, Globals.szLdapSuffix) ) {
               if ( *pszParmValue )
                       pstrcat(suffix, ",");
               pstrcat(suffix, Globals.szLdapSuffix);
       }
       string_set( ptr, suffix );
       return True;
}

/***************************************************************************
 Handle setting ldap suffix and determines whether ldap machine suffix needs
 to be set as well.
***************************************************************************/

static BOOL handle_ldap_suffix( const char *pszParmValue, char **ptr)
{
       pstring suffix;
       pstring user_suffix;
       pstring machine_suffix;
               
       pstrcpy(suffix, pszParmValue);

       if (! *Globals.szLdapMachineSuffix )
               string_set(&Globals.szLdapMachineSuffix, suffix);
       if (! *Globals.szLdapUserSuffix ) 
               string_set(&Globals.szLdapUserSuffix, suffix);
       
       if (! strstr(Globals.szLdapMachineSuffix, suffix)) {
               pstrcpy(machine_suffix, Globals.szLdapMachineSuffix);
               if ( *Globals.szLdapMachineSuffix )
                       pstrcat(machine_suffix, ",");
               pstrcat(machine_suffix, suffix);
               string_set(&Globals.szLdapMachineSuffix, machine_suffix);       
       }

       if (! strstr(Globals.szLdapUserSuffix, suffix)) {
               pstrcpy(user_suffix, Globals.szLdapUserSuffix);
               if ( *Globals.szLdapUserSuffix )
                       pstrcat(user_suffix, ",");
               pstrcat(user_suffix, suffix);   
               string_set(&Globals.szLdapUserSuffix, user_suffix);
       } 

       string_set(ptr, suffix); 
       return True;
}

static BOOL handle_acl_compatibility(const char *pszParmValue, char **ptr)
{
	if (strequal(pszParmValue, "auto"))
		string_set(ptr, "");
	else if (strequal(pszParmValue, "winnt"))
		string_set(ptr, "winnt");
	else if (strequal(pszParmValue, "win2k"))
		string_set(ptr, "win2k");
	else
		return False;

	return True;
}
/***************************************************************************
 Initialise a copymap.
***************************************************************************/

static void init_copymap(service * pservice)
{
	int i;
	SAFE_FREE(pservice->copymap);
	pservice->copymap = (BOOL *)malloc(sizeof(BOOL) * NUMPARAMETERS);
	if (!pservice->copymap)
		DEBUG(0,
		      ("Couldn't allocate copymap!! (size %d)\n",
		       (int)NUMPARAMETERS));
	else
		for (i = 0; i < NUMPARAMETERS; i++)
			pservice->copymap[i] = True;
}

/***************************************************************************
 Return the local pointer to a parameter given the service number and the 
 pointer into the default structure.
***************************************************************************/

void *lp_local_ptr(int snum, void *ptr)
{
	return (void *)(((char *)ServicePtrs[snum]) + PTR_DIFF(ptr, &sDefault));
}

/***************************************************************************
 Process a parameter for a particular service number. If snum < 0
 then assume we are in the globals.
***************************************************************************/

BOOL lp_do_parameter(int snum, const char *pszParmName, const char *pszParmValue)
{
	int parmnum, i, slen;
	void *parm_ptr = NULL;	/* where we are going to store the result */
	void *def_ptr = NULL;
	pstring vfskey;
	char *sep;
	param_opt_struct *paramo;

	parmnum = map_parameter(pszParmName);

	if (parmnum < 0) {
		if ((sep=strchr(pszParmName, ':')) != NULL) {
			*sep = 0;
			ZERO_STRUCT(vfskey);
			pstr_sprintf(vfskey, "%s:%s:", 
				(snum >= 0) ? lp_servicename(snum) : "global", pszParmName);
			slen = strlen(vfskey);
			safe_strcat(vfskey, sep+1, sizeof(pstring));
			trim_string(vfskey+slen, " ", " ");
			paramo = smb_xmalloc(sizeof(param_opt_struct));
			paramo->key = strdup(vfskey);
			paramo->value = strdup(pszParmValue);
			DLIST_ADD(param_opt, paramo);
			*sep = ':';
			return (True);
		}
		DEBUG(0, ("Ignoring unknown parameter \"%s\"\n", pszParmName));
		return (True);
	}

	if (parm_table[parmnum].flags & FLAG_DEPRECATED) {
		DEBUG(1, ("WARNING: The \"%s\" option is deprecated\n",
			  pszParmName));
	}

	def_ptr = parm_table[parmnum].ptr;

	/* we might point at a service, the default service or a global */
	if (snum < 0) {
		parm_ptr = def_ptr;
	} else {
		if (parm_table[parmnum].class == P_GLOBAL) {
			DEBUG(0,
			      ("Global parameter %s found in service section!\n",
			       pszParmName));
			return (True);
		}
		parm_ptr =
			((char *)ServicePtrs[snum]) + PTR_DIFF(def_ptr,
							    &sDefault);
	}

	if (snum >= 0) {
		if (!ServicePtrs[snum]->copymap)
			init_copymap(ServicePtrs[snum]);

		/* this handles the aliases - set the copymap for other entries with
		   the same data pointer */
		for (i = 0; parm_table[i].label; i++)
			if (parm_table[i].ptr == parm_table[parmnum].ptr)
				ServicePtrs[snum]->copymap[i] = False;
	}

	/* if it is a special case then go ahead */
	if (parm_table[parmnum].special) {
		parm_table[parmnum].special(pszParmValue, (char **)parm_ptr);
		return (True);
	}

	/* now switch on the type of variable it is */
	switch (parm_table[parmnum].type)
	{
		case P_BOOL:
			set_boolean(parm_ptr, pszParmValue);
			break;

		case P_BOOLREV:
			set_boolean(parm_ptr, pszParmValue);
			*(BOOL *)parm_ptr = !*(BOOL *)parm_ptr;
			break;

		case P_INTEGER:
			*(int *)parm_ptr = atoi(pszParmValue);
			break;

		case P_CHAR:
			*(char *)parm_ptr = *pszParmValue;
			break;

		case P_OCTAL:
			sscanf(pszParmValue, "%o", (int *)parm_ptr);
			break;

		case P_LIST:
			*(char ***)parm_ptr = str_list_make(pszParmValue, NULL);
			break;

		case P_STRING:
			string_set(parm_ptr, pszParmValue);
			break;

		case P_USTRING:
			string_set(parm_ptr, pszParmValue);
			strupper(*(char **)parm_ptr);
			break;

		case P_GSTRING:
			pstrcpy((char *)parm_ptr, pszParmValue);
			break;

		case P_UGSTRING:
			pstrcpy((char *)parm_ptr, pszParmValue);
			strupper((char *)parm_ptr);
			break;

		case P_ENUM:
			for (i = 0; parm_table[parmnum].enum_list[i].name; i++) {
				if (strequal
				    (pszParmValue,
				     parm_table[parmnum].enum_list[i].name)) {
					*(int *)parm_ptr =
						parm_table[parmnum].
						enum_list[i].value;
					break;
				}
			}
			break;
		case P_SEP:
			break;
	}

	return (True);
}

/***************************************************************************
 Process a parameter.
***************************************************************************/

static BOOL do_parameter(const char *pszParmName, const char *pszParmValue)
{
	if (!bInGlobalSection && bGlobalOnly)
		return (True);

	DEBUGADD(4, ("doing parameter %s = %s\n", pszParmName, pszParmValue));

	return (lp_do_parameter(bInGlobalSection ? -2 : iServiceIndex,
				pszParmName, pszParmValue));
}

/***************************************************************************
 Print a parameter of the specified type.
***************************************************************************/

static void print_parameter(struct parm_struct *p, void *ptr, FILE * f)
{
	int i;
	switch (p->type)
	{
		case P_ENUM:
			for (i = 0; p->enum_list[i].name; i++) {
				if (*(int *)ptr == p->enum_list[i].value) {
					fprintf(f, "%s",
						p->enum_list[i].name);
					break;
				}
			}
			break;

		case P_BOOL:
			fprintf(f, "%s", BOOLSTR(*(BOOL *)ptr));
			break;

		case P_BOOLREV:
			fprintf(f, "%s", BOOLSTR(!*(BOOL *)ptr));
			break;

		case P_INTEGER:
			fprintf(f, "%d", *(int *)ptr);
			break;

		case P_CHAR:
			fprintf(f, "%c", *(char *)ptr);
			break;

		case P_OCTAL:
			fprintf(f, "%s", octal_string(*(int *)ptr));
			break;

		case P_LIST:
			if ((char ***)ptr && *(char ***)ptr) {
				char **list = *(char ***)ptr;
				
				for (; *list; list++)
					fprintf(f, "%s%s", *list,
						((*(list+1))?", ":""));
			}
			break;

		case P_GSTRING:
		case P_UGSTRING:
			if ((char *)ptr) {
				fprintf(f, "%s", (char *)ptr);
			}
			break;

		case P_STRING:
		case P_USTRING:
			if (*(char **)ptr) {
				fprintf(f, "%s", *(char **)ptr);
			}
			break;
		case P_SEP:
			break;
	}
}

/***************************************************************************
 Check if two parameters are equal.
***************************************************************************/

static BOOL equal_parameter(parm_type type, void *ptr1, void *ptr2)
{
	switch (type) {
		case P_BOOL:
		case P_BOOLREV:
			return (*((BOOL *)ptr1) == *((BOOL *)ptr2));

		case P_INTEGER:
		case P_ENUM:
		case P_OCTAL:
			return (*((int *)ptr1) == *((int *)ptr2));

		case P_CHAR:
			return (*((char *)ptr1) == *((char *)ptr2));
		
		case P_LIST:
			return str_list_compare(*(char ***)ptr1, *(char ***)ptr2);

		case P_GSTRING:
		case P_UGSTRING:
		{
			char *p1 = (char *)ptr1, *p2 = (char *)ptr2;
			if (p1 && !*p1)
				p1 = NULL;
			if (p2 && !*p2)
				p2 = NULL;
			return (p1 == p2 || strequal(p1, p2));
		}
		case P_STRING:
		case P_USTRING:
		{
			char *p1 = *(char **)ptr1, *p2 = *(char **)ptr2;
			if (p1 && !*p1)
				p1 = NULL;
			if (p2 && !*p2)
				p2 = NULL;
			return (p1 == p2 || strequal(p1, p2));
		}
		case P_SEP:
			break;
	}
	return (False);
}

/***************************************************************************
 Initialize any local varients in the sDefault table.
***************************************************************************/

void init_locals(void)
{
	/* None as yet. */
}

/***************************************************************************
 Process a new section (service). At this stage all sections are services.
 Later we'll have special sections that permit server parameters to be set.
 Returns True on success, False on failure. 
***************************************************************************/

static BOOL do_section(const char *pszSectionName)
{
	BOOL bRetval;
	BOOL isglobal = ((strwicmp(pszSectionName, GLOBAL_NAME) == 0) ||
			 (strwicmp(pszSectionName, GLOBAL_NAME2) == 0));
	bRetval = False;

	/* if we were in a global section then do the local inits */
	if (bInGlobalSection && !isglobal)
		init_locals();

	/* if we've just struck a global section, note the fact. */
	bInGlobalSection = isglobal;

	/* check for multiple global sections */
	if (bInGlobalSection) {
		DEBUG(3, ("Processing section \"[%s]\"\n", pszSectionName));
		return (True);
	}

	if (!bInGlobalSection && bGlobalOnly)
		return (True);

	/* if we have a current service, tidy it up before moving on */
	bRetval = True;

	if (iServiceIndex >= 0)
		bRetval = service_ok(iServiceIndex);

	/* if all is still well, move to the next record in the services array */
	if (bRetval) {
		/* We put this here to avoid an odd message order if messages are */
		/* issued by the post-processing of a previous section. */
		DEBUG(2, ("Processing section \"[%s]\"\n", pszSectionName));

		if ((iServiceIndex = add_a_service(&sDefault, pszSectionName))
		    < 0) {
			DEBUG(0, ("Failed to add a new service\n"));
			return (False);
		}
	}

	return (bRetval);
}


/***************************************************************************
 Determine if a partcular base parameter is currentl set to the default value.
***************************************************************************/

static BOOL is_default(int i)
{
	if (!defaults_saved)
		return False;
	switch (parm_table[i].type) {
		case P_LIST:
			return str_list_compare (parm_table[i].def.lvalue, 
						*(char ***)parm_table[i].ptr);
		case P_STRING:
		case P_USTRING:
			return strequal(parm_table[i].def.svalue,
					*(char **)parm_table[i].ptr);
		case P_GSTRING:
		case P_UGSTRING:
			return strequal(parm_table[i].def.svalue,
					(char *)parm_table[i].ptr);
		case P_BOOL:
		case P_BOOLREV:
			return parm_table[i].def.bvalue ==
				*(BOOL *)parm_table[i].ptr;
		case P_CHAR:
			return parm_table[i].def.cvalue ==
				*(char *)parm_table[i].ptr;
		case P_INTEGER:
		case P_OCTAL:
		case P_ENUM:
			return parm_table[i].def.ivalue ==
				*(int *)parm_table[i].ptr;
		case P_SEP:
			break;
	}
	return False;
}

/***************************************************************************
Display the contents of the global structure.
***************************************************************************/

static void dump_globals(FILE *f)
{
	int i;
	param_opt_struct *data;
	char *s;
	
	fprintf(f, "# Global parameters\n[global]\n");

	for (i = 0; parm_table[i].label; i++)
		if (parm_table[i].class == P_GLOBAL &&
		    parm_table[i].ptr &&
		    (i == 0 || (parm_table[i].ptr != parm_table[i - 1].ptr))) {
			if (defaults_saved && is_default(i))
				continue;
			fprintf(f, "\t%s = ", parm_table[i].label);
			print_parameter(&parm_table[i], parm_table[i].ptr, f);
			fprintf(f, "\n");
	}
	if (param_opt != NULL) {
		data = param_opt;
		while(data) {
		    if (((s=strstr(data->key, "global")) == data->key) &&
			(*(s+strlen("global")) == ':')) {
			    fprintf(f, "\t%s = %s\n", s+strlen("global")+1, data->value);
		    }
		    data = data->next;
		}
        }

}

/***************************************************************************
 Return True if a local parameter is currently set to the global default.
***************************************************************************/

BOOL lp_is_default(int snum, struct parm_struct *parm)
{
	int pdiff = PTR_DIFF(parm->ptr, &sDefault);

	return equal_parameter(parm->type,
			       ((char *)ServicePtrs[snum]) + pdiff,
			       ((char *)&sDefault) + pdiff);
}

/***************************************************************************
 Display the contents of a single services record.
***************************************************************************/

static void dump_a_service(service * pService, FILE * f)
{
	int i;
	param_opt_struct *data;
	char *s, *sn;
	
	if (pService != &sDefault)
		fprintf(f, "\n[%s]\n", pService->szService);

	for (i = 0; parm_table[i].label; i++)
		if (parm_table[i].class == P_LOCAL &&
		    parm_table[i].ptr &&
		    (*parm_table[i].label != '-') &&
		    (i == 0 || (parm_table[i].ptr != parm_table[i - 1].ptr))) {
			int pdiff = PTR_DIFF(parm_table[i].ptr, &sDefault);

			if (pService == &sDefault) {
				if (defaults_saved && is_default(i))
					continue;
			} else {
				if (equal_parameter(parm_table[i].type,
						    ((char *)pService) +
						    pdiff,
						    ((char *)&sDefault) +
						    pdiff))
					continue;
			}

			fprintf(f, "\t%s = ", parm_table[i].label);
			print_parameter(&parm_table[i],
					((char *)pService) + pdiff, f);
			fprintf(f, "\n");
	}
	if (param_opt != NULL) {
		data = param_opt;
		sn = (pService == &sDefault) ? "global" : pService->szService;
		while(data) {
		    if (((s=strstr(data->key, sn)) == data->key) &&
			(*(s+strlen(sn)) == ':')) {
			    fprintf(f, "\t%s = %s\n", s+strlen(sn)+1, data->value);
		    }
		    data = data->next;
		}
        }
}


/***************************************************************************
 Return info about the next service  in a service. snum==-1 gives the globals.
 Return NULL when out of parameters.
***************************************************************************/

struct parm_struct *lp_next_parameter(int snum, int *i, int allparameters)
{
	if (snum == -1) {
		/* do the globals */
		for (; parm_table[*i].label; (*i)++) {
			if (parm_table[*i].class == P_SEPARATOR)
				return &parm_table[(*i)++];

			if (!parm_table[*i].ptr
			    || (*parm_table[*i].label == '-'))
				continue;

			if ((*i) > 0
			    && (parm_table[*i].ptr ==
				parm_table[(*i) - 1].ptr))
				continue;

			return &parm_table[(*i)++];
		}
	} else {
		service *pService = ServicePtrs[snum];

		for (; parm_table[*i].label; (*i)++) {
			if (parm_table[*i].class == P_SEPARATOR)
				return &parm_table[(*i)++];

			if (parm_table[*i].class == P_LOCAL &&
			    parm_table[*i].ptr &&
			    (*parm_table[*i].label != '-') &&
			    ((*i) == 0 ||
			     (parm_table[*i].ptr !=
			      parm_table[(*i) - 1].ptr)))
			{
				int pdiff =
					PTR_DIFF(parm_table[*i].ptr,
						 &sDefault);

				if (allparameters ||
				    !equal_parameter(parm_table[*i].type,
						     ((char *)pService) +
						     pdiff,
						     ((char *)&sDefault) +
						     pdiff))
				{
					return &parm_table[(*i)++];
				}
			}
		}
	}

	return NULL;
}


#if 0
/***************************************************************************
 Display the contents of a single copy structure.
***************************************************************************/
static void dump_copy_map(BOOL *pcopymap)
{
	int i;
	if (!pcopymap)
		return;

	printf("\n\tNon-Copied parameters:\n");

	for (i = 0; parm_table[i].label; i++)
		if (parm_table[i].class == P_LOCAL &&
		    parm_table[i].ptr && !pcopymap[i] &&
		    (i == 0 || (parm_table[i].ptr != parm_table[i - 1].ptr)))
		{
			printf("\t\t%s\n", parm_table[i].label);
		}
}
#endif

/***************************************************************************
 Return TRUE if the passed service number is within range.
***************************************************************************/

BOOL lp_snum_ok(int iService)
{
	return (LP_SNUM_OK(iService) && ServicePtrs[iService]->bAvailable);
}

/***************************************************************************
 Auto-load some home services.
***************************************************************************/

static void lp_add_auto_services(char *str)
{
	char *s;
	char *p;
	int homes;

	if (!str)
		return;

	s = strdup(str);
	if (!s)
		return;

	homes = lp_servicenumber(HOMES_NAME);

	for (p = strtok(s, LIST_SEP); p; p = strtok(NULL, LIST_SEP)) {
		char *home = get_user_home_dir(p);

		if (lp_servicenumber(p) >= 0)
			continue;

		if (home && homes >= 0)
			lp_add_home(p, homes, p, home);
	}
	SAFE_FREE(s);
}

/***************************************************************************
 Auto-load one printer.
***************************************************************************/

void lp_add_one_printer(char *name, char *comment)
{
	int printers = lp_servicenumber(PRINTERS_NAME);
	int i;

	if (lp_servicenumber(name) < 0) {
		lp_add_printer(name, printers);
		if ((i = lp_servicenumber(name)) >= 0) {
			string_set(&ServicePtrs[i]->comment, comment);
			ServicePtrs[i]->autoloaded = True;
		}
	}
}

/***************************************************************************
 Announce ourselves as a print server.
***************************************************************************/

void update_server_announce_as_printserver(void)
{
	default_server_announce |= SV_TYPE_PRINTQ_SERVER;	
}

/***************************************************************************
 Have we loaded a services file yet?
***************************************************************************/

BOOL lp_loaded(void)
{
	return (bLoaded);
}

/***************************************************************************
 Unload unused services.
***************************************************************************/

void lp_killunused(BOOL (*snumused) (int))
{
	int i;
	for (i = 0; i < iNumServices; i++) {
		if (!VALID(i))
			continue;

		if (!snumused || !snumused(i)) {
			ServicePtrs[i]->valid = False;
			free_service(ServicePtrs[i]);
		}
	}
}

/***************************************************************************
 Unload a service.
***************************************************************************/

void lp_killservice(int iServiceIn)
{
	if (VALID(iServiceIn)) {
		ServicePtrs[iServiceIn]->valid = False;
		free_service(ServicePtrs[iServiceIn]);
	}
}

/***************************************************************************
 Save the curent values of all global and sDefault parameters into the 
 defaults union. This allows swat and testparm to show only the
 changed (ie. non-default) parameters.
***************************************************************************/

static void lp_save_defaults(void)
{
	int i;
	for (i = 0; parm_table[i].label; i++) {
		if (i > 0 && parm_table[i].ptr == parm_table[i - 1].ptr)
			continue;
		switch (parm_table[i].type) {
			case P_LIST:
				str_list_copy(&(parm_table[i].def.lvalue),
					    *(const char ***)parm_table[i].ptr);
				break;
			case P_STRING:
			case P_USTRING:
				if (parm_table[i].ptr) {
					parm_table[i].def.svalue = strdup(*(char **)parm_table[i].ptr);
				} else {
					parm_table[i].def.svalue = NULL;
				}
				break;
			case P_GSTRING:
			case P_UGSTRING:
				if (parm_table[i].ptr) {
					parm_table[i].def.svalue = strdup((char *)parm_table[i].ptr);
				} else {
					parm_table[i].def.svalue = NULL;
				}
				break;
			case P_BOOL:
			case P_BOOLREV:
				parm_table[i].def.bvalue =
					*(BOOL *)parm_table[i].ptr;
				break;
			case P_CHAR:
				parm_table[i].def.cvalue =
					*(char *)parm_table[i].ptr;
				break;
			case P_INTEGER:
			case P_OCTAL:
			case P_ENUM:
				parm_table[i].def.ivalue =
					*(int *)parm_table[i].ptr;
				break;
			case P_SEP:
				break;
		}
	}
	defaults_saved = True;
}

/*******************************************************************
 Set the server type we will announce as via nmbd.
********************************************************************/

static void set_server_role(void)
{
	server_role = ROLE_STANDALONE;

	switch (lp_security()) {
		case SEC_SHARE:
			if (lp_domain_logons())
				DEBUG(0, ("Server's Role (logon server) conflicts with share-level security\n"));
			break;
		case SEC_SERVER:
		case SEC_DOMAIN:
		case SEC_ADS:
			if (lp_domain_logons()) {
				server_role = ROLE_DOMAIN_PDC;
				break;
			}
			server_role = ROLE_DOMAIN_MEMBER;
			break;
		case SEC_USER:
			if (lp_domain_logons()) {

				if (Globals.bDomainMaster) /* auto or yes */ 
					server_role = ROLE_DOMAIN_PDC;
				else
					server_role = ROLE_DOMAIN_BDC;
			}
			break;
		default:
			DEBUG(0, ("Server's Role undefined due to unknown security mode\n"));
			break;
	}

	DEBUG(10, ("set_server_role: role = "));

	switch(server_role) {
	case ROLE_STANDALONE:
		DEBUGADD(10, ("ROLE_STANDALONE\n"));
		break;
	case ROLE_DOMAIN_MEMBER:
		DEBUGADD(10, ("ROLE_DOMAIN_MEMBER\n"));
		break;
	case ROLE_DOMAIN_BDC:
		DEBUGADD(10, ("ROLE_DOMAIN_BDC\n"));
		break;
	case ROLE_DOMAIN_PDC:
		DEBUGADD(10, ("ROLE_DOMAIN_PDC\n"));
		break;
	}
}

/***************************************************************************
 Load the services array from the services file. Return True on success, 
 False on failure.
***************************************************************************/

BOOL lp_load(const char *pszFname, BOOL global_only, BOOL save_defaults,
	     BOOL add_ipc)
{
	pstring n2;
	BOOL bRetval;
	param_opt_struct *data, *pdata;

	pstrcpy(n2, pszFname);
	standard_sub_basic(current_user_info.smb_name, n2,sizeof(n2));

	add_to_file_list(pszFname, n2);

	bRetval = False;

	DEBUG(3, ("lp_load: refreshing parameters\n"));
	
	bInGlobalSection = True;
	bGlobalOnly = global_only;

	init_globals();
	debug_init();

	if (save_defaults)
	{
		init_locals();
		lp_save_defaults();
	}

	if (param_opt != NULL) {
		data = param_opt;
		while (data) {
			SAFE_FREE(data->key);
			SAFE_FREE(data->value);
			pdata = data->next;
			SAFE_FREE(data);
			data = pdata;
		}
		param_opt = NULL;
	}
	
	/* We get sections first, so have to start 'behind' to make up */
	iServiceIndex = -1;
	bRetval = pm_process(n2, do_section, do_parameter);

	/* finish up the last section */
	DEBUG(4, ("pm_process() returned %s\n", BOOLSTR(bRetval)));
	if (bRetval)
		if (iServiceIndex >= 0)
			bRetval = service_ok(iServiceIndex);

	lp_add_auto_services(lp_auto_services());

	if (add_ipc) {
		/* When 'restrict anonymous = 2' guest connections to ipc$
		   are denied */
		lp_add_ipc("IPC$", (lp_restrict_anonymous() < 2));
		lp_add_ipc("ADMIN$", False);
	}

	set_server_role();
	set_default_server_announce_type();

	bLoaded = True;

	/* Now we check bWINSsupport and set szWINSserver to 127.0.0.1 */
	/* if bWINSsupport is true and we are in the client            */
	if (in_client && Globals.bWINSsupport) {
		lp_do_parameter(-1, "wins server", "127.0.0.1");
	}

	init_iconv();

	return (bRetval);
}

/***************************************************************************
 Reset the max number of services.
***************************************************************************/

void lp_resetnumservices(void)
{
	iNumServices = 0;
}

/***************************************************************************
 Return the max number of services.
***************************************************************************/

int lp_numservices(void)
{
	return (iNumServices);
}

/***************************************************************************
Display the contents of the services array in human-readable form.
***************************************************************************/

void lp_dump(FILE *f, BOOL show_defaults, int maxtoprint)
{
	int iService;

	if (show_defaults)
		defaults_saved = False;

	dump_globals(f);

	dump_a_service(&sDefault, f);

	for (iService = 0; iService < maxtoprint; iService++)
		lp_dump_one(f, show_defaults, iService);
}

/***************************************************************************
Display the contents of one service in human-readable form.
***************************************************************************/

void lp_dump_one(FILE * f, BOOL show_defaults, int snum)
{
	if (VALID(snum)) {
		if (ServicePtrs[snum]->szService[0] == '\0')
			return;
		dump_a_service(ServicePtrs[snum], f);
	}
}

/***************************************************************************
Return the number of the service with the given name, or -1 if it doesn't
exist. Note that this is a DIFFERENT ANIMAL from the internal function
getservicebyname()! This works ONLY if all services have been loaded, and
does not copy the found service.
***************************************************************************/

int lp_servicenumber(const char *pszServiceName)
{
	int iService;
        fstring serviceName;
 
 
	for (iService = iNumServices - 1; iService >= 0; iService--) {
		if (VALID(iService) && ServicePtrs[iService]->szService) {
			/*
			 * The substitution here is used to support %U is
			 * service names
			 */
			fstrcpy(serviceName, ServicePtrs[iService]->szService);
			standard_sub_basic(current_user_info.smb_name, serviceName,sizeof(serviceName));
			if (strequal(serviceName, pszServiceName))
				break;
		}
	}

	if (iService < 0)
		DEBUG(7,("lp_servicenumber: couldn't find %s\n", pszServiceName));

	return (iService);
}

/*******************************************************************
 A useful volume label function. 
********************************************************************/

char *volume_label(int snum)
{
	char *ret = lp_volume(snum);
	if (!*ret)
		return lp_servicename(snum);
	return (ret);
}


/*******************************************************************
 Set the server type we will announce as via nmbd.
********************************************************************/

static void set_default_server_announce_type(void)
{
	default_server_announce = 0;
	default_server_announce |= SV_TYPE_WORKSTATION;
	default_server_announce |= SV_TYPE_SERVER;
	default_server_announce |= SV_TYPE_SERVER_UNIX;

	switch (lp_announce_as()) {
		case ANNOUNCE_AS_NT_SERVER:
			default_server_announce |= SV_TYPE_SERVER_NT;
			/* fall through... */
		case ANNOUNCE_AS_NT_WORKSTATION:
			default_server_announce |= SV_TYPE_NT;
			break;
		case ANNOUNCE_AS_WIN95:
			default_server_announce |= SV_TYPE_WIN95_PLUS;
			break;
		case ANNOUNCE_AS_WFW:
			default_server_announce |= SV_TYPE_WFW;
			break;
		default:
			break;
	}

	switch (lp_server_role()) {
		case ROLE_DOMAIN_MEMBER:
			default_server_announce |= SV_TYPE_DOMAIN_MEMBER;
			break;
		case ROLE_DOMAIN_PDC:
			default_server_announce |= SV_TYPE_DOMAIN_CTRL;
			break;
		case ROLE_DOMAIN_BDC:
			default_server_announce |= SV_TYPE_DOMAIN_BAKCTRL;
			break;
		case ROLE_STANDALONE:
		default:
			break;
	}
	if (lp_time_server())
		default_server_announce |= SV_TYPE_TIME_SOURCE;

	if (lp_host_msdfs())
		default_server_announce |= SV_TYPE_DFS_SERVER;
}

/***********************************************************
 returns role of Samba server
************************************************************/

int lp_server_role(void)
{
	return server_role;
}

/***********************************************************
 If we are PDC then prefer us as DMB
************************************************************/

BOOL lp_domain_master(void)
{
	if (Globals.bDomainMaster == Auto)
		return (lp_server_role() == ROLE_DOMAIN_PDC);

	return Globals.bDomainMaster;
}

/***********************************************************
 If we are DMB then prefer us as LMB
************************************************************/

BOOL lp_preferred_master(void)
{
	if (Globals.bPreferredMaster == Auto)
		return (lp_local_master() && lp_domain_master());

	return Globals.bPreferredMaster;
}

/*******************************************************************
 Remove a service.
********************************************************************/

void lp_remove_service(int snum)
{
	ServicePtrs[snum]->valid = False;
}

/*******************************************************************
 Copy a service.
********************************************************************/

void lp_copy_service(int snum, const char *new_name)
{
	char *oldname = lp_servicename(snum);
	do_section(new_name);
	if (snum >= 0) {
		snum = lp_servicenumber(new_name);
		if (snum >= 0)
			lp_do_parameter(snum, "copy", oldname);
	}
}


/*******************************************************************
 Get the default server type we will announce as via nmbd.
********************************************************************/

int lp_default_server_announce(void)
{
	return default_server_announce;
}

/*******************************************************************
 Split the announce version into major and minor numbers.
********************************************************************/

int lp_major_announce_version(void)
{
	static BOOL got_major = False;
	static int major_version = DEFAULT_MAJOR_VERSION;
	char *vers;
	char *p;

	if (got_major)
		return major_version;

	got_major = True;
	if ((vers = lp_announce_version()) == NULL)
		return major_version;

	if ((p = strchr_m(vers, '.')) == 0)
		return major_version;

	*p = '\0';
	major_version = atoi(vers);
	return major_version;
}

int lp_minor_announce_version(void)
{
	static BOOL got_minor = False;
	static int minor_version = DEFAULT_MINOR_VERSION;
	char *vers;
	char *p;

	if (got_minor)
		return minor_version;

	got_minor = True;
	if ((vers = lp_announce_version()) == NULL)
		return minor_version;

	if ((p = strchr_m(vers, '.')) == 0)
		return minor_version;

	p++;
	minor_version = atoi(p);
	return minor_version;
}

/***********************************************************
 Set the global name resolution order (used in smbclient).
************************************************************/

void lp_set_name_resolve_order(char *new_order)
{
	Globals.szNameResolveOrder = new_order;
}

const char *lp_printername(int snum)
{
	const char *ret = _lp_printername(snum);
	if (ret == NULL || (ret != NULL && *ret == '\0'))
		ret = lp_const_servicename(snum);

	return ret;
}


/****************************************************************
 Compatibility fn. for 2.2.2 code.....
*****************************************************************/

void get_private_directory(pstring privdir)
{
	pstrcpy (privdir, lp_private_dir());
}

/***********************************************************
 Allow daemons such as winbindd to fix their logfile name.
************************************************************/

void lp_set_logfile(const char *name)
{
	extern pstring debugf;
	string_set(&Globals.szLogFile, name);
	pstrcpy(debugf, name);
}

/*******************************************************************
 Return the NetBIOS called name.
********************************************************************/

const char *get_called_name(void)
{
	extern fstring local_machine;
	static fstring called_name;

	if (! *local_machine)
		return global_myname();

	/*
	 * Windows NT/2k uses "*SMBSERVER" and XP uses "*SMBSERV"
	 * arrggg!!! but we've already rewritten the client's
	 * netbios name at this point...
	 */

	if (*local_machine) {
		if (!StrCaseCmp(local_machine, "_SMBSERVER") || !StrCaseCmp(local_machine, "_SMBSERV")) {
			fstrcpy(called_name, get_my_primary_ip());
			DEBUG(8,("get_called_name: assuming that client used IP address [%s] as called name.\n",
				called_name));
			return called_name;
		}
	}

	return local_machine;
}

/*******************************************************************
 Return the max print jobs per queue.
********************************************************************/

int lp_maxprintjobs(int snum)
{
	int maxjobs = LP_SNUM_OK(snum) ? ServicePtrs[snum]->iMaxPrintJobs : sDefault.iMaxPrintJobs;
	if (maxjobs <= 0 || maxjobs >= PRINT_MAX_JOBID)
		maxjobs = PRINT_MAX_JOBID - 1;

	return maxjobs;
}