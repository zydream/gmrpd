/* gmr.h */
#ifndef gmr_h__
#define gmr_h__
#include "garp.h"
#include "gid.h"
#include "gip.h"
/******************************************************************************
 * GMR : GARP MULTICAST REGISTRATION APPLICATION : GARP ATTRIBUTES
 ******************************************************************************
 */
typedef enum
{
    All_attributes,
    Legacy_attribute,
    Multicast_attribute
} Attribute_type;
typedef enum
{
    Forward_all,
    Forward_unregistered
} Legacy_control;
typedef enum
{
    Number_of_legacy_controls = 1
};
/******************************************************************************
 * GMR : GARP MULTICAST REGISTRATION APPLICATION : CREATION, DESTRUCTION
 ******************************************************************************
 */
extern Boolean gmr_create_gmr(int process_id, unsigned vlan_id,
                              void **gmr);
/*
 * Creates a new instance of GMR, allocating and initializing a control
 * block, returning True and a pointer to this instance if creation succeeds.
 * Also creates instances of MCD (the MultiCast registration Database) and
 * of GIP (which controls information propagation).
 *
 * Ports are created by the system and added to GMR separately (see
 * gmr_added_port() and gmr_removed_port() below).
 *
 * The operating system supplied process_id is for use in subsequent calls
 * to operating system services. The system itself ensures the temporal
 * scope of process_id, guarding against timers yet to expire for destroyed
 * processes, etc. Although process_id will be implicitly supplied by many
 * if not most systems, it is made explicit in this implementation for
 * clarity.
 *
 * The vlan_id provides the context for this instance of GMR.
 * vlan_id 0 is taken to refer to the base LAN, i.e., the LAN as seen by
 * 802.1D prior to the invention of VLANs. This assumption may be subject
 * to further 802.1 discussion.
 */
extern void gmr_destroy_gmr(void *gmr);
/*
 * Destroys an instance of GMR, destroying and deallocating the associated
 * instances of MCD and GIP, and any instances of GID remaining.
 */
extern void gmr_added_port(void *gmr, int port_no);
/*
 * The system has created a new port for this application and added it to
 * the ring of GID ports. This function should provide any management
 * initilization required for the port for legacy control or multicast
 * filtering attributes, such as might be stored in a permanent database
 * either specifically for the port or as part of a template.
 *
 * Newly created ports are ‘connected’ for the purpose of GARP information
 * propagation using the separate function gip_connect_port(). Prior to
 * doing this, the system should initialize the newly created ports with
 * any permanent database controls for specific multicast values.
 *
 * It is assumed that new ports will be ‘connected’ correctly before the
 * application continues. The rules for connection are not encoded within
 * GMR. They depend on the relaying connectivity of the system as a whole,
 * and can be summarized as follows:
 *
 * if (gmr->vlan_id == Lan)
 * {
 * if stp_forwarding(port_no) gmr_connect_port(port_no);
 * }
 * else if vlan_forwarding(vlan_id, port_no)
 * {
 * gmr_connect_port(port_no);
 * }
 *
 * As the system continues to run it should invoke gmr_disconnect_port()
 * and gmr_connect_port() as required to maintain the required connectivity.
 */
extern void gmr_removed_port(void *gmr, int port_no);
/*
 * The system has removed and destroyed the GID port. This function should
 * provide any application-specific cleanup required.
 */
/******************************************************************************
 * GMR : GARP MULTICAST REGISTRATION APPLICATION : JOIN, LEAVE INDICATIONS
 ******************************************************************************
 */
extern void gmr_join_indication(void *gmr, Gid *my_port,
                                unsigned joining_gid_index);
/*
 * Deals with joins for both Legacy Attributes and Multicast Attributes.
 * The former are represented by the first few GID indexes, and give rise
 * to three cases:
 *
 * 1. Neither Forward All or Forward Unregistered are currently set (i.e.,
 * registered for this port), so the Filtering Database is in
 * “filter_by_default” mode, and the only multicasts that are being
 * forwarded through (out of) this port are those “registered here.”
 * In addition there may be other entries in the Filtering Database
 * whose effect on this port is currently duplicated by
 * “filter_by_default”: if an entry for a multicast is present for any
 * port, the model of the Filtering Database requires that its filter or
 * forward behavior be represented explicitly for all other ports
 * - there is no per port setting which means “behave as default” mode.
 *
 * 2. Forward Unregistered is currently set, but Forward All is not.
 * The Filtering Database is in “forward_by_default” mode. If a Filtering
 * Database entry has been made for a multicast for any port, it specifies
 * filtering for this port if the multicast is not registered here but
 * is registered for a port to which this port is connected (in the GIP
 * sense), and forwarding otherwise (i.e., multicast registered here or
 * not registered for any port to which this port is connected).
 *
 * 3. Forward All is currently set and takes precedence - Forward
 * Unregistered may or may not be set. The Filtering Database is in
 * “forward_by_default” mode. If a Filtering Database entry has been
 * made for any port, it specifies forwarding for this port. Not all
 * multicasts registered for this port have been entered into the
 * Filtering Database.
 *
 * If a call to “fdb_filter” or “fdb_forward,” made for a given port and
 * multicast address, causes a Filtering Database entry to be created, the
 * other ports are set to filter or forward for that address according to
 * the setting of “forward_by_default” or “filter_by_default” current when
 * the call is made. This behavior can be used to avoid temporary filtering
 * glitches.
 *
 * This function, gmr_join_indication(), changes filtering database entries
 * for the port that gives rise to the indication alone. If another port
 * is in Legacy mode B (Forward_unregistered set (registered) for that port,
 * but not Forward_all), then registration of a multicast address on this
 * port can cause it to be filtered on that other port. This is handled by
 * gmr_join_propagated() for the other ports that may be affected. It will
 * be called as a consequence of the GIP propagation of the newly registered
 * attribute (multicast address).
 */
extern void gmr_join_propagated(void *gmr, Gid *my_port,
                                unsigned joining_gid_index);
/*
 *
 */
extern void gmr_leave_indication(void *gmr, Gid *my_port,
                                 unsigned leaving_gid_index);
/*
 *
 */
extern void gmr_leave_propagated(void *gmr, Gid *my_port,
                                 unsigned leaving_gid_index);
/*
 *
 */
/******************************************************************************
 * GMR : GARP MULTICAST REGISTRATION APPLICATION : PROTOCOL & MGT EVENTS
 ******************************************************************************
 */
extern void gmr_rcv(void *gmr, Gid *my_port, Pdu *pdu);
/*
 * Process an entire received pdu for this instance of GMR.
 */
extern void gmr_tx(void *gmr, Gid *my_port);
/*
 * Transmit a pdu for this instance of GMR.
 */
#endif /* gmr_h__ */