/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
#include "hybrid_ac79_platform.h"

static void emit(hybrid_ac79_platform_t *p, hybrid_ac79_event_type_t type,
                 uint8_t index, int16_t value)
{
 hybrid_ac79_event_t e;
 if (!p->emit) return;
 e.type=type; e.index=index; e.value=value;
 p->emit(p->emit_context,&e);
}
static uint8_t debounce(uint8_t old, uint8_t raw, uint8_t *count)
{
 if (raw == old) { *count=0u; return old; }
 if (*count < 255u) ++*count;
 if (raw) {
  if (*count >= HYBRID_AC79_DEBOUNCE_PRESS) { *count=0u; return 1u; }
 } else if (*count >= HYBRID_AC79_DEBOUNCE_RELEASE) {
  *count=0u; return 0u;
 }
 return old;
}
int hybrid_ac79_platform_init(hybrid_ac79_platform_t *p,
 const hybrid_ac79_board_ops_t *board, hybrid_ac79_event_fn fn, void *ctx)
{
 uint32_t i;
 if (!p || !board || !board->init || !board->scan_inputs) return -1;
 for(i=0;i<sizeof(*p);++i)((uint8_t*)p)[i]=0;
 p->board=board; p->emit=fn; p->emit_context=ctx;
 if(board->init(board->context)!=0) return -2;
 p->initialized=1u;
 return 0;
}
int hybrid_ac79_platform_poll(hybrid_ac79_platform_t *p)
{
 hybrid_ac79_input_snapshot_t s;
 uint32_t i;
 if(!p || !p->initialized || !p->board) return -1;
 for(i=0;i<sizeof(s);++i)((uint8_t*)&s)[i]=0;
 if(p->board->scan_inputs(p->board->context,&s)!=0) return -2;
 for(i=0;i<HYBRID_AC79_KEYS;++i) {
  uint32_t bit=1u<<i; uint8_t old=(uint8_t)!!(p->stable_keys&bit);
  uint8_t next=debounce(old,(uint8_t)!!(s.keys&bit),&p->key_count[i]);
  if(next!=old) { if(next)p->stable_keys|=bit; else p->stable_keys&=~bit;
   emit(p,next?HYBRID_HW_KEY_DOWN:HYBRID_HW_KEY_UP,(uint8_t)i,0); }
 }
 for(i=0;i<HYBRID_AC79_BUTTONS;++i) {
  uint16_t bit=(uint16_t)(1u<<i); uint8_t old=(uint8_t)!!(p->stable_buttons&bit);
  uint8_t next=debounce(old,(uint8_t)!!(s.buttons&bit),&p->button_count[i]);
  if(next!=old) { if(next)p->stable_buttons|=bit; else p->stable_buttons&=(uint16_t)~bit;
   emit(p,next?HYBRID_HW_BUTTON_DOWN:HYBRID_HW_BUTTON_UP,(uint8_t)i,0); }
 }
 for(i=0;i<HYBRID_AC79_PADS;++i) {
  uint16_t bit=(uint16_t)(1u<<i); uint8_t old=(uint8_t)!!(p->stable_pads&bit);
  uint8_t next=debounce(old,(uint8_t)!!(s.pads&bit),&p->pad_count[i]);
  if(next!=old) { if(next)p->stable_pads|=bit; else p->stable_pads&=(uint16_t)~bit;
   emit(p,next?HYBRID_HW_PAD_DOWN:HYBRID_HW_PAD_UP,(uint8_t)i,0); }
 }
 for(i=0;i<HYBRID_AC79_ENCODERS;++i) if(s.encoder_delta[i])
  emit(p,HYBRID_HW_ENCODER,(uint8_t)i,s.encoder_delta[i]);
 for(i=0;i<HYBRID_AC79_FADERS;++i) if(s.fader[i]!=p->last_fader[i]) {
  p->last_fader[i]=s.fader[i]; emit(p,HYBRID_HW_FADER,(uint8_t)i,(int16_t)s.fader[i]);
 }
 for(i=0;i<HYBRID_AC79_WHEELS;++i) if(s.wheel[i]!=p->last_wheel[i]) {
  p->last_wheel[i]=s.wheel[i]; emit(p,HYBRID_HW_WHEEL,(uint8_t)i,(int16_t)s.wheel[i]);
 }
 p->previous=s;
 return 0;
}
int hybrid_ac79_platform_lcd(hybrid_ac79_platform_t *p,uint8_t data,
 const uint8_t *bytes,uint16_t length)
{
 if(!p || !p->initialized || !p->board || !p->board->lcd_write || (!bytes&&length)) return -1;
 return p->board->lcd_write(p->board->context,(uint8_t)!!data,bytes,length);
}
int hybrid_ac79_platform_led(hybrid_ac79_platform_t *p,uint8_t column,uint8_t rows)
{
 if(!p || !p->initialized || !p->board || !p->board->led_write) return -1;
 return p->board->led_write(p->board->context,column,rows);
}
