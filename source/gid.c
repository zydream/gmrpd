/* gid.c */
#include "sys.h"
#include "gid.h"
#include "gidtt.h"
#include "gip.h"
#include "garp.h"
/******************************************************************************
 * GID : GARP INFORMATION DISTRIBUTION PROTOCOL : CREATION, DESTRUCTION
 ******************************************************************************
 */
static Boolean gid_create_gid(Garp *application, int port_no, void **gid)
{ /*
   * Creates a new instance of GID.
   */
    Gid *my_port;
    if (!sysmalloc(sizeof(Gid), &my_port))
        goto gid_creation_failure;
    my_port->application = application;
    my_port->port_no = port_no;
    my_port->next_in_port_ring = my_port;
    my_port->next_in_connected_ring = my_port;
    my_port->is_enabled = False;
    my_port->is_connected = False;
    my_port->is_point_to_point = True;
    my_port->cschedule_tx_now = False;
    my_port->cstart_join_timer = False;
    my_port->cstart_leave_timer = False;
    my_port->tx_now_scheduled = False;
    my_port->join_timer_running = False;
    my_port->leave_timer_running = False;
    my_port->hold_tx = False;
    my_port->join_timeout = Gid_default_join_time;
    my_port->leave_timeout_4 = Gid_default_leave_time / 4;
    my_port->hold_timeout = Gid_default_hold_time;
    if (!sysmalloc(sizeof(Gid_machine) * (application->max_gid_index + 2),
                   &my_port->machines))
        goto gid_mcreation_failure;
    my_port->leaveall_countdown = Gid_leaveall_count;
    my_port->leaveall_timeout_n = Gid_default_leaveall_time /
                                  Gid_leaveall_count;
    systime_start_timer(my_port->application->process_id,
                        gid_leaveall_timer_expired,
                        my_port->port_no,
                        my_port->leaveall_timeout_n);
    my_port->tx_pending = False;
    my_port->last_transmitted = application->last_gid_used;
    my_port->last_to_transmit = application->last_gid_used;
    my_port->untransmit_machine = application->last_gid_used + 1;
    *gid = my_port;
    return (True);
gid_mcreation_failure:
    sysfree(my_port);
gid_creation_failure:
    return (False);
}
static void gid_destroy_gid(Gid *gid)
{ /*
   * Destroys the instance of GID, releasing previously allocated space.
   * Sends leave indications to the application for previously registered
   * attributes.
   */
    unsigned gid_index;
    for (gid_index = 0; gid_index <= gid->application->last_gid_used;
         gid_index++)
    {
        if (gid_registered_here(gid, gid_index))
            gid->application->leave_indication_fn(gid->application,
                                                  gid, gid_index);
    }
    sysfree(gid->machines);
    sysfree(gid);
}
static Gid *gid_add_port(Gid *existing_ports, Gid *new_port)
{ /*
   * Adds new_port to the port ring.
   */
    Gid *prior;
    Gid *next;
    int new_port_no;
    if (existing_ports != NULL)
    {
        new_port_no = new_port->port_no;
        next = existing_ports;
        for (;;)
        {
            prior = next;
            next = prior->next_in_port_ring;
            if (prior->port_no <= new_port_no)
            {
                if ((next->port_no <= prior->port_no) || (next->port_no > new_port_no))
                    break;
            }
            else /* if (prior_>port_no > new_port_no) */
            {
                if ((next->port_no <= prior->port_no) && (next->port_no > new_port_no))
                    break;
            }
        }
        if (prior->port_no == new_port_no)
            syserr_panic();
        prior->next_in_port_ring = new_port;
        new_port->next_in_port_ring = next;
    }
    new_port->is_enabled = True;
    return (new_port);
}
static Gid *gid_remove_port(Gid *my_port)
{
    Gid *prior;
    Gid *next;
    prior = my_port;
    while ((next = prior->next_in_port_ring) != my_port)
        prior = next;
    prior->next_in_port_ring = my_port->next_in_port_ring;
    if (prior == my_port)
        return (NULL);
    else
        return (prior);
}
Boolean gid_create_port(Garp *application, int port_no)
{
    Gid *my_port;
    if (!gid_find_port(application->gid, port_no, &my_port))
    {
        if (gid_create_gid(application, port_no, &my_port))
        {
            application->gid = gid_add_port(application->gid, my_port);
            application->added_port_fn(application, port_no);
            return (True);
        }
    }
    return (False);
}
void gid_destroy_port(Garp *application, int port_no)
{
    Gid *my_port;
    if (gid_find_port(application->gid, port_no, &my_port))
    {
        gip_disconnect_port(application, port_no);
        application->gid = gid_remove_port(my_port);
        gid_destroy_gid(my_port);
        application->removed_port_fn(application, port_no);
    }
}
/******************************************************************************
 * GID : GARP INFORMATION DISTRIBUTION PROTOCOL : USEFUL FUNCTIONS
 ******************************************************************************
 */
