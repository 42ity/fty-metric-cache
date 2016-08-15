/*  =========================================================================
    bios_agent_rt - Listens on all metrics in order to remeber the last ones.

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
    bios_agent_rt - Listens on all metrics in order to remeber the last ones.
@discuss
@end
*/
#include <getopt.h>

#include "agent_rt_classes.h"

#define str(x) #x

static const char *AGENT_NAME = "agent-rt";
static const char *ENDPOINT = "ipc://@/malamute";
static const char *STATE_FILE = "/var/lib/bios/bios-agent-rt/state_file";

#define DEFAULT_LOG_LEVEL LOG_WARNING

void usage () {
    puts ("bios-agent-rt [options] ...\n"
          "  --log-level / -l       bios log level\n"
          "                         overrides setting in env. variable BIOS_LOG_LEVEL\n"
          "  --state-file / -s      TODO\n"
          "  --help / -h            this information\n"
          );
}

int get_log_level (const char *level) {
    if (streq (level, str(LOG_DEBUG))) {
        return LOG_DEBUG;
    }
    else
    if (streq (level, str(LOG_INFO))) {
        return LOG_INFO;
    }
    else
    if (streq (level, str(LOG_WARNING))) {
        return LOG_WARNING;
    }
    else
    if (streq (level, str(LOG_ERR))) {
        return LOG_ERR;
    }
    else
    if (streq (level, str(LOG_CRIT))) {
        return LOG_CRIT;
    }
    return -1;
}


int main (int argc, char *argv [])
{
    int help = 0;
    int log_level = -1;
    char *state_file = NULL;

    while (true) {
        static struct option long_options[] =
        {
            {"help",            no_argument,        0,  1},
            {"log-level",       required_argument,  0,  'l'},
            {"state-file",      required_argument,  0,  's'},
            {0,                 0,                  0,  0}
        };

        int option_index = 0;
        int c = getopt_long (argc, argv, "hl:s:", long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
            case 'l':
            {
                log_level = get_log_level (optarg);
                break;
            }
            case 's':
            {
                state_file = optarg;
                break;
            }
            case 'h':
            default:
            {
                help = 1;
                break;
            }
        }
    }
    if (help) {
        usage ();
        return EXIT_FAILURE;
    }

    // log_level cascade (priority ascending)
    //  1. default value
    //  2. env. variable
    //  3. command line argument
    //  4. actor message - NOT IMPLEMENTED YET
    if (log_level == -1) {
        char *env_log_level = getenv ("BIOS_LOG_LEVEL");
        if (env_log_level) {
            log_level = get_log_level (env_log_level);
            if (log_level == -1)
                log_level = DEFAULT_LOG_LEVEL;
        }
        else {
            log_level = DEFAULT_LOG_LEVEL;
        }
    }
    log_set_level (log_level);

    // Set default state file on empty
    if (!state_file) {
        state_file = (char *) STATE_FILE;
    }

    zactor_t *rt_server = zactor_new (bios_agent_rt_server, (void *) NULL);
    if (!rt_server) {
        log_critical ("zactor_new (task = 'bios_agent_rt_server', args = 'NULL') failed");
        return EXIT_FAILURE;
    }
    zstr_sendx (rt_server,  "CONFIGURE", state_file, NULL);
    zstr_sendx (rt_server,  "CONNECT", ENDPOINT, AGENT_NAME, NULL);
    zstr_sendx (rt_server,  "CONSUMER", BIOS_PROTO_STREAM_METRICS, ".*", NULL);

    while (true) {
        char *message = zstr_recv (rt_server);
        if (message) {
            puts (message);
            zstr_free (&message);
        }
        else {
            puts ("interrupted");
            break;
        }
    }

    zactor_destroy (&rt_server);
    return EXIT_SUCCESS;
}
