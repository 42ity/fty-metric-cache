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
    int filler;     //  Declare class properties here
};


//  --------------------------------------------------------------------------
//  Create a new rt

rt_t *
rt_new (void)
{
    rt_t *self = (rt_t *) zmalloc (sizeof (rt_t));
    assert (self);
    //  Initialize class properties here
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the rt

void
rt_destroy (rt_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        rt_t *self = *self_p;
        //  Free class properties here
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
rt_test (bool verbose)
{
    printf (" * rt: ");

    //  @selftest
    //  Simple create/destroy test
    rt_t *self = rt_new ();
    assert (self);
    rt_destroy (&self);
    //  @end
    printf ("OK\n");
}
