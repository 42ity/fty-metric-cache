/*  =========================================================================
    rt - agent rt structure

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
    rt - agent rt structure
@discuss
@end
*/

#include "agent_rt_classes.h"

//  Structure of our class

struct _rt_t {
    zhashx_t *devices;     // hash of hashes ("device name", ("measurement", bios_proto_t*))
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
//  Store bios_proto_t message transfering ownership

void
rt_put (rt_t *self, bios_proto_t **message_p)
{
    assert (self);
    assert (message_p);
    
    bios_proto_t *message = *message_p;

    if (!message)
        return;

    uint64_t timestamp = (uint64_t) zclock_mono ();
    bios_proto_aux_insert (message, "time", "%" PRIu64, timestamp); 

    zhashx_t *device = (zhashx_t *) zhashx_lookup (self->devices, bios_proto_element_src (message));
    if (!device) {
        zhashx_t *metrics = zhashx_new ();
        zhashx_set_destructor (metrics, (zhashx_destructor_fn *) bios_proto_destroy);

        int rv = zhashx_insert (self->devices, bios_proto_element_src (message), metrics);
        assert (rv == 0);
        device = metrics;
    }
    zhashx_update (device, bios_proto_type (message), message);
    *message_p = NULL;
}

//  --------------------------------------------------------------------------
//  Get specific measurement for given device or NULL when no data

bios_proto_t *
rt_get (rt_t *self, const char *element, const char *measurement)
{
    assert (self);
    assert (element);
    assert (measurement);

    zhashx_t *device = (zhashx_t *) zhashx_lookup (self->devices, element);
    if (!device)
        return NULL;
    return (bios_proto_t *) zhashx_lookup (device, measurement);
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
    uint64_t timestamp = (uint64_t) zclock_mono ();
    zhashx_t *device = (zhashx_t *) zhashx_first (self->devices);
    while (device) {
        bios_proto_t *metric = (bios_proto_t *) zhashx_first (device);
        while (metric) {
            uint64_t time = bios_proto_aux_number (metric, "time", 0);
            if (time + (bios_proto_ttl (metric) * 1000) < timestamp) {
                zhashx_delete (device, (const char *) zhashx_cursor (device));
            } 
            metric = (bios_proto_t *) zhashx_next (device);
        }
        device = (zhashx_t *) zhashx_next (self->devices);
    }
}

//  --------------------------------------------------------------------------
//  Print
void
rt_print (rt_t *self)
{
    assert (self);
    zhashx_t *device = (zhashx_t *) zhashx_first (self->devices);
    while (device) {
        zsys_debug ("%s", (const char *) zhashx_cursor (self->devices));

        bios_proto_t *metric = (bios_proto_t *) zhashx_first (device);
        while (metric) {
            zsys_debug ("\t%s  -  %" PRIu64" %s %s %s %s %" PRIu32,
                    (const char *) zhashx_cursor (device),
                    bios_proto_aux_number (metric, "time", 0),
                    bios_proto_type (metric),
                    bios_proto_element_src (metric),
                    bios_proto_value (metric),
                    bios_proto_unit (metric),
                    bios_proto_ttl (metric));
            metric = (bios_proto_t *) zhashx_next (device);
        }
        device = (zhashx_t *) zhashx_next (self->devices);
    }
}



//  --------------------------------------------------------------------------
//  Self test of this class

static bios_proto_t *
test_metric_new (
        const char *type,
        const char *element,
        const char *value,
        const char *unit,
        uint32_t ttl
        )
{
    bios_proto_t *metric = bios_proto_new (BIOS_PROTO_METRIC);
    bios_proto_set_type (metric, "%s", type);
    bios_proto_set_element_src (metric, "%s", element);
    bios_proto_set_unit (metric, "%s", unit);
    bios_proto_set_value (metric, "%s", value);
    bios_proto_set_ttl (metric, ttl);
    return metric;
}

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
rt_test (bool verbose)
{
    printf (" * rt: \n");

    //  @selftest
    
    rt_t *self = rt_new ();
    assert (self);
    rt_destroy (&self);
    assert (self == NULL);
    rt_destroy (&self);
    rt_destroy (NULL);

    self = rt_new ();

    // fill
    bios_proto_t *metric = test_metric_new ("temp", "ups", "15", "C", 10);
    rt_put (self, &metric);    

    metric = test_metric_new ("humidity", "ups", "40", "%", 20);
    rt_put (self, &metric);

    metric = test_metric_new ("humidity", "epdu", "21", "%", 10);
    rt_put (self, &metric);

    metric = test_metric_new ("load.input", "switch", "134", "V", 20);
    rt_put (self, &metric);
    
    metric = test_metric_new ("amperes", "switch", "50", "A", 30);
    rt_put (self, &metric);

    // test
    bios_proto_t *proto = rt_get (self, "ups", "temp");
    test_assert_proto (proto, "temp", "ups", "15", "C", 10);

    proto = rt_get (self, "ups", "humidity");
    test_assert_proto (proto, "humidity", "ups", "40", "%", 20);
    
    proto = rt_get (self, "epdu", "humidity");
    test_assert_proto (proto, "humidity", "epdu", "21", "%", 10);

    proto = rt_get (self, "switch", "load.input");
    test_assert_proto (proto, "load.input", "switch", "134", "V", 20);
    
    proto = rt_get (self, "switch", "amperes");
    test_assert_proto (proto, "amperes", "switch", "50", "A", 30);

    proto = rt_get (self, "bubba", "delka"); // non-existing
    assert (proto == NULL);

    // rt_get_element
    zhashx_t *r = rt_get_element (self, "fsfwe");
    assert (r == NULL);
    r = rt_get_element (self, "ups");
    assert (r);
    assert (zhashx_size (r) == 2);

    proto = (bios_proto_t *) zhashx_lookup (r, "temp");
    assert (proto);
    test_assert_proto (proto, "temp", "ups", "15", "C", 10);

    proto = (bios_proto_t *) zhashx_lookup (r, "humidity");
    assert (proto);
    test_assert_proto (proto, "humidity", "ups", "40", "%", 20);

    assert (zhashx_lookup (r, "load.input") == NULL);

    // purge imediatelly, nothing should be removed
    rt_purge (self);
    proto = rt_get (self, "ups", "temp");
    test_assert_proto (proto, "temp", "ups", "15", "C", 10);

    proto = rt_get (self, "ups", "humidity");
    test_assert_proto (proto, "humidity", "ups", "40", "%", 20);
    
    proto = rt_get (self, "epdu", "humidity");
    test_assert_proto (proto, "humidity", "epdu", "21", "%", 10);

    proto = rt_get (self, "switch", "load.input");
    test_assert_proto (proto, "load.input", "switch", "134", "V", 20);
    
    proto = rt_get (self, "switch", "amperes");
    test_assert_proto (proto, "amperes", "switch", "50", "A", 30);

    // change (ups, temp) and test
    metric = test_metric_new ("temp", "ups", "170", "K", 8);
    rt_put (self, &metric);   
    proto = rt_get (self, "ups", "temp");
    test_assert_proto (proto, "temp", "ups", "170", "K", 8);

    // change (switch, load.input)
    metric = test_metric_new ("load.input", "switch", "1000", "kV", 21);
    rt_put (self, &metric);
    
    zsys_debug ("Sleeping 8500 ms");
    zclock_sleep (8500);
    rt_purge (self);

    // test 
    proto = rt_get (self, "ups", "temp");
    assert (proto == NULL);

    proto = rt_get (self, "ups", "humidity");
    test_assert_proto (proto, "humidity", "ups", "40", "%", 20);
    
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
