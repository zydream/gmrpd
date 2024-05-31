/* gidtt.c */
#include "gidtt.h"
#include "gid.h"
/******************************************************************************
 * GIDTT : GID PROTOCOL TRANSITION TABLES : IMPLEMENTATION OVERVIEW
 ******************************************************************************
 */
/* This implementation of GID uses transition tables directly. This makes
 * the implementation appear very bulky, but the net code size impact may be
 * less than the page of code required for an algorithmic-based implementation,
 * depending on the processor. A processing-based implementation is planned as
 * both alternatives may be interesting.
 *
 * The Applicant and the Registrar use separate transition tables. Both use
 * a general transition table to handle most events, and separate smaller
 * tables to determine behavior when a transmit opportunity occurs (both
 * Applicant and Registrar), and when the leave timer expires (Registrar only).
 *
 * The stored states support management control directly - which leads to a
 * total of 14 applicant states and 18 registrar states (the registrar states
 * also incorporate leave timer support):
 *
 * The Applicant may be in one of the following management states:
 *
 * 1. Normal
 *
 * 2. No protocol
 *
 * The protocol machine is held quiet and never sends a message, even
 * to prompt another GID participant on the same LAN to respond. In
 * this state the Applicant does messages on the media so it can
 * be toggled between Normal and No protocol with minimum
 * disruption.
 *
 * The Registrar may be in one of the following management states:
 *
 * 1. Normal Registration.
 *
 * 2. Registration Fixed.
 *
 * The Registrar always reports “In” to the application and to GIP
 * whatever has occurred on the LAN.
 *
 * 3. Registration Forbidden.
 *
 * The Registrar always reports “Empty” to the application and to GIP.
 *
 * A set of small tables is used to report aspects of the management state of
 * both applicant and registrar.
 *
 * The main applicant transition table (applicant_tt) is indexed by current
 * applicant state and GID event, and returns
 *
 * 1. The new applicant state.
 *
 * 2. A start join timer instruction, when required.
 *
 *
 *
 * The main registrar transition table (registrar_tt) is indexed by current
 * registrar state and GID event, and returns
 *
 * 1. The new registrar state.
 *
 * 2. A join indication or a leave indication, when required.
 *
 * 3. A start leave timer instruction, when required.
 *
 * The only user interface to both these tables is through the public
 * function gidtt_event(), which accepts and returns Gid_events (to report
 * join or leave indications), and which writes timer start requests to the
 * GID scratchpad directly.
 *
 * The Applicant transmit transition table (applicant_txtt) returns the new
 * applicant state, the message to be transmitted, and whether the join timer
 * should be restarted to transmit a further message. A modifier that
 * determines whether a Join (selected for transmission by the Applicant table)
 * should be transmitted as a JoinIn or as a JoinEmpty is taken from a
 * Registrar state reporting table. The Registrar state is never modified by
 * transmission.
 */
/******************************************************************************
 * GIDTT : GID PROTOCOL TRANSITION TABLE : TABLE ENTRY DEFINITIONS
 ******************************************************************************
 */
enum Applicant_states
{
    Va,  /* Very anxious, active */
    Aa,  /* Anxious, active */
    Qa,  /* Quiet, active */
    La,  /* Leaving, active */
    Vp,  /* Very anxious, passive */
    Ap,  /* Anxious, passive */
    Qp,  /* Quiet, passive */
    Vo,  /* Very anxious observer */
    Ao,  /* Anxious observer */
    Qo,  /* Quiet observer */
    Lo,  /* Leaving observer */
    Von, /* Very anxious observer, non-participant */
    Aon, /* Anxious observer, non-participant */
    Qon  /* Quiet_observer, non-participant */
};
enum Registrar_states
{ /* In, Leave, Empty, but with Leave states implementing a countdown for the
   * leave timer.
   */
  Inn,
  Lv,
  L3,
  L2,
  L1,
  Mt,
  Inr, /* In, registration fixed */
  Lvr,
  L3r,
  L2r,
  L1r,
  Mtr,
  Inf, /* In, registration forbidden */
  Lvf,
  L3f,
  L2f,
  L1f,
  Mtf
};
enum
{
    Number_of_applicant_states = Qon + 1
}; /* for array sizing */
enum
{
    Number_of_registrar_states = Mtf + 1
}; /* for array sizing */
enum Timers
{
    Nt = 0, /* No timer action */
    Jt = 1, /* cstart_join_timer */
    Lt = 1  /* cstart_leave_timer */
};
enum Applicant_msg
{
    Nm = 0, /* No message to transmit */
    Jm,     /* Transmit a Join */
    Lm,     /* Transmit a Leave */
    Em      /* Transmit an Empty */
};
enum Registrar_indications
{
    Ni = 0,
    Li = 1,
    Ji = 2
};
/******************************************************************************
 * GIDTT : GID PROTOCOL TRANSITION TABLES : TRANSITION TABLE STRUCTURE
 ******************************************************************************
 */
