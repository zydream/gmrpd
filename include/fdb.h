/* fdb.h */
#ifndef fdb_h__
#define fdb_h__
#include "sys.h"
/******************************************************************************
 * FDB : FILTERING DATABASE INTERFACE
 ******************************************************************************
 */
extern void fdb_filter(unsigned vlan_id, int port_no, Mac_address address);
extern void fdb_forward(unsigned vlan_id, int port_no, Mac_address address);
extern void fdb_filter_by_default(unsigned vlan_id, int port_no);
extern void fdb_forward_by_default(unsigned vlan_id, int port_no);
#endif /* fdb_h__ */