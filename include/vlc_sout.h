/*****************************************************************************
 * stream_output.h : stream output module
 *****************************************************************************
 * Copyright (C) 2002-2005 the VideoLAN team
 * $Id$
 *
 * Authors: Christophe Massiot <massiot@via.ecp.fr>
 *          Laurent Aimar <fenrir@via.ecp.fr>
 *          Eric Petit <titer@videolan.org>
 *          Jean-Paul Saman <jpsaman #_at_# m2x.nl>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#if !defined( __LIBVLC__ )
  #error You are not libvlc or one of its plugins. You cannot include this file
#endif

#ifndef _VLC_SOUT_H_
#define _VLC_SOUT_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <vlc_es.h>

/** Stream output instance */
struct sout_instance_t
{
    VLC_COMMON_MEMBERS

    char *psz_sout;
    char *psz_chain;

    /* meta data (Read only) XXX it won't be set before the first packet received */
    vlc_meta_t          *p_meta;

    /** count of output that can't control the space */
    int                 i_out_pace_nocontrol;

    vlc_mutex_t         lock;
    sout_stream_t       *p_stream;

    /** Private */
    sout_instance_sys_t *p_sys;
};

/****************************************************************************
 * sout_stream_id_t: opaque (private for all sout_stream_t)
 ****************************************************************************/
typedef struct sout_stream_id_t  sout_stream_id_t;

/** Stream output access_output */
struct sout_access_out_t
{
    VLC_COMMON_MEMBERS

    module_t                *p_module;

    sout_instance_t         *p_sout;

    char                    *psz_access;
    config_chain_t              *p_cfg;

    int                      i_writes;
    /** Local counter reset each time it is transferred to stats */
    int64_t                  i_sent_bytes;

    char                    *psz_name;
    sout_access_out_sys_t   *p_sys;
    int                     (*pf_seek)( sout_access_out_t *, off_t );
    int                     (*pf_read)( sout_access_out_t *, block_t * );
    int                     (*pf_write)( sout_access_out_t *, block_t * );
};

VLC_EXPORT( sout_access_out_t *,sout_AccessOutNew, ( sout_instance_t *, char *psz_access, char *psz_name ) );
VLC_EXPORT( void,               sout_AccessOutDelete, ( sout_access_out_t * ) );
VLC_EXPORT( int,                sout_AccessOutSeek,   ( sout_access_out_t *, off_t ) );
VLC_EXPORT( int,                sout_AccessOutRead,   ( sout_access_out_t *, block_t * ) );
VLC_EXPORT( int,                sout_AccessOutWrite,  ( sout_access_out_t *, block_t * ) );

/** Muxer structure */
struct  sout_mux_t
{
    VLC_COMMON_MEMBERS
    module_t            *p_module;

    sout_instance_t     *p_sout;

    char                *psz_mux;
    config_chain_t          *p_cfg;

    sout_access_out_t   *p_access;

    int                 (*pf_addstream)( sout_mux_t *, sout_input_t * );
    int                 (*pf_delstream)( sout_mux_t *, sout_input_t * );
    int                 (*pf_mux)      ( sout_mux_t * );
    int                 (*pf_control)  ( sout_mux_t *, int, va_list );

    /* here are all inputs accepted by muxer */
    int                 i_nb_inputs;
    sout_input_t        **pp_inputs;

    /* mux private */
    sout_mux_sys_t      *p_sys;

    /* XXX private to stream_output.c */
    /* if muxer doesn't support adding stream at any time then we first wait
     *  for stream then we refuse all stream and start muxing */
    vlc_bool_t  b_add_stream_any_time;
    vlc_bool_t  b_waiting_stream;
    /* we wait one second after first stream added */
    mtime_t     i_add_stream_start;
};