typedef struct /* Applicant_tt_entry */
{
    unsigned new_app_state : 5; /* {Applicant_states} */
    unsigned cstart_join_timer : 1;
} Applicant_tt_entry;
typedef struct /* Registrar_tt_entry */
{
    unsigned new_reg_state : 5;
    unsigned indications : 2;
    unsigned cstart_leave_timer : 1;
} Registrar_tt_entry;
typedef struct /* Applicant_txtt_entry */
{
    unsigned new_app_state : 5;
    unsigned msg_to_transmit : 2; /* Applicant_msgs */
    unsigned cstart_join_timer : 1;
} Applicant_txtt_entry;
typedef struct /* Registrar_leave_timer_entry */
{
    unsigned new_reg_state : 5; /* Registrar_states */
    unsigned leave_indication : 1;
    unsigned cstart_leave_timer : 1;
} Registrar_leave_timer_entry;
/******************************************************************************
 * GIDTT : GID PROTOCOL: MAIN APPLICANT TRANSITION TABLE
 ******************************************************************************
 */
static Applicant_tt_entry
    applicant_tt[Number_of_gid_rcv_events + Number_of_gid_req_events +
                 Number_of_gid_amgt_events + Number_of_gid_rmgt_events]
                [Number_of_applicant_states] =
                    { /*
                       * General applicant transition table. See description above.
                       */
                     {/* Gid_null */
                      /*Va */ {Va, Nt}, /*Aa */ {Aa, Nt}, /*Qa */ {Qa, Nt}, /*La */ {La, Nt},
                      /*Vp */ {Vp, Nt}, /*Ap */ {Ap, Nt}, /*Vp */ {Vp, Nt},
                      /*Vo */ {Vo, Nt}, /*Ao */ {Ao, Nt}, /*Qo */ {Qo, Nt}, /*Lo */ {Lo, Nt},
                      /*Von*/ {Von, Nt}, /*Aon*/ {Aon, Nt}, /*Qon*/ {Qon, Nt}},
                     {/* Gid_rcv_leaveempty */
                      /*Va */ {Vp, Nt}, /*Aa */ {Vp, Nt}, /*Qa */ {Vp, Jt}, /*La */ {Vo, Nt},
                      /*Vp */ {Vp, Nt}, /*Ap */ {Vp, Nt}, /*Qp */ {Vp, Jt},
                      /*Vo */ {Lo, Nt}, /*Ao */ {Lo, Nt}, /*Qo */ {Lo, Jt}, /*Lo */ {Vo, Nt},
                      /*Von*/ {Von, Nt}, /*Aon*/ {Von, Nt}, /*Qon*/ {Von, Nt}},
                     {/* Gid_rcv_leavein */
                      /*Va */ {Va, Nt}, /*Aa */ {Va, Nt}, /*Qa */ {Vp, Jt}, /*La */ {La, Nt},
                      /*Vp */ {Vp, Nt}, /*Ap */ {Vp, Nt}, /*Qp */ {Vp, Jt},
                      /*Vo */ {Lo, Nt}, /*Ao */ {Lo, Nt}, /*Qo */ {Lo, Jt}, /*Lo */ {Vo, Nt},
                      /*Von*/ {Von, Nt}, /*Aon*/ {Von, Nt}, /*Qon*/ {Von, Nt}},
                     {/* Gid_rcv_empty */
                      /*Va */ {Va, Nt}, /*Aa */ {Va, Nt}, /*Qa */ {Va, Jt}, /*La */ {La, Nt},
                      /*Vp */ {Vp, Nt}, /*Ap */ {Vp, Nt}, /*Qp */ {Vp, Jt},
                      /*Vo */ {Vo, Nt}, /*Ao */ {Vo, Nt}, /*Qo */ {Vo, Nt}, /*Lo */ {Vo, Nt},
                      /*Von*/ {Von, Nt}, /*Aon*/ {Von, Nt}, /*Qon*/ {Von, Nt}},
                     {/* Gid_rcv_joinempty */
                      /*Va */ {Va, Nt}, /*Aa */ {Va, Nt}, /*Qa */ {Va, Jt}, /*La */ {Vo, Nt},
                      /*Vp */ {Vp, Nt}, /*Ap */ {Vp, Nt}, /*Qp */ {Vp, Jt},
                      /*Vo */ {Vo, Nt}, /*Ao */ {Vo, Nt}, /*Qo */ {Vo, Jt}, /*Lo */ {Vo, Nt},
                      /*Von*/ {Von, Nt}, /*Aon*/ {Von, Nt}, /*Qon*/ {Von, Jt}},
                     {/* Gid_rcv_joinin */
                      /*Va */ {Aa, Nt}, /*Aa */ {Qa, Nt}, /*Qa */ {Qa, Nt}, /*La */ {La, Nt},
                      /*Vp */ {Ap, Nt}, /*Ap */ {Qp, Nt}, /*Qp */ {Qp, Nt},
                      /*Vo */ {Ao, Nt}, /*Ao */ {Qo, Nt}, /*Qo */ {Qo, Nt}, /*Lo */ {Ao, Nt},
                      /*Von*/ {Aon, Nt}, /*Aon*/ {Qon, Nt}, /*Qon*/ {Qon, Nt}},
                     {/* Gid_join, join request. Handles repeated joins, i.e., joins for
                       * states that are already in. Does not provide feedback for joins
                       * that are forbidden by management controls; the expectation is
                       * that this table will not be directly used by new management
                       * requests.
                       */
                      /*Va */ {Va, Nt}, /*Aa */ {Aa, Nt}, /*Qa */ {Qa, Nt}, /*La */ {Va, Nt},
                      /*Vp */ {Vp, Nt}, /*Ap */ {Ap, Nt}, /*Qp */ {Qp, Nt},
                      /*Vo */ {Vp, Jt}, /*Ao */ {Ap, Jt}, /*Qo */ {Qp, Nt}, /*Lo */ {Vp, Nt},
                      /*Von*/ {Von, Nt}, /*Aon*/ {Aon, Nt}, /*Qon*/ {Qon, Nt}},
                     {/* Gid_leave, leave request. See comments for join requests above. */
                      /*Va */ {La, Nt}, /*Aa */ {La, Nt}, /*Qa */ {La, Jt}, /*La */ {La, Nt},
                      /*Vp */ {Vo, Nt}, /*Ap */ {Ao, Nt}, /*Qp */ {Qo, Nt},
                      /*Vo */ {Vo, Nt}, /*Ao */ {Ao, Nt}, /*Qo */ {Qo, Nt}, /*Lo */ {Lo, Nt},
                      /*Von*/ {Von, Nt}, /*Aon*/ {Aon, Nt}, /*Qon*/ {Qon, Nt}},
                     {/* Gid_normal_operation */
                      /*Va */ {Vp, Nt}, /*Aa */ {Vp, Nt}, /*Qa */ {Vp, Jt}, /*La */ {La, Nt},
                      /*Vp */ {Vp, Nt}, /*Ap */ {Vp, Nt}, /*Qp */ {Vp, Jt},
                      /*Vo */ {Va, Nt}, /*Ao */ {Va, Nt}, /*Qo */ {Va, Jt}, /*Lo */ {Lo, Nt},
                      /*Von*/ {Va, Nt}, /*Aon*/ {Va, Nt}, /*Qon*/ {Va, Jt}},
                     {/* Gid_no_protocol */
                      /*Va */ {Von, Nt}, /*Aa */ {Aon, Nt}, /*Qa */ {Qon, Nt}, /*La */ {Von, Nt},
                      /*Vp */ {Von, Nt}, /*Ap */ {Aon, Nt}, /*Qp */ {Qon, Nt},
                      /*Vo */ {Von, Nt}, /*Ao */ {Aon, Nt}, /*Qo */ {Qon, Nt}, /*Lo */ {Von, Nt},
                      /*Von*/ {Von, Nt}, /*Aon*/ {Aon, Nt}, /*Qon*/ {Qon, Nt}},
                     {/* Gid_normal_registration, same as Gid_null for the Applicant */
                      /*Va */ {Va, Nt}, /*Aa */ {Aa, Nt}, /*Qa */ {Qa, Nt}, /*La */ {La, Nt},
                      /*Vp */ {Vp, Nt}, /*Ap */ {Ap, Nt}, /*Vp */ {Vp, Nt},
                      /*Vo */ {Vo, Nt}, /*Ao */ {Ao, Nt}, /*Qo */ {Qo, Nt}, /*Lo */ {Lo, Nt},
                      /*Von*/ {Von, Nt}, /*Aon*/ {Aon, Nt}, /*Qon*/ {Qon, Nt}},
                     {/* Gid_fix_registration, same as Gid_null for the Applicant */
                      /*Va */ {Va, Nt}, /*Aa */ {Aa, Nt}, /*Qa */ {Qa, Nt}, /*La */ {La, Nt},
                      /*Vp */ {Vp, Nt}, /*Ap */ {Ap, Nt}, /*Vp */ {Vp, Nt},
                      /*Vo */ {Vo, Nt}, /*Ao */ {Ao, Nt}, /*Qo */ {Qo, Nt}, /*Lo */ {Lo, Nt},
                      /*Von*/ {Von, Nt}, /*Aon*/ {Aon, Nt}, /*Qon*/ {Qon, Nt}},
                     {/* Gid_forbid_registration, same as Gid_null for the Applicant */
                      /*Va */ {Va, Nt}, /*Aa */ {Aa, Nt}, /*Qa */ {Qa, Nt}, /*La */ {La, Nt},
                      /*Vp */ {Vp, Nt}, /*Ap */ {Ap, Nt}, /*Vp */ {Vp, Nt},
                      /*Vo */ {Vo, Nt}, /*Ao */ {Ao, Nt}, /*Qo */ {Qo, Nt}, /*Lo */ {Lo, Nt},
                      /*Von*/ {Von, Nt}, /*Aon*/ {Aon, Nt}, /*Qon*/ {Qon, Nt}}};
