/* gidtt.h */
#ifndef gidtt_h__
#define gidtt_h__
#include "gid.h"
/******************************************************************************
 * GIDTT : GARP INFORMATION DISTRIBUTION PROTOCOL : TRANSITION TABLES
 ******************************************************************************
 */
extern Gid_event gidtt_event(Gid *my_port,
                             Gid_machine *machine,
                             Gid_event event);
extern Gid_event gidtt_tx(Gid *my_port,
                          Gid_machine *machine);
extern Gid_event gidtt_leave_timer_expiry(Gid *my_port,
                                          Gid_machine *machine);
extern Boolean gidtt_in(Gid_machine *machine);
/*
 * Returns True if the Registrar is in, or if registration is fixed.
 */
extern Boolean gidtt_machine_active(Gid_machine *machine);
/*
 * Returns False iff the Registrar is Normal registration, Empty, and the
 * Application is Normal membership, Very Anxious Observer.
 */
extern void gidtt_states(Gid_machine *machine,
                         Gid_states *state);
/*
 * Reports the the GID machine state : Gid_applicant_state,
 * Gid_applicant_mgt, Gid_registrar_state, Gid_registrar_mgt.
 */
#endif /* gidtt_h__ */