/* gmr.c */
#include "gmr.h"
#include "gid.h"
#include "gip.h"
#include "garp.h"
#include "gmd.h"
#include "gmf.h"
#include "fdb.h"
/******************************************************************************
 * GMR : GARP MULTICAST REGISTRATION APPLICATION : IMPLEMENTATION SIZING
 ******************************************************************************
 */
enum
{
    Max_multicasts = 100
};
enum
{
    Number_of_gid_machines = Number_of_legacy_controls + Max_multicasts
};
enum
{
    Unused_index = Number_of_gid_machines
};
/******************************************************************************
 * GMR : GARP MULTICAST REGISTRATION APPLICATION : CREATION, DESTRUCTION
 ******************************************************************************
 */
typedef struct /* Gmr */
{
    Garp g;
    unsigned vlan_id;
    void *gmd;
    unsigned number_of_gmd_entries;
    unsigned last_gmd_used_plus1;
} Gmr;
Boolean gmr_create_gmr(int process_id, unsigned vlan_id, void **gmr)
{ /*
   */
    Gmr *my_gmr;
    if (!sysmalloc(sizeof(Gmr), &my_gmr))
        goto gmr_creation_failure;
    my_gmr->g.process_id = process_id;
    my_gmr->g.gid = NULL;
    if (!gip_create_gip(Number_of_gid_machines, &my_gmr->g.gip))
        goto gip_creation_failure;
    my_gmr->g.max_gid_index = Number_of_gid_machines - 1;
    my_gmr->g.last_gid_used = Number_of_legacy_controls - 1;
    my_gmr->g.join_indication_fn = gmr_join_indication;
    my_gmr->g.leave_indication_fn = gmr_leave_indication;
    my_gmr->g.join_propagated_fn = gmr_join_propagated;
    my_gmr->g.leave_propagated_fn = gmr_leave_propagated;
    my_gmr->g.transmit_fn = gmr_tx;
    my_gmr->g.added_port_fn = gmr_added_port;
    my_gmr->g.removed_port_fn = gmr_removed_port;
    my_gmr->vlan_id = vlan_id;
    if (!gmd_create_gmd(Max_multicasts, &my_gmr->gmd))
        goto gmd_creation_failure;
    my_gmr->number_of_gmd_entries = Max_multicasts;
    my_gmr->last_gmd_used_plus1 = 0;
    *gmr = my_gmr;
    return (True);
gmd_creation_failure:
    gip_destroy_gip(my_gmr->g.gip);
gip_creation_failure:
    sysfree(my_gmr);
gmr_creation_failure:
    return (False);
}
void gmr_destroy_gmr(void *gmr)
{
    Gid *my_port;
    Gmr *my_gmr = (Gmr *)gmr;
    gmd_destroy_gmd(my_gmr->gmd);
    gip_destroy_gip(my_gmr->g.gip);
    while ((my_port = my_gmr->g.gid) != NULL)
        gid_destroy_port(&my_gmr->g, my_port->port_no);
}
void gmr_added_port(void *gmr, int port_no)
{ /*
   * Provide any management initialization of legacy control or multicast
   * attributes from templates here for the new port.
   */
  Gmr *my_gmr = (Gmr *)gmr;
}
void gmr_removed_port(void *gmr, int port_no)
{ /*
   * Provide any GMR specific cleanup or management alert functions for the
   * removed port.
   */
  Gmr *my_gmr = (Gmr *)gmr;
}
/******************************************************************************
 * GMR : GARP MULTICAST REGISTRATION APPLICATION : JOIN, LEAVE INDICATIONS
 ******************************************************************************
 */
