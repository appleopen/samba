/* 
   Unix SMB/CIFS implementation.
   Authentication utility functions
   Copyright (C) Andrew Tridgell 1992-1998
   Copyright (C) Andrew Bartlett 2001
   Copyright (C) Jeremy Allison 2000-2001
   Copyright (C) Rafal Szczesniak 2002

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

#include "includes.h"

#undef DBGC_CLASS
#define DBGC_CLASS DBGC_AUTH

extern DOM_SID global_sid_World;
extern DOM_SID global_sid_Network;
extern DOM_SID global_sid_Builtin_Guests;
extern DOM_SID global_sid_Authenticated_Users;


/****************************************************************************
 Create a UNIX user on demand.
****************************************************************************/

static int smb_create_user(const char *unix_user, const char *homedir)
{
	pstring add_script;
	int ret;

	pstrcpy(add_script, lp_adduser_script());
	if (! *add_script)
		return -1;
	all_string_sub(add_script, "%u", unix_user, sizeof(pstring));
	if (homedir)
		all_string_sub(add_script, "%H", homedir, sizeof(pstring));
	ret = smbrun(add_script,NULL);
	DEBUG(3,("smb_create_user: Running the command `%s' gave %d\n",add_script,ret));
	return ret;
}

/****************************************************************************
 Add and Delete UNIX users on demand, based on NTSTATUS codes.
****************************************************************************/

void smb_user_control(const auth_usersupplied_info *user_info, auth_serversupplied_info *server_info, NTSTATUS nt_status)
{
	struct passwd *pwd=NULL;

	if (NT_STATUS_IS_OK(nt_status)) {

		if (!(server_info->sam_fill_level & SAM_FILL_UNIX)) {
			
			/*
			 * User validated ok against Domain controller.
			 * If the admin wants us to try and create a UNIX
			 * user on the fly, do so.
			 */
			
			if(lp_adduser_script() && !(pwd = Get_Pwnam(user_info->internal_username.str))) {
				smb_create_user(user_info->internal_username.str, NULL);
			}
		}
	}
}

/****************************************************************************
 Create a SAM_ACCOUNT - either by looking in the pdb, or by faking it up from
 unix info.
****************************************************************************/

NTSTATUS auth_get_sam_account(const char *user, SAM_ACCOUNT **account) 
{
	BOOL pdb_ret;
	NTSTATUS nt_status;
	if (!NT_STATUS_IS_OK(nt_status = pdb_init_sam(account))) {
		return nt_status;
	}
	
	become_root();
	pdb_ret = pdb_getsampwnam(*account, user);
	unbecome_root();

	if (!pdb_ret) {
		
		struct passwd *pass = Get_Pwnam(user);
		if (!pass) 
			return NT_STATUS_NO_SUCH_USER;

		if (!NT_STATUS_IS_OK(nt_status = pdb_fill_sam_pw(*account, pass))) {
			return nt_status;
		}
	}
	return NT_STATUS_OK;
}

/****************************************************************************
 Create an auth_usersupplied_data structure
****************************************************************************/

static NTSTATUS make_user_info(auth_usersupplied_info **user_info, 
                               const char *smb_name, 
                               const char *internal_username,
                               const char *client_domain, 
                               const char *domain,
                               const char *wksta_name, 
                               DATA_BLOB lm_pwd, DATA_BLOB nt_pwd,
                               DATA_BLOB plaintext, 
                               uint32 auth_flags, BOOL encrypted)
{

	DEBUG(5,("attempting to make a user_info for %s (%s)\n", internal_username, smb_name));

	*user_info = malloc(sizeof(**user_info));
	if (!user_info) {
		DEBUG(0,("malloc failed for user_info (size %d)\n", sizeof(*user_info)));
		return NT_STATUS_NO_MEMORY;
	}

	ZERO_STRUCTP(*user_info);

	DEBUG(5,("making strings for %s's user_info struct\n", internal_username));

	(*user_info)->smb_name.str = strdup(smb_name);
	if ((*user_info)->smb_name.str) { 
		(*user_info)->smb_name.len = strlen(smb_name);
	} else {
		free_user_info(user_info);
		return NT_STATUS_NO_MEMORY;
	}
	
	(*user_info)->internal_username.str = strdup(internal_username);
	if ((*user_info)->internal_username.str) { 
		(*user_info)->internal_username.len = strlen(internal_username);
	} else {
		free_user_info(user_info);
		return NT_STATUS_NO_MEMORY;
	}

	(*user_info)->domain.str = strdup(domain);
	if ((*user_info)->domain.str) { 
		(*user_info)->domain.len = strlen(domain);
	} else {
		free_user_info(user_info);
		return NT_STATUS_NO_MEMORY;
	}

	(*user_info)->client_domain.str = strdup(client_domain);
	if ((*user_info)->client_domain.str) { 
		(*user_info)->client_domain.len = strlen(client_domain);
	} else {
		free_user_info(user_info);
		return NT_STATUS_NO_MEMORY;
	}

	(*user_info)->wksta_name.str = strdup(wksta_name);
	if ((*user_info)->wksta_name.str) { 
		(*user_info)->wksta_name.len = strlen(wksta_name);
	} else {
		free_user_info(user_info);
		return NT_STATUS_NO_MEMORY;
	}

	DEBUG(5,("making blobs for %s's user_info struct\n", internal_username));

	(*user_info)->lm_resp = data_blob(lm_pwd.data, lm_pwd.length);
	(*user_info)->nt_resp = data_blob(nt_pwd.data, nt_pwd.length);
	(*user_info)->plaintext_password = data_blob(plaintext.data, plaintext.length);

	(*user_info)->encrypted = encrypted;
	(*user_info)->auth_flags = auth_flags;

	DEBUG(10,("made an %sencrypted user_info for %s (%s)\n", encrypted ? "":"un" , internal_username, smb_name));

	return NT_STATUS_OK;
}

