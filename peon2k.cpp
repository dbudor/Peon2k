#include <stdint.h>
#include <stdio.h>
#include <windows.h>

#include "defs.h"   //some addresses and constants
#include "names.h"  //some constants
#include "patch.h"  //Fois patcher

#ifdef DEBUG
#define log(p, fmt, ...) msg_log(p, fmt, __VA_ARGS__)
#include <stdarg.h>

void msg_log(int p, const char* fmt, ...) {
  char buffer[256];
  va_list args;
  va_start(args, fmt);
  _vsnprintf(buffer, sizeof(buffer), fmt, args);
  va_end(args);
  ((void (*)(char*, int, int))F_MAP_MSG)(buffer, (byte)p, 50);
}

#else
#define log(p, fmt, ...)
#endif

#define UNIT_SCORE_TABLE 0x004CF190

static void hook(int adr, PROC* p, char* func) {
  *p = patch_call((char*)adr, func);
}

byte rune_owners[48];

void bullet_create(WORD x, WORD y, byte id) {
  ((int* (*)(WORD, WORD, byte))F_BULLET_CREATE)(x, y, id);
}

void unit_kill(int* u) {
  ((void (*)(int*))F_UNIT_KILL)(u);  // original war2 func kills unit
}

#define F_SET_KILLS_SCORE 0x0043D760
void set_kills_score(byte player, int* u) {
  ((void (*)(byte, int*))F_SET_KILLS_SCORE)(player, u);
}

void rune_bullet(int runeid, WORD x, WORD y, byte id, int* u) {
  short* tt = (short*)RUNEMAP_TIMERS;
  byte* xt = (byte*)RUNEMAP_X;
  byte* yt = (byte*)RUNEMAP_Y;

  byte xx = (x - 0x10) >> 5;
  byte yy = (y - 0x10) >> 5;

  tt[runeid] = 0x800;
  xt[runeid] = xx;
  yt[runeid] = yy;

  byte player_id = *((byte*)((uintptr_t)u + S_OWNER));
  rune_owners[runeid] = player_id;
  bullet_create(x, y, id);
}

void rune_unit_hit(int* u, int runeptr) {
  int rune_id = (runeptr - RUNEMAP_TIMERS) / 2;
  unsigned short hp = *((unsigned short*)((uintptr_t)u + S_HP));
  byte rune_owner = rune_owners[rune_id];
  rune_owners[rune_id] = 0xff;
  if (hp < 50) {
    unit_kill(u);
    if (rune_owner != 0xff) set_kills_score(rune_owner, u);
  } else {
    *((unsigned short*)((uintptr_t)u + S_HP)) -= 50;
  }
}

void patch_rune() {
  char o1[] = "\x56\x8B\x74\x24\x10\x56";
  char o2[] =
      "\x50\x90\x90\x90\x90\x90\x83\xC4\x08\x5E\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90";
  PATCH_SET((char*)0x0044359A, o1);
  PATCH_SET((char*)0x004435B2, o2);
  patch_call((char*)0x004435B3, (char*)rune_bullet);

  char o3[] = "\x56\x8B\x74\x24\x10\x56";
  char o4[] =
      "\x50\x90\x90\x90\x90\x90\x83\xC4\x08\x5E\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90";
  PATCH_SET((char*)0x00443670, o3);
  PATCH_SET((char*)0x00443686, o4);
  patch_call((char*)0x00443687, (char*)rune_bullet);

  char o5[] = "\x52\x8B\x54\x24\x10\x52\x90\x90\x90\x90\x90\x90";
  char o6[] = "\x56\x90\x90\x90\x90\x90\x83\xC4\x08\x5A\x90\x90\x90\x90\x90";
  PATCH_SET((char*)0x00443743, o5);
  PATCH_SET((char*)0x0044375F, o6);
  patch_call((char*)0x00443760, (char*)rune_bullet);

  char o7[] = "\x56\x8B\x74\x24\x10\x56";
  char o8[] =
      "\x50\x90\x90\x90\x90\x90\x83\xC4\x08\x5E\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90";
  PATCH_SET((char*)0x004437E8, o7);
  PATCH_SET((char*)0x004437FE, o8);
  patch_call((char*)0x004437FF, (char*)rune_bullet);

  char o9[] =
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x55\x53\x90\x90\x90\x90\x90\x83\xC4"
      "\x08";
  PATCH_SET((char*)0x0044244C, o9);
  patch_call((char*)0x00442457, (char*)rune_unit_hit);
}

