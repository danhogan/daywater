#ifndef NFLSTREET_MENU_SCREEN_H
#define NFLSTREET_MENU_SCREEN_H
#include <stddef.h>
#include <stdint.h>
/* Original 801CF8C8 logical pointer relocation on caller-owned bytes.
   base is a supplied serialization address, not a live game RAM address.
   Result 1=fresh, FFFFFFFF=already relocated. Invalid input is atomic. */
int nflstreet_ui_relocate(uint8_t*,size_t,uint32_t base,uint32_t*result);
/* Original 801D3890 / 801D3948 / 801D38E4 selection; numeric selector kinds
   0=mask8000, 1=mask4000, 2=unmasked low2FFF with supplied object ID.
   Scripts are returned as owned-file offsets. No input mapping or VM execution. */
int nflstreet_ui_event(const uint8_t*,size_t,uint32_t base,unsigned node,
 unsigned kind,uint16_t event,uint16_t object,uint32_t*script_offset,unsigned*found);
#endif
