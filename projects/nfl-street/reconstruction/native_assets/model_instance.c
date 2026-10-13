#include "model_instance.h"
#include <string.h>
static uint32_t word(const uint8_t*p){return(uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
static void put(uint8_t*p,uint32_t v){p[0]=(uint8_t)(v>>24);p[1]=(uint8_t)(v>>16);p[2]=(uint8_t)(v>>8);p[3]=(uint8_t)v;}
int nflstreet_model_instance_initialize(uint8_t instance[240],uint32_t model,uint32_t palette,uint32_t materials){
 if(!instance)return 0;uint8_t out[240];memcpy(out,instance,sizeof out);
 put(out,1);put(out+12,0);put(out+16,0);put(out+0xe0,0);put(out+0xe4,0);put(out+0xe8,0);put(out+0xec,0);
 memset(out+0x1c,0xff,64);put(out+0x14,word(out+0x14)|15);put(out+0x18,word(out+0x18)|15);
 put(out+4,word(out+4)|0x411);put(out+8,word(out+8)|0x411);put(out+0xe0,palette);put(out+0xe8,materials);
 if(model){put(out+12,model);put(out+0x14,UINT32_MAX);put(out+0x18,UINT32_MAX);}
 memcpy(instance,out,sizeof out);return 1;
}