/******************************************************************************
 * GIDTT : GID PROTOCOL: MAIN REGISTRAR TRANSITION TABLE
 ******************************************************************************
 */
static Registrar_tt_entry
    registrar_tt[Number_of_gid_rcv_events + Number_of_gid_req_events +
                 Number_of_gid_amgt_events + Number_of_gid_rmgt_events]
                [Number_of_registrar_states] =
                    {
                        {/* Gid_null */
                         /*In */ {Inn, Ni, Nt},
                         /*Lv */ {Lv, Ni, Nt},
                         /*L3 */ {L3, Ni, Nt}, /*L2 */ {L2, Ni, Nt}, /*L1 */ {L1, Ni, Nt},
                         /*Mt */ {Mt, Ni, Nt},
                         /*Inr*/ {Inr, Ni, Nt},
                         /*Lvr*/ {Lvr, Ni, Nt},
                         /*L3r*/ {L3r, Ni, Nt}, /*L2r*/ {L2r, Ni, Nt}, /*L1r*/ {L1r, Ni, Nt},
                         /*Mtr*/ {Mtr, Ni, Nt},
                         /*Inf*/ {Inf, Ni, Nt},
                         /*Lvf*/ {Lvf, Ni, Nt},
                         /*L3f*/ {L3f, Ni, Nt}, /*L2f*/ {L2f, Ni, Nt}, /*L1f*/ {L1f, Ni, Nt},
                         /*Mtf*/ {Mtf, Ni, Nt}},
                        {/* Gid_rcv_leave */
                         /*Inn*/ {Lv, Ni, Lt},
                         /*Lv */ {Lv, Ni, Nt},
                         /*L3 */ {L3, Ni, Nt}, /*L2 */ {L2, Ni, Nt}, /*L1 */ {L1, Ni, Nt},
                         /*Mt */ {Mt, Ni, Nt},
                         /*Inr*/ {Lvr, Ni, Lt},
                         /*Lvr*/ {Lvr, Ni, Nt},
                         /*L3r*/ {L3r, Ni, Nt}, /*L2r*/ {L2r, Ni, Nt}, /*L1r*/ {L1r, Ni, Nt},
                         /*Mtr*/ {Mtr, Ni, Nt},
                         /*Inf*/ {Lvf, Ni, Lt},
                         /*Lvf*/ {Lvf, Ni, Nt},
                         /*L3f*/ {L3f, Ni, Nt}, /*L2f*/ {L2f, Ni, Nt}, /*L1f*/ {L1f, Ni, Nt},
                         /*Mtf*/ {Mtf, Ni, Nt}},
                        {/* Gid_rcv_empty */
                         /*Inn*/ {Inn, Ni, Nt},
                         /*Lv */ {Lv, Ni, Nt},
                         /*L3 */ {L3, Ni, Nt}, /*L2 */ {L2, Ni, Nt}, /*L1 */ {L1, Ni, Nt},
                         /*Mt */ {Mt, Ni, Nt},
                         /*Inr*/ {Inr, Ni, Nt},
                         /*Lvr*/ {Lvr, Ni, Nt},
                         /*L3r*/ {L3r, Ni, Nt}, /*L2r*/ {L2r, Ni, Nt}, /*L1r*/ {L1r, Ni, Nt},
                         /*Mtr*/ {Mtr, Ni, Nt},
                         /*Inf*/ {Inf, Ni, Nt},
                         /*Lvf*/ {Lvf, Ni, Nt},
                         /*L3f*/ {L3f, Ni, Nt}, /*L2f*/ {L2f, Ni, Nt}, /*L1f*/ {L1f, Ni, Nt},
                         /*Mtf*/ {Mtf, Ni, Nt}},
                        {/* Gid_rcv_joinempty */
                         /*Inn*/ {Inn, Ni, Nt},
                         /*Lv */ {Inn, Ni, Nt},
                         /*L3 */ {Inn, Ni, Nt}, /*L2 */ {Inn, Ni, Nt}, /*L1 */ {Inn, Ni, Nt},
                         /*Mt */ {Inn, Ji, Nt},
                         /*Inr*/ {Inr, Ni, Nt},
                         /*Lvr*/ {Inr, Ni, Nt},
                         /*L3r*/ {Inr, Ni, Nt}, /*L2r*/ {Inr, Ni, Nt}, /*L1r*/ {Inr, Ni, Nt},
                         /*Mtr*/ {Inr, Ni, Nt},
                         /*Inf*/ {Inf, Ni, Nt},
                         /*Lvf*/ {Inf, Ni, Nt},
                         /*L3f*/ {Inf, Ni, Nt}, /*L2f*/ {Inf, Ni, Nt}, /*L1f*/ {Inf, Ni, Nt},
                         /*Mtf*/ {Inf, Ni, Nt}},
                        {/* Gid_rcv_joinin */
                         /*Inn*/ {Inn, Ni, Nt},
                         /*Lv */ {Inn, Ni, Nt},
                         /*L3 */ {Inn, Ni, Nt}, /*L2 */ {Inn, Ni, Nt}, /*L1 */ {Inn, Ni, Nt},
                         /*Mt */ {Inn, Ji, Nt},
                         /*Inr*/ {Inr, Ni, Nt},
                         /*Lvr*/ {Inr, Ni, Nt},
                         /*L3r*/ {Inr, Ni, Nt}, /*L2r*/ {Inr, Ni, Nt}, /*L1r*/ {Inr, Ni, Nt},
                         /*Mtr*/ {Inr, Ni, Nt},
                         /*Inf*/ {Inf, Ni, Nt},
                         /*Lvf*/ {Inf, Ni, Nt},
                         /*L3f*/ {Inf, Ni, Nt}, /*L2f*/ {Inf, Ni, Nt}, /*L1f*/ {Inf, Ni, Nt},
                         /*Mtf*/ {Inf, Ni, Nt}},
                        {/* Gid_normal_operation, same as Gid_null for the Registrar */
                         /*In */ {Inn, Ni, Nt},
                         /*Lv */ {Lv, Ni, Nt},
                         /*L3 */ {L3, Ni, Nt}, /*L2 */ {L2, Ni, Nt}, /*L1 */ {L1, Ni, Nt},
                         /*Mt */ {Mt, Ni, Nt},
                         /*Inr*/ {Inr, Ni, Nt},
                         /*Lvr*/ {Lvr, Ni, Nt},
                         /*L3r*/ {L3r, Ni, Nt}, /*L2r*/ {L2r, Ni, Nt}, /*L1r*/ {L1r, Ni, Nt},
                         /*Mtr*/ {Mtr, Ni, Nt},
                         /*Inf*/ {Inf, Ni, Nt},
                         /*Lvf*/ {Lvf, Ni, Nt},
                         /*L3f*/ {L3f, Ni, Nt}, /*L2f*/ {L2f, Ni, Nt}, /*L1f*/ {L1f, Ni, Nt},
                         /*Mtf*/ {Mtf, Ni, Nt}},
                        {/* Gid_no_protocol, same as Gid_null for the Registrar */
                         /*In */ {Inn, Ni, Nt},
                         /*Lv */ {Lv, Ni, Nt},
                         /*L3 */ {L3, Ni, Nt}, /*L2 */ {L2, Ni, Nt}, /*L1 */ {L1, Ni, Nt},
                         /*Mt */ {Mt, Ni, Nt},
                         /*Inr*/ {Inr, Ni, Nt},
                         /*Lvr*/ {Lvr, Ni, Nt},
                         /*L3r*/ {L3r, Ni, Nt}, /*L2r*/ {L2r, Ni, Nt}, /*L1r*/ {L1r, Ni, Nt},
                         /*Mtr*/ {Mtr, Ni, Nt},
                         /*Inf*/ {Inf, Ni, Nt},
                         /*Lvf*/ {Lvf, Ni, Nt},
                         /*L3f*/ {L3f, Ni, Nt}, /*L2f*/ {L2f, Ni, Nt}, /*L1f*/ {L1f, Ni, Nt},
                         /*Mtf*/ {Mtf, Ni, Nt}},
                        {/* Gid_normal_registration */
                         /*Inn*/ {Inn, Ni, Nt},
                         /*Lv */ {Lv, Ni, Nt},
                         /*L3 */ {L3, Ni, Nt}, /*L2 */ {L2, Ni, Nt}, /*L1 */ {L1, Ni, Nt},
                         /*Mt */ {Mt, Ni, Nt},
                         /*Inr*/ {Inn, Ni, Nt},
                         /*Lvr*/ {Lv, Ni, Nt},
                         /*L3r*/ {L3, Ni, Nt}, /*L2r*/ {L2, Ni, Nt}, /*L1r*/ {L1, Ni, Nt},
                         /*Mtr*/ {Mt, Li, Nt},
                         /*Inf*/ {Inn, Ji, Nt},
                         /*Lvf*/ {Lv, Ji, Nt},
                         /*L3f*/ {L3, Ji, Nt}, /*L2f*/ {L2, Ji, Nt}, /*L1f*/ {L1, Ji, Nt},
                         /*Mtf*/ {Mt, Ni, Nt}},
                        {/* Gid_fix_registration */
                         /*Inn*/ {Inr, Ni, Nt},
                         /*Lv */ {Lvr, Ni, Nt},
                         /*L3 */ {L3r, Ni, Nt}, /*L2 */ {L2r, Ni, Nt}, /*L1 */ {L1r, Ni, Nt},
                         /*Mt */ {Mtr, Ji, Nt},
                         /*Inr*/ {Inr, Ni, Nt},
                         /*Lvr*/ {Lvr, Ni, Nt},
                         /*L3r*/ {L3r, Ni, Nt}, /*L2r*/ {L2r, Ni, Nt}, /*L1r*/ {L1r, Ni, Nt},
                         /*Mtr*/ {Mtr, Ni, Nt},
                         /*Inf*/ {Inr, Ji, Nt},
                         /*Lvf*/ {Lvr, Ji, Nt},
                         /*L3f*/ {L3r, Ji, Nt}, /*L2f*/ {L2r, Ji, Nt}, /*L1f*/ {L1r, Ji, Nt},
                         /*Mtf*/ {Mtr, Ji, Nt}},
                        {/* Gid_forbid_registration */
                         /*Inn*/ {Inf, Li, Nt},
                         /*Lv */ {Lvf, Li, Nt},
                         /*L3 */ {L3f, Li, Nt}, /*L2 */ {L2f, Li, Nt}, /*L1 */ {L1f, Li, Nt},
                         /*Mt */ {Mtf, Ni, Nt},
                         /*Inr*/ {Inr, Li, Nt},
                         /*Lvr*/ {Lvr, Li, Nt},
                         /*L3r*/ {L3r, Li, Nt}, /*L2r*/ {L2r, Li, Nt}, /*L1r*/ {L1r, Li, Nt},
                         /*Mtr*/ {Mtr, Li, Nt},
                         /*Inf*/ {Inf, Ni, Nt},
                         /*Lvf*/ {Lvf, Ni, Nt},
                         /*L3f*/ {L3f, Ni, Nt}, /*L2f*/ {L2f, Ni, Nt}, /*L1f*/ {L1f, Ni, Nt},
                         /*Mtf*/ {Mtf, Ni, Nt}}};
