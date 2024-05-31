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
void prw_rdrec_init(Pdu *pdu, Gpdu *gpdu){
    return;
}
Boolean prw_rdrec(Gpdu gpdu){
    return False;
}
Boolean prw_skiprec(Gpdu gpdu){
    return False;
}
#define Garp_terminating_record_id 0x0000