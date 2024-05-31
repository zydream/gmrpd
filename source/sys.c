/*sys.c*/
#include "sys.h"
/******************************************************************************
 * SYS : SYSTEM SUPPLIED MEMORY ALLOCATION ROUTINES
 ******************************************************************************
 */
Boolean sysmalloc(int size, void **allocated)
{
    return False;
}
void sysfree(void *allocated)
{
    return;
}
/******************************************************************************
 * SYSPDU : SYSTEM SUPPLIED PDU ACCESS PRIMITIVES
 ******************************************************************************
 */
typedef void Pdu;
Boolean syspdu_alloc(Pdu **pdu)
{
    return False;
}
void syspdu_free(Pdu *pdu)
{
    return;
}
Boolean rdcheck(Pdu *pdu, int number_of_octets_remaining)
{
    return False;
}
Boolean rdoctet(Pdu *pdu, Octet *val)
{
    return False;
}
Boolean rdint16(Pdu *pdu, Int16 val)
{
    return False;
}
Boolean rdskip(Pdu *pdu, int number_to_skip)
{
    return False;
}
void syspdu_tx(Pdu *pdu, int port_no)
{
    return;
}
/******************************************************************************
 * SYSTIME : SYSTEM SUPPLIED SCHEDULING FUNCTIONS
 ******************************************************************************
 */
void systime_start_random_timer(int process_id,
                                void (*expiry_fn)(void *, int instance_id),
                                int instance_id,
                                int timeout)
{
    return;
}
void systime_start_timer(int process_id,
                         void (*expiry_fn)(void *, int instance_id),
                         int instance_id,
                         int timeout)
{
    return;
}
void systime_schedule(int process_id,
                      void (*expiry_fn)(void *, int instance_id),
                      int instance_id)
{
    return;
}
/******************************************************************************
 * SYSERR : FATAL ERROR HANDLING
 ******************************************************************************
 */
void syserr_panic()
{
    exit(1);
}