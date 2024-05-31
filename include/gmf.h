/* gmf.h */
#ifndef gmf_h__
#define gmf_h__
#include "sys.h"
#include "prw.h"
#include "gmr.h"
/******************************************************************************
 * GMF : GARP MULTICAST REGISTRATION APPLICATION PDU FORMATTING
 ******************************************************************************
 */
typedef struct
{ /*
   * This data structure saves the temporary state required to parse GMR
   * PDUs in particular. Gpdu provides a common basis for GARP application
   * formatters; additional state can be added here as required by GMF.
   */
    Gpdu gpdu;
} Gmf;
typedef struct /* Gmf_msg_data */
{
    Attribute_type attribute;
    Gid_event event;
    Mac_address key1;
    Mac_address key2;
    Legacy_control legacy_control;
} Gmf_msg;
extern void gmf_rdmsg_init(Pdu *pdu, Gmf **gmf);
extern void gmf_wrmsg_init(Gmf *gmf, Pdu *pdu, int vlan_id);
extern Boolean gmf_rdmsg(Gmf *gmf, Gmf_msg *msg);
extern Boolean gmf_wrmsg(Gmf *gmf, Gmf_msg *msg);
#endif /* gmf_h__ */