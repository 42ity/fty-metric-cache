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
};

void
rt_print_metrics (zhashx_t *self);

void
rt_destroyer (zhashx_t **self_p)
{
  zhashx_destroy(self_p);
}

//  --------------------------------------------------------------------------
//  Create a new rt

rt_t *
rt_new (void)
{
    rt_t *self = (rt_t *) zmalloc (sizeof (rt_t));
    assert (self);
    
    self->hash = zhashx_new();
    assert(self->hash);
    zhashx_set_destructor(self->hash, (zhashx_destructor_fn *) rt_destroyer);
    
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

        if (self->hash)
	        zhashx_destroy(&self->hash);
	
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Store bios_proto_t message and destroy it
void
rt_put (rt_t *self, bios_proto_t **msg_p)
{
    zhashx_t *device = NULL;
    if (self)
        device = (zhashx_t*) zhashx_lookup (self->hash, bios_proto_element_src(*msg_p));
    
	bios_proto_t *new_p = bios_proto_dup(*msg_p);
	bios_proto_aux_insert(new_p, "time","%lld",(long long) zclock_mono());
	
    if (!device) {
        device = zhashx_new();
        assert (device);
	
    	zhashx_set_destructor (device, (zhashx_destructor_fn *) bios_proto_destroy);
	
    	zhashx_insert(device, bios_proto_type(*msg_p), new_p);
	    zhashx_insert(self->hash, bios_proto_element_src(*msg_p), device);
	}
    else {
        bios_proto_t *metric = NULL;
    	metric =(bios_proto_t*) zhashx_lookup(device, bios_proto_type(*msg_p));
	
    	if (!metric)
	        zhashx_insert(device, bios_proto_type(*msg_p), new_p);
	   	else {
		  zhashx_delete(device, bios_proto_type(*msg_p));
		  zhashx_insert(device, bios_proto_type(*msg_p), new_p);
		}
            
    }
    bios_proto_destroy(msg_p);
}

void
rt_purge (rt_t *self, const char *device, const char *metric){
    zhashx_t *dev = NULL;
    dev =(zhashx_t*) zhashx_lookup(self->hash, device);
    if(dev){
        zhashx_delete(dev, metric);
	if(zhashx_size(dev) == 0)
	  zhashx_delete(self->hash, device);
    }
}
//  --------------------------------------------------------------------------
//  Print
void
rt_print (rt_t *self)
{
  if((self)&&(self->hash)){
    zhashx_t *aux =(zhashx_t*) zhashx_first (self->hash);
    printf ("\n\n\nDevice: %s\n",(char *) zhashx_cursor (self->hash));
    
    if(aux)rt_print_metrics(aux);
    while((aux =(zhashx_t*) zhashx_next(self->hash))){
      printf ("\n\n\nDevice: %s\n",(char *) zhashx_cursor (self->hash));
      if(aux)rt_print_metrics(aux);
    }
    
  }
}

void
rt_print_metrics (zhashx_t *self)
{
  if(self){
    bios_proto_t *aux =(bios_proto_t*) zhashx_first (self);
    printf ("\n---> Metric: %s\n",(char *) zhashx_cursor (self));
    if(aux) bios_proto_print(aux);
    
    while((aux =(bios_proto_t*) zhashx_next(self))){
      printf ("\n---> Metric: %s\n",(char *) zhashx_cursor (self));
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
    
    metric = bios_proto_new (BIOS_PROTO_METRIC);
    assert (metric);
    
    bios_proto_set_type (metric, "%s", "load.output");
    bios_proto_set_element_src (metric, "%s", "swtich");
    bios_proto_set_unit (metric, "%s", "V");
    bios_proto_set_value (metric, "%s", "134");
    bios_proto_set_ttl (metric, 3330);
    
    rt_put (self, &metric);
    assert (metric == NULL); // Make sure message is deleted
    
    printf("\n----------> Elements:\n\n");
    rt_print (self); // test print
    
    rt_purge(self,"swtich","load");
    rt_purge(self,"swtich","load.output");
    rt_purge(self,"swtich","load.input");
    rt_purge(self,"UPS.ROZ33","temperature");
    rt_purge(self,"ePDU2.ROZ33","humidity");
    
    printf("\n\nElements after rf_purge() everything except UPS.ROZ33 - humidity\n\n");
    
    rt_print (self); // test print

    printf ("\n\nput () on UPS.ROZ33 - humidity\n");
    
    metric = bios_proto_new (BIOS_PROTO_METRIC);
    assert (metric);

    bios_proto_set_type (metric, "%s", "humidity");
    bios_proto_set_element_src (metric, "%s", "UPS.ROZ33");
    bios_proto_set_unit (metric, "%s", "%");
    bios_proto_set_value (metric, "%s", "73");
    bios_proto_set_ttl (metric, 123);
    
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
