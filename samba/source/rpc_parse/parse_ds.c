/* 
 *  Unix SMB/CIFS implementation.
 *  RPC Pipe client / server routines
 
 *  Copyright (C) Gerald Carter				2002-2003
 *  
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

#include "includes.h"

/************************************************************************
************************************************************************/

static BOOL ds_io_dominfobasic( const char *desc, prs_struct *ps, int depth, DSROLE_PRIMARY_DOMAIN_INFO_BASIC **basic)
{
	DSROLE_PRIMARY_DOMAIN_INFO_BASIC *p = *basic;
	
	if ( UNMARSHALLING(ps) )
		p = *basic = (DSROLE_PRIMARY_DOMAIN_INFO_BASIC *)prs_alloc_mem(ps, sizeof(DSROLE_PRIMARY_DOMAIN_INFO_BASIC));
		
	if ( !p )
		return False;
		
	if ( !prs_uint16("machine_role", ps, depth, &p->machine_role) )
		return False;
	if ( !prs_uint16("unknown", ps, depth, &p->unknown) )
		return False;

	if ( !prs_uint32("flags", ps, depth, &p->flags) )
		return False;

	if ( !prs_uint32("netbios_ptr", ps, depth, &p->netbios_ptr) )
		return False;
	if ( !prs_uint32("dnsname_ptr", ps, depth, &p->dnsname_ptr) )
		return False;
	if ( !prs_uint32("forestname_ptr", ps, depth, &p->forestname_ptr) )
		return False;
		
	if ( !prs_uint8s(False, "domain_guid", ps, depth, p->domain_guid.info, GUID_SIZE) )
		return False;
		
	if ( !smb_io_unistr2( "netbios_domain", &p->netbios_domain, p->netbios_ptr, ps, depth) )
		return False;
	if ( !prs_align(ps) )
		return False;
	
	if ( !smb_io_unistr2( "dns_domain", &p->dns_domain, p->dnsname_ptr, ps, depth) )
		return False;
	if ( !prs_align(ps) )
		return False;
	
	if ( !smb_io_unistr2( "forest_domain", &p->forest_domain, p->forestname_ptr, ps, depth) )
		return False;
	if ( !prs_align(ps) )
		return False;
	
		
	return True;
		
}

/************************************************************************
************************************************************************/

