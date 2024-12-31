#include <stdint.h>
#include <stdio.h>
#include <windows.h>

#include "defs.h"   //some addresses and constants
#include "names.h"  //some constants
#include "patch.h"  //Fois patcher

void hook(int adr, PROC *p, char *func) { *p = patch_call((char *)adr, func); }

byte rune_owners[48];

void bullet_create(WORD x, WORD y, byte id) {
  ((int *(*)(WORD, WORD, byte))F_BULLET_CREATE)(x, y, id);
}

void unit_kill(int *u) {
  ((void (*)(int *))F_UNIT_KILL)(u);  // original war2 func kills unit
}

#define F_SET_KILLS_SCORE 0x0043D760
void set_kills_score(byte player, int *u) {
  ((void (*)(byte, int *))F_SET_KILLS_SCORE)(player, u);
}

void rune_bullet(int runeid, WORD x, WORD y, byte id, int *u) {
  short *tt = (short *)RUNEMAP_TIMERS;
  byte *xt = (byte *)RUNEMAP_X;
  byte *yt = (byte *)RUNEMAP_Y;

  byte xx = (x - 0x10) >> 5;
  byte yy = (y - 0x10) >> 5;

  tt[runeid] = 0x800;
  xt[runeid] = xx;
  yt[runeid] = yy;

  byte player_id = *((byte *)((uintptr_t)u + S_OWNER));
  rune_owners[runeid] = player_id;
  bullet_create(x, y, id);
}

void rune_unit_hit(int *u, int runeptr) {
  int rune_id = (runeptr - RUNEMAP_TIMERS) / 2;
  unsigned short hp = *((unsigned short *)((uintptr_t)u + S_HP));
  byte rune_owner = rune_owners[rune_id];
  rune_owners[rune_id] = 0xff;
  if (hp < 50) {
    unit_kill(u);
    if (rune_owner != 0xff) set_kills_score(rune_owner, u);
  } else {
    *((unsigned short *)((uintptr_t)u + S_HP)) -= 50;
  }
}

void patch_rune() {
  char o1[] = "\x56\x8B\x74\x24\x10\x56";
  char o2[] =
      "\x50\x90\x90\x90\x90\x90\x83\xC4\x08\x5E\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90";
  PATCH_SET((char *)0x0044359A, o1);
  PATCH_SET((char *)0x004435B2, o2);
  patch_call((char *)0x004435B3, (char *)rune_bullet);

  char o3[] = "\x56\x8B\x74\x24\x10\x56";
  char o4[] =
      "\x50\x90\x90\x90\x90\x90\x83\xC4\x08\x5E\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90";
  PATCH_SET((char *)0x00443670, o3);
  PATCH_SET((char *)0x00443686, o4);
  patch_call((char *)0x00443687, (char *)rune_bullet);

  char o5[] = "\x52\x8B\x54\x24\x10\x52\x90\x90\x90\x90\x90\x90";
  char o6[] = "\x56\x90\x90\x90\x90\x90\x83\xC4\x08\x5A\x90\x90\x90\x90\x90";
  PATCH_SET((char *)0x00443743, o5);
  PATCH_SET((char *)0x0044375F, o6);
  patch_call((char *)0x00443760, (char *)rune_bullet);

  char o7[] = "\x56\x8B\x74\x24\x10\x56";
  char o8[] =
      "\x50\x90\x90\x90\x90\x90\x83\xC4\x08\x5E\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90";
  PATCH_SET((char *)0x004437E8, o7);
  PATCH_SET((char *)0x004437FE, o8);
  patch_call((char *)0x004437FF, (char *)rune_bullet);

  char o9[] =
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x55\x53\x90\x90\x90\x90\x90\x83\xC4"
      "\x08";
  PATCH_SET((char *)0x0044244C, o9);
  patch_call((char *)0x00442457, (char *)rune_unit_hit);
}

