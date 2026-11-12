/* SPDX-License-Identifier: Apache-2.0
 * SMK-37 HYBRID adapter around Google's MSFA / Dexed DX7 synthesis core.
 * The six-operator synthesis engine is upstream MSFA; this file is the WASM host.
 */
#include <stdint.h>
#include <string.h>
#include "msfa/synth_unit.h"
#include "msfa/ringbuffer.h"
static RingBuffer g_ring;
static SynthUnit *g_synth=0;
static uint8_t g_bank[4096];
static int16_t g_output[128];
extern "C" {
__attribute__((visibility("default"))) int dx7_init(uint32_t sample_rate) {
  if(sample_rate<8000 || sample_rate>192000)return -1;
  if(g_synth)return 0;
  SynthUnit::Init((double)sample_rate);
  g_synth=new SynthUnit(&g_ring);
  memset(g_bank,0,sizeof(g_bank));
  return g_synth?0:-2;
}
__attribute__((visibility("default"))) uint32_t dx7_bank_ptr(void){return (uint32_t)(uintptr_t)g_bank;}
__attribute__((visibility("default"))) int dx7_load_bank(void){if(!g_synth)return -1;g_synth->loadBank(g_bank);return 0;}
__attribute__((visibility("default"))) int dx7_select_program(uint32_t p){if(!g_synth||p>31)return -1;g_synth->selectProgram((int)p);return 0;}
__attribute__((visibility("default"))) void dx7_midi(uint32_t status,uint32_t d1,uint32_t d2){if(g_synth)g_synth->sendMidi((uint8_t)status,(uint8_t)d1,(uint8_t)d2);}
__attribute__((visibility("default"))) void dx7_panic(void){if(g_synth)g_synth->allNotesOff();}
__attribute__((visibility("default"))) int dx7_render(uint32_t frames){if(!g_synth||!frames||frames>128)return -1;g_synth->GetSamples((int)frames,g_output);return 0;}
__attribute__((visibility("default"))) uint32_t dx7_output_ptr(void){return (uint32_t)(uintptr_t)g_output;}
}
