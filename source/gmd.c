#include "sys.h"
/******************************************************************************
 * GMD : GARP MULTICAST REGISTRATION APPLICATION DATABASE
 ******************************************************************************
 */
Boolean gmd_create_gmd(unsigned max_multicasts, void **gmd)
{
    return False;
}
/*
 * Creates a new instance of GMD, allocating space for up to max_multicasts
 * MAC addresses.
 *
 * Returns True if the creation succeeded together with a pointer to the
 * GMD information.
 */
void gmd_destroy_gmd(void *gmd)
{
    return;
}
/*
 * Destroys the instance of gmd, releasing previously allocated database and
 * control space.
 */
Boolean gmd_find_entry(void *my_gmd, Mac_address key,
                       unsigned *found_at_index)
{
    return False;
}
Boolean gmd_create_entry(void *my_gmd, Mac_address key,
                         unsigned *created_at_index)
{
    return False;
}
Boolean gmd_delete_entry(void *my_gmd,
                         unsigned delete_at_index)
{
    return False;
}
Boolean gmd_get_key(void *my_gmd, unsigned index, Mac_address *key)
{
    return False;
}