/* gip.h */
#ifndef gip_h__
#define gip_h__
#include "gid.h"
/******************************************************************************
 * GIP : GARP INFORMATION PROPAGATION : CREATION, DESTRUCTION
 ******************************************************************************
 */
extern Boolean gip_create_gip(unsigned max_attributes, unsigned **gip);
/*
 * Creates a new instance of GIP, allocating space for propagation counts
 * for up to max_attributes.
 *
 * Returns True if the creation succeeded together with a pointer to the
 * GIP information. This pointer is passed to gid_create_port() for ports
 * using this instance of GIP and is saved along with GID information.
 */
extern void gip_destroy_gip(void *gip);
/*
 * Destroys the instance of GIP, releasing previously allocated space.
 */
/******************************************************************************
 * GIP : GARP INFORMATION PROPAGATION : PROPAGATION FUNCTIONS
 ******************************************************************************
 */
extern void gip_connect_port(Garp *application, int port_no);
/*
 * Finds the port, checks that it is not already connected, and connects
 * it into the GIP propagation ring that uses GIP field(s) in GID control
 * blocks to link the source port to the ports to which the information is
 * to be propagated.
 *
 * Propagates joins from and to the other already connected ports as
 * necessary.
 */
extern void gip_disconnect_port(Garp *application, int port_no);
/*
 * Checks to ensure that the port is connected, and then disconnects it from
 * the GIP propagation ring. Propagates leaves to the other ports that
 * remain in the ring and causes leaves to my_port as necessary.
 */
extern void gip_propagate_join(Gid *my_port, unsigned index);
/*
 * Propagates a join indication for a single attribute (identified
 * by a combination of its attribute class and index) from my_port to other
 * ports, causing join requests to those other ports if required.
 *
 * GIP maintains a joined membership count for the connected ports for each
 * attribute (in a given context) so that leaves are not caused when joins
 * from other ports would maintain membership.
 *
 * Because this count is maintained by ‘dead-reckoning’ it is important
 * that this function only be called when there is a change indication for
 * the source port and index.
 */
extern void gip_propagate_leave(Gid *my_port, unsigned index);
/*
 * Propagates a leave indication for a single attribute, causing leave
 * requests to those other ports if required.
 *
 * See the comments for gip_propagate_join() before reading further.
 * This function decrements the ‘dead-reckoning’ membership count.
 */
extern Boolean gip_propagates_to(Gid *my_port, unsigned index);
/*
 * True if any other port is propagating the attribute associated with index
 * to my_port.
 */
extern void gip_do_actions(Gid *my_port);
/*
 * Calls GID to carry out GID ‘scratchpad’ actions accumulated during this
 * invocation of GARP for all the ports in the GIP propagation list,
 * including the source port.
 */
#endif /* gip_h__ */