/******************************************************************************
 * GIDTT : GID PROTOCOL : APPLICANT TRANSMIT TABLE
 ******************************************************************************
 */
static Applicant_txtt_entry
    applicant_txtt[Number_of_applicant_states] =
        {
            /*Va */ {Aa, Jm, Jt}, /*Aa */ {Qa, Jm, Nt}, /*Qa */ {Qa, Nm, Nt}, /*La */ {Vo, Lm, Nt},
            /*Vp */ {Aa, Jm, Jt}, /*Ap */ {Qa, Jm, Nt}, /*Qp */ {Qp, Nm, Nt},
            /*Vo */ {Vo, Nm, Nt}, /*Ao */ {Ao, Nm, Nt}, /*Qo */ {Qo, Nm, Nt}, /*Lo */ {Vo, Nm, Nt},
            /*Von*/ {Von, Nm, Nt}, /*Aon*/ {Aon, Nm, Nt}, /*Qon*/ {Qon, Nm, Nt}};
/******************************************************************************
 * GIDTT : GID PROTOCOL : REGISTRAR LEAVE TIMER TABLE
 ******************************************************************************
 */
static Registrar_leave_timer_entry
    registrar_leave_timer_table[Number_of_registrar_states] =
        {
            /*Inn*/ {Inn, Ni, Nt},
            /*Lv */ {L3, Ni, Lt}, /*L3 */ {L2, Ni, Lt}, /*L2 */ {L1, Ni, Lt}, /*L1 */ {Mt, Li, Nt},
            /*Mt */ {Mt, Ni, Nt},
            /*Inr*/ {Inr, Ni, Nt},
            /*Lvr*/ {L3r, Ni, Lt}, /*L3r*/ {L2r, Ni, Lt}, /*L2r*/ {L1r, Ni, Lt}, /*L1r*/ {Mtr, Ni, Nt},
            /*Mtr*/ {Mtr, Ni, Nt},
            /*Inf*/ {Inf, Ni, Nt},
            /*Lvf*/ {L3f, Ni, Lt}, /*L3f*/ {L2f, Ni, Lt}, /*L2f*/ {L1f, Ni, Lt}, /*L1f*/ {Mtf, Ni, Nt},
            /*Mtf*/ {Mtf, Ni, Nt}};
