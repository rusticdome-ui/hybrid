#include <assert.h>
#include <stdint.h>
int hybrid_wasm_init(void);
int hybrid_wasm_process(uint32_t);
uintptr_t hybrid_wasm_output_ptr(void);
uintptr_t hybrid_wasm_pcm_ptr(uint32_t);
int hybrid_wasm_submit_pcm(uint32_t,uint32_t);
int hybrid_wasm_set_mix(uint32_t,uint32_t,int32_t,uint32_t,uint32_t);
static int32_t clamp_q15_ref(int32_t x){return x>32767?32767:(x< -32768?-32768:x);}
int main(void){
 uint32_t id,i;uintptr_t out;int32_t *pcm[3],*mixed;
 const uint32_t slots[3]={0,1,3};const int32_t left[3]={1234,2000,-300},right[3]={-2345,1000,-400};
 assert(hybrid_wasm_init()==0);
 for(id=0;id<3;id++){uint32_t slot=slots[id];assert(hybrid_wasm_set_mix(slot,32767,0,0,0)==0);pcm[id]=(int32_t*)(uintptr_t)hybrid_wasm_pcm_ptr(slot);assert(pcm[id]!=0);for(i=0;i<64;i++){pcm[id][2*i]=left[id];pcm[id][2*i+1]=right[id];}assert(hybrid_wasm_submit_pcm(slot,64)==0);}
 assert(hybrid_wasm_process(64)==0);out=hybrid_wasm_output_ptr();mixed=(int32_t*)(uintptr_t)out;
 for(i=0;i<64;i++){assert(mixed[2*i]==clamp_q15_ref(left[0]+left[1]+left[2]));assert(mixed[2*i+1]==clamp_q15_ref(right[0]+right[1]+right[2]));}
 return 0;
}