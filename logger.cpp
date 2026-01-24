#include <stdint.h>
#include <stdio.h>
#include <windows.h>

#include "defs.h"   //some addresses and constants
#include "names.h"  //some constants
#include "patch.h"  //Fois patcher

static void hook(int adr, PROC *p, char *func) {
  *p = patch_call((char *)adr, func);
}

#define F_COMMAND_HANDLER 0x004558C0
#define F_CHATROOM_HANDLER 0x00415940
#define F_GET_TICK_COUNT 0x00460500

#define GetTickCount() (((int (*)(void))F_GET_TICK_COUNT)())

PROC g_proc_00436E72;
PROC g_proc_0045C173;

FILE *txt = NULL;
FILE *bin = NULL;

int log_chatroom(unsigned char *action) {
  int len = action[0];
  int player = action[1];
  int ctr = action[3];  // | (action[4] << 8) | (action[5] << 16);

  int cmd = action[6];

  const char *cmd_name = "UNKNOWN";
  int tick = GetTickCount();

  if (len < 8 || player < 0 || player > 8) goto out;
  switch (cmd) {
    case 0x00:
    case 0x01:
    case 0x06:
      goto out; /* TICK */
  }

  if (txt == NULL) {
    txt = fopen("log.txt", "a+");
  }

  if (bin == NULL) {
    bin = fopen("log.bin", "a+b");
  }

  fwrite(action, 1, len, bin);
  fflush(bin);

  fprintf(txt,
          "tick: %08X len: %3d player: %1d ctr: %02X cmd: %02x %-16s data: ",
          tick, len, player, ctr, cmd, cmd_name);

  for (int i = 8; i < len; i++) {
    fprintf(txt, "%02X ", action[i]);
  }

#if 0
  fprintf(txt, "\n#");
  for (int i = 0; i < len; i++) {
    fprintf(txt, "%02X ", action[i]);
  }
#endif

  fprintf(txt, "\n");
  fflush(txt);

out:
  return ((int (*)(unsigned char *))g_proc_0045C173)(action);
}

void log_action(unsigned char *action) {
  int len = action[0];
  int player = action[1];
  int ctr = action[3];  // | (action[4] << 8) | (action[5] << 16);

  int cmd = action[6];

  const char *cmd_name = "UNKNOWN";
  int tick = GetTickCount();

  switch (cmd) {
    case 0x01:
      goto out; /* TICK */
    case 0x10:
      cmd_name = "GAMEOVER";
      break;
    case 0x16:
      cmd_name = "SAVE GAME";
      break;
    case 0x17:
      cmd_name = "RETURN TO CHAT";
      break;
    case 0x18:
      cmd_name = "LOAD GAME";
      break;
    case 0x1b:
      cmd_name = "SELECT";
      break;
    case 0x1c:
      cmd_name = "CHOOSE TARGET";
      break;
    case 0x1d:
      cmd_name = "CANCEL TARGET";
      break;
    case 0x1e:
      cmd_name = "BUILD";
      break;
    case 0x1f:
      cmd_name = "ACTION";
      break;
    case 0x20:
      cmd_name = "CANCEL BUILD";
      break;
    case 0x21:
      cmd_name = "TRAIN/RESEARCH";
      break;
    case 0x22:
      cmd_name = "CANCEL TRAIN";
      break;
    case 0x23:
      cmd_name = "MESSAGE";
      break;
    case 0x24:
      cmd_name = "CHEAT";
      break;
    case 0x25:
      cmd_name = "UNLOAD TRANS";
      break;
    case 0x26:
      cmd_name = "ALLY";
      break;
    case 0x27:
      cmd_name = "GAME SPEED";
      break;
    case 0x28:
      cmd_name = "RIGHT CLICK";
      break;
    case 0x29:
      cmd_name = "PAUSE";
      break;
    case 0x2A:
      cmd_name = "RESUME";
      break;
  }

  if (txt == NULL) {
    txt = fopen("log.txt", "a+");
  }

  if (bin == NULL) {
    bin = fopen("log.bin", "a+b");
  }

  fwrite(action, 1, len, bin);
  fflush(bin);

  fprintf(txt,
          "tick: %08X len: %3d player: %1d ctr: %02X cmd: %02x %-16s data: ",
          tick, len, player, ctr, cmd, cmd_name);

  for (int i = 8; i < len; i++) {
    fprintf(txt, "%02X ", action[i]);
  }

#if 0
  fprintf(txt, "\n#");
  for (int i = 0; i < len; i++) {
    fprintf(txt, "%02X ", action[i]);
  }
#endif

  fprintf(txt, "\n");
  fflush(txt);

out:
  ((void (*)(unsigned char *))g_proc_00436E72)(action);
}

extern "C" __declspec(dllexport) void w2p_init() {
  hook(0x0045C264, &g_proc_0045C173, (char *)log_chatroom);
  hook(0x00436E72, &g_proc_00436E72, (char *)log_action);
}
