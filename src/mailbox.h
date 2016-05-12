/*  =========================================================================
    mailbox - mailbox deliver

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

#ifndef MAILBOX_H_INCLUDED
#define MAILBOX_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _mailbox_t mailbox_t;

//  @interface

/*
 * Work in Progress
 *
 * sender sends one of the following multipart string messages
 *  1) GET/element
 *
 * agent-rt replies with the following multipart frame message
 *  2) OK/frame  
 *      where 'frame' == zhashx_pack ()
 *
 * TODO: subject, GET/element/type, 
 */

//  Perform mailbox deliver protocol
AGENT_RT_EXPORT void
    mailbox_perform (mlm_client_t *client, zmsg_t **msg_p, rt_t *data);

//  Self test of this class
AGENT_RT_EXPORT void
    mailbox_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
