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
    zhashx_t *hash;
    void (*print)(rt_t*);
};

void
rt_print_metrics (rt_t *self);
//  --------------------------------------------------------------------------
//  Create a new rt

rt_t *
rt_new (void)
{
    rt_t *self = (rt_t *) zmalloc (sizeof (rt_t));
    assert (self);
    
    self->hash = zhashx_new();
    assert(self->hash);
    zhashx_set_destructor(self->hash, (zhashx_destructor_fn *) rt_destroy);
    self->print = rt_print;
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the rt

void
rt_destroy (rt_t **self_p)
{
    if (!self_p) return;
    if (*self_p) {
        rt_t *self = *self_p;
        if(self->hash)
	    //zhashx_destroy(&self->hash);
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Store bios_proto_t message and destroy it
void
rt_put (rt_t *self, bios_proto_t **msg_p)
{
    rt_t *device = NULL;
    device =(rt_t*) zhashx_lookup(self->hash, bios_proto_element_src(*msg_p));
    
    if(!device){
        device = (rt_t *) zmalloc (sizeof (rt_t));
        assert(device);
	device->hash = zhashx_new();
	assert(device->hash);
	zhashx_set_destructor(device->hash, (zhashx_destructor_fn *) bios_proto_destroy);
	device->print = rt_print_metrics;
	zhashx_insert(device->hash, bios_proto_type(*msg_p), bios_proto_dup(*msg_p));
	zhashx_insert(self->hash, bios_proto_element_src(*msg_p), device);
	
    /*} else {
        bios_proto_t *metric = NULL;
	metric =(bios_proto_t*) zhashx_lookup(device->hash, bios_proto_type(*msg_p));
	if(!metric)
	    zhashx_insert(device->hash, bios_proto_type(*msg_p), bios_proto_dup(*msg_p));
	else zsys_debug("element already exist\n");*/
    }
    bios_proto_destroy(msg_p);
}

//  --------------------------------------------------------------------------
//  Print
void
rt_print (rt_t *self)
{
  printf("hola!!!!!\n");
  if(self->hash){
    rt_t *aux =(rt_t*) zhashx_first (self->hash);
    printf ("Device: %s\n",(char *) zhashx_cursor (self->hash));
    
    if(aux->print)aux->print(aux);
    while((aux =(rt_t*) zhashx_next(self->hash))){
      printf ("Device: %s\n",(char *) zhashx_cursor (self->hash));
      if(aux->print)aux->print(aux);
    }
    
  }
}

void
rt_print_metrics (rt_t *self)
{
  if(self->hash){
    bios_proto_t *aux =(bios_proto_t*) zhashx_first (self->hash);
    printf ("Metric: %s\n",(char *) zhashx_cursor (self->hash));
    if(aux) bios_proto_print(aux);
    
    while((aux =(bios_proto_t*) zhashx_next(self->hash))){
      printf ("Device: %s\n",(char *) zhashx_cursor (self->hash));
      if(aux) bios_proto_print(aux);
    }
    
  }
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
rt_test (bool verbose)
{
    printf (" * rt: ");

    //  @selftest
    //  Simple test #1
    rt_t *self = rt_new ();
    assert (self);

    bios_proto_t *metric = bios_proto_new (BIOS_PROTO_METRIC);
    assert (metric);

    bios_proto_set_type (metric, "%s", "temperature");
    bios_proto_set_element_src (metric, "%s", "UPS.ROZ33");
    bios_proto_set_unit (metric, "%s", "C");
    bios_proto_set_value (metric, "%s", "15");
    bios_proto_set_ttl (metric, 300);

    rt_put (self, &metric);
    assert (metric == NULL); // Make sure message is deleted

    metric = bios_proto_new (BIOS_PROTO_METRIC);
    assert (metric);

    bios_proto_set_type (metric, "%s", "humidity");
    bios_proto_set_element_src (metric, "%s", "UPS.ROZ33");
    bios_proto_set_unit (metric, "%s", "%");
    bios_proto_set_value (metric, "%s", "40");
    bios_proto_set_ttl (metric, 300);
    
    rt_put (self, &metric);
    assert (metric == NULL); // Make sure message is deleted

    metric = bios_proto_new (BIOS_PROTO_METRIC);
    assert (metric);

    bios_proto_set_type (metric, "%s", "humidity");
    bios_proto_set_element_src (metric, "%s", "ePDU2.ROZ33");
    bios_proto_set_unit (metric, "%s", "%");
    bios_proto_set_value (metric, "%s", "21");
    bios_proto_set_ttl (metric, 330);
    
    rt_put (self, &metric);
    assert (metric == NULL); // Make sure message is deleted

    metric = bios_proto_new (BIOS_PROTO_METRIC);
    assert (metric);

    bios_proto_set_type (metric, "%s", "load.input");
    bios_proto_set_element_src (metric, "%s", "swtich");
    bios_proto_set_unit (metric, "%s", "V");
    bios_proto_set_value (metric, "%s", "134");
    bios_proto_set_ttl (metric, 3330);
    
    rt_put (self, &metric);
    assert (metric == NULL); // Make sure message is deleted

    rt_print (self); // test print

    rt_destroy (&self);
    assert (self == NULL);
    rt_destroy (&self); // Make sure double delete does not crash
    assert (self == NULL);
    rt_destroy (NULL); // Make sure NULL delete does not crash

    //  @end
    printf ("OK\n");
}