/****************************************************************************
 Create an auth_usersupplied_data structure after appropriate mapping.
****************************************************************************/

NTSTATUS make_user_info_map(auth_usersupplied_info **user_info, 
			    const char *smb_name, 
			    const char *client_domain, 
			    const char *wksta_name, 
			    DATA_BLOB lm_pwd, DATA_BLOB nt_pwd,
			    DATA_BLOB plaintext, 
			    uint32 ntlmssp_flags, BOOL encrypted)
{
	const char *domain;
	fstring internal_username;
	fstrcpy(internal_username, smb_name);
	map_username(internal_username); 
	
	DEBUG(5, ("make_user_info_map: Mapping user [%s]\\[%s] from workstation [%s]\n",
	      client_domain, smb_name, wksta_name));
	
	if (lp_allow_trusted_domains() && *client_domain) {

		/* the client could have given us a workstation name
		   or other crap for the workgroup - we really need a
		   way of telling if this domain name is one of our
		   trusted domain names 

		   Also don't allow "" as a domain, fixes a Win9X bug 
		   where it doens't supply a domain for logon script
		   'net use' commands.

		   The way I do it here is by checking if the fully
		   qualified username exists. This is rather reliant
		   on winbind, but until we have a better method this
		   will have to do 
		*/

		domain = client_domain;

		if ((smb_name) && (*smb_name)) { /* Don't do this for guests */
			char *user = NULL;
			if (asprintf(&user, "%s%s%s", 
				 client_domain, lp_winbind_separator(), 
				 smb_name) < 0) {
				DEBUG(0, ("make_user_info_map: asprintf() failed!\n"));
				return NT_STATUS_NO_MEMORY;
			}

			DEBUG(5, ("make_user_info_map: testing for user %s\n", user));
			
			if (Get_Pwnam(user) == NULL) {
				DEBUG(5, ("make_user_info_map: test for user %s failed\n", user));
				domain = lp_workgroup();
				DEBUG(5, ("make_user_info_map: trusted domain %s doesn't appear to exist, using %s\n", 
					  client_domain, domain));
			} else {
				DEBUG(5, ("make_user_info_map: using trusted domain %s\n", domain));
			}
			SAFE_FREE(user);
		}
	} else {
		domain = lp_workgroup();
	}
	
	return make_user_info(user_info, 
			      smb_name, internal_username,
			      client_domain, domain,
			      wksta_name, 
			      lm_pwd, nt_pwd,
			      plaintext, 
			      ntlmssp_flags, encrypted);
	
}

/****************************************************************************
 Create an auth_usersupplied_data, making the DATA_BLOBs here. 
 Decrypt and encrypt the passwords.
****************************************************************************/

BOOL make_user_info_netlogon_network(auth_usersupplied_info **user_info, 
				     const char *smb_name, 
				     const char *client_domain, 
				     const char *wksta_name, 
				     const uchar *lm_network_pwd, int lm_pwd_len,
				     const uchar *nt_network_pwd, int nt_pwd_len)
{
	BOOL ret;
	NTSTATUS nt_status;
	DATA_BLOB lm_blob = data_blob(lm_network_pwd, lm_pwd_len);
	DATA_BLOB nt_blob = data_blob(nt_network_pwd, nt_pwd_len);
	DATA_BLOB plaintext_blob = data_blob(NULL, 0);
	uint32 auth_flags = AUTH_FLAG_NONE;

	if (lm_pwd_len)
		auth_flags |= AUTH_FLAG_LM_RESP;
	if (nt_pwd_len == 24) {
		auth_flags |= AUTH_FLAG_NTLM_RESP; 
	} else if (nt_pwd_len != 0) {
		auth_flags |= AUTH_FLAG_NTLMv2_RESP; 
	}

	nt_status = make_user_info_map(user_info,
	                              smb_name, client_domain, 
                                  wksta_name, 
	                              lm_blob, nt_blob,
	                              plaintext_blob, 
	                              auth_flags, True);
	
	ret = NT_STATUS_IS_OK(nt_status) ? True : False;
		
	data_blob_free(&lm_blob);
	data_blob_free(&nt_blob);
	return ret;
}

/****************************************************************************
 Create an auth_usersupplied_data, making the DATA_BLOBs here. 
 Decrypt and encrypt the passwords.
****************************************************************************/