enum sout_mux_query_e
{
    /* capabilities */
    MUX_CAN_ADD_STREAM_WHILE_MUXING,    /* arg1= vlc_bool_t *,      res=cannot fail */
    /* properties */
    MUX_GET_ADD_STREAM_WAIT,            /* arg1= vlc_bool_t *,      res=cannot fail */
    MUX_GET_MIME,                       /* arg1= char **            res=can fail    */
};

struct sout_input_t
{
    sout_instance_t *p_sout;

    es_format_t     *p_fmt;
    block_fifo_t    *p_fifo;

    void            *p_sys;
};


VLC_EXPORT( sout_mux_t *,   sout_MuxNew,          ( sout_instance_t*, char *, sout_access_out_t * ) );
VLC_EXPORT( sout_input_t *, sout_MuxAddStream,    ( sout_mux_t *, es_format_t * ) );
VLC_EXPORT( void,           sout_MuxDeleteStream, ( sout_mux_t *, sout_input_t * ) );
VLC_EXPORT( void,           sout_MuxDelete,       ( sout_mux_t * ) );
VLC_EXPORT( void,           sout_MuxSendBuffer, ( sout_mux_t *, sout_input_t  *, block_t * ) );

static inline int sout_MuxControl( sout_mux_t *p_mux, int i_query, ... )
{
    va_list args;
    int     i_result;

    va_start( args, i_query );
    i_result = p_mux->pf_control( p_mux, i_query, args );
    va_end( args );
    return i_result;
}

/****************************************************************************
 * sout_stream:
 ****************************************************************************/
struct sout_stream_t
{
    VLC_COMMON_MEMBERS

    module_t          *p_module;
    sout_instance_t   *p_sout;

    char              *psz_name;
    config_chain_t        *p_cfg;
    char              *psz_next;

    /* Subpicture unit */
    spu_t             *p_spu;

    /* add, remove a stream */
    sout_stream_id_t *(*pf_add)( sout_stream_t *, es_format_t * );
    int               (*pf_del)( sout_stream_t *, sout_stream_id_t * );
    /* manage a packet */
    int               (*pf_send)( sout_stream_t *, sout_stream_id_t *, block_t* );

    /* private */
    sout_stream_sys_t *p_sys;
};

VLC_EXPORT( sout_stream_t *, sout_StreamNew, ( sout_instance_t *, char *psz_chain ) );
VLC_EXPORT( void,            sout_StreamDelete, ( sout_stream_t * ) );

static inline sout_stream_id_t *sout_StreamIdAdd( sout_stream_t *s, es_format_t *fmt )
{
    return s->pf_add( s, fmt );
}
static inline int sout_StreamIdDel( sout_stream_t *s, sout_stream_id_t *id )
{
    return s->pf_del( s, id );
}
static inline int sout_StreamIdSend( sout_stream_t *s, sout_stream_id_t *id, block_t *b )
{
    return s->pf_send( s, id, b );
}

/****************************************************************************
 * Announce handler mess
 ****************************************************************************/
struct sap_session_t;

struct session_descriptor_t
{
    char *psz_name;
    char *psz_uri;
    int i_port;
    int i_payload;   /* SAP Payload type */

    char *psz_group;

    char *psz_sdp;
    vlc_bool_t b_rtp;
};

VLC_EXPORT( int,                sout_AnnounceRegister, (sout_instance_t *,session_descriptor_t*, announce_method_t* ) );
VLC_EXPORT(session_descriptor_t*,sout_AnnounceRegisterSDP, (sout_instance_t *,const char *, const char *, announce_method_t* ) );
VLC_EXPORT( int,                sout_AnnounceUnRegister, (sout_instance_t *,session_descriptor_t* ) );

VLC_EXPORT(session_descriptor_t*,sout_AnnounceSessionCreate, (void) );
VLC_EXPORT(void,                 sout_AnnounceSessionDestroy, (session_descriptor_t *) );
VLC_EXPORT(announce_method_t*,   sout_SAPMethod, (void) );

#ifdef __cplusplus
}
#endif

#endif