void patch_heal_max_hp(int max) {
  char o1[2];
  short s = (short)max;
  memcpy(o1, &s, sizeof(s));
  PATCH_SET((char*)0x00442656, o1);
  PATCH_SET((char*)0x0044265B, o1);
}

#define F_CRITTER_CREATE 0x00451B50  // x y critter flag
int* critter_create(WORD x, WORD y, byte id, byte f) {
  return ((int* (*)(WORD, WORD, byte, byte))F_CRITTER_CREATE)(x, y, id, f);
}

void polymorph_unit_kill(int* u, int* caster) {
  byte id = *((byte*)((uintptr_t)u + S_ID));
  short x, y;
  int* target;
  if (id == U_PEASANT || id == U_PEON)
    target = caster;
  else
    target = u;

  *((byte*)((uintptr_t)target + S_FLAGS3)) |= SF_HIDDEN;
  *((byte*)((uintptr_t)target + S_SEQ_FLAG)) |= 0x20;
  unit_kill(target);
  x = *((short*)((uintptr_t)target + S_X));
  y = *((short*)((uintptr_t)target + S_Y));
  x <<= 5;
  y <<= 5;
  critter_create(x, y, U_CRITTER, 0x0f);
  bullet_create(x + 16, y + 16, 0x16);
  set_kills_score(*((byte*)((uintptr_t)caster + S_OWNER)), target);
  return;
}

void patch_polymorph() {
  char o1[] =
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x57\x56";
  PATCH_SET((char*)0x00442B9C, o1);
  patch_call((char*)0x00442BB2, (char*)polymorph_unit_kill);
  char o2[] =
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x83\xc4\x18";
  PATCH_SET((char*)0x00442BB7, o2);
}

void unholy_hp_shield(int* target, int* caster) {
  byte id = *((byte*)((uintptr_t)target + S_ID));
  if (id == U_PEASANT || id == U_PEON) {
    unsigned short hp = *((unsigned short*)((uintptr_t)caster + S_HP));
    if (hp >= 2) *((unsigned short*)((uintptr_t)caster + S_HP)) = hp >> 1;
  } else {
    unsigned short hp = *((unsigned short*)((uintptr_t)target + S_HP));
    if (hp >= 2) *((unsigned short*)((uintptr_t)target + S_HP)) = hp >> 1;
    *((unsigned short*)((uintptr_t)target + S_SHIELD)) = 0x1f4;
  }
}

void patch_unholy() {
  char o1[] = "\x57\x56";
  patch_call((char*)0x00443452, (char*)unholy_hp_shield);
  PATCH_SET((char*)0x00443450, o1);  // push edi; push esi
  char o2[] =
      "\x83\xc4\x08\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90";
  PATCH_SET((char*)0x00443457, o2);  // add esp, 8; nop...
}

#define SCORE 0x004ACE00

void transport_kills(int player, int* u) {
  int* p = (int*)UNITS_MASSIVE;  // pointer to units
  p = (int*)(*p);
  int k = *(int*)UNITS_NUMBER;
  while (k > 0) {
    byte flags3 = *((byte*)((uintptr_t)p + S_FLAGS3));
    byte id = *((byte*)((uintptr_t)p + S_ID));
    if ((flags3 & (SF_UNIT_FREE | SF_DIEING | SF_DEAD)) == 0 &&
        (flags3 & SF_HIDDEN) != 0 &&
        (*(int*)((uintptr_t)UNIT_GLOBAL_FLAGS + id * 4) & IS_MAN) != 0 &&
        *(int*)((uintptr_t)p + S_ORDER_UNIT_POINTER) == (int)u) {
      ((short*)KILLS_UNITS)[player]++;
      ((int*)SCORE)[player] += (int)((short*)UNIT_SCORE_TABLE)[id];
    }
    p = (int*)((uintptr_t)p + 0x98);
    k--;
  }
}

