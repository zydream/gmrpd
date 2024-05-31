/* garp.h */
#ifndef garp_h__
#define garp_h__
#include "sys.h"
/******************************************************************************
 * GARP : GENERIC ATTRIBUTE REGISTRATION PROTOCOL : COMMON APPLICATION ELEMENTS
 ******************************************************************************
 */
typedef struct /* Garp */
{              /*
                * Each GARP, i.e., each instance of an application that uses the GARP
                * protocol, is represented as a struct or control block with common initial
                * fields. These comprise pointers to application-specific functions that
                * are by the GID and GIP components to signal protocol events to the
                * application, and other controls common to all applications. The pointers
                * include a pointer to the instances of GID (one per port) for the application,
                * and to GIP (one per application). The signaling functions include the
                * addition and removal of ports, which the application should use to
                * initialize port attributes with any management state required.
                */
    int process_id;
    void *gid;
    unsigned *gip;
    unsigned max_gid_index;
    unsigned last_gid_used;
    void (*join_indication_fn)(void *, void *my_port, unsigned joining_gid_index);
    void (*leave_indication_fn)(void *, void *gid,
                                unsigned leaving_gid_index);
    void (*join_propagated_fn)(void *, void *gid,
                               unsigned joining_gid_index);
    void (*leave_propagated_fn)(void *, void *gid,
                                unsigned leaving_gid_index);
    void (*transmit_fn)(void *, void *gid);
    void (*receive_fn)(void *, void *gid, Pdu *pdu);
    void (*added_port_fn)(void *, int port_no);
    void (*removed_port_fn)(void *, int port_no);
} Garp;
#endif /* garp_h__ */