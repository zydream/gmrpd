/* gip.c */
#include "gid.h"
#include "gip.h"
/******************************************************************************
 * GIP : GARP INFORMATION PROPAGATION : CREATION, DESTRUCTION
 ******************************************************************************
 */
Boolean gip_create_gip(unsigned max_attributes, unsigned **gip)
{ /*
   * GIP maintains a set of propagation counts for up to max attributes.
   * It currently maintains no additional information, so the GIP instance
   * is represented directly by a pointer to the array of propagation counts.
   */
    unsigned *my_gip;
    if (!sysmalloc(sizeof(unsigned) * max_attributes, &my_gip))
        goto gip_creation_failure;
    *gip = my_gip;
    return (True);
gip_creation_failure:
    return (False);
}
void gip_destroy_gip(void *gip)
{ /*
   *
   */
    sysfree(gip);
}
/******************************************************************************
 * GIP : GARP INFORMATION PROPAGATION : CONNECT, DISCONNECT PORTS
 ******************************************************************************
 */
static void gip_connect_into_ring(Gid *my_port)
{
    Gid *first_connected, *last_connected;
    my_port->is_connected = True;
    my_port->next_in_connected_ring = my_port;
    first_connected = my_port;
    do
        first_connected = first_connected->next_in_port_ring;
    while (!first_connected->is_connected);
    my_port->next_in_connected_ring = first_connected;
    last_connected = first_connected;
    while (last_connected->next_in_connected_ring != first_connected)
        last_connected = last_connected->next_in_connected_ring;
    last_connected->next_in_connected_ring = my_port;
}
static void gip_disconnect_from_ring(Gid *my_port)
{
    Gid *first_connected, *last_connected;
    first_connected = my_port->next_in_connected_ring;
    my_port->next_in_connected_ring = my_port;
    my_port->is_connected = False;
    last_connected = first_connected;
    while (last_connected->next_in_connected_ring != my_port)
        last_connected = last_connected->next_in_connected_ring;
    last_connected->next_in_connected_ring = first_connected;
}
void gip_connect_port(Garp *application, int port_no)
{ /*
   * If a GID instance for this application and port number is found, is
   * enabled, and is not already connected, then connect that port into the
   * GIP propagation ring.
   *
   * Propagate every attribute that has been registered (i.e., the Registrar
   * appears not to be Empty) on any other connected port, and that has in
   * consequence a nonzero propagation count, to this port, generating a join
   * request.
   *
   * Propagate every attribute that has been registered on this port and not
   * on any others (having a propagation count of zero prior to connecting this
   * port) to all the connected ports, updating propagation counts.
   *
   * Action any timers required. Mark the port as connected.
   *
   */
    Gid *my_port;
    unsigned gid_index;
    if (gid_find_port(application->gid, port_no, &my_port))
    {
        if ((!my_port->is_enabled) || (my_port->is_connected))
            return;
        gip_connect_into_ring(my_port);
        for (gid_index = 0; gid_index <= application->last_gid_used;
             gid_index++)
        {
            if (gip_propagates_to(my_port, gid_index))
                gid_join_request(my_port, gid_index);
            if (gid_registered_here(my_port, gid_index))
                gip_propagate_join(my_port, gid_index);
        }
        gip_do_actions(my_port);
        my_port->is_connected = True;
    }
}
void gip_disconnect_port(Garp *application, int port_no)
{ /*
   * Reverses the operations performed by gip_connect_port().
   */
    Gid *my_port;
    unsigned gid_index;
    if (gid_find_port(application->gid, port_no, &my_port))
    {
        if ((!my_port->is_enabled) || (!my_port->is_connected))
            return;
        for (gid_index = 0; gid_index <= application->last_gid_used;
             gid_index++)
        {
            if (gip_propagates_to(my_port, gid_index))
                gid_leave_request(my_port, gid_index);
            if (gid_registered_here(my_port, gid_index))
                gip_propagate_leave(my_port, gid_index);
        }
        gip_do_actions(my_port);
        gip_disconnect_from_ring(my_port);
        my_port->is_connected = False;
    }
}
/******************************************************************************
 * GIP : GARP INFORMATION PROPAGATION : PROPAGATE SINGLE ATTRIBUTES
 ******************************************************************************
 */
void gip_propagate_join(Gid *my_port, unsigned gid_index)
{ /*
   * Propagates a join indication, causing join requests to other ports
   * if required.
   *
   * The join needs to be propagated if either (a) this is the first port in
   * the connected group to register membership, or (b) there is one other port
   * in the group registering membership, but no further port that would cause
   * a join request to that port.
   *
   */
    unsigned joining_members;
    Gid *to_port;
    if (my_port->is_connected)
    {
        joining_members = (my_port->application->gip[gid_index] += 1);
        if (joining_members <= 2)
        {
            to_port = my_port;
            while ((to_port = to_port->next_in_connected_ring) != my_port)
            {
                if ((joining_members == 1) || (gid_registered_here(to_port, gid_index)))
                {
                    gid_join_request(to_port, gid_index);
                    to_port->application->join_propagated_fn(
                        my_port->application,
                        my_port, gid_index);
                }
            }
        }
    }
}
void gip_propagate_leave(Gid *my_port, unsigned gid_index)
{
    /* Propagates a leave indication for a single attribute, causing leave
     * requests to those other ports if required.
     *
     * See the comments for gip_propagate_join() before reading further.
     * This function decrements the ‘dead-reckoning’ membership count.
     *
     * The first step is to check that this port is connected to any others; if
     * not, the leave indication should not be propagated, nor should the joined
     * membership be decremented. Otherwise, the leave will need to be propagated
     * if this is either (a) the last port in the connected group to register
     * membership, or (b) there is one other port in the group registering
     * membership, in which case the leave request needs to be sent to that
     * port alone.
     */
    unsigned remaining_members;
    Gid *to_port;
    if (my_port->is_connected)
    {
        remaining_members = (my_port->application->gip[gid_index] -= 1);
        if (remaining_members <= 1)
        {
            to_port = my_port;
            while ((to_port = to_port->next_in_connected_ring) != my_port)
            {
                if ((remaining_members == 0) || (gid_registered_here(to_port, gid_index)))
                {
                    gid_leave_request(to_port, gid_index);
                    to_port->application->leave_propagated_fn(
                        my_port->application,
                        my_port, gid_index);
                }
            }
        }
    }
}
Boolean gip_propagates_to(Gid *my_port, unsigned gid_index)
{
    if ((my_port->is_connected) && ((my_port->application->gip[gid_index] == 2) || ((my_port->application->gip[gid_index] == 1) && (!gid_registered_here(my_port, gid_index)))))
        return (True);
    else
        return (False);
}
/******************************************************************************
 * GIP : GARP INFORMATION PROPAGATION : ACTION TIMERS
 ******************************************************************************
 */
void gip_do_actions(Gid *my_port)
{ /*
   * Calls GID to carry out GID ‘scratchpad’ actions accumulated during this
   * invocation of GARP for all the ports in the GIP ring, including my port.
   */
    Gid *this_port = my_port;
    do
        gid_do_actions(this_port);
    while ((this_port = this_port->next_in_connected_ring) != my_port);
}