void building_kills(int player, int* u) {
  int* p = (int*)UNITS_MASSIVE;  // pointer to units
  p = (int*)(*p);
  int k = *(int*)UNITS_NUMBER;
  while (k > 0) {
    byte flags3 = *((byte*)((uintptr_t)p + S_FLAGS3));
    byte id = *((byte*)((uintptr_t)p + S_ID));
    if ((flags3 & (SF_UNIT_FREE | SF_DIEING | SF_DEAD)) == 0 &&
        (flags3 & SF_HIDDEN) != 0 &&
        (*(int*)((uintptr_t)UNIT_GLOBAL_FLAGS + id * 4) & IS_MAN) != 0 &&
        *(int*)((uintptr_t)p + S_ORDER_UNIT_POINTER) == (int)u) {
      unsigned short hp = *((unsigned short*)((uintptr_t)u + S_HP));
      if (hp == 0) {
        ((short*)KILLS_UNITS)[player]++;
        ((int*)SCORE)[player] += (int)((short*)UNIT_SCORE_TABLE)[id];
      }
    }
    p = (int*)((uintptr_t)p + 0x98);
    k--;
  }
}

int inc_kills(int player, int* u) {
  player &= 0xff;
  *((unsigned short*)((uintptr_t)u + S_HP)) = 0;
  // set hp to 0 so we know it was killed, not out of resources
#ifdef DEBUG
  if (1) {
#else
  if (player != (int)*(byte*)((uintptr_t)u + S_OWNER)) {
#endif

    byte id = *((byte*)((uintptr_t)u + S_ID));
    if (id >= U_FARM) {
      ((short*)KILLS_BUILDINGS)[player]++;
      building_kills(player, u);
    } else if (id != U_CRITTER) {
      ((short*)KILLS_UNITS)[player]++;
    }
    ((int*)SCORE)[player] += (int)((short*)UNIT_SCORE_TABLE)[id];

    if (*(int*)(UNIT_GLOBAL_FLAGS + id * 4) & IS_TRANSPORT) {
      transport_kills(player, u);
    }
  }
  log(player, "kills: %d, razings: %d, score: %d",
      (int)((short*)KILLS_UNITS)[player],
      (int)((short*)KILLS_BUILDINGS)[player], ((int*)SCORE)[player]);
  return player;
}

typedef int(__cdecl* place_callback)(int*);
#define PLACE_UNIT_POSITION 0x00443A40

#define ORDER_FUNCTIONS_MOVE 0x00495EE4
int fix_enter_dead_building(int* u) {
  log(*((byte*)((uintptr_t)u + S_OWNER)), "%s", "unit entering dead building");
  *((int*)((uintptr_t)u + S_ORDER_UNIT_POINTER)) = 0;
  *((int*)((uintptr_t)u + S_PEON_GOLDMINE_POINTER)) = 0;
  *((byte*)((uintptr_t)u + S_PEON_FLAGS)) = 0;
  *((byte*)((uintptr_t)u + S_ORDER)) = ORDER_MOVE;
  int fnp = *(int*)(ORDER_FUNCTIONS_MOVE);
  int loc = *((int*)((uintptr_t)u + S_X));
  int size = 0x00040004;
  int target = *(int*)((uintptr_t)u + S_ORDER_X);
  *(short*)0x4BE260 = *((byte*)((uintptr_t)u + 42)) & 0xff;
  *(byte*)0x4BE270 = 0;
  if (((int (*)(int*, int, int, int, place_callback))PLACE_UNIT_POSITION)(
          &loc, size, target, 0xCu, (place_callback)0x4512A0)) {
    int x = loc & 0xff;
    int y = (loc >> 16) & 0xff;
    *((byte*)((uintptr_t)u + S_FLAGS3)) &= ~8u;
    *((short*)((uintptr_t)u + S_X)) = x;
    *((short*)((uintptr_t)u + S_Y)) = y;
    *((short*)((uintptr_t)u + S_DRAW_X)) = x * 32;
    *((short*)((uintptr_t)u + S_DRAW_Y)) = y * 32;
    ((int (*)(int*))F_UNIT_PLACE)(u);
    *((byte*)((uintptr_t)u + S_SEQ_FLAG)) |= 0x20u;
    *((byte*)((uintptr_t)u + S_FACE)) = ((int (*)(int*))0x429DF0)(u);
    ((void (*)(int*, byte))0x453130)(u, 2);
    log(*((byte*)((uintptr_t)u + S_OWNER)), "%s", "saved");
    return ((int (*)(int*))fnp)(u);
  } else {
    log(*((byte*)((uintptr_t)u + S_OWNER)), "%s", "sorry");
    ((void (*)(int*))F_UNIT_KILL)(u);
    return 1;
  }
}

void patch_kills_score() {
  patch_ljmp((char*)0x0043D760, (char*)inc_kills);
  patch_call((char*)0x004246D0, (char*)fix_enter_dead_building);
  char o1[] = "\x90\x90\x90\x90\x90";
  PATCH_SET((char*)0x004246D8, o1);
}

extern int new_exit_pos(int*, int, int, int, place_callback);

int place_unit_position(int* pos, int size, int target, int steps,
                        place_callback callback) {
  if (*pos == target) {
    return ((int (*)(int*, int, int, int, place_callback))PLACE_UNIT_POSITION)(
        pos, size, target, steps, callback);
  }
  return new_exit_pos(pos, size, target, steps, callback);
}

void patch_unit_exit_building() {
  patch_call((char*)0x00451223, (char*)place_unit_position);
  patch_call((char*)0x00451951, (char*)place_unit_position);
  patch_call((char*)0x00451A09, (char*)place_unit_position);
  patch_call((char*)0x00451AC9, (char*)place_unit_position);
  patch_call((char*)0x00452A2E, (char*)place_unit_position);
}

void manacost(byte id, byte c) {
  // spells cost of mana
  char mana[] = "\x01";
  mana[0] = c;
  PATCH_SET((char*)(MANACOST + 2 * id), mana);
}

void repair_cat(bool b) {
  // peon can repair unit if it have transport flag OR catapult flag
  if (b) {
    char r1[] = "\xeb\x75\x90\x90\x90";  // f6 c4 04 74 14
    PATCH_SET((char*)REPAIR_FLAG_CHECK2, r1);
    char r2[] = "\x66\xa9\x04\x04\x74\x9c\xeb\x86";
    PATCH_SET((char*)REPAIR_CODE_CAVE, r2);
  } else {
    char r1[] = "\xf6\xc4\x04\x74\x14";
    PATCH_SET((char*)REPAIR_FLAG_CHECK2, r1);
  }
}

void unitscore(byte id, short score) {
  // spells cost of mana
  char s[] = "RR";
  short* ss = (short*)s;
  ss[0] = score;
  PATCH_SET((char*)(UNIT_SCORE_TABLE + 2 * id), s);
}

void* def_name = NULL;
void* def_sound = NULL;
char ajmoo_name[] = "PEON2K\\ajmoo.wav\x00";
void* ajmoo_sound = NULL;

PROC g_proc_00442D74;
void play_sound_bloodlust(int* u, int sound) {
  byte id = *((byte*)((uintptr_t)u + S_ID));
  if (id == U_PALADIN || id == U_KNIGHT) {
    int sid = sound + 68;
    def_name = (void*)*(int*)(SOUNDS_FILES_LIST + 8 + 24 * sid);
    def_sound = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * sid);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * sid),
                   (DWORD)ajmoo_name);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * sid),
                   (DWORD)ajmoo_sound);
    ((void (*)(int*, int))g_proc_00442D74)(u, sound);
    ajmoo_sound = (void*)*(int*)(SOUNDS_FILES_LIST + 16 + 24 * sid);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 8 + 24 * sid), (DWORD)def_name);
    patch_setdword((DWORD*)(SOUNDS_FILES_LIST + 16 + 24 * sid),
                   (DWORD)def_sound);
  } else {
    ((void (*)(int*, int))g_proc_00442D74)(u, sound);
  }
}

