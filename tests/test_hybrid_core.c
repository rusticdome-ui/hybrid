#include "hybrid_core.h"
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef struct {int on,off,last_note,renders;} mock_t;
static int mock_init(void*p){(void)p;return 0;}
static void mock_on(void*p,uint8_t n,uint8_t v){mock_t*m=(mock_t*)p;(void)v;m->on++;m->last_note=n;}
static void mock_off(void*p,uint8_t n){mock_t*m=(mock_t*)p;m->off++;m->last_note=n;}
static void mock_render(void*p,int32_t*out,uint16_t frames){mock_t*m=(mock_t*)p;uint16_t i;m->renders++;for(i=0;i<frames;i++){out[2*i]=10000;out[2*i+1]=10000;}}
static const hybrid_engine_ops_t ops={"mock",mock_init,0,mock_on,mock_off,0,mock_render,0,0,0};
int main(void){hybrid_core_t h;mock_t m;int32_t out[HYBRID_MAX_FRAMES*2],pcm[128];uint32_t i;memset(&m,0,sizeof(m));hybrid_init_defaults(&h);assert(hybrid_register_engine(&h,HYBRID_AMB,&ops,&m)==0);assert(hybrid_start(&h)==0);hybrid_set_route(&h,HYBRID_AMB,HYBRID_ROUTE_ZONE,48,72);hybrid_note_on(&h,60,100,1);hybrid_note_on(&h,40,100,1);assert(m.on==1&&m.last_note==60);h.engine[HYBRID_AMB].config.transpose=12;hybrid_note_off(&h,60,1);assert(m.off==1&&m.last_note==72);hybrid_set_route(&h,HYBRID_AMB,HYBRID_ROUTE_GLOBAL,0,127);hybrid_set_volume(&h,HYBRID_AMB,16384);for(i=0;i<128;i++)pcm[i]=(i&1)?-12000:12000;assert(hybrid_submit_pcm(&h,HYBRID_AMB,pcm,64)==0);assert(hybrid_submit_pcm(&h,HYBRID_AMB,pcm,64)==-2);assert(hybrid_process(&h,out,64)==0);assert(out[0]>=5990&&out[0]<=6010&&out[1]<=-5990&&out[1]>=-6010);assert(m.renders==0);assert(hybrid_process(&h,out,64)==0);assert(m.renders==1&&out[0]>=4990&&out[0]<=5010);assert(hybrid_submit_pcm(&h,HYBRID_AMB,pcm,32)==0);assert(hybrid_process(&h,out,64)==-2);assert(h.engine[HYBRID_AMB].pcm_valid==0);assert(hybrid_scene_capture(&h,0)==0);hybrid_set_volume(&h,HYBRID_AMB,0);assert(hybrid_scene_recall(&h,0,0)==0);assert(h.engine[HYBRID_AMB].config.volume==16384);hybrid_set_mute(&h,HYBRID_AMB,1);assert(hybrid_process(&h,out,64)==0&&out[0]==0);puts("SMK-37 HYBRID core + PCM ingress tests: PASS");return 0;}