#define F_CRITTER_CREATE 0x00451B50  // x y critter flag
void critter_create(WORD x, WORD y, byte id, byte f) {
  ((int *(*)(WORD, WORD, byte, byte))F_CRITTER_CREATE)(x, y, id, f);
}

void polymorph_unit_kill(int *u, int *caster) {
  byte id = *((byte *)((uintptr_t)u + S_ID));
  short x, y;
  int *target;
  if (id == U_PEASANT || id == U_PEON)
    target = caster;
  else
    target = u;

  *((byte *)((uintptr_t)target + S_FLAGS3)) |= SF_HIDDEN;
  *((byte *)((uintptr_t)target + S_SEQ_FLAG)) |= 0x20;
  unit_kill(target);
  x = *((short *)((uintptr_t)target + S_X));
  y = *((short *)((uintptr_t)target + S_Y));
  x <<= 5;
  y <<= 5;
  critter_create(x, y, U_CRITTER, 0x0f);
  bullet_create(x + 16, y + 16, 0x16);
  set_kills_score(*((byte *)((uintptr_t)caster + S_OWNER)), target);
  return;
}

void patch_polymorph() {
  char o1[] =
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x57\x56";
  PATCH_SET((char *)0x00442B9C, o1);
  patch_call((char *)0x00442BB2, (char *)polymorph_unit_kill);
  char o2[] =
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90"
      "\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x83\xc4\x18";
  PATCH_SET((char *)0x00442BB7, o2);
}

void unholy_hp_shield(int *target, int *caster) {
  byte id = *((byte *)((uintptr_t)target + S_ID));
  if (id == U_PEASANT || id == U_PEON) {
    unsigned short hp = *((unsigned short *)((uintptr_t)caster + S_HP));
    if (hp >= 2) *((unsigned short *)((uintptr_t)caster + S_HP)) = hp >> 1;
  } else {
    unsigned short hp = *((unsigned short *)((uintptr_t)target + S_HP));
    if (hp >= 2) *((unsigned short *)((uintptr_t)target + S_HP)) = hp >> 1;
    *((unsigned short *)((uintptr_t)target + S_SHIELD)) = 0x1f4;
  }
}

void patch_unholy() {
  char o1[] = "\x57\x56";
  patch_call((char *)0x00443452, (char *)unholy_hp_shield);
  PATCH_SET((char *)0x00443450, o1);  // push edi; push esi
  char o2[] =
      "\x83\xc4\x08\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90\x90";
  PATCH_SET((char *)0x00443457, o2);  // add esp, 8; nop...
}

#define KILL_COUNT 0x004AD328
int inc_kills(int player, int *u) {
  player &= 0xff;
  byte id = *((byte *)((uintptr_t)u + S_ID));
  if (id != U_CRITTER) {
    ((short *)KILL_COUNT)[player]++;
  }
  return player;
}

void patch_critter_kills() {
  char o1[] = "\x51\x50\x90\x90\x90\x90\x90\x66\x83\xc4\x08\x90\x90";
  PATCH_SET((char *)0x0043D773, o1);
  patch_call((char *)0x0043D775, (char *)inc_kills);
}

typedef int(__cdecl *place_callback)(int *);
#define PLACE_UNIT_POSITION 0x00443A40
extern int new_exit_pos(int *, int, int, int, place_callback);

int place_unit_position(int *pos, int size, int target, int steps,
                        place_callback callback) {
  if (*pos == target) {
    return ((int (*)(int *, int, int, int, place_callback))PLACE_UNIT_POSITION)(
        pos, size, target, steps, callback);
  }
  return new_exit_pos(pos, size, target, steps, callback);
}

// 00451223 00451951 00451A09 00451AC9 00452A2E

void patch_unit_exit_building() {
  patch_call((char *)0x00451223, (char *)place_unit_position);
  patch_call((char *)0x00451951, (char *)place_unit_position);
  patch_call((char *)0x00451A09, (char *)place_unit_position);
  patch_call((char *)0x00451AC9, (char *)place_unit_position);
  patch_call((char *)0x00452A2E, (char *)place_unit_position);
}