Boolean gid_find_port(Gid *first_port, int port_no, void **gid)
{
    Gid *next_port = first_port;
    while (next_port->port_no != port_no)
    {
        if ((next_port = next_port->next_in_port_ring) == first_port)
            return (False);
    }
    *gid = next_port;
    return (True);
}
Gid *gid_next_port(Gid *this_port)
{
    return (this_port->next_in_port_ring);
}
/******************************************************************************
 * GID : GARP INFORMATION DISTRIBUTION PROTOCOL : MGT
 ******************************************************************************
 */
void gid_read_attribute_state(Gid *my_port, unsigned index, Gid_states *state)
{ /*
   */
    gidtt_states(&my_port->machines[index], state);
}
void gid_manage_attribute(Gid *my_port, unsigned index, Gid_event directive)
{ /*
   *
   */
    Gid_machine *machine;
    Gid_event event;
    machine = &my_port->machines[index];
    event = gidtt_event(my_port, machine, directive);
    if (event == Gid_join)
    {
        my_port->application->join_indication_fn(my_port->application,
                                                 my_port, index);
        gip_propagate_join(my_port, index);
    }
    else if (event == Gid_leave)
    {
        my_port->application->leave_indication_fn(my_port->application,
                                                  my_port, index);
        gip_propagate_leave(my_port, index);
    }
}
Boolean gid_find_unused(Garp *application, unsigned from_index,
                        unsigned *found_index)
{
    unsigned gid_index;
    Gid *check_port;
    gid_index = from_index;
    check_port = application->gid;
    for (;;)
    {
        if (gidtt_machine_active(&check_port->machines[gid_index]))
        {
            if (gid_index++ > application->last_gid_used)
                return (False);
            check_port = application->gid;
        }
        else if ((check_port = check_port->next_in_port_ring) == application->gid)
        {
            *found_index = gid_index;
            return (True);
        }
    }
}
/******************************************************************************
 * GID : GARP INFORMATION DISTRIBUTION PROTOCOL : EVENT PROCESSSING
 ******************************************************************************
 */
static void gid_leaveall(Gid *my_port)
{ /*
   * only for shared media at present
   */
    unsigned i;
    Garp *application;
    application = my_port->application;
    for (i = 0; i <= application->last_gid_used; i++)
        (void)gidtt_event(my_port, &my_port->machines[i], Gid_rcv_leaveempty);
}
void gid_rcv_leaveall(Gid *my_port)
{
    my_port->leaveall_countdown = Gid_leaveall_count;
    gid_leaveall(my_port);
}
void gid_rcv_msg(Gid *my_port, unsigned index, Gid_event msg)
{
    Gid_machine *machine;
    Gid_event event;
    machine = &my_port->machines[index];
    event = gidtt_event(my_port, machine, msg);
    if (event == Gid_join)
    {
        my_port->application->join_indication_fn(my_port->application,
                                                 my_port, index);
        gip_propagate_join(my_port, index);
    }
    else if (event == Gid_leave)
    {
        my_port->application->leave_indication_fn(my_port->application,
                                                  my_port, index);
        gip_propagate_leave(my_port, index);
    }
}
void gid_join_request(Gid *my_port, unsigned gid_index)
{
    (void)(gidtt_event(my_port, &my_port->machines[gid_index], Gid_join));
}
void gid_leave_request(Gid *my_port, unsigned gid_index)
{
    (void)(gidtt_event(my_port, &my_port->machines[gid_index], Gid_leave));
}
Boolean gid_registrar_in(Gid_machine *machine)
{
    return (gidtt_in(machine));
}
/******************************************************************************
 * GID : GARP INFORMATION DISTRIBUTION PROTOCOL : RECEIVE PROCESSING
 ******************************************************************************
 */
