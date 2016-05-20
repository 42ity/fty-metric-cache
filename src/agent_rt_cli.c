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

const char *endpoint = "ipc://@/malamute";
zpoller_t *poller = NULL;

void send_test(){
    mlm_client_t *producer = mlm_client_new ();
    mlm_client_connect (producer, endpoint, 1000, "PRODUCER");
    mlm_client_set_producer (producer, "METRICS");
    
    zmsg_t *msg = bios_proto_encode_metric (NULL, "temperature", "ups", "30", "C", 15);
    int rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);
    
    msg = bios_proto_encode_metric (NULL, "temperature", "ups2", "30", "C", 25);
    rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);
    
    msg = bios_proto_encode_metric (NULL, "humidity", "ups", "30", "h", 5);
    rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);
    
    msg = bios_proto_encode_metric (NULL, "humidity", "ups2", "30", "h", 10);
    rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);
    
    mlm_client_destroy(&producer);
}

void print_device(const char *device, mlm_client_t *ui){
    zmsg_t *send = zmsg_new ();
    zmsg_addstr (send, "");
    zmsg_addstr (send, "DEVICE_INFO");
    zmsg_addstr (send, device);
    
    int rv = mlm_client_sendto (ui, "agent-rt", RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
}

void list_devices(mlm_client_t *ui){
    zmsg_t *send = zmsg_new ();
    zmsg_addstr (send, "");
    zmsg_addstr (send, "PRINT_DEVICES");
    
    int rv = mlm_client_sendto (ui, "agent-rt", RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
}

/*void print_all(mlm_client_t *ui){
    zmsg_t *send = zmsg_new ();
    zmsg_addstr (send, "");
    zmsg_addstr (send, "PRINT");
    
    int rv = mlm_client_sendto (ui, "agent-rt", RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
}*/

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
    
    mlm_client_t *ui = mlm_client_new ();
    mlm_client_connect (ui, endpoint, 1000, "UI");
    
    mlm_client_t *cli = mlm_client_new ();
    mlm_client_connect (cli, endpoint, 1000, "CLI");
    
    //if(argc == 1) 
    //  print_all(ui);
    //else 
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
            list_devices(ui);
            break;
        }
        else{
            print_device(argv [argn], ui);
            break;
        }
    }
    
    //Debigging
    send_test();
    //End debugging
    
    if (verbose)
        zsys_info ("agent_rt_cli - Command line interface for agent-rt");
    
        zmsg_t *msg = reciver (cli, 1000);
        if (msg) {
            char *reply = zmsg_popstr (msg);
            printf ("%s", reply);
            free (reply);
            zmsg_destroy (&msg);
            zclock_sleep (100);
        }
        else
            zsys_error ("No agent response");
    
    mlm_client_destroy(&cli);
    mlm_client_destroy(&ui);
    return 0;
}
