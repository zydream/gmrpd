/* prw.h */
#ifndef prw_h__
#define prw_h__
#include "sys.h"
/******************************************************************************
 * PRW : PDU READ WRITE ACCESS
 ******************************************************************************
 */
typedef struct
{
    Pdu *pdu;
    int record_id;
    int record_len;
} Gpdu;
extern void prw_rdrec_init(Pdu *pdu, Gpdu *gpdu);
extern Boolean prw_rdrec(Gpdu gpdu);
extern Boolean prw_skiprec(Gpdu gpdu);
#define Garp_terminating_record_id 0x0000
#endif /* prw_h__ */