#include "gen/ajmoo.h"

void patch_bloodlust_sound() {
  FILE* fp;
  DWORD dwAttrib = GetFileAttributes("PEON2K");
  if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
    CreateDirectory("PEON2K", NULL);
    if ((fp = fopen("PEON2K\\ajmoo.wav", "w+b")) != NULL) {
      fwrite(ajmoo_wav, 1, ajmoo_wav_len, fp);
      fclose(fp);
    }
  }
  hook(0x00442D74, &g_proc_00442D74, (char*)play_sound_bloodlust);
}

void patch_max_units() {
  char s[] = "\x90\x90";
  PATCH_SET((char*)0x42052b, s);
}

#define MAX_VISIONS 32
short vision_ttl;
int vision_range_sq;
bool vision_see_ally;

struct vision {
  short x;
  short y;
  short ttl;
  short player;
};

struct vision visions[MAX_VISIONS];

struct vision* next_vision() {
  int i;
  struct vision* best = NULL;
  struct vision* p = visions;
  for (i = 0; i < MAX_VISIONS; i++, p++) {
    if (!p->ttl) return p;
    if (best == NULL || best->ttl < p->ttl) best = p;
  }
  return best;
}

PROC g_proc_004424F5;

void spell_vision(WORD x, WORD y, int* u) {
  ((void (*)(WORD, WORD, int*))g_proc_004424F5)(x, y, u);
  byte player_id = *((byte*)((uintptr_t)u + S_OWNER));
  if (player_id == *(byte*)LOCAL_PLAYER) {
    short ux = *((short*)((uintptr_t)u + S_X));
    short uy = *((short*)((uintptr_t)u + S_Y));
    int dx = ux - x;
    int dy = uy - y;
    if (dx * dx + dy * dy > 100) {
      ((void (*)(byte, byte))F_MINIMAP_CLICK)((byte)x, (byte)y);
    }
  }

  if (!vision_range_sq) return;
  struct vision* v = next_vision();
  v->x = (short)x;
  v->y = (short)y;
  v->player = (short)player_id;
  v->ttl = vision_ttl;
}