void gid_rcv_pdu(Garp *application, int port_no, void *pdu)
{ /*
   * If a GID instance for this application and port number is found and is
   * enabled, pass the PDU to the application, which will parse it (using the
   * application’s own PDU formatting conventions) and call gid_rcv_msg() for
   * each of the conceptual GID message components read from the PDU. Once
   * the application is finished with the PDU, call gip_do_actions() to start
   * timers as recorded in the GID scratchpad for this port and any ports to
   * which it may have propagated joins or leaves.
   *
   * Finally release the received pdu.
   */
    Gid *my_port;
    if (gid_find_port(application->gid, port_no, &my_port))
    {
        if (my_port->is_enabled)
        {
            application->receive_fn(application, my_port, pdu);
            gip_do_actions(my_port);
        }
    }
    /* gid_rlse_rcv_pdu: Insert any system specific action required. */
}
/******************************************************************************
 * GID : GARP INFORMATION DISTRIBUTION PROTOCOL : TRANSMIT PROCESSSING
 ******************************************************************************
 */
Gid_event gid_next_tx(Gid *my_port, unsigned *index)
{ /*
   * Check to see if a leaveall should be sent; if so, return Gid_tx_leaveall;
   * otherwise, scan the GID machines for messsages that require transmission.
   *
   * Machines will be checked for potential transmission starting with the
   * machine following last_transmitted and up to and including last_to_transmit.
   * If all machines have transmitted, last_transmitted equals last_to_transmit
   * and tx_pending is False (in this case tx_pending distinguished the
   * case of ‘all machines are yet to be checked for transmission’ from ‘all have
   * been checked.’)
   *
   * If tx_pending is True and all machines are yet to be checked, transmission
   * will start from the machine with GID index 0, rather than from immediately
   * following last_transmitted.
   */
    unsigned check_index;
    unsigned stop_after;
    Gid_event msg;
    if (my_port->hold_tx)
        return (Gid_null);
    if (my_port->leaveall_countdown == 0)
    {
        my_port->leaveall_countdown = Gid_leaveall_count;
        systime_start_timer(my_port->application->process_id,
                            gid_leaveall_timer_expired,
                            my_port->port_no,
                            my_port->leaveall_timeout_n);
        return (Gid_tx_leaveall);
    }
    if (!my_port->tx_pending)
        return (Gid_null);
    check_index = my_port->last_transmitted + 1;
    stop_after = my_port->last_to_transmit;
    if (stop_after < check_index)
        stop_after = my_port->application->last_gid_used;
    for (;; check_index++)
    {
        if (check_index > stop_after)
        {
            if (stop_after == my_port->last_to_transmit)
            {
                my_port->tx_pending = False;
                return (Gid_null);
            }
            else if (stop_after == my_port->application->last_gid_used)
            {
                check_index = 0;
                stop_after = my_port->last_to_transmit;
            }
        }
        if ((msg = gidtt_tx(my_port, &my_port->machines[check_index])) != Gid_null)
        {
            *index = my_port->last_transmitted = check_index;
            my_port->machines[my_port->untransmit_machine].applicant =
                my_port->machines[check_index].applicant;
            my_port->tx_pending = (check_index == stop_after);
            return (msg);
        }
    }
} /* end for(;;) */
void gid_untx(Gid *my_port)
{
    my_port->machines[my_port->last_transmitted].applicant =
        my_port->machines[my_port->untransmit_machine].applicant;
    if (my_port->last_transmitted == 0)
        my_port->last_transmitted = my_port->application->last_gid_used;
    else
        my_port->last_transmitted--;
    my_port->tx_pending = True;
}
/******************************************************************************
 * GID : GARP INFORMATION DISTRIBUTION PROTOCOL : TIMER PROCESSING
 ******************************************************************************
 */