BOOL ds_io_q_getprimdominfo( const char *desc, prs_struct *ps, int depth, DS_Q_GETPRIMDOMINFO *q_u)
{
	prs_debug(ps, depth, desc, "ds_io_q_getprimdominfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if ( !prs_uint16( "level", ps, depth, &q_u->level ) )
		return False;
		
	return True;
}

/************************************************************************
************************************************************************/

BOOL ds_io_r_getprimdominfo( const char *desc, prs_struct *ps, int depth, DS_R_GETPRIMDOMINFO *r_u)
{
	prs_debug(ps, depth, desc, "ds_io_r_getprimdominfo");
	depth++;

	if(!prs_align(ps))
		return False;

	if ( !prs_uint32( "ptr", ps, depth, &r_u->ptr ) )
		return False;
		
	if ( r_u->ptr )
	{
		if ( !prs_uint16( "level", ps, depth, &r_u->level ) )
			return False;
	
		if ( !prs_uint16( "unknown0", ps, depth, &r_u->unknown0 ) )
			return False;
		
		switch ( r_u->level )
		{
			case DsRolePrimaryDomainInfoBasic:
				if ( !ds_io_dominfobasic( "dominfobasic", ps, depth, &r_u->info.basic ) )
					return False;
				break;
			default:
				return False;
		}
	}

	if ( !prs_align(ps) )
		return False;
	
	if ( !prs_ntstatus("status", ps, depth, &r_u->status ) )
		return False;		
		
	return True;
}

/************************************************************************
 initialize a DS_ENUM_DOM_TRUSTS structure
************************************************************************/

BOOL init_q_ds_enum_domain_trusts( DS_Q_ENUM_DOM_TRUSTS *q, const char *server, 
                                 uint32 flags )
{
	int len;

	q->flags = flags;
	
	if ( server && *server )
		q->server_ptr = 1;
	else
		q->server_ptr = 0;

	len = q->server_ptr ? strlen(server)+1 : 0;

	init_unistr2( &q->server, server, len );
		
	return True;
}

/************************************************************************
************************************************************************/

static BOOL ds_io_domain_trusts( const char *desc, prs_struct *ps, int depth, DS_DOMAIN_TRUSTS *trust)
{
	prs_debug(ps, depth, desc, "ds_io_dom_trusts_ctr");
	depth++;

	if ( !prs_uint32( "netbios_ptr", ps, depth, &trust->netbios_ptr ) )
		return False;
	
	if ( !prs_uint32( "dns_ptr", ps, depth, &trust->dns_ptr ) )
		return False;
	
	if ( !prs_uint32( "flags", ps, depth, &trust->flags ) )
		return False;
	
	if ( !prs_uint32( "parent_index", ps, depth, &trust->parent_index ) )
		return False;
	
	if ( !prs_uint32( "trust_type", ps, depth, &trust->trust_type ) )
		return False;
	
	if ( !prs_uint32( "trust_attributes", ps, depth, &trust->trust_attributes ) )
		return False;
	
	if ( !prs_uint32( "sid_ptr", ps, depth, &trust->sid_ptr ) )
		return False;
	
	if ( !prs_uint8s(False, "guid", ps, depth, trust->guid.info, GUID_SIZE) )
		return False;
	
	return True;	
}

/************************************************************************
************************************************************************/

static BOOL ds_io_dom_trusts_ctr( const char *desc, prs_struct *ps, int depth, DS_DOMAIN_TRUSTS_CTR *ctr)
{
	int i;

	prs_debug(ps, depth, desc, "ds_io_dom_trusts_ctr");
	depth++;
	
	if ( !prs_uint32( "ptr", ps, depth, &ctr->ptr ) )
		return False;
	
	if ( !prs_uint32( "max_count", ps, depth, &ctr->max_count ) )
		return False;
	
	/* are we done? */
	
	if ( ctr->max_count == 0 )
		return True;
	
	/* allocate the domain trusts array are parse it */
	
	ctr->trusts = (DS_DOMAIN_TRUSTS*)talloc(ps->mem_ctx, sizeof(DS_DOMAIN_TRUSTS)*ctr->max_count);
	
	if ( !ctr->trusts )
		return False;
	
	/* this stinks; the static portion o fthe structure is read here and then
	   we need another loop to read the UNISTR2's and SID's */
	   
	for ( i=0; i<ctr->max_count;i++ ) {
		if ( !ds_io_domain_trusts("domain_trusts", ps, depth, &ctr->trusts[i] ) )
			return False;
	}

	for ( i=0; i<ctr->max_count; i++ ) {
	
		if ( !smb_io_unistr2("netbios_domain", &ctr->trusts[i].netbios_domain, ctr->trusts[i].netbios_ptr, ps, depth) )
			return False;

		if(!prs_align(ps))
			return False;
		
		if ( !smb_io_unistr2("dns_domain", &ctr->trusts[i].dns_domain, ctr->trusts[i].dns_ptr, ps, depth) )
			return False;

		if(!prs_align(ps))
			return False;
			
		if ( ctr->trusts[i].sid_ptr ) {
			if ( !smb_io_dom_sid2("sid", &ctr->trusts[i].sid, ps, depth ) )
				return False;		
		}
	}
	
	return True;
}

/************************************************************************
 initialize a DS_ENUM_DOM_TRUSTS request
************************************************************************/

BOOL ds_io_q_enum_domain_trusts( const char *desc, prs_struct *ps, int depth, DS_Q_ENUM_DOM_TRUSTS *q_u)
{
	prs_debug(ps, depth, desc, "ds_io_q_enum_domain_trusts");
	depth++;

	if ( !prs_align(ps) )
		return False;
	
	if ( !prs_uint32( "server_ptr", ps, depth, &q_u->server_ptr ) )
		return False;
	
	if ( !smb_io_unistr2("server", &q_u->server, q_u->server_ptr, ps, depth) )
			return False;
	
	if ( !prs_align(ps) )
		return False;
	
	if ( !prs_uint32( "flags", ps, depth, &q_u->flags ) )
		return False;
	
	return True;
}

/************************************************************************
************************************************************************/

BOOL ds_io_r_enum_domain_trusts( const char *desc, prs_struct *ps, int depth, DS_R_ENUM_DOM_TRUSTS *r_u)
{
	prs_debug(ps, depth, desc, "ds_io_r_enum_domain_trusts");
	depth++;

	if(!prs_align(ps))
		return False;

	if ( !prs_uint32( "num_domains", ps, depth, &r_u->num_domains ) )
		return False;
		
	if ( r_u->num_domains ) {
		if ( !ds_io_dom_trusts_ctr("domains", ps, depth, &r_u->domains ) )
			return False;
	}
		
	if(!prs_align(ps))
		return False;
			
	if ( !prs_ntstatus("status", ps, depth, &r_u->status ) )
		return False;		
		
	return True;
}


