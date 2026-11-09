/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include "hybrid_core.h"
#include "hybrid_native_ports.h"
int main(void)
{
    hybrid_core_t h;
    int32_t out[64];
    hybrid_init_defaults(&h);
    assert(hybrid_register_native_engines(&h)==0);
    assert(hybrid_start(&h)==0);
    assert(h.engine[HYBRID_AMB].ops != 0);
    assert(h.engine[HYBRID_DUB].ops != 0);
    assert(h.engine[HYBRID_ZEN].ops != 0);
    assert(h.engine[HYBRID_GHOULBOX].ops == 0);
    assert(hybrid_process(&h,out,32)==0);
    assert(out[0] == 6); /* 1 + 2 + 3 Q15 units, unity gains */
    hybrid_note_on(&h,69,100,1);
    hybrid_note_off(&h,69,1);
    hybrid_panic(&h);
    return 0;
}
