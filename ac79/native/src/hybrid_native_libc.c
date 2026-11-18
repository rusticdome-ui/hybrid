/* SPDX-License-Identifier: GPL-3.0-only
 * Tiny freestanding runtime helper required by pi32v2 aggregate assignments.
 * This target intentionally links no hosted C library.
 */
#include <stddef.h>
void *memcpy(void *destination, const void *source, size_t count)
{
    unsigned char *d=(unsigned char *)destination;
    const unsigned char *s=(const unsigned char *)source;
    size_t i;
    for(i=0;i<count;i++) d[i]=s[i];
    return destination;
}
