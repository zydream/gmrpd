/* sys.h */
#ifndef sys_h__
#define sys_h__
#include <stddef.h>
/******************************************************************************
 * SYS : SYSTEM SUPPLIED ROUTINES AND PRIMITIVES
 ******************************************************************************
 *
 * This header file represents system supplied routines and primitives grouped
 * into five categories:
 *
 * SYS : System conventions.
 *
 * SYSMEM : General memory allocation.
 *
 * SYSPDU : Protocol buffer allocation, access, transmit, and receive.
 *
 * SYSTIME : Scheduling routines : immediate, in fixed (approximate) time, in
 * random time period.
 *
 * SYSERR : System error routines for gross errors in program logic detected
 * in the course of execution, for which there is no sensible course
 * of action at the point of detection.
 */
/******************************************************************************
 * SYS : SYSTEM CONVENTIONS
 ******************************************************************************
 */
typedef enum
{
    False = 0,
    True = 1
} Boolean;
enum
{
    Zero = 0,
    One = 1,
    Two = 2
};
typedef unsigned char Octet;
typedef unsigned short Int16;
typedef unsigned char *Mac_address;
/******************************************************************************
 * SYS : SYSTEM SUPPLIED MEMORY ALLOCATION ROUTINES
 ******************************************************************************
 */
extern Boolean sysmalloc(int size, void **allocated);
extern void sysfree(void *allocated);
/******************************************************************************
 * SYSPDU : SYSTEM SUPPLIED PDU ACCESS PRIMITIVES
 ******************************************************************************
 */
typedef void Pdu;
extern Boolean syspdu_alloc(Pdu **pdu);
extern void syspdu_free(Pdu *pdu);
extern Boolean rdcheck(Pdu *pdu, int number_of_octets_remaining);
extern Boolean rdoctet(Pdu *pdu, Octet *val);
extern Boolean rdint16(Pdu *pdu, Int16 val);
extern Boolean rdskip(Pdu *pdu, int number_to_skip);
extern void syspdu_tx(Pdu *pdu, int port_no);
/******************************************************************************
 * SYSTIME : SYSTEM SUPPLIED SCHEDULING FUNCTIONS
 ******************************************************************************
 */
extern void systime_start_random_timer(int process_id,
                                       void (*expiry_fn)(void *, int instance_id),
                                       int instance_id,
                                       int timeout);
extern void systime_start_timer(int process_id,
                                void (*expiry_fn)(void *, int instance_id),
                                int instance_id,
                                int timeout);
extern void systime_schedule(int process_id,
                             void (*expiry_fn)(void *, int instance_id),
                             int instance_id);
/******************************************************************************
 * SYSERR : FATAL ERROR HANDLING
 ******************************************************************************
 */
extern void syserr_panic();
#endif /* sys_h__ */