/* SPDX-License-Identifier: GPL-3.0-only */
#ifndef HYBRID_AC79_CALIBRATION_H
#define HYBRID_AC79_CALIBRATION_H
#include <stdint.h>
#define HYBRID_AC79_CAL_ANCHORS 5u
#define HYBRID_AC79_CAL_KEYS 37u
#define HYBRID_AC79_CAL_PADS 16u
#define HYBRID_AC79_CAL_MAGIC 0x534d4b43u
typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t key[HYBRID_AC79_CAL_KEYS][HYBRID_AC79_CAL_ANCHORS];
    uint16_t pad[HYBRID_AC79_CAL_PADS][HYBRID_AC79_CAL_ANCHORS];
    uint16_t pedal[HYBRID_AC79_CAL_ANCHORS];
    uint32_t crc32;
} hybrid_ac79_calibration_t;
enum { HYBRID_CAL_KEY=0, HYBRID_CAL_PAD=1, HYBRID_CAL_PEDAL=2 };
void hybrid_ac79_calibration_seal(hybrid_ac79_calibration_t *cal);
int hybrid_ac79_calibration_validate(const hybrid_ac79_calibration_t *cal);
int hybrid_ac79_calibration_set(hybrid_ac79_calibration_t *cal,unsigned kind,unsigned index,unsigned anchor,uint16_t raw);
uint16_t hybrid_ac79_calibration_map(const uint16_t *curve,uint16_t raw);
#endif
