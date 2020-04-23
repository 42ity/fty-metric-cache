/*  =========================================================================
    fty_metric_cache_cli - Command line interface for agent-rt

    Copyright (C) 2014 - 2020 Eaton

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
     fty_metric_cache_cli - Command line interface for agent-rt
@discuss
@end
*/

#include "fty_metric_cache_classes.h"
#include <time.h>

const char *endpoint = "ipc://@/malamute";
zpoller_t *poller = NULL;

void print_device(const char *device, const char* filter, mlm_client_t *cli){
    zmsg_t *send = zmsg_new ();
    zmsg_addstr (send, "");
    zmsg_addstr (send, "GET");
    zmsg_addstr (send, device);
    if(NULL!=filter){
        zmsg_addstr (send, filter);
    }

    int rv = mlm_client_sendto (cli, FTY_METRIC_CACHE_MAILBOX, RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
}

void list_devices(mlm_client_t *cli){
    zmsg_t *send = zmsg_new ();
    zmsg_addstr (send, "");
    zmsg_addstr (send, "LIST");

    int rv = mlm_client_sendto (cli, FTY_METRIC_CACHE_MAILBOX, RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
}

zmsg_t *
reciver (mlm_client_t *client, int timeout)
{
    if (zsys_interrupted)
        return NULL;

    if (!poller)
        poller = zpoller_new (mlm_client_msgpipe (client), NULL);

    zsock_t *which = (zsock_t *) zpoller_wait (poller, timeout);
    if (which == mlm_client_msgpipe (client)) {
        zmsg_t *reply = mlm_client_recv (client);
        return reply;
    }
    return NULL;
}

int main (int argc, char *argv [])
{
    ftylog_setInstance("fty-metric-cache-cli", LOG_CONFIG);
    ftylog_setLogLevelInfo(ftylog_getInstance());

    mlm_client_t *client = mlm_client_new ();
    if ( !client ) {
        log_error ("agent-rt-cli:\tlm_client_new memory error");
        return -1;
    }

    int rv = mlm_client_connect (client, endpoint, 1000, "CLI");
    if ( rv == -1 ) {
        log_error ("agent-rt-cli:\tCannot connect to malamute on '%s'", endpoint);
        mlm_client_destroy (&client);
        return -1;
    }

    bool verbose = false;
    for (int argn = 1; argn < argc; argn++) {
        if (    streq (argv [argn], "--help")
             || streq (argv [argn], "-h"))
        {
            puts ("agent-rt-cli [options]");
            puts ("agent-rt-cli device [filter] print all information about the device");
            puts ("             device          device name or a regex");
            puts ("             [filter]        regex filter to select specific metric name");
            puts ("  --list / -l                print list of devices known to the agent");
            puts ("  --verbose / -v             verbose output");
            puts ("  --help / -h                this information");
            break;
        }
        else
        if (    streq (argv [argn], "--verbose")
             || streq (argv [argn], "-v"))
        {
            verbose = true;
        }
        else
        if (    streq (argv [argn], "--list")
             || streq (argv [argn], "-l"))
        {
            list_devices(client);
            break;
        }
        else {
            char* filter=(argn==(argc-2))?argv[argn+1]:NULL;
            print_device(argv [argn], filter, client);
            break;
        }
    }

    if (verbose)
        ftylog_setVeboseMode(ftylog_getInstance());

    //log_debug ("agent_rt_cli - Command line interface for agent-rt");

    zmsg_t *msg = reciver (client, 1000);
    if (msg) {
        char *uuid = zmsg_popstr (msg);
        char *confirmation = zmsg_popstr (msg);
        char *command = zmsg_popstr (msg);
        char *reply = NULL;
        if(streq(command, "LIST")){
            reply = zmsg_popstr (msg);
            log_info ("%s", reply);
            free (reply);
        }else{
            log_info ("Device: %s", command);
            char _bufftime[sizeof "YYYY-MM-DDTHH:MM:SSZ"];
            zmsg_t *msg_part = zmsg_popmsg(msg);
            fty_proto_t *fty_p_element;
            while(msg_part){
                fty_p_element = fty_proto_decode(&msg_part);
                uint64_t _time = fty_proto_time (fty_p_element);
                strftime(_bufftime, sizeof _bufftime, "%FT%TZ", gmtime((const time_t*)&_time));
                log_info ("%s(ttl=%" PRIu32"s) %20s@%s = %s%s",
                        _bufftime,
                        fty_proto_ttl (fty_p_element),
                        fty_proto_type (fty_p_element),
                        fty_proto_name (fty_p_element),
                        fty_proto_value (fty_p_element),
                        fty_proto_unit (fty_p_element));
                fty_proto_destroy(&fty_p_element);
                msg_part = zmsg_popmsg(msg);
            }
        }
        free (uuid);
        free (confirmation);
        free (command);

        zmsg_destroy (&msg);
        zclock_sleep (100);
    }
    else
        log_error ("No agent response");

    mlm_client_destroy (&client);
    return 0;
}
