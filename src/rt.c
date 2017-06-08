/*  =========================================================================
    rt - fty metric cache structure

    Copyright (C) 2014 - 2017 Eaton

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
    rt - fty metric cahce structure
@discuss
@end
*/

#include "fty_metric_cache_classes.h"

//  Structure of our class

struct _rt_t {
    zhashx_t *devices;      // hash of hashes ("device name", ("measurement", fty_proto_t*))
};

//  --------------------------------------------------------------------------
//  Create a new rt

rt_t *
rt_new (void)
{
    rt_t *self = (rt_t *) zmalloc (sizeof (rt_t));
    assert (self);

    self->devices = zhashx_new ();
    zhashx_set_destructor (self->devices, (zhashx_destructor_fn *) zhashx_destroy);
    return self;
}

//  --------------------------------------------------------------------------
//  Destroy the rt

void
rt_destroy (rt_t **self_p)
{
    if (!self_p)
        return;
    if (*self_p) {
        rt_t *self = *self_p;

        zhashx_destroy (&self->devices);

        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Store fty_proto_t message transfering ownership

void
rt_put (rt_t *self, fty_proto_t **message_p)
{
    assert (self);
    assert (message_p);

    fty_proto_t *message = *message_p;

    if (!message)
        return;

    if ( !fty_proto_time (message) ) {
        // If time not set, assign time = NOW()
        fty_proto_set_time (message, (uint64_t) zclock_time () / 1000);
    }

    zhashx_t *device = (zhashx_t *) zhashx_lookup (self->devices, fty_proto_name (message));
    if (!device) {
        zhashx_t *metrics = zhashx_new ();
        zhashx_set_destructor (metrics, (zhashx_destructor_fn *) fty_proto_destroy);

        int rv = zhashx_insert (self->devices, fty_proto_name (message), metrics);
        assert (rv == 0);
        device = metrics;
    }
    zhashx_update (device, fty_proto_type (message), message);
    *message_p = NULL;
}

//  --------------------------------------------------------------------------
//  Get specific measurement for given device or NULL when no data

fty_proto_t *
rt_get (rt_t *self, const char *element, const char *measurement)
{
    assert (self);
    assert (element);
    assert (measurement);

    zhashx_t *device = (zhashx_t *) zhashx_lookup (self->devices, element);
    if (!device)
        return NULL;
    return (fty_proto_t *) zhashx_lookup (device, measurement);
}

//  --------------------------------------------------------------------------
//  Get all measurements for given device or NULL when no data

zhashx_t *
rt_get_element (rt_t *self, const char *element)
{
    assert (self);
    assert (element);

    return (zhashx_t *) zhashx_lookup (self->devices, element);
}

//  --------------------------------------------------------------------------
//  Purge expired data

void
rt_purge (rt_t *self)
{
    assert (self);
    uint64_t timestamp_s = (uint64_t) zclock_time () / 1000;
    zhashx_t *device = (zhashx_t *) zhashx_first (self->devices);
    while (device) {
        fty_proto_t *metric = (fty_proto_t *) zhashx_first (device);

        zlistx_t *to_delete = zlistx_new ();
        zlistx_set_destructor (to_delete, (czmq_destructor *) zstr_free);
        zlistx_set_duplicator (to_delete, (czmq_duplicator *) strdup);

        while (metric) {
            uint64_t time_s = fty_proto_time (metric);

            if (timestamp_s - time_s > fty_proto_ttl (metric)) {
                zlistx_add_end (to_delete, (void *) zhashx_cursor (device));
            }
            metric = (fty_proto_t *) zhashx_next (device);
        }
        char *cursor = (char *) zlistx_first (to_delete);
        while (cursor) {
            zhashx_delete (device, (char *) cursor);
            cursor = (char *) zlistx_next (to_delete);
        }
        zlistx_destroy (&to_delete);
        device = (zhashx_t *) zhashx_next (self->devices);
    }
}

//  Load rt from disk
//  If 'fullpath' is NULL does nothing
//  0 - success, -1 - error
int
rt_load (rt_t *self, const char *fullpath)
{
    assert (self);
    if (!fullpath)
        return 0;

    zfile_t *file = zfile_new (NULL, fullpath);
    if (!file) {
        log_error ("zfile_new (path = NULL, file = '%s') failed.", fullpath);
        return -1;
    }
    if (!zfile_is_regular (file)) {
        log_error ("zfile_is_regular () == false");
        zfile_close (file);
        zfile_destroy (&file);
        return -1;
    }
    if (zfile_input (file) == -1) {
        zfile_close (file);
        zfile_destroy (&file);
        log_error ("zfile_input () failed; filename = '%s'", zfile_filename (file, NULL));
        return -1;
    }

    off_t cursize = zfile_cursize (file);
    if (cursize == 0) {
        log_debug ("state file '%s' is empty", zfile_filename (file, NULL));
        zfile_close (file);
        zfile_destroy (&file);
        return 0;
    }

    zchunk_t *chunk = zchunk_read (zfile_handle (file), cursize);
    assert (chunk);
    zframe_t *frame = zframe_new (zchunk_data (chunk), zchunk_size (chunk));
    assert (frame);
    zchunk_destroy (&chunk);

    zfile_close (file);
    zfile_destroy (&file);

    /* Note: Protocol data uses 8-byte sized words, and zmsg_XXcode and file
     * functions deal with platform-dependent unsigned size_t and signed off_t.
     * The off_t is a difficult one to print portably, SO suggests casting to
     * the intmax type and printing that :)
     * https://stackoverflow.com/questions/586928/how-should-i-print-types-like-off-t-and-size-t
     */
    uint64_t offset = 0;
    log_debug ("zfile_cursize == %jd", (intmax_t)cursize);

    while (offset < cursize) {
        byte *prefix = zframe_data (frame) + offset;
        byte *data = zframe_data (frame) + offset + sizeof (uint64_t);
        offset += (uint64_t) *prefix +  sizeof (uint64_t);
        log_debug ("prefix == %" PRIu64 "; offset = %jd ", (uint64_t ) *prefix, (intmax_t)offset);

/* Note: the CZMQ_VERSION_MAJOR comparison below actually assumes versions
 * we know and care about - v3.0.2 (our legacy default, already obsoleted
 * by upstream), and v4.x that is in current upstream master. If the API
 * evolves later (incompatibly), these macros will need to be amended.
 */
        zmsg_t *zmessage = NULL;
#if CZMQ_VERSION_MAJOR == 3
        zmessage = zmsg_decode (data, (size_t) *prefix);
#else
        {
            zframe_t *fr = zframe_new (data, (size_t) *prefix);
            zmessage = zmsg_decode (fr);
            zframe_destroy (&fr);
        }
#endif
        assert (zmessage);
        fty_proto_t *metric = fty_proto_decode (&zmessage); // zmessage destroyed
        if (! metric) {
            // state file is broken
            zsys_error ("state file '%s' is broken", fullpath);
            break;
        }
        rt_put (self, &metric);
    }

    zframe_destroy (&frame);
    return 0;
}

//  Save rt to disk
//  If 'fullpath' is NULL does nothing
//  0 - success, -1 - error
int
rt_save (rt_t *self, const char *fullpath)
{
    assert (self);
    if (!fullpath)
        return 0;

    zfile_t *file = zfile_new (NULL, fullpath);
    if (!file) {
        log_error ("zfile_new (path = NULL, file = '%s') failed.", fullpath);
        return -1;
    }

    zfile_remove (file);

    if (zfile_output (file) == -1) {
        log_error ("zfile_output () failed; filename = '%s'", zfile_filename (file, NULL));
        zfile_close (file);
        zfile_destroy (&file);
        return -1;
    }

    zchunk_t *chunk = zchunk_new (NULL, 0); // TODO: this can be tweaked to
                                            // avoid a lot of allocs
    assert (chunk);

    /* Note: Protocol data uses 8-byte sized words, and zmsg_XXcode and file
     * functions deal with platform-dependent unsigned size_t and signed off_t
     */
    zhashx_t *device = (zhashx_t *) zhashx_first (self->devices);
    while (device) {
        log_debug ("%s", (const char *) zhashx_cursor (self->devices));

        fty_proto_t *metric = (fty_proto_t *) zhashx_first (device);
        while (metric) {
            uint64_t size = 0;  // Note: the zmsg_encode() and zframe_size()
                                // below return a platform-dependent size_t,
                                // but in protocol we use fixed uint64_t
            assert ( sizeof(size_t) <= sizeof(uint64_t) );
            zframe_t *frame = NULL;
            fty_proto_t *duplicate = fty_proto_dup (metric);
            assert (duplicate);
            zmsg_t *zmessage = fty_proto_encode (&duplicate); // duplicate destroyed here
            assert (zmessage);

/* Note: the CZMQ_VERSION_MAJOR comparison below actually assumes versions
 * we know and care about - v3.0.2 (our legacy default, already obsoleted
 * by upstream), and v4.x that is in current upstream master. If the API
 * evolves later (incompatibly), these macros will need to be amended.
 */
#if CZMQ_VERSION_MAJOR == 3
            {
                byte *buffer = NULL;
                size = zmsg_encode (zmessage, &buffer);

                assert (buffer);
                assert (size > 0);
                frame = zframe_new (buffer, size);
                free (buffer); buffer = NULL;
            }
#else
            frame = zmsg_encode (zmessage);
            size = zframe_size (frame);
#endif
            zmsg_destroy (&zmessage);
            assert (frame);
            assert (size > 0);

            // prefix
// FIXME: originally this was for uint64_t, should it be sizeof (size) instead?
// Also is usage of uint64_t here really warranted (e.g. dictated by protocol)?
            zchunk_extend (chunk, (const void *) &size, sizeof (uint64_t));
            // data
            zchunk_extend (chunk, (const void *) zframe_data (frame), zframe_size (frame));

            zframe_destroy (&frame);

            metric = (fty_proto_t *) zhashx_next (device);
        }
        device = (zhashx_t *) zhashx_next (self->devices);
    }

    if (zchunk_write (chunk, zfile_handle (file)) == -1) {
        log_error ("zchunk_write () failed.");
    }

    zchunk_destroy (&chunk);
    zfile_close (file);
    zfile_destroy (&file);
    return 0;
}

//  --------------------------------------------------------------------------
//  Print
void
rt_print (rt_t *self)
{
    // Note: no "if (verbose)" checks in this dedicated routine
    assert (self);
    zhashx_t *device = (zhashx_t *) zhashx_first (self->devices);
    while (device) {
        zsys_debug ("%s", (const char *) zhashx_cursor (self->devices));

        fty_proto_t *metric = (fty_proto_t *) zhashx_first (device);
        while (metric) {
            zsys_debug ("\t%s  -  %" PRIu64" %s %s %s %s %" PRIu32,
                    (const char *) zhashx_cursor (device),
                    fty_proto_time (metric),
                    fty_proto_type (metric),
                    fty_proto_name (metric),
                    fty_proto_value (metric),
                    fty_proto_unit (metric),
                    fty_proto_ttl (metric));
            metric = (fty_proto_t *) zhashx_next (device);
        }
        device = (zhashx_t *) zhashx_next (self->devices);
    }
}

//  --------------------------------------------------------------------------
//  Print list devices
char *
rt_get_list_devices (rt_t *self)
{
    char *devices = (char *) calloc(1024, sizeof(char));
    assert(devices);
    int limit = 1024;
    int conter = 0;
    assert (self);
    zhashx_t *device = (zhashx_t *) zhashx_first (self->devices);
    while (device) {
        conter += (strlen((const char *) zhashx_cursor (self->devices)) + 1);
        if(conter >= limit){
          limit += 1024;
          devices = (char *) realloc(devices, limit);
          assert(devices);
        }
        strcat(devices, (const char *) zhashx_cursor (self->devices));
        strcat(devices, "\n");
        device = (zhashx_t *) zhashx_next (self->devices);
    }
    return devices;
}


//  --------------------------------------------------------------------------
//  Self test of this class

static fty_proto_t *
test_metric_new (
        const char *type,
        const char *element,
        const char *value,
        const char *unit,
        uint32_t ttl
        )
{
    fty_proto_t *metric = fty_proto_new (FTY_PROTO_METRIC);
    fty_proto_set_type (metric, "%s", type);
    fty_proto_set_name (metric, "%s", element);
    fty_proto_set_unit (metric, "%s", unit);
    fty_proto_set_value (metric, "%s", value);
    fty_proto_set_ttl (metric, ttl);
    return metric;
}

static void
test_assert_proto (
        fty_proto_t *p,
        const char *type,
        const char *element,
        const char *value,
        const char *unit,
        uint32_t ttl)
{
    assert (p);
    assert (streq (fty_proto_type (p), type));
    assert (streq (fty_proto_name (p), element));
    assert (streq (fty_proto_unit (p), unit));
    assert (streq (fty_proto_value (p), value));
    assert (fty_proto_ttl (p) == ttl);
}


void
rt_test (bool verbose)
{
    printf (" * rt: \n");

    // Note: If your selftest reads SCMed fixture data, please keep it in
    // src/selftest-ro; if your test creates filesystem objects, please
    // do so under src/selftest-rw. They are defined below along with a
    // usecase (asert) to make compilers happy.
    const char *SELFTEST_DIR_RO = "src/selftest-ro";
    const char *SELFTEST_DIR_RW = "src/selftest-rw";
    assert (SELFTEST_DIR_RO);
    assert (SELFTEST_DIR_RW);
    // std::string str_SELFTEST_DIR_RO = std::string(SELFTEST_DIR_RO);
    // std::string str_SELFTEST_DIR_RW = std::string(SELFTEST_DIR_RW);

    //  @selftest

    rt_t *self = rt_new ();
    assert (self);
    rt_destroy (&self);
    assert (self == NULL);
    rt_destroy (&self);
    rt_destroy (NULL);

    self = rt_new ();

    // fill
    fty_proto_t *metric = test_metric_new ("temp", "ups", "15", "C", 20);
    rt_put (self, &metric);

    metric = test_metric_new ("humidity", "ups", "40", "%", 10);
    rt_put (self, &metric);

    metric = test_metric_new ("ahoy", "ups", "90", "%", 8);
    rt_put (self, &metric);

    metric = test_metric_new ("humidity", "epdu", "21", "%", 10);
    rt_put (self, &metric);

    metric = test_metric_new ("load.input", "switch", "134", "V", 20);
    rt_put (self, &metric);

    metric = test_metric_new ("amperes", "switch", "50", "A", 30);
    rt_put (self, &metric);

    // test
    fty_proto_t *proto = rt_get (self, "ups", "temp");
    test_assert_proto (proto, "temp", "ups", "15", "C", 20);

    proto = rt_get (self, "ups", "ahoy");
    test_assert_proto (proto, "ahoy", "ups", "90", "%", 8);

    proto = rt_get (self, "ups", "humidity");
    test_assert_proto (proto, "humidity", "ups", "40", "%", 10);

    proto = rt_get (self, "epdu", "humidity");
    test_assert_proto (proto, "humidity", "epdu", "21", "%", 10);

    proto = rt_get (self, "switch", "load.input");
    test_assert_proto (proto, "load.input", "switch", "134", "V", 20);

    proto = rt_get (self, "switch", "amperes");
    test_assert_proto (proto, "amperes", "switch", "50", "A", 30);

    proto = rt_get (self, "bubba", "delka"); // non-existing
    assert (proto == NULL);

    // save/load
    char *test_state_file = zsys_sprintf ("%s/test_state_file", SELFTEST_DIR_RW);
    assert (test_state_file != NULL);
    int rv = rt_save (self, test_state_file);
    assert (rv == 0);
    rt_t *loaded = rt_new ();
    rv = rt_load (loaded, test_state_file);
    assert (rv == 0);
    zstr_free (&test_state_file);

    proto = rt_get (loaded, "ups", "temp");
    test_assert_proto (proto, "temp", "ups", "15", "C", 20);

    proto = rt_get (loaded, "ups", "ahoy");
    test_assert_proto (proto, "ahoy", "ups", "90", "%", 8);

    proto = rt_get (loaded, "ups", "humidity");
    test_assert_proto (proto, "humidity", "ups", "40", "%", 10);

    proto = rt_get (loaded, "epdu", "humidity");
    test_assert_proto (proto, "humidity", "epdu", "21", "%", 10);

    proto = rt_get (loaded, "switch", "load.input");
    test_assert_proto (proto, "load.input", "switch", "134", "V", 20);

    proto = rt_get (loaded, "switch", "amperes");
    test_assert_proto (proto, "amperes", "switch", "50", "A", 30);
    rt_destroy (&loaded);

    // rt_get_element
    zhashx_t *r = rt_get_element (self, "fsfwe");
    assert (r == NULL);
    r = rt_get_element (self, "ups");
    assert (r);
    assert (zhashx_size (r) == 3);

    proto = (fty_proto_t *) zhashx_lookup (r, "temp");
    assert (proto);
    test_assert_proto (proto, "temp", "ups", "15", "C", 20);

    proto = (fty_proto_t *) zhashx_lookup (r, "ahoy");
    assert (proto);
    test_assert_proto (proto, "ahoy", "ups", "90", "%", 8);

    proto = (fty_proto_t *) zhashx_lookup (r, "humidity");
    assert (proto);
    test_assert_proto (proto, "humidity", "ups", "40", "%", 10);

    assert (zhashx_lookup (r, "load.input") == NULL);

    // purge imediatelly, nothing should be removed
    rt_purge (self);
    proto = rt_get (self, "ups", "temp");
    test_assert_proto (proto, "temp", "ups", "15", "C", 20);

    proto = rt_get (self, "ups", "ahoy");
    test_assert_proto (proto, "ahoy", "ups", "90", "%", 8);

    proto = rt_get (self, "ups", "humidity");
    test_assert_proto (proto, "humidity", "ups", "40", "%", 10);

    proto = rt_get (self, "epdu", "humidity");
    test_assert_proto (proto, "humidity", "epdu", "21", "%", 10);

    proto = rt_get (self, "switch", "load.input");
    test_assert_proto (proto, "load.input", "switch", "134", "V", 20);

    proto = rt_get (self, "switch", "amperes");
    test_assert_proto (proto, "amperes", "switch", "50", "A", 30);

    // change (ups, humidity) and test
    metric = test_metric_new ("humidity", "ups", "33", "%", 8);
    rt_put (self, &metric);
    proto = rt_get (self, "ups", "humidity");
    test_assert_proto (proto, "humidity", "ups", "33", "%", 8);

    // change (switch, load.input)
    metric = test_metric_new ("load.input", "switch", "1000", "kV", 21);
    rt_put (self, &metric);

    zsys_debug ("Sleeping 9500 ms");
    zclock_sleep (9500);
    rt_purge (self);

    // test
    proto = rt_get (self, "ups", "temp");
    test_assert_proto (proto, "temp", "ups", "15", "C", 20);

    proto = rt_get (self, "ups", "ahoy");
    assert (proto == NULL);

    proto = rt_get (self, "ups", "humidity");
    assert (proto == NULL);

    proto = rt_get (self, "epdu", "humidity");
    test_assert_proto (proto, "humidity", "epdu", "21", "%", 10);

    proto = rt_get (self, "switch", "load.input");
    test_assert_proto (proto, "load.input", "switch", "1000", "kV", 21);

    proto = rt_get (self, "switch", "amperes");
    test_assert_proto (proto, "amperes", "switch", "50", "A", 30);

    zsys_debug ("Sleeping 12600 ms");
    zclock_sleep (12600);
    rt_purge (self);

    // test
    proto = rt_get (self, "ups", "temp");
    assert (proto == NULL);

    proto = rt_get (self, "ups", "humidity");
    assert (proto == NULL);

    proto = rt_get (self, "epdu", "humidity");
    assert (proto == NULL);

    proto = rt_get (self, "switch", "load.input");
    assert (proto == NULL);

    proto = rt_get (self, "switch", "amperes");
    test_assert_proto (proto, "amperes", "switch", "50", "A", 30);

    zsys_debug ("Sleeping 9000 ms");
    zclock_sleep (9000);
    rt_purge (self);

    // test
    proto = rt_get (self, "ups", "temp");
    assert (proto == NULL);

    proto = rt_get (self, "ups", "humidity");
    assert (proto == NULL);

    proto = rt_get (self, "epdu", "humidity");
    assert (proto == NULL);

    proto = rt_get (self, "switch", "load.input");
    assert (proto == NULL);

    proto = rt_get (self, "switch", "amperes");
    assert (proto == NULL);

    // purge on empty
    rt_purge (self);

    rt_destroy (&self);

    //  @end
    printf ("OK\n");
}
