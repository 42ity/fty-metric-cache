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

/*
@header
    mailbox - mailbox deliver
@discuss
@end
*/

#include "agent_rt_classes.h"

#define RFC_RT_DATA_SUBJECT "latest-rt-data"

//  --------------------------------------------------------------------------
//  Perform mailbox deliver protocol
void
mailbox_perform (mlm_client_t *client, zmsg_t **msg_p, rt_t *data)
{
    assert (client);
    assert (msg_p);
    assert (data);

    if (!*msg_p)
        return;
    zmsg_t *msg = *msg_p;
    
    // check subject
    if (!streq (mlm_client_subject (client), RFC_RT_DATA_SUBJECT)) {
        zmsg_destroy (msg_p);
        log_warning ("Message with bad subject received. Sender: '%s', Subject: '%s'.",
                mlm_client_sender (client), mlm_client_subject (client));
        return;
    }
    // check uuid 
    char *uuid = zmsg_popstr (msg);
    if (!uuid) {
        zmsg_destroy (msg_p);
        log_warning (
            "Bad message. Expected multipart string message `uuid/GET/element`"
            " - 'uuid' field missing. Sender: '%s', Subject: '%s'.",
                mlm_client_sender (client), mlm_client_subject (client));
        return;
    }
    // check 'GET' command
    char *command = zmsg_popstr (msg);
    if (!command || !streq (command, "GET")) {
        zmsg_destroy (msg_p);
        zstr_free (&command);        
        zstr_free (&uuid);
        log_warning (
            "Bad message. Expected multipart string message `uuid/GET/element`"
            " - 'GET' missing or different string. Sender: '%s', Subject: '%s'.",
                mlm_client_sender (client), mlm_client_subject (client));
        return;
    }
    zstr_free (&command);
    // check element
    char *element = zmsg_popstr (msg);
    if (!element) {
        zmsg_destroy (msg_p);
        zstr_free (&uuid);
        log_warning (
            "Bad message. Expected multipart string message `uuid/GET/element`"
            " - 'GET' missing or different string. Sender: '%s', Subject: '%s'.",
                mlm_client_sender (client), mlm_client_subject (client));
        return;
    }
    zmsg_destroy (msg_p);

    zmsg_t *reply = zmsg_new ();
    zmsg_addstr (reply, uuid);
    zmsg_addstr (reply, "OK");
    zmsg_addstr (reply, element);

    zhashx_t *hash = rt_get_element (data, element);
    zframe_t *frame = NULL;
    if (!hash) {
        hash = zhashx_new ();
        frame = zhashx_pack (hash);
        zhashx_destroy (&hash);
    }
    else {
        frame = zhashx_pack (hash);
    }
    zmsg_append (reply, &frame);

    zstr_free (&uuid);
    zstr_free (&element);

    int rv = mlm_client_sendto (client, mlm_client_sender (client), RFC_ALERTS_LIST_SUBJECT, NULL, 5000, &reply);
    if (rv != 0) {
        log_error ("mlm_client_sendto (sender = '%s', subject = '%s', timeout = '5000') failed.",
                mlm_client_sender (client), RFC_RT_DATA_SUBJECT);
    }
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
mailbox_test (bool verbose)
{
    printf (" * mailbox: ");

    //  @selftest
    //  @end
    printf ("Empty test - OK\n");
}
