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


/* RFC-todo

 Connects UI peer to RT-PROVIDER peer.

 The UI peer sends the following message using MAILBOX SEND to RT-PROVIDER peer:

    1) uuid/GET/element - Request latest real time measurements of element

    where
        * '/' indicates a multipart _string_ message
        * 'uuid' is unique universal identifier
        * 'element' is name of asset element
        * subject of the message MUST be "latest-rt-data"
        
 The RT-PROVIDER peer MUST respond with one of the following messages:
    
    2) uuid/OK/element/data^i

    where
        * '/' indicates a multipart _frame_ message
        * 'uuid' is string, MUST be repeated from request message 1)
        * 'element' is string, MUST be repeated from request message 1)
        * 'data^i' is anywhere between 0 to N frames, each with encoded bios_proto_t METRIC
            (i.e. one of latest real time measurements of requested element).
            Zero frames mean given element  has no latest measurements or does not exist.
        * subject of the message MUST be repeated from request message 1)

 In case the UI peer sends a message to RT-PROVIDER not conforming to 1), i.e. the message
 has bad format or the subject is incorrect, RT-PROVIDER peer SHALL NOT respond back.
 
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
