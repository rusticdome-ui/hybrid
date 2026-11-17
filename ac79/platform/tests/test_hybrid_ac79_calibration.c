#include <assert.h>
#include <string.h>
#include "hybrid_ac79_calibration.h"
int main(void){
 hybrid_ac79_calibration_t c;uint16_t curve[5]={100,200,300,400,500};memset(&c,0,sizeof c);
 c.magic=HYBRID_AC79_CAL_MAGIC;c.version=1;
 for(unsigned i=0;i<37;i++)for(unsigned j=0;j<5;j++)c.key[i][j]=(uint16_t)(100+j*100);
 for(unsigned i=0;i<16;i++)for(unsigned j=0;j<5;j++)c.pad[i][j]=(uint16_t)(100+j*100);
 for(unsigned j=0;j<5;j++)c.pedal[j]=(uint16_t)(100+j*100);
 assert(hybrid_ac79_calibration_validate(&c));assert(hybrid_ac79_calibration_map(curve,100)==0);
 assert(hybrid_ac79_calibration_map(curve,300)==512);assert(hybrid_ac79_calibration_map(curve,500)==1023);
 assert(hybrid_ac79_calibration_set(&c,HYBRID_CAL_PAD,15,4,999)==0);assert(c.pad[15][4]==999);
 assert(hybrid_ac79_calibration_set(&c,HYBRID_CAL_KEY,37,0,10)==-2);c.key[0][3]=c.key[0][2];
 assert(!hybrid_ac79_calibration_validate(&c));return 0;
}