void manacost(byte id, byte c) {
  // spells cost of mana
  char mana[] = "\x1";
  mana[0] = c;
  PATCH_SET((char *)(MANACOST + 2 * id), mana);
}

void repair_cat(bool b) {
  // peon can repair unit if it have transport flag OR catapult flag
  if (b) {
    char r1[] = "\xeb\x75\x90\x90\x90";  // f6 c4 04 74 14
    PATCH_SET((char *)REPAIR_FLAG_CHECK2, r1);
    char r2[] = "\x66\xa9\x04\x04\x74\x9c\xeb\x86";
    PATCH_SET((char *)REPAIR_CODE_CAVE, r2);
  } else {
    char r1[] = "\xf6\xc4\x04\x74\x14";
    PATCH_SET((char *)REPAIR_FLAG_CHECK2, r1);
  }
}

#define UNIT_SCORE_TABLE 0x004CF190
void unitscore(byte id, short score) {
  // spells cost of mana
  char s[] = "RR";
  short *ss = (short *)s;
  ss[0] = score;
  PATCH_SET((char *)(UNIT_SCORE_TABLE + 2 * id), s);
}

PROC g_proc_0041F7E4;
int load_game(int a) {
  int original = ((int (*)(int))g_proc_0041F7E4)(a);
  memset(rune_owners, 0xff, 48);
  return original;
}

void *def_name = NULL;
void *def_sound = NULL;
char ajmoo_name[] = "PEON2K\\ajmoo.wav\x0";
void *ajmoo_sound = NULL;

PROC g_proc_00442D74;
void play_sound_bloodlust(int *u, int sound) {
  byte id = *((byte *)((uintptr_t)u + S_ID));
  if (id == U_PALADIN || id == U_KNIGHT) {
    int sid = sound + 68;
    def_name = (void *)*(int *)(SOUNDS_FILES_LIST + 8 + 24 * sid);
    def_sound = (void *)*(int *)(SOUNDS_FILES_LIST + 16 + 24 * sid);
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 8 + 24 * sid),
                   (DWORD)ajmoo_name);
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 16 + 24 * sid),
                   (DWORD)ajmoo_sound);
    ((void (*)(int *, int))g_proc_00442D74)(u, sound);
    ajmoo_sound = (void *)*(int *)(SOUNDS_FILES_LIST + 16 + 24 * sid);
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 8 + 24 * sid),
                   (DWORD)def_name);
    patch_setdword((DWORD *)(SOUNDS_FILES_LIST + 16 + 24 * sid),
                   (DWORD)def_sound);
  } else {
    ((void (*)(int *, int))g_proc_00442D74)(u, sound);
  }
}

#include "gen/ajmoo.h"

void patch_bloodlust_sound() {
  FILE *fp;
  DWORD dwAttrib = GetFileAttributes("PEON2K");
  if (dwAttrib == INVALID_FILE_ATTRIBUTES) {
    CreateDirectory("PEON2K", NULL);
    if ((fp = fopen("PEON2K\\ajmoo.wav", "w+b")) != NULL) {
      fwrite(ajmoo_wav, 1, ajmoo_wav_len, fp);
      fclose(fp);
    }
  }
  hook(0x00442D74, &g_proc_00442D74, (char *)play_sound_bloodlust);
}

extern void setup_dump(void);

extern "C" __declspec(dllexport) void w2p_init() {
  setup_dump();
  manacost(HEAL, 2);
  unitscore(U_PEASANT, 300);
  unitscore(U_PEON, 300);
  unitscore(U_CRITTER, 0);
  repair_cat(true);
  patch_rune();
  patch_polymorph();
  patch_unholy();
  patch_critter_kills();
  hook(0x0041F7E4, &g_proc_0041F7E4, (char *)load_game);
  patch_unit_exit_building();
  patch_bloodlust_sound();
}
