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

#define POLL_INTERVAL 30000

static void
s_handle_poll (rt_t *data)
{
    rt_purge (data);
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

    mailbox_perform (client, message_p, data);

    zmsg_destroy (message_p);
}

static void
s_handle_stream (mlm_client_t *client, zmsg_t **message_p, rt_t *data)
{
    assert (client);
    assert (message_p && *message_p);
   
    bios_proto_t *proto = bios_proto_decode (message_p);
    rt_put (data, &proto);
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

static void
test_assert_proto (
        bios_proto_t *p,
        const char *type,
        const char *element,
        const char *value,
        const char *unit,
        uint32_t ttl)
{
    assert (p);
    assert (streq (bios_proto_type (p), type));
    assert (streq (bios_proto_element_src (p), element));
    assert (streq (bios_proto_unit (p), unit));
    assert (streq (bios_proto_value (p), value));
    assert (bios_proto_ttl (p) == ttl);
}

void
bios_agent_rt_server_test (bool verbose)
{
    static const char* endpoint = "inproc://bios-agent-rt-server-test";

    printf (" * bios_agent_rt_server: ");
    //  @selftest
    
    zactor_t *server = zactor_new (mlm_server, (void*) "Malamute");
    zstr_sendx (server, "BIND", endpoint, NULL);
    if (verbose)
        zstr_send (server, "VERBOSE");

    mlm_client_t *producer = mlm_client_new ();
    mlm_client_connect (producer, endpoint, 1000, "PRODUCER");
    mlm_client_set_producer (producer, "METRICS");

    mlm_client_t *ui = mlm_client_new ();
    mlm_client_connect (ui, endpoint, 1000, "UI");

    zactor_t *rt = zactor_new (bios_agent_rt_server, (void*) NULL);
    zstr_sendx (rt, "CONNECT", endpoint, "agent-rt", NULL);
    zstr_sendx (rt, "CONSUMER", "METRICS", ".*", NULL);
    zclock_sleep (100);

    zmsg_t *msg = bios_proto_encode_metric (NULL, "temperature", "ups", "30", "C", 5);
    int rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);
    zclock_sleep (100);

    msg = bios_proto_encode_metric (NULL, "humidity", "ups", "45", "%", 5);
    rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);
    zclock_sleep (100);

    msg = bios_proto_encode_metric (NULL, "temperature", "epdu", "25", "C", 60);
    rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);
    zclock_sleep (100);

    msg = bios_proto_encode_metric (NULL, "realpower.default", "switch", "100", "W", 55);
    rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);
    zclock_sleep (100);

    // ===============================================
    // Test case #1:
    //      GET ups
    // Expected:
    //      2 measurements
    // ===============================================
    zmsg_t *send = zmsg_new ();
    zmsg_addstr (send, "12345");
    zmsg_addstr (send, "GET");
    zmsg_addstr (send, "ups");
    rv = mlm_client_sendto (ui, "agent-rt", RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
    zmsg_t *reply = mlm_client_recv (ui);
    assert (reply);
    assert (streq (mlm_client_subject (ui), RFC_RT_DATA_SUBJECT));
    zmsg_print (reply);

    char *uuid = zmsg_popstr (reply);
    assert (uuid);
    assert (streq (uuid, "12345"));
    zstr_free (&uuid);

    char *command = zmsg_popstr (reply);
    assert (command);
    assert (streq (command, "OK"));
    zstr_free (&command);

    char *element = zmsg_popstr (reply);
    assert (element);
    assert (streq (element, "ups"));
    zstr_free (&element);

    zmsg_t *encoded = zmsg_popmsg (reply);
    assert (encoded);

    bios_proto_t *proto = bios_proto_decode (&encoded);
    test_assert_proto (proto, "humidity", "ups", "45", "%", 5);
    bios_proto_destroy (&proto);

    encoded = zmsg_popmsg (reply);
    assert (encoded);
    proto = bios_proto_decode (&encoded);
    test_assert_proto (proto, "temperature", "ups", "30", "C", 5);
    bios_proto_destroy (&proto);
 
    encoded = zmsg_popmsg (reply);
    assert (encoded == NULL);   

    zmsg_destroy (&reply);

    // ===============================================
    // Test case #2:
    //      1. Change epdu data
    //      2. GET epdu
    // Expected:
    //      1 changed measurement
    // ===============================================

    msg = bios_proto_encode_metric (NULL, "temperature", "epdu", "70", "C", 29);
    rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);
    zclock_sleep (10);

    send = zmsg_new ();
    zmsg_addstr (send, "12345");
    zmsg_addstr (send, "GET");
    zmsg_addstr (send, "epdu");
    rv = mlm_client_sendto (ui, "agent-rt", RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
    reply = mlm_client_recv (ui);
    assert (reply);
    assert (streq (mlm_client_subject (ui), RFC_RT_DATA_SUBJECT));
    zmsg_print (reply);

    uuid = zmsg_popstr (reply);
    assert (uuid);
    assert (streq (uuid, "12345"));
    zstr_free (&uuid);

    command = zmsg_popstr (reply);
    assert (command);
    assert (streq (command, "OK"));
    zstr_free (&command);

    element = zmsg_popstr (reply);
    assert (element);
    assert (streq (element, "epdu"));
    zstr_free (&element);

    encoded = zmsg_popmsg (reply);
    assert (encoded);

    proto = bios_proto_decode (&encoded);
    test_assert_proto (proto, "temperature", "epdu", "70", "C", 29);
    bios_proto_destroy (&proto);

    encoded = zmsg_popmsg (reply);
    assert (encoded == NULL);   
    
    zmsg_destroy (&reply);

    // ===============================================
    // Test case #3:
    //      1. Wait 30500 miliseconds
    //      2. GET epdu
    //      3. GET ups
    // Expected:
    //      0 measurements
    //      0 measurements
    // ===============================================
    zsys_debug ("Sleeping 30500 miliseconds");
    zclock_sleep (30500);

    send = zmsg_new ();
    zmsg_addstr (send, "1234567");
    zmsg_addstr (send, "GET");
    zmsg_addstr (send, "epdu");
    rv = mlm_client_sendto (ui, "agent-rt", RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
    reply = mlm_client_recv (ui);
    assert (reply);
    assert (streq (mlm_client_subject (ui), RFC_RT_DATA_SUBJECT));
    zmsg_print (reply);

    uuid = zmsg_popstr (reply);
    assert (uuid);
    assert (streq (uuid, "1234567"));
    zstr_free (&uuid);

    command = zmsg_popstr (reply);
    assert (command);
    assert (streq (command, "OK"));
    zstr_free (&command);

    element = zmsg_popstr (reply);
    assert (element);
    assert (streq (element, "epdu"));
    zstr_free (&element);

    encoded = zmsg_popmsg (reply);
    assert (encoded == NULL);

    zmsg_destroy (&reply);

    send = zmsg_new ();
    zmsg_addstr (send, "12345");
    zmsg_addstr (send, "GET");
    zmsg_addstr (send, "ups");
    rv = mlm_client_sendto (ui, "agent-rt", RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
    reply = mlm_client_recv (ui);
    assert (reply);
    assert (streq (mlm_client_subject (ui), RFC_RT_DATA_SUBJECT));
    zmsg_print (reply);

    uuid = zmsg_popstr (reply);
    assert (uuid);
    assert (streq (uuid, "12345"));
    zstr_free (&uuid);

    command = zmsg_popstr (reply);
    assert (command);
    assert (streq (command, "OK"));
    zstr_free (&command);

    element = zmsg_popstr (reply);
    assert (element);
    assert (streq (element, "ups"));
    zstr_free (&element);

    encoded = zmsg_popmsg (reply);
    assert (encoded == NULL);

    zmsg_destroy (&reply);

    // ===============================================
    // Test case #4:
    //      1. Wait 35000 miliseconds
    //      2. GET switch
    // Expected:
    //      0 measurements
    // ===============================================
    zsys_debug ("Sleeping 30500 miliseconds");
    zclock_sleep (30500);

    send = zmsg_new ();
    zmsg_addstr (send, "8CB3E9A9649B4BEF8DE225E9C2CEBB38");
    zmsg_addstr (send, "GET");
    zmsg_addstr (send, "switch");
    rv = mlm_client_sendto (ui, "agent-rt", RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
    reply = mlm_client_recv (ui);
    assert (reply);
    assert (streq (mlm_client_subject (ui), RFC_RT_DATA_SUBJECT));
    zmsg_print (reply);

    uuid = zmsg_popstr (reply);
    assert (uuid);
    assert (streq (uuid, "8CB3E9A9649B4BEF8DE225E9C2CEBB38"));
    zstr_free (&uuid);

    command = zmsg_popstr (reply);
    assert (command);
    assert (streq (command, "OK"));
    zstr_free (&command);

    element = zmsg_popstr (reply);
    assert (element);
    assert (streq (element, "switch"));
    zstr_free (&element);

    encoded = zmsg_popmsg (reply);
    assert (encoded == NULL);

    zmsg_destroy (&reply);


    zactor_destroy (&rt);
    mlm_client_destroy (&ui);
    mlm_client_destroy (&producer);
    zactor_destroy (&server);
    //  @end
    printf ("OK\n");
}