/******************************************************************************
 * GIDTT : GID PROTOCOL : STATE REPORTING TABLES
 ******************************************************************************
 */
static Gid_applicant_state applicant_state_table[Number_of_applicant_states] =
    {
        /*Va */ {Very_anxious}, /*Aa */ {Anxious}, /*Qa */ {Quiet}, /*La */ {Leaving},
        /*Vp */ {Very_anxious}, /*Ap */ {Anxious}, /*Qp */ {Quiet},
        /*Vo */ {Very_anxious}, /*Ao */ {Anxious}, /*Qo */ {Quiet}, /*Lo */ {Leaving},
        /*Von*/ {Very_anxious}, /*Aon*/ {Anxious}, /*Qon*/ {Quiet}};
static Gid_applicant_mgt applicant_mgt_table[Number_of_applicant_states] =
    {
        /*Va */ {Normal}, /*Aa */ {Normal}, /*Qa */ {Normal},
        /*La */ {Normal},
        /*Vp */ {Normal}, /*Ap */ {Normal}, /*Qp */ {Normal},
        /*Vo */ {Normal}, /*Ao */ {Normal}, /*Qo */ {Normal},
        /*Lo */ {Normal},
        /*Von*/ {No_protocol}, /*Aon*/ {No_protocol}, /*Qon*/ {No_protocol}};