#define NETWORK_MODE 0x004AE21C

void apply_visions(int* u) {
  int i;
  struct vision* p = visions;
  int visible = 0;

  short x = *((short*)((uintptr_t)u + S_X));
  short y = *((short*)((uintptr_t)u + S_Y));
  byte player_id = *((byte*)((uintptr_t)u + S_OWNER));

  bool enhanced = (bool)*((byte*)NETWORK_MODE);
  if (enhanced) {
    if (player_id < 8) {
      visible = *(byte*)((uintptr_t)VIZ + player_id);
    }
  } else {
    visible |= 1 << player_id;
  }

  if (vision_see_ally && player_id < 8) {
    int a;
    byte* bp = (byte*)((uintptr_t)ALLY + 16 * player_id);
    for (a = 0; a < 8; a++) {
      if (*(bp + a)) {
        visible |= 1 << a;
      }
    }
  }

  if (vision_range_sq) {
    for (i = 0; i < MAX_VISIONS; i++, p++) {
      if (!p->ttl) {
        continue;
      }
      int dx = p->x - x;
      int dy = p->y - y;
      if (dx * dx + dy * dy <= vision_range_sq) {
        visible |= 1 << p->player;
      }
    }
  }
  *((byte*)((uintptr_t)u + 41)) = (byte)visible;
}

void expire_visions() {
  if (!vision_range_sq) return;

  int i;
  struct vision* p = visions;
  for (i = 0; i < MAX_VISIONS; i++, p++) {
    if (p->ttl > 0) p->ttl--;
  }
}

PROC g_proc_0042A4A1;
void new_game(int a, int b, long c) {
  memset(visions, 0, MAX_VISIONS * sizeof(struct vision));
  ((void (*)(int, int, long))g_proc_0042A4A1)(a, b, c);  // original
}

PROC g_proc_0041F7E4;
int load_game(int a) {
  int original = ((int (*)(int))g_proc_0041F7E4)(a);
  memset(rune_owners, 0xff, 48);
  memset(visions, 0, MAX_VISIONS * sizeof(struct vision));
  return original;
}

void patch_vision(int range, int ttl, bool see_ally) {
  vision_range_sq = range * range;
  vision_ttl = ttl;
  vision_see_ally = see_ally;

  hook(0x004424F5, &g_proc_004424F5, (char*)spell_vision);
  char o1[] =
      "\x89\xD0\x83\xE8\x44\x50\xE8\x77\xFF\xFF\xFF\x58\x90\x90\x90\x90"
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90\x90\x90\x90\x90";
  PATCH_SET((char*)0x004540DE, o1);
  patch_call((char*)0x004540E4, (char*)apply_visions);

  char o2[] =
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90\x90\x90\x90\x90";
  PATCH_SET((char*)0x004425B2, o2);
}