BOOL make_user_info_netlogon_interactive(auth_usersupplied_info **user_info, 
					 const char *smb_name, 
					 const char *client_domain, 
					 const char *wksta_name, 
					 const uchar chal[8], 
					 const uchar lm_interactive_pwd[16], 
					 const uchar nt_interactive_pwd[16], 
					 const uchar *dc_sess_key)
{
	char lm_pwd[16];
	char nt_pwd[16];
	unsigned char local_lm_response[24];
	unsigned char local_nt_response[24];
	unsigned char key[16];
	uint32 auth_flags = AUTH_FLAG_NONE;
	
	ZERO_STRUCT(key);
	memcpy(key, dc_sess_key, 8);
	
	if (lm_interactive_pwd) memcpy(lm_pwd, lm_interactive_pwd, sizeof(lm_pwd));
	if (nt_interactive_pwd) memcpy(nt_pwd, nt_interactive_pwd, sizeof(nt_pwd));
	
#ifdef DEBUG_PASSWORD
	DEBUG(100,("key:"));
	dump_data(100, (char *)key, sizeof(key));
	
	DEBUG(100,("lm owf password:"));
	dump_data(100, lm_pwd, sizeof(lm_pwd));
	
	DEBUG(100,("nt owf password:"));
	dump_data(100, nt_pwd, sizeof(nt_pwd));
#endif
	
	SamOEMhash((uchar *)lm_pwd, key, sizeof(lm_pwd));
	SamOEMhash((uchar *)nt_pwd, key, sizeof(nt_pwd));
	
#ifdef DEBUG_PASSWORD
	DEBUG(100,("decrypt of lm owf password:"));
	dump_data(100, lm_pwd, sizeof(lm_pwd));
	
	DEBUG(100,("decrypt of nt owf password:"));
	dump_data(100, nt_pwd, sizeof(nt_pwd));
#endif
	
	SMBOWFencrypt((const unsigned char *)lm_pwd, chal, local_lm_response);
	SMBOWFencrypt((const unsigned char *)nt_pwd, chal, local_nt_response);
	
	/* Password info paranoia */
	ZERO_STRUCT(lm_pwd);
	ZERO_STRUCT(nt_pwd);
	ZERO_STRUCT(key);

	{
		BOOL ret;
		NTSTATUS nt_status;
		DATA_BLOB local_lm_blob = data_blob(local_lm_response, sizeof(local_lm_response));
		DATA_BLOB local_nt_blob = data_blob(local_nt_response, sizeof(local_nt_response));
		DATA_BLOB plaintext_blob = data_blob(NULL, 0);

		if (lm_interactive_pwd)
			auth_flags |= AUTH_FLAG_LM_RESP;
		if (nt_interactive_pwd)
			auth_flags |= AUTH_FLAG_NTLM_RESP; 

		nt_status = make_user_info_map(user_info, 
		                               smb_name, client_domain, 
		                               wksta_name, 
		                               local_lm_blob,
		                               local_nt_blob,
		                               plaintext_blob, 
		                               auth_flags, True);
		
		ret = NT_STATUS_IS_OK(nt_status) ? True : False;
		data_blob_free(&local_lm_blob);
		data_blob_free(&local_nt_blob);
		return ret;
	}
}


/****************************************************************************
 Create an auth_usersupplied_data structure
****************************************************************************/

BOOL make_user_info_for_reply(auth_usersupplied_info **user_info, 
			      const char *smb_name, 
			      const char *client_domain,
			      const uint8 chal[8],
			      DATA_BLOB plaintext_password)
{

	DATA_BLOB local_lm_blob;
	DATA_BLOB local_nt_blob;
	NTSTATUS ret = NT_STATUS_UNSUCCESSFUL;
	uint32 auth_flags = AUTH_FLAG_NONE;
			
	/*
	 * Not encrypted - do so.
	 */
	
	DEBUG(5,("make_user_info_for_reply: User passwords not in encrypted format.\n"));
	
	if (plaintext_password.data) {
		unsigned char local_lm_response[24];
		
#ifdef DEBUG_PASSWORD
		DEBUG(10,("Unencrypted password (len %d):\n",plaintext_password.length));
		dump_data(100, plaintext_password.data, plaintext_password.length);
#endif

		SMBencrypt( (const uchar *)plaintext_password.data, (const uchar*)chal, local_lm_response);
		local_lm_blob = data_blob(local_lm_response, 24);
		
		/* We can't do an NT hash here, as the password needs to be
		   case insensitive */
		local_nt_blob = data_blob(NULL, 0); 
		
		auth_flags = (AUTH_FLAG_PLAINTEXT | AUTH_FLAG_LM_RESP);
	} else {
		local_lm_blob = data_blob(NULL, 0); 
		local_nt_blob = data_blob(NULL, 0); 
	}
	
	ret = make_user_info_map(user_info, smb_name,
	                         client_domain, 
	                         get_remote_machine_name(),
	                         local_lm_blob,
	                         local_nt_blob,
	                         plaintext_password, 
	                         auth_flags, False);
	
	data_blob_free(&local_lm_blob);
	return NT_STATUS_IS_OK(ret) ? True : False;
}

/****************************************************************************
 Create an auth_usersupplied_data structure
****************************************************************************/

NTSTATUS make_user_info_for_reply_enc(auth_usersupplied_info **user_info, 
                                      const char *smb_name,
                                      const char *client_domain, 
                                      DATA_BLOB lm_resp, DATA_BLOB nt_resp)
{
	uint32 auth_flags = AUTH_FLAG_NONE;

	DATA_BLOB no_plaintext_blob = data_blob(NULL, 0); 
	
	if (lm_resp.length == 24) {
		auth_flags |= AUTH_FLAG_LM_RESP;
	}
	if (nt_resp.length == 0) {
	} else if (nt_resp.length == 24) {
		auth_flags |= AUTH_FLAG_NTLM_RESP;
	} else {
		auth_flags |= AUTH_FLAG_NTLMv2_RESP;
	}

	return make_user_info_map(user_info, smb_name, 
				 client_domain, 
				 get_remote_machine_name(), 
				 lm_resp, 
				 nt_resp, 
				 no_plaintext_blob, 
				 auth_flags, True);
}