static Gid_registrar_state registrar_state_table[Number_of_registrar_states] =
    {
        /*Inn*/ {In},
        /*Lv */ {Leave}, /*L3 */ {Leave}, /*L2 */ {Leave}, /*L1 */ {Leave}, /*Mt */ {Empty},
        /*Inr*/ {In},
        /*Lvr*/ {Leave}, /*L3r*/ {Leave}, /*L2r*/ {Leave}, /*L1r*/ {Leave}, /*Mtr*/ {Empty},
        /*Inf*/ {In},
        /*Lvf*/ {Leave}, /*L3f*/ {Leave}, /*L2f*/ {Leave}, /*L1f*/ {Leave}, /*Mtf*/ {Empty}};
static Gid_registrar_mgt registrar_mgt_table[Number_of_registrar_states] =
    {
        /*Inn*/ {Normal_registration},
        /*Lv */ {Normal_registration}, /*L3 */ {Normal_registration},
        /*L2 */ {Normal_registration}, /*L1 */ {Normal_registration},
        /*Mt */ {Normal_registration},
        /*Inr*/ {Registration_fixed},
        /*Lvr*/ {Registration_fixed}, /*L3r*/ {Registration_fixed},
        /*L2r*/ {Registration_fixed}, /*L1r*/ {Registration_fixed},
        /*Mtr*/ {Registration_fixed},
        /*Inf*/ {Registration_forbidden},
        /*Lvf*/ {Registration_forbidden}, /*L3f*/ {Registration_forbidden},
        /*L2f*/ {Registration_forbidden}, /*L1f*/ {Registration_forbidden},
        /*Mtf*/ {Registration_forbidden}};
