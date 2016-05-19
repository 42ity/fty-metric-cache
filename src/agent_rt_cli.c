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

void send_test(){
    mlm_client_t *producer = mlm_client_new ();
    mlm_client_connect (producer, endpoint, 1000, "PRODUCER");
    mlm_client_set_producer (producer, "METRICS");
    zmsg_t *msg = bios_proto_encode_metric (NULL, "temperature", "ups", "30", "C", 5);
    int rv = mlm_client_send (producer, "Nobody here cares about this.", &msg);
    assert (rv == 0);
    
    mlm_client_destroy(&producer);
}

void print_all(mlm_client_t *ui){
    zmsg_t *send = zmsg_new ();
    zmsg_addstr (send, "");
    zmsg_addstr (send, "PRINT");
    
    int rv = mlm_client_sendto (ui, "agent-rt", RFC_RT_DATA_SUBJECT, NULL, 5000, &send);
    assert (rv == 0);
}

int main (int argc, char *argv [])
{
    bool verbose = false;
    int argn;
    for (argn = 1; argn < argc; argn++) {
        if (streq (argv [argn], "--help")
        ||  streq (argv [argn], "-h")) {
            puts ("agent-rt-cli [options] ...");
            puts ("  --verbose / -v         verbose test output");
            puts ("  --help / -h            this information");
            return 0;
        }
        else
        if (streq (argv [argn], "--verbose")
        ||  streq (argv [argn], "-v"))
            verbose = true;
        else {
            printf ("Unknown option: %s\n", argv [argn]);
            return 1;
        }
    }
    
    mlm_client_t *ui = mlm_client_new ();
    mlm_client_connect (ui, endpoint, 1000, "UI");
    
    send_test();
    print_all(ui);
    
    if (verbose)
        zsys_info ("agent_rt_cli - Command line interface for agent-rt");
    
    mlm_client_destroy(&ui);
    return 0;
}