void gid_do_actions(Gid *my_port)
{ /*
   * Carries out ‘scratchpad’ actions accumulated in this invocation of GID,
   * and outstanding ‘immediate’ transmissions and join timer starts that
   * have been delayed by the operation of the hold timer. Note the way in
   * which the hold timer works here. It could have been specified just to
   * impose a minimum spacing on transmissions - and run in parallel with the
   * join timer - with the effect that the longer of the hold timer and actual
   * join timer values would have determined the actual transmission time.
   * This approach was not taken because it could have led to bunching
   * transmissions at the hold time.
   *
   * The procedure restarts the join timer if there are still transmissions
   * pending (if leaveall_countdown is zero. a Leaveall is to be sent; if
   * tx_pending is true, individual machines may have messages to send.)
   */
    int my_port_no = my_port->port_no;
    if (my_port->cstart_join_timer)
    {
        my_port->last_to_transmit = my_port->last_transmitted;
        my_port->tx_pending = True;
        my_port->cstart_join_timer = False;
    }
    if (!my_port->hold_tx)
    {
        if (my_port->cschedule_tx_now)
        {
            if (!my_port->tx_now_scheduled)
                systime_schedule(my_port->application->process_id,
                                 gid_join_timer_expired,
                                 my_port->port_no);
            my_port->cschedule_tx_now = False;
        }
        else if ((my_port->tx_pending || (my_port->leaveall_countdown == 0)) && (!my_port->join_timer_running))
        {
            systime_start_random_timer(my_port->application->process_id,
                                       gid_join_timer_expired,
                                       my_port->port_no,
                                       my_port->join_timeout);
            my_port->join_timer_running = True;
        }
    }
    if (my_port->cstart_leave_timer && (!my_port->leave_timer_running))
    {
        systime_start_timer(my_port->application->process_id,
                            gid_leave_timer_expired,
                            my_port->port_no,
                            my_port->leave_timeout_4);
    }
    my_port->cstart_leave_timer = False;
}
void gid_leave_timer_expired(Garp *application, int port_no)
{
    Gid *my_port;
    unsigned gid_index;
    if (gid_find_port(application->gid, port_no, &my_port))
    {
        for (gid_index = 0; gid_index < my_port->application->last_gid_used;
             gid_index++)
        {
            if (gidtt_leave_timer_expiry(my_port, &my_port->machines[gid_index]) == Gid_leave)
            {
                my_port->application->leave_indication_fn(my_port->application,
                                                          my_port, gid_index);
                gip_propagate_leave(my_port, gid_index);
            }
        }
    }
}
void gid_leaveall_timer_expired(Garp *application, int port_no)
{
    Gid *my_port;
    if (gid_find_port(application->gid, port_no, &my_port))
    {
        if (my_port->leaveall_countdown > 1)
            my_port->leaveall_countdown--;
        else
        {
            gid_leaveall(my_port);
            my_port->leaveall_countdown = 0;
            my_port->cstart_join_timer = True;
            if ((!my_port->join_timer_running) && (!my_port->hold_tx))
                systime_start_random_timer(my_port->application->process_id,
                                           gid_join_timer_expired,
                                           my_port->port_no,
                                           my_port->join_timeout);
            my_port->cstart_join_timer = False;
            my_port->join_timer_running = True;
        }
    }
}
void gid_join_timer_expired(Garp *application, int port_no)
{
    Gid *my_port;
    if (gid_find_port(application->gid, port_no, &my_port))
    {
        if (my_port->is_enabled)
        {
            application->transmit_fn(application, my_port);
            systime_start_timer(my_port->application->process_id,
                                gid_hold_timer_expired,
                                my_port->port_no,
                                my_port->hold_timeout);
        }
    }
}
void gid_hold_timer_expired(Garp *application, int port_no)
{
    Gid *my_port;
    if (gid_find_port(application->gid, port_no, &my_port))
    {
        my_port->hold_tx = False;
        gid_do_actions(my_port);
    }
}

Boolean gid_registered_here(Gid *my_port, unsigned gid_index)
{  /*
    * Returns TRUE if the Registrar is not Empty, or if Registration is fixed.
    */
    return gidtt_in(&my_port->machines[gid_index]);
#if 0    
    if ( my_port->application->is_registered_fn
            (my_port->application, my_port, gid_index) )
        return TRUE;
    else
        return FALSE;
#endif        
}