/****************************************************************************
 Create a guest user_info blob, for anonymous authenticaion.
****************************************************************************/

BOOL make_user_info_guest(auth_usersupplied_info **user_info) 
{
	DATA_BLOB lm_blob = data_blob(NULL, 0);
	DATA_BLOB nt_blob = data_blob(NULL, 0);
	DATA_BLOB plaintext_blob = data_blob(NULL, 0);
	uint32 auth_flags = AUTH_FLAG_NONE;
	NTSTATUS nt_status;

	nt_status = make_user_info(user_info, 
			      "","", 
			      "","", 
			      "", 
			      nt_blob, lm_blob,
			      plaintext_blob, 
			      auth_flags, True);
			      
	return NT_STATUS_IS_OK(nt_status) ? True : False;
}

/****************************************************************************
 prints a NT_USER_TOKEN to debug output.
****************************************************************************/

void debug_nt_user_token(int dbg_class, int dbg_lev, NT_USER_TOKEN *token)
{
	fstring sid_str;
	size_t     i;
	
	if (!token) {
		DEBUGC(dbg_class, dbg_lev, ("NT user token: (NULL)\n"));
		return;
	}
	
	DEBUGC(dbg_class, dbg_lev, ("NT user token of user %s\n",
				    sid_to_string(sid_str, &token->user_sids[0]) ));
	DEBUGADDC(dbg_class, dbg_lev, ("contains %i SIDs\n", token->num_sids));
	for (i = 0; i < token->num_sids; i++)
		DEBUGADDC(dbg_class, dbg_lev, ("SID[%3i]: %s\n", i, 
					       sid_to_string(sid_str, &token->user_sids[i])));
}

/****************************************************************************
 prints a UNIX 'token' to debug output.
****************************************************************************/

void debug_unix_user_token(int dbg_class, int dbg_lev, uid_t uid, gid_t gid, int n_groups, gid_t *groups)
{
	int     i;
	DEBUGC(dbg_class, dbg_lev, ("UNIX token of user %ld\n", (long int)uid));

	DEBUGADDC(dbg_class, dbg_lev, ("Primary group is %ld and contains %i supplementary groups\n", (long int)gid, n_groups));
	for (i = 0; i < n_groups; i++)
		DEBUGADDC(dbg_class, dbg_lev, ("Group[%3i]: %ld\n", i, 
			(long int)groups[i]));
}

/****************************************************************************
 Create the SID list for this user.
****************************************************************************/

static NTSTATUS create_nt_user_token(const DOM_SID *user_sid, const DOM_SID *group_sid, 
				     int n_groupSIDs, DOM_SID *groupSIDs, 
				     BOOL is_guest, NT_USER_TOKEN **token)
{
	NTSTATUS       nt_status = NT_STATUS_OK;
	NT_USER_TOKEN *ptoken;
	int i;
	int sid_ndx;
	
	if ((ptoken = malloc( sizeof(NT_USER_TOKEN) ) ) == NULL) {
		DEBUG(0, ("create_nt_token: Out of memory allocating token\n"));
		nt_status = NT_STATUS_NO_MEMORY;
		return nt_status;
	}

	ZERO_STRUCTP(ptoken);

	ptoken->num_sids = n_groupSIDs + 5;

	if ((ptoken->user_sids = (DOM_SID *)malloc( sizeof(DOM_SID) * ptoken->num_sids )) == NULL) {
		DEBUG(0, ("create_nt_token: Out of memory allocating SIDs\n"));
		nt_status = NT_STATUS_NO_MEMORY;
		return nt_status;
	}
	
	memset((char*)ptoken->user_sids,0,sizeof(DOM_SID) * ptoken->num_sids);
	
	/*
	 * Note - user SID *MUST* be first in token !
	 * se_access_check depends on this.
	 *
	 * Primary group SID is second in token. Convention.
	 */

	sid_copy(&ptoken->user_sids[PRIMARY_USER_SID_INDEX], user_sid);
	if (group_sid)
		sid_copy(&ptoken->user_sids[PRIMARY_GROUP_SID_INDEX], group_sid);

	/*
	 * Finally add the "standard" SIDs.
	 * The only difference between guest and "anonymous" (which we
	 * don't really support) is the addition of Authenticated_Users.
	 */

	sid_copy(&ptoken->user_sids[2], &global_sid_World);
	sid_copy(&ptoken->user_sids[3], &global_sid_Network);

	if (is_guest)
		sid_copy(&ptoken->user_sids[4], &global_sid_Builtin_Guests);
	else
		sid_copy(&ptoken->user_sids[4], &global_sid_Authenticated_Users);
	
	sid_ndx = 5; /* next available spot */

	for (i = 0; i < n_groupSIDs; i++) {
		size_t check_sid_idx;
		for (check_sid_idx = 1; check_sid_idx < ptoken->num_sids; check_sid_idx++) {
			if (sid_equal(&ptoken->user_sids[check_sid_idx], 
				      &groupSIDs[i])) {
				break;
			}
		}
		
		if (check_sid_idx >= ptoken->num_sids) /* Not found already */ {
			sid_copy(&ptoken->user_sids[sid_ndx++], &groupSIDs[i]);
		} else {
			ptoken->num_sids--;
		}
	}
	
	debug_nt_user_token(DBGC_AUTH, 10, ptoken);
	
	*token = ptoken;

	return nt_status;
}

