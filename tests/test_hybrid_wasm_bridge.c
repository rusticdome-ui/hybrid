#include <assert.h>
#include <stdint.h>
int hybrid_wasm_init(void);
int hybrid_wasm_process(uint32_t);
void hybrid_wasm_note_on(uint32_t,uint32_t,uint32_t);
uint32_t hybrid_wasm_output_ptr(void);
int main(void){assert(hybrid_wasm_init()==0);hybrid_wasm_note_on(60,100,0);assert(hybrid_wasm_process(64)==0);assert(hybrid_wasm_output_ptr()!=0);return 0;}
