/* SPDX-License-Identifier: GPL-3.0-only
 * Host-only stand-ins: validate adapter wiring, not upstream DSP sound.
 */
#include <stdint.h>
void amb_en_init(void) {}
void amb_en_set(int p,int v) {(void)p;(void)v;}
void amb_en_play(int on) {(void)on;}
void amb_en_panic(void) {}
void amb_en_render(int32_t *o,uint32_t n,uint32_t g) {(void)g;for(uint32_t i=0;i<n*2u;i++)o[i]=256;}
void dub_en_init(void) {}
void dub_en_set(int p,int v) {(void)p;(void)v;}
void dub_en_play(int on) {(void)on;}
void dub_en_freeze(int on) {(void)on;}
void dub_en_panic(void) {}
void dub_en_render(int32_t *o,uint32_t n,uint32_t g) {(void)g;for(uint32_t i=0;i<n*2u;i++)o[i]=512;}
void zen_fm_init(void) {}
void zen_fm_set(int p,int v) {(void)p;(void)v;}
void zen_fm_note_vel(int id,int on,float c,int v) {(void)id;(void)on;(void)c;(void)v;}
void zen_fm_panic(void) {}
void zen_fm_render(int32_t *o,uint32_t n,uint32_t g) {(void)g;for(uint32_t i=0;i<n*2u;i++)o[i]=768;}