PROC g_proc_0041856C;
int unit_damaged(int* u) {
  byte flags3 = *((byte*)((uintptr_t)u + S_FLAGS3));
  byte id = *((byte*)((uintptr_t)u + S_ID));
  int* target = (int*)*(int*)((uintptr_t)u + S_ORDER_UNIT_POINTER);
  if ((flags3 & (SF_UNIT_FREE | SF_DIEING | SF_DEAD | SF_HIDDEN)) == 0 &&
      (id == U_HTANKER || id == U_OTANKER) && !target) {
    bool has_oil = *((byte*)((uintptr_t)u + S_PEON_FLAGS)) & PEON_LOADED;
    unsigned short hp = *((unsigned short*)((uintptr_t)u + S_HP));
    bool has_damage = hp <= (*(WORD*)(UNIT_HP_TABLE + 2 * id)) * 3 / 4;
    byte chops = *((byte*)((uintptr_t)u + S_PEON_TREE_CHOPS));
    if (has_oil && has_damage && !chops) {
      *((byte*)((uintptr_t)u + S_PEON_TREE_CHOPS)) = 1;
      int x = (*((short*)((uintptr_t)u + S_X)) + 4) * 32;
      int y = (*((short*)((uintptr_t)u + S_Y)) - 4) * 32;
      int* demon = critter_create(x, y, U_DEMON, 0x0f);
      *((byte*)((uintptr_t)demon + S_PEON_TREE_CHOPS)) = 120;
      *(int*)((uintptr_t)demon + S_ORDER_UNIT_POINTER) = (int)u;
      *(unsigned short*)((uintptr_t)u + S_NEXT_FIRE) = 1000;
      ((void (*)(int*))0x410B30)(demon);
    }
  }
  return ((int (*)(int*))g_proc_0041856C)(u);
}

#define sub_40AF00(n, u, x, y) \
  ((void (*)(int, int*, int, int))0x40AF00)((n), (u), (x), (y))

int tanker_kaboom(int* u) {
  int x = *((short*)((uintptr_t)u + S_X));
  int y = *((short*)((uintptr_t)u + S_Y));
  sub_40AF00(0, u, x + 2, y + 2);
  sub_40AF00(1, u, x - 2, y + 2);
  sub_40AF00(2, u, x + 2, y - 2);
  sub_40AF00(3, u, x - 2, y - 2);
  sub_40AF00(4, u, x, y + 2);
  sub_40AF00(5, u, x, y - 2);
  sub_40AF00(6, u, x + 2, y);
  sub_40AF00(7, u, x - 2, y);
  unit_kill(u);
  ((void (*)(int, int))0x422F60)(x, y);
  return 1;
}

int burn_tankers() {
  int* p = (int*)UNITS_MASSIVE;  // pointer to units
  p = (int*)(*p);
  int k = *(int*)UNITS_NUMBER;
  while (k > 0) {
    byte flags3 = *((byte*)((uintptr_t)p + S_FLAGS3));
    byte id = *((byte*)((uintptr_t)p + S_ID));
    if ((flags3 & (SF_UNIT_FREE | SF_DIEING | SF_DEAD)) == 0) {
      if (id == U_HTANKER || id == U_OTANKER) {
        if (flags3 & SF_HIDDEN) {
          *((byte*)((uintptr_t)p + S_PEON_TREE_CHOPS)) = 0;
        } else {
          unsigned short hp = *((unsigned short*)((uintptr_t)p + S_HP));
          byte chops = *((byte*)((uintptr_t)p + S_PEON_TREE_CHOPS));
          if (chops) {
            if (++chops >= 10) {
              chops = 1;
              hp--;
            }
            if (hp <= 0) {
              *((byte*)((uintptr_t)p + S_PEON_TREE_CHOPS)) = 0;
              tanker_kaboom(p);
            } else {
              *((unsigned short*)((uintptr_t)p + S_HP)) = hp;
              *((byte*)((uintptr_t)p + S_PEON_TREE_CHOPS)) = chops;
            }
          }
        }
      } else if (id == U_DEMON) {
        byte chops = *((byte*)((uintptr_t)p + S_PEON_TREE_CHOPS));
        if (chops > 0) {
          *((byte*)((uintptr_t)p + S_PEON_TREE_CHOPS)) = --chops;
          if (!chops) unit_kill(p);
        }
      }
    }
    p = (int*)((int)p + 0x98);
    k--;
  }
  return 0;
}

void patch_offensive_tankers() {
  hook(0x0041856C, &g_proc_0041856C, (char*)unit_damaged);
}

struct unit_cost {
  bool changed;
  short gold;
  short wood;
  short oil;
};

struct unit_cost unit_cost_overrides[112];

void override_unit_cost(int u, int gold, int wood, int oil) {
  unit_cost_overrides[u].changed = true;
  unit_cost_overrides[u].gold = gold;
  unit_cost_overrides[u].wood = wood;
  unit_cost_overrides[u].oil = oil;
}

#define F_SHOW_PRICE 0x0044C730
#define F_NOT_ENOUGH 0x0044A650
#define RCOST_OIL ((int*)0x004AD410)
#define RCOST_GOLD ((int*)0x004AD414)
#define RCOST_TIME ((short*)0x004AD41C)
#define RCOST_LUMBER ((int*)0x004AD420)