void gmr_join_indication(void *gmr, Gid *my_port, unsigned joining_gid_index)
{ /*
   * This implementation of gmr_join_indication() respects the three cases
   * described in the header file for the state of the Filtering Database and
   * the registered Legacy controls (Forward All, Forward Unregistered) and
   * Multicasts. It makes some, but not a perfect, attempt to optimize
   * calls to the Filtering Database when one Legacy mode transitions to
   * another.
   *
   */
    unsigned gmd_index;
    unsigned gid_index;
    Mac_address key;
    Gmr *my_gmr = (Gmr *)gmr;
    if (!gid_registered_here(my_port, Forward_all))
    {
        if ((joining_gid_index == Forward_all) || (joining_gid_index == Forward_unregistered))
        {
            gmd_index = 0;
            gid_index = gmd_index + Number_of_legacy_controls;
            while (gmd_index < my_gmr->last_gmd_used_plus1)
            {
                if (!gid_registered_here(my_port, gid_index))
                {
                    if (joining_gid_index == Forward_all)
                    {
                        gmd_get_key(my_gmr->gmd, gmd_index, &key);
                        fdb_forward(my_gmr->vlan_id, my_port->port_no, key);
                    }
                    else if (!gip_propagates_to(my_port, gid_index))
                    { /*(joining_gid_index == Forward_unregistered) */
                        gmd_get_key(my_gmr->gmd, gmd_index, &key);
                        fdb_forward(my_gmr->vlan_id, my_port->port_no, key);
                    }
                }
                gmd_index++;
                gid_index++;
            }
            fdb_forward_by_default(my_gmr->vlan_id, my_port->port_no);
        }
        else /* Multicast Attribute */
        {
            gmd_index = joining_gid_index - Number_of_legacy_controls;
            gmd_get_key(my_gmr->gmd, gmd_index, &key);
            fdb_forward(my_gmr->vlan_id, my_port->port_no, key);
        }
    }
}
void gmr_join_propagated(void *gmr, Gid *my_port, unsigned joining_gid_index)
{ /*
   *
   */
    unsigned gmd_index;
    Mac_address key;
    Gmr *my_gmr = (Gmr *)gmr;
    if (joining_gid_index >= Number_of_legacy_controls)
    { /* Multicast attribute */
        if ((!gid_registered_here(my_port, Forward_all)) && (gid_registered_here(my_port, Forward_unregistered)) && (!gid_registered_here(my_port, joining_gid_index)))
        {
            gmd_index = joining_gid_index - Number_of_legacy_controls;
            gmd_get_key(my_gmr->gmd, gmd_index, &key);
            fdb_filter(my_gmr->vlan_id, my_port->port_no, key);
        }
    }
}
void gmr_leave_indication(void *gmr, Gid *my_port, unsigned leaving_gid_index)
{ /*
   *
   */
  Gmr *my_gmr = (Gmr *)gmr;
    unsigned gmd_index;
    unsigned gid_index;
    Boolean mode_a;
    Boolean mode_c;
    Mac_address key;
    mode_a = gid_registered_here(my_port, Forward_all);
    mode_c = !gid_registered_here(my_port, Forward_unregistered);
    if ((leaving_gid_index == Forward_all) || ((!mode_a) && (leaving_gid_index == Forward_unregistered)))
    {
        gmd_index = 0;
        gid_index = gmd_index + Number_of_legacy_controls;
        while (gmd_index < my_gmr->last_gmd_used_plus1)
        {
            if (!gid_registered_here(my_port, gid_index))
            {
                if (mode_c || gip_propagates_to(my_port, gid_index))
                {
                    gmd_get_key(my_gmr->gmd, gmd_index, &key);
                    fdb_filter(my_gmr->vlan_id, my_port->port_no, key);
                }
            }
            gmd_index++;
            gid_index++;
        }
        if (mode_c)
            fdb_filter_by_default(my_gmr->vlan_id, my_port->port_no);
    }
    else if (!mode_a)
    {
        if (mode_c || gip_propagates_to(my_port, gid_index))
        { /* Multicast Attribute */
            gmd_index = leaving_gid_index - Number_of_legacy_controls;
            gmd_get_key(my_gmr->gmd, gmd_index, &key);
            fdb_filter(my_gmr->vlan_id, my_port->port_no, key);
        }
    }
}
void gmr_leave_propagated(void *gmr, Gid *my_port, unsigned leaving_gid_index)
{ /*
   *
   */
    unsigned gmd_index;
    Mac_address key;
    Gmr *my_gmr = (Gmr *)gmr;
    if (leaving_gid_index >= Number_of_legacy_controls)
    { /* Multicast attribute */
        if ((!gid_registered_here(my_port, Forward_all)) && (gid_registered_here(my_port, Forward_unregistered)) && (!gid_registered_here(my_port, leaving_gid_index)))
        {
            gmd_index = leaving_gid_index - Number_of_legacy_controls;
            gmd_get_key(my_gmr->gmd, gmd_index, &key);
            fdb_forward(my_gmr->vlan_id, my_port->port_no, key);
        }
    }
}
/******************************************************************************
 * GMR : GARP MULTICAST REGISTRATION APPLICATION : RECEIVE MESSAGE PROCESSING
 ******************************************************************************
 */