/****************************************************************************
 Create the SID list for this user.
****************************************************************************/

NT_USER_TOKEN *create_nt_token(uid_t uid, gid_t gid, int ngroups, gid_t *groups, BOOL is_guest)
{
	DOM_SID user_sid;
	DOM_SID group_sid;
	DOM_SID *group_sids;
	NT_USER_TOKEN *token;
	int i;

	if (!uid_to_sid(&user_sid, uid)) {
		return NULL;
	}
	if (!gid_to_sid(&group_sid, gid)) {
		return NULL;
	}

	group_sids   = malloc(sizeof(DOM_SID) * ngroups);
	if (!group_sids) {
		DEBUG(0, ("create_nt_token: malloc() failed for DOM_SID list!\n"));
		return NULL;
	}

	for (i = 0; i < ngroups; i++) {
		if (!gid_to_sid(&(group_sids)[i], (groups)[i])) {
			DEBUG(1, ("create_nt_token: failed to convert gid %ld to a sid!\n", (long int)groups[i]));
			SAFE_FREE(group_sids);
			return NULL;
		}
	}

	if (!NT_STATUS_IS_OK(create_nt_user_token(&user_sid, &group_sid, 
						  ngroups, group_sids, is_guest, &token))) {
		SAFE_FREE(group_sids);
		return NULL;
	}

	SAFE_FREE(group_sids);

	return token;
}

/******************************************************************************
 * this function returns the groups (SIDs) of the local SAM the user is in.
 * If this samba server is a DC of the domain the user belongs to, it returns 
 * both domain groups and local / builtin groups. If the user is in a trusted
 * domain, or samba is a member server of a domain, then this function returns
 * local and builtin groups the user is a member of. 
 *
 * currently this is a hack, as there is no sam implementation that is capable
 * of groups.
 ******************************************************************************/

static NTSTATUS get_user_groups_from_local_sam(SAM_ACCOUNT *sampass,
					       int *n_groups, DOM_SID **groups,	gid_t **unix_groups)
{
	uid_t             uid;
	gid_t             gid;
	int               n_unix_groups;
	int               i;
	struct passwd    *usr;	

	*n_groups = 0;
	*groups   = NULL;

	if (!IS_SAM_UNIX_USER(sampass)) {
		DEBUG(1, ("user %s does not have a unix identity!\n", pdb_get_username(sampass)));
		return NT_STATUS_NO_SUCH_USER;
	}

	uid = pdb_get_uid(sampass);
	gid = pdb_get_gid(sampass);
	
	n_unix_groups = groups_max();
	if ((*unix_groups = malloc( sizeof(gid_t) * n_unix_groups ) ) == NULL) {
		DEBUG(0, ("get_user_groups_from_local_sam: Out of memory allocating unix group list\n"));
		passwd_free(&usr);
		return NT_STATUS_NO_MEMORY;
	}
	
	if (sys_getgrouplist(pdb_get_username(sampass), gid, *unix_groups, &n_unix_groups) == -1) {
		gid_t *groups_tmp;
		groups_tmp = Realloc(*unix_groups, sizeof(gid_t) * n_unix_groups);
		if (!groups_tmp) {
			SAFE_FREE(*unix_groups);
			passwd_free(&usr);
			return NT_STATUS_NO_MEMORY;
		}
		*unix_groups = groups_tmp;

		if (sys_getgrouplist(pdb_get_username(sampass), gid, *unix_groups, &n_unix_groups) == -1) {
			DEBUG(0, ("get_user_groups_from_local_sam: failed to get the unix group list\n"));
			SAFE_FREE(*unix_groups);
			passwd_free(&usr);
			return NT_STATUS_NO_SUCH_USER; /* what should this return value be? */
		}
	}

	debug_unix_user_token(DBGC_CLASS, 5, uid, gid, n_unix_groups, *unix_groups);
	
	if (n_unix_groups > 0) {
		*groups   = malloc(sizeof(DOM_SID) * n_unix_groups);
		if (!*groups) {
			DEBUG(0, ("get_user_group_from_local_sam: malloc() failed for DOM_SID list!\n"));
			SAFE_FREE(*unix_groups);
			return NT_STATUS_NO_MEMORY;
		}
	}

	*n_groups = n_unix_groups;

	for (i = 0; i < *n_groups; i++) {
		if (!gid_to_sid(&(*groups)[i], (*unix_groups)[i])) {
			DEBUG(1, ("get_user_groups_from_local_sam: failed to convert gid %ld to a sid!\n", (long int)(*unix_groups)[i+1]));
			SAFE_FREE(*groups);
			SAFE_FREE(*unix_groups);
			return NT_STATUS_NO_SUCH_USER;
		}
	}
		     
	return NT_STATUS_OK;
}

/***************************************************************************
 Make a user_info struct
***************************************************************************/

static NTSTATUS make_server_info(auth_serversupplied_info **server_info, SAM_ACCOUNT *sampass)
{
	*server_info = malloc(sizeof(**server_info));
	if (!*server_info) {
		DEBUG(0,("make_server_info: malloc failed!\n"));
		return NT_STATUS_NO_MEMORY;
	}
	ZERO_STRUCTP(*server_info);

	(*server_info)->sam_fill_level = SAM_FILL_ALL;
	(*server_info)->sam_account    = sampass;

	return NT_STATUS_OK;
}

