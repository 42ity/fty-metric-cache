/*  =========================================================================
    bios_agent_rt_server - Actor listening on metrics with request reply protocol

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
    bios_agent_rt_server - Actor listening on metrics with request reply protocol
@discuss
@end
*/

#include "agent_rt_classes.h"

#define POLL_INTERVAL 10000



static void
s_handle_poll (rt_t *data)
{
  rt_purge(data);
}

static void
s_handle_service (mlm_client_t *client, zmsg_t **message_p)
{
    assert (client);
    assert (message_p && *message_p);

    log_error ("Service deliver is not implemented.");

    zmsg_destroy (message_p);
}

static void
s_handle_mailbox (mlm_client_t *client, zmsg_t **message_p, rt_t *data)
{

    assert (client);
    assert (message_p && *message_p);
    zmsg_destroy (message_p);
    zsys_debug ("============= START ==================");
    rt_print (data);
    zsys_debug ("=============== END ===========");
}

static void
s_handle_stream (mlm_client_t *client, zmsg_t **message_p, rt_t *data)
{
    assert (client);
    assert (message_p && *message_p);
    bios_proto_t **aux = (bios_proto_t**)malloc(sizeof(bios_proto_t*));
    *aux = bios_proto_decode(message_p);
    rt_put(data, aux);
    free(aux);
// store the mesage in data, overwriting any old data _put ()
}

void
bios_agent_rt_server (zsock_t *pipe, void *args)
{

    mlm_client_t *client = mlm_client_new ();
    if (!client) {
        log_critical ("mlm_client_new () failed");
        return;
    }

    zpoller_t *poller = zpoller_new (pipe, mlm_client_msgpipe (client), NULL);
    if (!poller) {
        log_critical ("zpoller_new () failed");
        mlm_client_destroy (&client);
        return;
    }

    rt_t *data = rt_new ();

    zsock_signal (pipe, 0);

    uint64_t timestamp = (uint64_t) zclock_mono ();
    uint64_t timeout = (uint64_t) POLL_INTERVAL;

    while (!zsys_interrupted) {
        void *which = zpoller_wait (poller, timeout);

        if (which == NULL) {
            if (zpoller_terminated (poller) || zsys_interrupted) {
                log_warning ("zpoller_terminated () or zsys_interrupted");
                break;
            }
            if (zpoller_expired (poller)) {
                s_handle_poll (data);
            }
            timestamp = (uint64_t) zclock_mono ();
            continue;
        }

        uint64_t now = (uint64_t) zclock_mono ();
        if (now - timestamp >= timeout) {
            s_handle_poll (data);
            timestamp = (uint64_t) zclock_mono ();
        }

        if (which == pipe) {
            zmsg_t *message = zmsg_recv (pipe);
            if (!message) {
                log_error ("Given `which == pipe`, function `zmsg_recv (pipe)` returned NULL");
                continue;
            }
            if (actor_commands (client, &message) == 1) {
                break;
            }
            continue;
        }

        // paranoid non-destructive assertion of a twisted mind 
        if (which != mlm_client_msgpipe (client)) {
            log_critical ("which was checked for NULL, pipe and now should have been `mlm_client_msgpipe (client)` but is not.");
            continue;
        }

        zmsg_t *message = mlm_client_recv (client);
        if (!message) {
            log_error ("Given `which == mlm_client_msgpipe (client)`, function `mlm_client_recv ()` returned NULL");
            continue;
        }

        const char *command = mlm_client_command (client);
        if (streq (command, "STREAM DELIVER")) {
            s_handle_stream (client, &message, data);
        }
        else
        if (streq (command, "MAILBOX DELIVER")) {
            s_handle_mailbox (client, &message, data);
        }
        else
        if (streq (command, "SERVICE DELIVER")) {
            s_handle_service (client, &message);
        }
        else {
            log_error ("Unrecognized mlm_client_command () = '%s'", command ? command : "(null)");
        }

        zmsg_destroy (&message);
    } // while (!zsys_interrupted)

    rt_destroy (&data);
    zpoller_destroy (&poller);
    mlm_client_destroy (&client);
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
bios_agent_rt_server_test (bool verbose)
{
    printf (" * bios_agent_rt_server: ");
    //  @selftest
   
    static const char* endpoint = "inproc://bios-agent-rt-server-test";

    zactor_t *server = zactor_new (mlm_server, (void*) "Malamute");
    zstr_sendx (server, "BIND", endpoint, NULL);
    if (verbose)
        zstr_send (server, "VERBOSE");

    mlm_client_t *producer = mlm_client_new ();
    mlm_client_connect (producer, endpoint, 1000, "producer");
    mlm_client_set_producer (producer, "METRICS");

    mlm_client_t *ui = mlm_client_new ();
    mlm_client_connect (ui, endpoint, 1000, "UI");

    zactor_t *rt = zactor_new (bios_agent_rt_server, (void*) NULL);
    zstr_sendx (rt, "CONNECT", endpoint, "agent-rt", NULL);
    zstr_sendx (rt, "CONSUMER", "METRICS", ".*", NULL);
    zclock_sleep (500);   //THIS IS A HACK TO SETTLE DOWN THINGS

    zmsg_t *msg = bios_proto_encode_metric (
        NULL,
        "temperature",
        "ups",
        "30",
        "C",
        60);
    assert (msg);
    int rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);


    msg = bios_proto_encode_metric (
        NULL,
        "humidity",
        "ups",
        "45",
        "%",
        60);
    assert (msg);
    rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);

    msg = bios_proto_encode_metric (
        NULL,
        "temperature",
        "epdu",
        "25",
        "C",
        60);
    assert (msg);
    rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);

    msg = bios_proto_encode_metric (
        NULL,
        "realpower.default",
        "epdu",
        "100",
        "W",
        60);
    assert (msg);
    rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);

    zsys_debug ("TRACE-REQUEST 1");
    msg = zmsg_new ();
    rv = mlm_client_sendto (ui, "agent-rt", "xyz", NULL, 5000, &msg);
    assert (rv == 0);
/*
    msg = mlm_client_recv (ui);
    assert (msg);    
    char *str = zmsg_popstr (msg);
    while (true) {

    }
    zmsg_destro (&msg);
*/
    zclock_sleep (3000);
    zactor_destroy (&rt);
    mlm_client_destroy (&ui);
    mlm_client_destroy (&producer);
    zactor_destroy (&server);

    //  @end
    printf ("OK\n");
}
