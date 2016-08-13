/*  =========================================================================
    agent_rt_cli - Command line interface for agent-rt

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
    agent_rt_cli - Command line interface for agent-rt
@discuss
@end
*/

#include "agent_rt_classes.h"
#include <time.h>

const char *endpoint = "ipc://@/malamute";
zpoller_t *poller = NULL;

void print_device(const char *device, mlm_client_t *cli){
    zmsg_t *send = zmsg_new ();
    zmsg_addstr (send, "");
    zmsg_addstr (send, "GET");
    zmsg_addstr (send, device);

    int rv = mlm_client_sendto (cli, "agent-rt", RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
}

void list_devices(mlm_client_t *cli){
    zmsg_t *send = zmsg_new ();
    zmsg_addstr (send, "");
    zmsg_addstr (send, "LIST");

    int rv = mlm_client_sendto (cli, "agent-rt", RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
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
    bool verbose = false;
    int argn;

    mlm_client_t *cli = mlm_client_new ();
    mlm_client_connect (cli, endpoint, 1000, "CLI");

      for (argn = 1; argn < argc; argn++) {
        if (streq (argv [argn], "--help")
        ||  streq (argv [argn], "-h")) {
            puts ("agent-rt-cli [options]");
            puts ("agent-rt-cli [device]    print all information of the device");
            puts ("  --list / -l            print list of devices in the agent");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --help / -h            this information");
            break;
        }
        else
        if (streq (argv [argn], "--verbose")
        ||  streq (argv [argn], "-v"))
            verbose = true;
        else
        if (streq (argv [argn], "--list")
        ||  streq (argv [argn], "-l")){
            list_devices(cli);
            break;
        }
        else{
            print_device(argv [argn], cli);
            break;
        }
    }

    if (verbose)
        zsys_info ("agent_rt_cli - Command line interface for agent-rt");

        zmsg_t *msg = reciver (cli, 1000);
        if (msg) {
            char *uuid = zmsg_popstr (msg);
            char *confirmation = zmsg_popstr (msg);
            char *command = zmsg_popstr (msg);
            char *reply = NULL;
            if(streq(command, "LIST")){
                reply = zmsg_popstr (msg);
                printf ("%s", reply);
                free (reply);
            }else{
                printf("Device: %s\n", command);
                char _bufftime[sizeof "YYYY-MM-DDTHH:MM:SSZ"];
                zmsg_t *msg_part = zmsg_popmsg(msg);
                bios_proto_t *bios_p_element;
                while(msg_part){
                  bios_p_element = bios_proto_decode(&msg_part);
                  uint64_t _time = bios_proto_aux_number (bios_p_element, "time", 0);
                  strftime(_bufftime, sizeof _bufftime, "%FT%TZ", gmtime((const time_t*)&_time));
                  printf("%s(ttl=%" PRIu32"s) %20s@%s = %s%s\n",
                    _bufftime,
                    bios_proto_ttl (bios_p_element),
                    bios_proto_type (bios_p_element),
                    bios_proto_element_src (bios_p_element),
                    bios_proto_value (bios_p_element),
                    bios_proto_unit (bios_p_element));
                  bios_proto_destroy(&bios_p_element);
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
            zsys_error ("No agent response");

    mlm_client_destroy(&cli);
    return 0;
}
