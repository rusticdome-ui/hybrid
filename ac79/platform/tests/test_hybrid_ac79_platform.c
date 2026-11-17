/* SPDX-License-Identifier: GPL-3.0-only */
#include <assert.h>
#include <stdint.h>
#include "hybrid_ac79_platform.h"
static hybrid_ac79_input_snapshot_t raw;
static unsigned init_count, event_count, key_down, key_up, fader_events, encoder_events;
static int board_init(void *ctx){(void)ctx;++init_count;return 0;}
static int scan(void *ctx,hybrid_ac79_input_snapshot_t *out){(void)ctx;*out=raw;raw.encoder_delta[0]=0;return 0;}
static int lcd(void *ctx,uint8_t data,const uint8_t *b,uint16_t n){(void)ctx;(void)data;(void)b;(void)n;return 0;}
static int led(void *ctx,uint8_t c,uint8_t r){(void)ctx;(void)c;(void)r;return 0;}
static void on_event(void *ctx,const hybrid_ac79_event_t *e){(void)ctx;++event_count;
 if(e->type==HYBRID_HW_KEY_DOWN)++key_down;
 if(e->type==HYBRID_HW_KEY_UP)++key_up;
 if(e->type==HYBRID_HW_FADER)++fader_events;
 if(e->type==HYBRID_HW_ENCODER)++encoder_events;
}
int main(void){
 hybrid_ac79_platform_t p;
 const hybrid_ac79_board_ops_t ops={board_init,scan,lcd,led,0};
 assert(hybrid_ac79_platform_init(&p,&ops,on_event,0)==0); assert(init_count==1);
 raw.keys=1u; assert(hybrid_ac79_platform_poll(&p)==0); assert(key_down==0);
 assert(hybrid_ac79_platform_poll(&p)==0); assert(key_down==1);
 raw.keys=0u; for(unsigned i=0;i<7;i++)assert(hybrid_ac79_platform_poll(&p)==0);
 assert(key_up==0); assert(hybrid_ac79_platform_poll(&p)==0); assert(key_up==1);
 raw.fader[2]=2048; raw.encoder_delta[0]=2;
 assert(hybrid_ac79_platform_poll(&p)==0); assert(fader_events==1); assert(encoder_events==1);
 assert(event_count==4);
 assert(hybrid_ac79_platform_lcd(&p,0,0,0)==0);
 assert(hybrid_ac79_platform_led(&p,0,0)==0);
 return 0;
}
