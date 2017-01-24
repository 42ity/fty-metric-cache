/*  =========================================================================
    rt - Metric cache structure

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

#ifndef RT_H_INCLUDED
#define RT_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#ifndef RT_T_DEFINED
typedef struct _rt_t rt_t;
#define RT_T_DEFINED
#endif

//  @interface
//  Create a new rt
FTY_METRIC_CACHE_EXPORT rt_t *
    rt_new (void);

//  Store fty_proto_t message transfering ownership
FTY_METRIC_CACHE_EXPORT void
    rt_put (rt_t *self, fty_proto_t **message);

//  Get specific measurement for given element or NULL when no data
//  Does not transfer ownership
FTY_METRIC_CACHE_EXPORT fty_proto_t *
    rt_get (rt_t *self, const char *element, const char *measurement);

//  Get all measurements for given element or NULL when no data
//  Does not transfer ownership
FTY_METRIC_CACHE_EXPORT zhashx_t *
    rt_get_element (rt_t *self, const char *element);

//  Purge expired data
FTY_METRIC_CACHE_EXPORT void
    rt_purge (rt_t *self);

//  Load rt from disk
//  If 'fullpath' is NULL does nothing
//  0 - success, -1 - error
FTY_METRIC_CACHE_EXPORT int
    rt_load (rt_t *self, const char *fullpath);

//  Save rt to disk
//  If 'fullpath' is NULL does nothing
//  0 - success, -1 - error
FTY_METRIC_CACHE_EXPORT int
    rt_save (rt_t *self, const char *fullpath);

//  Print
FTY_METRIC_CACHE_EXPORT void
    rt_print (rt_t *self);

//  Print list devices
FTY_METRIC_CACHE_EXPORT char *
    rt_get_list_devices  (rt_t *self);

//  Print info of device
FTY_METRIC_CACHE_EXPORT char *
    rt_get_device_info (const char *name, rt_t *self);
//  Destroy the rt
FTY_METRIC_CACHE_EXPORT void
    rt_destroy (rt_t **self_p);

//  Self test of this class
FTY_METRIC_CACHE_EXPORT void
    rt_test (bool verbose);

//  @end

#ifdef __cplusplus
}
#endif

#endif