int check_resources(int unit, unsigned char id) {
  int player = (int)*((byte*)((uintptr_t)unit + S_OWNER));

  int gold_cost, lumber_cost, oil_cost;
  if (id < 112 && unit_cost_overrides[id].changed) {
    gold_cost = unit_cost_overrides[id].gold;
    lumber_cost = unit_cost_overrides[id].wood;
    oil_cost = unit_cost_overrides[id].oil;
  } else {
    gold_cost = 10 * ((byte*)UNIT_GOLD_TABLE)[id];
    lumber_cost = 10 * ((byte*)UNIT_LUMBER_TABLE)[id];
    oil_cost = 10 * ((byte*)UNIT_OIL_TABLE)[id];
  }

  *RCOST_TIME = 2 * ((byte*)UNIT_TIME_TABLE)[id];
  *RCOST_GOLD = gold_cost;
  *RCOST_LUMBER = lumber_cost;
  *RCOST_OIL = oil_cost;

  if (id < 0x3A) {
    int food_cap = ((short*)FOOD_LIMIT)[player];
    if ((bool)*((byte*)NETWORK_MODE) && food_cap > 200) food_cap = 200;
    if (food_cap <= ((int*)ALL_UNITS)[player] - ((short*)NPC)[player])
      return ((int (*)(int))F_NOT_ENOUGH)(438);
  }
  if (((int*)GOLD)[player] < gold_cost)
    return ((int (*)(int))F_NOT_ENOUGH)(439);
  if (((int*)LUMBER)[player] < lumber_cost)
    return ((int (*)(int))F_NOT_ENOUGH)(440);
  if (((int*)OIL)[player] < oil_cost) return ((int (*)(int))F_NOT_ENOUGH)(441);
  return 0;
}

int show_unit_price(int id) {
  int gold, lumber, oil;
  if (id < 112 && unit_cost_overrides[id].changed) {
    gold = unit_cost_overrides[id].gold;
    lumber = unit_cost_overrides[id].wood;
    oil = unit_cost_overrides[id].oil;
  } else {
    gold = 10 * ((byte*)UNIT_GOLD_TABLE)[id];
    lumber = 10 * ((byte*)UNIT_LUMBER_TABLE)[id];
    oil = 10 * ((byte*)UNIT_OIL_TABLE)[id];
  }
  return ((int (*)(int, int, int))F_SHOW_PRICE)(gold, lumber, oil);
}

void patch_unit_prices() {
  memset(unit_cost_overrides, 0, sizeof(unit_cost_overrides));
  patch_ljmp((char*)0x0040DCD4, (char*)check_resources);
  char o1[] =
      "\x50\xE8\xBA\x18\x00\x00\x58\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90";
  PATCH_SET((char*)0x0044AE70, o1);
  patch_call((char*)0x0044AE71, (char*)show_unit_price);
}

PROC g_proc_0045271B;
void game_tick() {
  ((void (*)())g_proc_0045271B)();  // original
#ifdef PATCH_VISION
  expire_visions();
#endif
  burn_tankers();
}

extern void setup_dump(void);

extern "C" __declspec(dllexport) void w2p_init() {
  setup_dump();
  manacost(HEAL, 2);
  patch_heal_max_hp(255);
  unitscore(U_PEASANT, 1500);
  unitscore(U_PEON, 1500);
  unitscore(U_CRITTER, 0);
  repair_cat(true);
  patch_rune();
  patch_polymorph();
  patch_unholy();
  patch_kills_score();
  hook(0x0041F7E4, &g_proc_0041F7E4, (char*)load_game);
  hook(0x0042A4A1, &g_proc_0042A4A1, (char*)new_game);
#if 0
  patch_unit_exit_building();
#endif
  patch_bloodlust_sound();
  patch_max_units();

#ifdef PATCH_VISION
  manacost(VISION, 150);
  patch_vision(/*range*/ 4, /*ttl*/ 50, /*see allied invis*/ 0);
#endif

  patch_offensive_tankers();
  hook(0x0045271B, &g_proc_0045271B, (char*)game_tick);

  patch_unit_prices();
  override_unit_cost(U_PEASANT, 3500, 1500, 1000);
  override_unit_cost(U_PEON, 3500, 1500, 1000);
}
