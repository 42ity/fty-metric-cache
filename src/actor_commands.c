/*  =========================================================================
    actor_commands - actor commands

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
    actor_commands - actor commands
@discuss
@end
*/

#include "agent_rt_classes.h"

//  Structure of our class

struct _actor_commands_t {
    int filler;     //  Declare class properties here
};


//  --------------------------------------------------------------------------
//  Create a new actor_commands

actor_commands_t *
actor_commands_new (void)
{
    actor_commands_t *self = (actor_commands_t *) zmalloc (sizeof (actor_commands_t));
    assert (self);
    //  Initialize class properties here
    return self;
}


//  --------------------------------------------------------------------------
//  Destroy the actor_commands

void
actor_commands_destroy (actor_commands_t **self_p)
{
    assert (self_p);
    if (*self_p) {
        actor_commands_t *self = *self_p;
        //  Free class properties here
        //  Free object itself
        free (self);
        *self_p = NULL;
    }
}

//  --------------------------------------------------------------------------
//  Self test of this class

void
actor_commands_test (bool verbose)
{
    printf (" * actor_commands: ");

    //  @selftest
    //  Simple create/destroy test
    actor_commands_t *self = actor_commands_new ();
    assert (self);
    actor_commands_destroy (&self);
    //  @end
    printf ("OK\n");
}
