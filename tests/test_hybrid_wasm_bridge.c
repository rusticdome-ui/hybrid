#include <assert.h>
#include <stdint.h>
int hybrid_wasm_init(void);
int hybrid_wasm_process(uint32_t);
void hybrid_wasm_note_on(uint32_t,uint32_t,uint32_t);
uint32_t hybrid_wasm_output_ptr(void);
uint32_t hybrid_wasm_pcm_ptr(uint32_t);
int hybrid_wasm_submit_pcm(uint32_t,uint32_t);
int main(void){
 uint32_t i,p,out;
 int32_t *pcm,*mixed;
 assert(hybrid_wasm_init()==0);
 hybrid_wasm_note_on(60,100,0);
 assert(hybrid_wasm_process(64)==0);
 assert(hybrid_wasm_output_ptr()!=0);
 p=hybrid_wasm_pcm_ptr(0);assert(p!=0);pcm=(int32_t*)(uintptr_t)p;
 for(i=0;i<128;i++)pcm[i]=(i&1u)?-2345:1234;
 assert(hybrid_wasm_submit_pcm(0,64)==0);
 assert(hybrid_wasm_submit_pcm(0,64)==-2);
 assert(hybrid_wasm_process(64)==0);
 out=hybrid_wasm_output_ptr();mixed=(int32_t*)(uintptr_t)out;
 assert(mixed[0]==1234&&mixed[1]==-2345);
 return 0;
}
