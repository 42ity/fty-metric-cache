/*  =========================================================================
    fty_metric_cache - Listens on all metrics in order to remeber the last ones.

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
    fty_metric_cache - Listens on all metrics in order to remeber the last ones.
@discuss
@end
*/
#include <getopt.h>

#include "fty_metric_cache_classes.h"

#define str(x) #x

static const char *ENDPOINT = "ipc://@/malamute";
static const char *STATE_FILE = "/var/lib/fty/fty-metric-cache/state_file";

void usage () {
    puts ("fty-metric-cache [options] ...\n"
          "  --verbose / -v         verbosity level\n"
          "  --state-file / -s      TODO\n"
          "  --help / -h            this information\n"
          );
}

int main (int argc, char *argv [])
{
    int help = 0;
    bool verbose = false;
    char *state_file = NULL;

    ftylog_setInstance("fty-metric-cache", LOG_CONFIG);
    while (true) {
        static struct option long_options[] =
        {
            {"help",            no_argument,        0,  1},
            {"verbose",         no_argument,        0,  'v'},
            {"state-file",      required_argument,  0,  's'},
            {0,                 0,                  0,  0}
        };

        int option_index = 0;
        int c = getopt_long (argc, argv, "hvs:", long_options, &option_index);
        if (c == -1)
            break;
        switch (c) {
            case 'v':
            {
                verbose = true;
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

    if (verbose)
        ftylog_setVeboseMode(ftylog_getInstance());

    // Set default state file on empty
    if (!state_file) {
        state_file = (char *) STATE_FILE;
    }

    zactor_t *rt_server = zactor_new (fty_metric_cache_server, (void *) NULL);
    if (!rt_server) {
        log_fatal ("zactor_new (task = 'fty_metric_cache_server', args = 'NULL') failed");
        return EXIT_FAILURE;
    }
    zstr_sendx (rt_server,  "CONFIGURE", state_file, NULL);
    zstr_sendx (rt_server,  "CONNECT", ENDPOINT, FTY_METRIC_CACHE_MAILBOX, NULL);
    zstr_sendx (rt_server,  "CONSUMER", FTY_PROTO_STREAM_METRICS, ".*", NULL);

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
