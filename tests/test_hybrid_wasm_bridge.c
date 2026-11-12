#include <assert.h>
#include <stdint.h>
int hybrid_wasm_init(void);
int hybrid_wasm_process(uint32_t);
void hybrid_wasm_note_on(uint32_t,uint32_t,uint32_t);
uintptr_t hybrid_wasm_output_ptr(void);
uintptr_t hybrid_wasm_pcm_ptr(uint32_t);
int hybrid_wasm_submit_pcm(uint32_t,uint32_t);
int hybrid_wasm_set_mix(uint32_t,uint32_t,int32_t,uint32_t,uint32_t);

static int32_t clamp_q15_ref(int32_t x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return x;
}

int main(void)
{
    uint32_t id, i;
    uintptr_t out;
    int32_t *pcm[3], *mixed;
    const int32_t left[3] = {1234, 2000, -300};
    const int32_t right[3] = {-2345, 1000, -400};

    assert(hybrid_wasm_init() == 0);
    /* Keep the fourth slot silent: this test checks only the three PCM inputs. */
    assert(hybrid_wasm_set_mix(3, 32767, 0, 1, 0) == 0);
    for (id = 0; id < 3; id++) {
        assert(hybrid_wasm_set_mix(id, 32767, 0, 0, 0) == 0);
        pcm[id] = (int32_t *)(uintptr_t)hybrid_wasm_pcm_ptr(id);
        assert(pcm[id] != 0);
        for (i = 0; i < 64; i++) {
            pcm[id][2u * i] = left[id];
            pcm[id][2u * i + 1u] = right[id];
        }
        assert(hybrid_wasm_submit_pcm(id, 64) == 0);
        assert(hybrid_wasm_submit_pcm(id, 64) == -2);
    }

    assert(hybrid_wasm_process(64) == 0);
    out = hybrid_wasm_output_ptr();
    mixed = (int32_t *)(uintptr_t)out;
    for (i = 0; i < 64; i++) {
        const int32_t expected_l = clamp_q15_ref(left[0] + left[1] + left[2]);
        const int32_t expected_r = clamp_q15_ref(right[0] + right[1] + right[2]);
        assert(mixed[2u * i] == expected_l);
        assert(mixed[2u * i + 1u] == expected_r);
    }
    return 0;
}
