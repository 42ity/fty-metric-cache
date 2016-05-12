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

// Send error mailbox message
static void
s_send_error_response (mlm_client_t *client, const char *subject, const char *reason) {
    assert (client);
    assert (subject);
    assert (reason);

    zmsg_t *reply  = zmsg_new ();
    assert (reply);

    zmsg_addstr (reply, "ERROR");
    zmsg_addstr (reply, reason);

    int rv = mlm_client_sendto (client, mlm_client_sender (client), subject, NULL, 5000, &reply);
    if (rv != 0) {
        zmsg_destroy (&reply);
        zsys_error (
                "mlm_client_sendto (sender = '%s', subject = '%s', timeout = '5000') failed.",
                mlm_client_sender (client), subject);
    }
}


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
 
    // TODO: check mlm_client_subject ()
   
    char *command = zmsg_popstr (msg);
    if (!command || !streq (command, "GET")) {
        s_send_error_response (client, "TODO", "BAD_MESSAGE"); // TODO
        zmsg_destroy (msg_p);
        return;
    }
    zstr_free (&command);

    char *element = zmsg_popstr (msg);
    if (!command || !streq (command, "GET")) {
        // TODO s_send_error_response () BAD_MESSAGE
        zmsg_destroy (msg_p);
        return;
    }
    zmsg_destroy (msg_p);

    // Too tire to continue  zhashx_t *r = rt_get_element ();
    zstr_free (&element);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
mailbox_test (bool verbose)
{
    printf (" * mailbox: ");

    //  @selftest
    //  Simple create/destroy test
    //  @end
    printf ("OK\n");
}
