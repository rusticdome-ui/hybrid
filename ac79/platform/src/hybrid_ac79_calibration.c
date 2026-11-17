/* SPDX-License-Identifier: GPL-3.0-only */
#include "hybrid_ac79_calibration.h"
static const uint16_t target[5]={0,256,512,768,1023};
static int curve_valid(const uint16_t *v)
{
    unsigned i;
    for(i=0;i<5u;i++)if(v[i]>1023u||(i&&v[i]<=v[i-1]))return 0;
    return 1;
}
int hybrid_ac79_calibration_validate(const hybrid_ac79_calibration_t *c)
{
    unsigned i;
    if(!c||c->magic!=HYBRID_AC79_CAL_MAGIC||c->version!=1u)return 0;
    for(i=0;i<HYBRID_AC79_CAL_KEYS;i++)if(!curve_valid(c->key[i]))return 0;
    for(i=0;i<HYBRID_AC79_CAL_PADS;i++)if(!curve_valid(c->pad[i]))return 0;
    return curve_valid(c->pedal);
}
int hybrid_ac79_calibration_set(hybrid_ac79_calibration_t *c,unsigned kind,unsigned index,unsigned anchor,uint16_t raw)
{
    uint16_t *v;
    if(!c||anchor>=5u||raw>1023u)return -1;
    if(kind==HYBRID_CAL_KEY){if(index>=37u)return -2;v=c->key[index];}
    else if(kind==HYBRID_CAL_PAD){if(index>=16u)return -2;v=c->pad[index];}
    else if(kind==HYBRID_CAL_PEDAL){if(index!=0u)return -2;v=c->pedal;}
    else return -3;
    v[anchor]=raw;
    return 0;
}
uint16_t hybrid_ac79_calibration_map(const uint16_t *v,uint16_t raw)
{
    unsigned i;
    if(!v||raw<=v[0])return 0u;
    if(raw>=v[4])return 1023u;
    for(i=1;i<5u;i++)if(raw<=v[i])
        return (uint16_t)(target[i-1]+((uint32_t)(raw-v[i-1])*(target[i]-target[i-1]))/(v[i]-v[i-1]));
    return 1023u;
}