/***************************************************************************
 Make (and fill) a user_info struct from a SAM_ACCOUNT
***************************************************************************/

NTSTATUS make_server_info_sam(auth_serversupplied_info **server_info, 
			      SAM_ACCOUNT *sampass)
{
	NTSTATUS nt_status = NT_STATUS_OK;
	const DOM_SID *user_sid = pdb_get_user_sid(sampass);
	const DOM_SID *group_sid = pdb_get_group_sid(sampass);
	int       n_groupSIDs = 0;
	DOM_SID  *groupSIDs   = NULL;
	gid_t    *unix_groups = NULL;
	NT_USER_TOKEN *token;
	BOOL is_guest;
	uint32 rid;

	if (!NT_STATUS_IS_OK(nt_status = make_server_info(server_info, sampass))) {
		return nt_status;
	}
	
	if (!NT_STATUS_IS_OK(nt_status 
			     = get_user_groups_from_local_sam(sampass, 
		&n_groupSIDs, &groupSIDs, &unix_groups)))
	{
		DEBUG(4,("get_user_groups_from_local_sam failed\n"));
		free_server_info(server_info);
		return nt_status;
	}
	
	is_guest = (sid_peek_rid(user_sid, &rid) && rid == DOMAIN_USER_RID_GUEST);

	if (!NT_STATUS_IS_OK(nt_status = create_nt_user_token(user_sid, group_sid,
							      n_groupSIDs, groupSIDs, is_guest, 
							      &token)))
	{
		DEBUG(4,("create_nt_user_token failed\n"));
		SAFE_FREE(groupSIDs);
		SAFE_FREE(unix_groups);
		free_server_info(server_info);
		return nt_status;
	}
	
	SAFE_FREE(groupSIDs);

	(*server_info)->n_groups = n_groupSIDs;
	(*server_info)->groups = unix_groups;

	(*server_info)->ptok = token;
	
	DEBUG(5,("make_server_info_sam: made server info for user %s\n",
		 pdb_get_username((*server_info)->sam_account)));

	return nt_status;
}

/***************************************************************************
 Make (and fill) a user_info struct from a 'struct passwd' by conversion 
 to a SAM_ACCOUNT
***************************************************************************/

NTSTATUS make_server_info_pw(auth_serversupplied_info **server_info, const struct passwd *pwd)
{
	NTSTATUS nt_status;
	SAM_ACCOUNT *sampass = NULL;
	if (!NT_STATUS_IS_OK(nt_status = pdb_init_sam_pw(&sampass, pwd))) {		
		return nt_status;
	}
	return make_server_info_sam(server_info, sampass);
}

/***************************************************************************
 Make (and fill) a user_info struct for a guest login.
***************************************************************************/

NTSTATUS make_server_info_guest(auth_serversupplied_info **server_info)
{
	NTSTATUS nt_status;
	SAM_ACCOUNT *sampass = NULL;
	DOM_SID guest_sid;

	if (!NT_STATUS_IS_OK(nt_status = pdb_init_sam(&sampass))) {
		return nt_status;
	}

	sid_copy(&guest_sid, get_global_sam_sid());
	sid_append_rid(&guest_sid, DOMAIN_USER_RID_GUEST);

	become_root();
	if (!pdb_getsampwsid(sampass, &guest_sid)) {
		unbecome_root();
		return NT_STATUS_NO_SUCH_USER;
	}
	unbecome_root();

	nt_status = make_server_info_sam(server_info, sampass);

	if (NT_STATUS_IS_OK(nt_status)) {
		(*server_info)->guest = True;
	}

	return nt_status;
}

/***************************************************************************
 Make a server_info struct from the info3 returned by a domain logon 
***************************************************************************/