static Boolean registrar_in_table[Number_of_registrar_states] =
    {
        /*Inn*/ {True}, /*Lv */ {True}, /*L3 */ {True}, /*L2 */ {True}, /*L1 */ {True},
        /*Mt */ {False},
        /*Inr*/ {True}, /*Lvr*/ {True}, /*L3r*/ {True}, /*L2r*/ {True}, /*L1r*/ {True},
        /*Mtr*/ {True},
        /*Inf*/ {False}, /*Lvf*/ {False}, /*L3f*/ {False}, /*L2f*/ {False}, /*L1f*/ {False},
        /*Mtf*/ {False}};
/******************************************************************************
 * GIDTT : GID PROTOCOL : RECEIVE EVENTS, USER REQUESTS, & MGT PROCESSING
 ******************************************************************************
 */
Gid_event gidtt_event(Gid *my_port, Gid_machine *machine, Gid_event event)
{ /*
   * Handles receive events and join or leave requests.
   */
    Applicant_tt_entry *atransition;
    Registrar_tt_entry *rtransition;
    atransition = &applicant_tt[event][machine->applicant];
    rtransition = &registrar_tt[event][machine->registrar];
    machine->applicant = atransition->new_app_state;
    machine->registrar = rtransition->new_reg_state;
    if ((event == Gid_join) && (atransition->cstart_join_timer))
        my_port->cschedule_tx_now = True;
    my_port->cstart_join_timer = my_port->cstart_join_timer || atransition->cstart_join_timer;
    my_port->cstart_leave_timer = my_port->cstart_leave_timer || rtransition->cstart_leave_timer;
    switch (rtransition->indications)
    {
    case Ji:
        return (Gid_join);
        break;
    case Li:
        return (Gid_leave);
        break;
    case Ni:
    default:
        return (Gid_null);
    }
}
Boolean gidtt_in(Gid_machine *machine)
{ /*
   *
   */
    return (registrar_in_table[machine->registrar]);
}
/******************************************************************************
 * GIDTT : GID PROTOCOL TRANSITION TABLES : TRANSMIT MESSAGES
 ******************************************************************************
 */
