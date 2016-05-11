/*  =========================================================================
    rt - agent rt structure

    Copyright (C) 2014 - 2015 Eaton                                        
                                                                           
    This program is free software; you can redistribute it and/or modify   
    it under the terms of the GNU General Public License as published by   
    the Free Software Foundation; either version 2 of the License, or      
    (at your option) any later version.                                    
                                                                           
    This program is distributed in the hope that it will be useful,        
    but WITHOUT ANY WARRANTY; without even the implied warranty of         
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the          
    GNU General Public License for more details.                           
                                                                           
    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.            
    =========================================================================
*/

#ifndef RT_H_INCLUDED
#define RT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _rt_t rt_t;

//  @interface
//  Create a new rt
AGENT_RT_EXPORT rt_t *
    rt_new (void);

//  Store bios_proto_t message and destroy it
AGENT_RT_EXPORT void
    rt_put (rt_t *self, bios_proto_t **msg_p);

//  Read
AGENT_RT_EXPORT bios_proto_t *
    rt_get (rt_t *self, const char *device, const char *metric);

//  Read all
AGENT_RT_EXPORT zhashx_t *
    rt_get_device (rt_t *self, const char *device);

// Purge
AGENT_RT_EXPORT void
    rt_purge (rt_t *self, const char *device, const char *metric);

//  Print
AGENT_RT_EXPORT void
    rt_print (rt_t *self);

//  Destroy the rt
AGENT_RT_EXPORT void
    rt_destroy (rt_t **self_p);

//  Self test of this class
AGENT_RT_EXPORT void
    rt_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