NTSTATUS make_server_info_info3(TALLOC_CTX *mem_ctx, 
				const char *internal_username,
				const char *sent_nt_username,
				const char *domain,
				auth_serversupplied_info **server_info, 
				NET_USER_INFO_3 *info3) 
{
	NTSTATUS nt_status = NT_STATUS_OK;

	const char *nt_domain;
	const char *nt_username;

	SAM_ACCOUNT *sam_account = NULL;
	DOM_SID user_sid;
	DOM_SID group_sid;

	struct passwd *passwd;

	uid_t uid;
	gid_t gid;

	int n_lgroupSIDs;
	DOM_SID *lgroupSIDs   = NULL;

	gid_t *unix_groups = NULL;
	NT_USER_TOKEN *token;

	DOM_SID *all_group_SIDs;
	size_t i;

	/* 
	   Here is where we should check the list of
	   trusted domains, and verify that the SID 
	   matches.
	*/

	sid_copy(&user_sid, &info3->dom_sid.sid);
	if (!sid_append_rid(&user_sid, info3->user_rid)) {
		return NT_STATUS_INVALID_PARAMETER;
	}
	
	sid_copy(&group_sid, &info3->dom_sid.sid);
	if (!sid_append_rid(&group_sid, info3->group_rid)) {
		return NT_STATUS_INVALID_PARAMETER;
	}

	if (!(nt_username = unistr2_tdup(mem_ctx, &(info3->uni_user_name)))) {
		/* If the server didn't give us one, just use the one we sent them */
		nt_username = sent_nt_username;
	}

	if (!(nt_domain = unistr2_tdup(mem_ctx, &(info3->uni_logon_dom)))) {
		/* If the server didn't give us one, just use the one we sent them */
		domain = domain;
	}

	if (winbind_sid_to_uid(&uid, &user_sid) 
	    && winbind_sid_to_gid(&gid, &group_sid) 
	    && ((passwd = getpwuid_alloc(uid)))) {
		nt_status = pdb_init_sam_pw(&sam_account, passwd);
		passwd_free(&passwd);
	} else {
		char *dom_user;
		dom_user = talloc_asprintf(mem_ctx, "%s%s%s", 
					   nt_domain,
					   lp_winbind_separator(),
					   internal_username);
		
		if (!dom_user) {
			DEBUG(0, ("talloc_asprintf failed!\n"));
			return NT_STATUS_NO_MEMORY;
		} else { 
		
			if (!(passwd = Get_Pwnam(dom_user))
				/* Only lookup local for the local
				   domain, we don't want this for
				   trusted domains */
			    && strequal(nt_domain, lp_workgroup())) {
				passwd = Get_Pwnam(internal_username);
			}
			    
			if (!passwd) {
				return NT_STATUS_NO_SUCH_USER;
			} else {
				nt_status = pdb_init_sam_pw(&sam_account, passwd);
			}
		}
	}
	
	if (!NT_STATUS_IS_OK(nt_status)) {
		DEBUG(0, ("make_server_info_info3: pdb_init_sam failed!\n"));
		return nt_status;
	}
		
	if (!pdb_set_user_sid(sam_account, &user_sid, PDB_CHANGED)) {
		pdb_free_sam(&sam_account);
		return NT_STATUS_UNSUCCESSFUL;
	}

	if (!pdb_set_group_sid(sam_account, &group_sid, PDB_CHANGED)) {
		pdb_free_sam(&sam_account);
		return NT_STATUS_UNSUCCESSFUL;
	}
		
	if (!pdb_set_nt_username(sam_account, nt_username, PDB_CHANGED)) {
		pdb_free_sam(&sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_domain(sam_account, nt_domain, PDB_CHANGED)) {
		pdb_free_sam(&sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_fullname(sam_account, unistr2_static(&(info3->uni_full_name)), PDB_CHANGED)) {
		pdb_free_sam(&sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_logon_script(sam_account, unistr2_static(&(info3->uni_logon_script)), PDB_CHANGED)) {
		pdb_free_sam(&sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_profile_path(sam_account, unistr2_static(&(info3->uni_profile_path)), PDB_CHANGED)) {
		pdb_free_sam(&sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_homedir(sam_account, unistr2_static(&(info3->uni_home_dir)), PDB_CHANGED)) {
		pdb_free_sam(&sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!pdb_set_dir_drive(sam_account, unistr2_static(&(info3->uni_dir_drive)), PDB_CHANGED)) {
		pdb_free_sam(&sam_account);
		return NT_STATUS_NO_MEMORY;
	}

	if (!NT_STATUS_IS_OK(nt_status = make_server_info(server_info, sam_account))) {
		DEBUG(4, ("make_server_info failed!\n"));
		pdb_free_sam(&sam_account);
		return nt_status;
	}

	/* Store the user group information in the server_info 
	   returned to the caller. */
	
	if (!NT_STATUS_IS_OK(nt_status 
			     = get_user_groups_from_local_sam(sam_account, 
							      &n_lgroupSIDs, 
							      &lgroupSIDs, 
							      &unix_groups)))
	{
		DEBUG(4,("get_user_groups_from_local_sam failed\n"));
		return nt_status;
	}

	(*server_info)->groups = unix_groups;
	(*server_info)->n_groups = n_lgroupSIDs;
	
	/* Create a 'combined' list of all SIDs we might want in the SD */
	all_group_SIDs   = malloc(sizeof(DOM_SID) * 
				  (n_lgroupSIDs + info3->num_groups2 +
				   info3->num_other_sids));
	if (!all_group_SIDs) {
		DEBUG(0, ("create_nt_token_info3: malloc() failed for DOM_SID list!\n"));
		SAFE_FREE(lgroupSIDs);
		return NT_STATUS_NO_MEMORY;
	}

	/* Copy the 'local' sids */
	memcpy(all_group_SIDs, lgroupSIDs, sizeof(DOM_SID) * n_lgroupSIDs);
	SAFE_FREE(lgroupSIDs);

	/* and create (by appending rids) the 'domain' sids */
	for (i = 0; i < info3->num_groups2; i++) {
		sid_copy(&all_group_SIDs[i+n_lgroupSIDs], &(info3->dom_sid.sid));
		if (!sid_append_rid(&all_group_SIDs[i+n_lgroupSIDs], info3->gids[i].g_rid)) {
			nt_status = NT_STATUS_INVALID_PARAMETER;
			DEBUG(3,("create_nt_token_info3: could not append additional group rid 0x%x\n",
				info3->gids[i].g_rid));			
			SAFE_FREE(lgroupSIDs);
			return nt_status;
		}
	}

	/* Copy 'other' sids.  We need to do sid filtering here to
 	   prevent possible elevation of privileges.  See:

           http://www.microsoft.com/windows2000/techinfo/administration/security/sidfilter.asp
         */

	for (i = 0; i < info3->num_other_sids; i++) 
		sid_copy(&all_group_SIDs[
				 n_lgroupSIDs + info3->num_groups2 + i],
			 &info3->other_sids[i].sid);
	
	/* Where are the 'global' sids... */

	/* can the user be guest? if yes, where is it stored? */
	if (!NT_STATUS_IS_OK(
		    nt_status = create_nt_user_token(
			    &user_sid, &group_sid,
			    n_lgroupSIDs + info3->num_groups2 + info3->num_other_sids, 
			    all_group_SIDs, False, &token))) {
		DEBUG(4,("create_nt_user_token failed\n"));
		SAFE_FREE(all_group_SIDs);
		return nt_status;
	}

	(*server_info)->ptok = token; 

	SAFE_FREE(all_group_SIDs);
	
	return NT_STATUS_OK;
}

/***************************************************************************
 Free a user_info struct
***************************************************************************/

void free_user_info(auth_usersupplied_info **user_info)
{
	DEBUG(5,("attempting to free (and zero) a user_info structure\n"));
	if (*user_info != NULL) {
		if ((*user_info)->smb_name.str) {
			DEBUG(10,("structure was created for %s\n", (*user_info)->smb_name.str));
		}
		SAFE_FREE((*user_info)->smb_name.str);
		SAFE_FREE((*user_info)->internal_username.str);
		SAFE_FREE((*user_info)->client_domain.str);
		SAFE_FREE((*user_info)->domain.str);
		SAFE_FREE((*user_info)->wksta_name.str);
		data_blob_free(&(*user_info)->lm_resp);
		data_blob_free(&(*user_info)->nt_resp);
		SAFE_FREE((*user_info)->interactive_password);
		data_blob_clear_free(&(*user_info)->plaintext_password);
		ZERO_STRUCT(**user_info);
	}
	SAFE_FREE(*user_info);
}

/***************************************************************************
 Clear out a server_info struct that has been allocated
***************************************************************************/

void free_server_info(auth_serversupplied_info **server_info)
{
	DEBUG(5,("attempting to free (and zero) a server_info structure\n"));
	if (*server_info != NULL) {
		pdb_free_sam(&(*server_info)->sam_account);

		/* call pam_end here, unless we know we are keeping it */
		delete_nt_token( &(*server_info)->ptok );
		SAFE_FREE((*server_info)->groups);
		ZERO_STRUCT(**server_info);
	}
	SAFE_FREE(*server_info);
}

/***************************************************************************
 Make an auth_methods struct
***************************************************************************/

BOOL make_auth_methods(struct auth_context *auth_context, auth_methods **auth_method) 
{
	if (!auth_context) {
		smb_panic("no auth_context supplied to make_auth_methods()!\n");
	}

	if (!auth_method) {
		smb_panic("make_auth_methods: pointer to auth_method pointer is NULL!\n");
	}

	*auth_method = talloc(auth_context->mem_ctx, sizeof(**auth_method));
	if (!*auth_method) {
		DEBUG(0,("make_auth_method: malloc failed!\n"));
		return False;
	}
	ZERO_STRUCTP(*auth_method);
	
	return True;
}

/****************************************************************************
 Delete a SID token.
****************************************************************************/

void delete_nt_token(NT_USER_TOKEN **pptoken)
{
    if (*pptoken) {
	    NT_USER_TOKEN *ptoken = *pptoken;
	    SAFE_FREE( ptoken->user_sids );
	    ZERO_STRUCTP(ptoken);
    }
    SAFE_FREE(*pptoken);
}

/****************************************************************************
 Duplicate a SID token.
****************************************************************************/

NT_USER_TOKEN *dup_nt_token(NT_USER_TOKEN *ptoken)
{
	NT_USER_TOKEN *token;

	if (!ptoken)
		return NULL;

    if ((token = (NT_USER_TOKEN *)malloc( sizeof(NT_USER_TOKEN) ) ) == NULL)
        return NULL;

    ZERO_STRUCTP(token);

    if ((token->user_sids = (DOM_SID *)memdup( ptoken->user_sids, sizeof(DOM_SID) * ptoken->num_sids )) == NULL) {
        SAFE_FREE(token);
        return NULL;
    }

    token->num_sids = ptoken->num_sids;

	return token;
}

/**
 * Squash an NT_STATUS in line with security requirements.
 * In an attempt to avoid giving the whole game away when users
 * are authenticating, NT replaces both NT_STATUS_NO_SUCH_USER and 
 * NT_STATUS_WRONG_PASSWORD with NT_STATUS_LOGON_FAILURE in certain situations 
 * (session setups in particular).
 *
 * @param nt_status NTSTATUS input for squashing.
 * @return the 'squashed' nt_status
 **/

NTSTATUS nt_status_squash(NTSTATUS nt_status)
{
	if NT_STATUS_IS_OK(nt_status) {
		return nt_status;		
	} else if NT_STATUS_EQUAL(nt_status, NT_STATUS_NO_SUCH_USER) {
		/* Match WinXP and don't give the game away */
		return NT_STATUS_LOGON_FAILURE;
		
	} else if NT_STATUS_EQUAL(nt_status, NT_STATUS_WRONG_PASSWORD) {
		/* Match WinXP and don't give the game away */
		return NT_STATUS_LOGON_FAILURE;
	} else {
		return nt_status;
	}  
}