Gid_event gidtt_tx(Gid *my_port,
                   Gid_machine *machine)
{ /*
   *
   */
    unsigned msg;
    unsigned rin;
    if ((msg = applicant_txtt[machine->applicant].msg_to_transmit) != Nm)
        rin = registrar_state_table[machine->registrar];
    my_port->cstart_join_timer = my_port->cstart_join_timer || applicant_txtt[machine->applicant].cstart_join_timer;
    switch (msg)
    {
    case Jm:
        return (rin != Empty ? Gid_tx_joinin : Gid_tx_joinempty);
        break;
    case Lm:
        return (rin != Empty ? Gid_tx_leavein : Gid_tx_leaveempty);
        break;
    case Em:
        return (Gid_tx_empty);
        break;
    case Nm:
    default:
        return (Gid_null);
    }
}
/******************************************************************************
 * GIDTT : GID PROTOCOL TRANSITION TABLES : LEAVE TIMER PROCESSING
 ******************************************************************************
 */
Gid_event gidtt_leave_timer_expiry(Gid *my_port,
                                   Gid_machine *machine)
{ /*
   *
   */
    Registrar_leave_timer_entry *rtransition;
    rtransition = &registrar_leave_timer_table[machine->registrar];
    machine->registrar = rtransition->new_reg_state;
    my_port->cstart_leave_timer = my_port->cstart_leave_timer || rtransition->cstart_leave_timer;
    return ((rtransition->leave_indication == Li) ? Gid_leave : Gid_null);
}
/******************************************************************************
 * GIDTT : GID PROTOCOL TRANSITION TABLES : STATE REPORTING
 ******************************************************************************
 */
Boolean gidtt_machine_active(Gid_machine *machine)
{
    if ((machine->applicant == Vo) && (machine->registrar == Mt))
        return (False);
    else
        return (True);
}
void gidtt_states(Gid_machine *machine, Gid_states *state)
{ /*
   *
   */
    state->applicant_state = applicant_state_table[machine->applicant];
    state->applicant_mgt = applicant_mgt_table[machine->applicant];
    state->registrar_state = registrar_state_table[machine->registrar];
    state->registrar_mgt = registrar_mgt_table[machine->registrar];
}