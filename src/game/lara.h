#ifndef TR1MAIN_GAME_LARA_H
#define TR1MAIN_GAME_LARA_H

#include "game/types.h"
#include "util.h"

// clang-format off
#define InitialiseLaraInventory ((void          __cdecl(*)(int level_id))0x00428170)
#define LaraControl             ((void          __cdecl(*)(int16_t item_num))0x00427850)
// clang-format on

void __cdecl InitialiseLara();
void __cdecl LaraAsWalk(ITEM_INFO* item, COLL_INFO* coll);
void __cdecl UseItem(__int16 object_num);

void TR1MInjectLara();
void TR1MInjectLaraMisc();

#endif