static void gmr_db_full(void *gmr, Gid *my_port)
{ /*
   * If it is desirable to be able to operate correctly with an undersized
   * database, add code here. The best approach seems to be to use GID
   * management controls to configure the attribute for the Legacy mode
   * control Forward_all to be Registration fixed on all ports on which join
   * messages have been discarded because their keys are not in the database.
   * Then start a retry timer, which attempts to scavenge space from the
   * database at a later time, and, if it succeeds, waits for a few LeaveAll
   * times before switching Forward_all back to Normal_registration.
   */
  Gmr *my_gmr = (Gmr *)gmr;
}
static void gmr_rcv_msg(void *gmr, Gid *my_port, Gmf_msg *msg)
{ /*
   * Process one received message.
   *
   * Dispatch messages by message event, and by attribute type (legacy mode
   * control, or multicast address) except in the case of the LeaveAll
   * message event, which applies equally to all attributes.
   *
   * A LeaveAll message never causes an indication (join or leave directly),
   * even for the point to point link protocol enhancements (where an
   * ordinary Leave does). No further work is needed here.
   *
   * A LeaveAllRange message is currently treated exactly as a LeaveAll
   * (i.e., the range is ignored).
   *
   * All the remaining messages refer to a single attribute (i.e., a single
   * registered group address). Try to find a matching entry in the MCD
   * database. If one is found, dispatch the message to a routine that will
   * handle both the local GID effects and the GIP propagation to other ports.
   *
   * If no entry is found, Leave and Empty messages can be discarded, but
   * JoinIn and JoinEmpty messages demand further treatment. First, an attempt
   * is made to create a new entry using free space (in the database, which
   * corresponds to a free GID machine set). If this fails, an attempt may be
   * made to recover space from a machine set that is in an unused or less
   * significant state. Finally, the database is considered full and the received
   * message is discarded.
   *
   * Once (if) an entry is found, Leave, Empty, JoinIn, and JoinEmpty are
   * all submitted to GID (gid_rcv_msg()).
   *
   * JoinIn and JoinEmpty may cause Join indications, which are then propagated
   * by GIP.
   *
   * On a shared medium, Leave and Empty will not give rise to indications
   * immediately. However, this routine does test for and propagate
   * Leave indications so that it can be used unchanged with a point-to-point
   * protocol enhancement.
   *
   */
  Gmr *my_gmr = (Gmr *)gmr;
    unsigned gmd_index = Unused_index;
    unsigned gid_index = Unused_index;
    if ((msg->event == Gid_rcv_leaveall) || (msg->event == Gid_rcv_leaveall_range))
    {
        gid_rcv_leaveall(my_port);
    }
    else
    {
        if (msg->attribute == Legacy_attribute)
        {
            gid_index = msg->legacy_control;
        }
        else if (!gmd_find_entry(my_gmr->gmd, msg->key1, &gmd_index))
        { /* && (msg->attribute == Multicast_attribute) */
            if ((msg->event == Gid_rcv_joinin) || (msg->event == Gid_rcv_joinempty))
            {
                if (!gmd_create_entry(my_gmr->gmd, msg->key1, &gmd_index))
                {
                    if (gid_find_unused(&my_gmr->g,
                                        Number_of_legacy_controls, &gid_index))
                    {
                        gmd_index = gid_index - Number_of_legacy_controls;
                        gmd_delete_entry(my_gmr->gmd, gmd_index);
                        (void)gmd_create_entry(my_gmr->gmd, msg->key1,
                                               &gmd_index);
                    }
                    else
                        gmr_db_full(my_gmr, my_port);
                }
            }
        }
        if (gmd_index != Unused_index)
            gid_index = gmd_index + Number_of_legacy_controls;
        if (gid_index != Unused_index)
            gid_rcv_msg(my_port, gid_index, msg->event);
    }
}
void gmr_rcv(void *gmr, Gid *my_port, Pdu *pdu)
{ /*
   * Process an entire received pdu for this instance of GMR: initialize
   * the Gmf pdu parsing routine, and, while messages last, read and process
   * them one at a time.
   */
    Gmf gmf;
    Gmf_msg msg;
    Gmr *my_gmr = (Gmr *)gmr;
    gmf_rdmsg_init(&gmf, pdu);
    while (gmf_rdmsg(&gmf, &msg))
        gmr_rcv_msg(my_gmr, my_port, &msg);
}
/******************************************************************************
 * GMR : GARP MULTICAST REGISTRATION APPLICATION : TRANSMIT PROCESSING
 ******************************************************************************
 */
static void gmr_tx_msg(void *gmr, unsigned gid_index, Gmf_msg *msg)
{
    unsigned gmd_index;
    Gmr *my_gmr = (Gmr *)gmr;
    if (msg->event == Gid_tx_leaveall)
    {
        msg->attribute = All_attributes;
    }
    else if (gid_index == Forward_all)
    {
        msg->attribute = Legacy_attribute;
        msg->legacy_control = Forward_all;
    }
    else /* index for Multicast_attribute */
    {
        msg->attribute = Multicast_attribute;
        gmd_index = gid_index - Number_of_legacy_controls;
        gmd_get_key(my_gmr->gmd, gmd_index, &msg->key1);
    }
}
void gmr_tx(void *gmr, Gid *my_port)
{ /*
   * Get and prepare a pdu for the transmission, if one is not available,
   * simply return; if there is more to transmit, GID will reschedule a call
   * to this function.
   *
   * Get messages to transmit from GID and pack them into the pdu using Gmf
   * (MultiCast pdu Formatter).
   */
    Pdu *pdu;
    Gmf gmf;
    Gmf_msg msg;
    Gid_event tx_event;
    unsigned gid_index;
    Gmr *my_gmr = (Gmr *)gmr;
    if ((tx_event = gid_next_tx(my_port, &gid_index)) != Gid_null)
    {
        if (syspdu_alloc(&pdu))
        {
            gmf_wrmsg_init(&gmf, pdu, my_gmr->vlan_id);
            do
            {
                msg.event = tx_event;
                gmr_tx_msg(my_gmr, gid_index, &msg);
                if (!gmf_wrmsg(&gmf, &msg))
                {
                    gid_untx(my_port);
                    break;
                }
            } while ((tx_event = gid_next_tx(my_port, &gid_index)) != Gid_null);
            syspdu_tx(pdu, my_port->port_no);
        }
    }
}