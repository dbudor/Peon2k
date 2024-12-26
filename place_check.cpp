//
// Created by Darko Budor on 24.12.2024..
//

typedef int (__cdecl *place_callback)(int *); 

typedef union xpos {
  struct {
    short x;
    short y;
  } c;
  int xy;
} position;

#define S 0
#define E 1
#define N 2
#define W 3

#define MAP_SIZE 128

#define ADVANCE(_pos, _dir, _spin)         \
  switch (_dir) {                          \
    case S:                                \
      if (++((_pos).c.y) >= bound_s) {     \
        _dir = (_dir + _spin + 0x100) % 4; \
      }                                    \
      break;                               \
    case E:                                \
      if (++((_pos).c.x) >= bound_e) {     \
        _dir = (_dir + _spin + 0x100) % 4; \
      }                                    \
      break;                               \
    case N:                                \
      if (--((_pos).c.y) <= bound_n) {     \
        _dir = (_dir + _spin + 0x100) % 4; \
      }                                    \
      break;                               \
    case W:                                \
      if (--((_pos).c.x) <= bound_w) {     \
        _dir = (_dir + _spin + 0x100) % 4; \
      }                                    \
  }

#define BOUND_CHECK(_pos)                                         \
  ((_pos).c.x >= 0 && (_pos).c.y >= 0 && (_pos).c.x < MAP_SIZE && \
   (_pos).c.y < MAP_SIZE)

int x;


int new_exit_pos(int *ipos, int isize, int itarget, int steps,
                 place_callback callback) {
  position *pos = (position *)ipos;
  position size, target;
  size.xy = isize;
  target.xy = itarget;

  int center2_x = pos->c.x * 2 + size.c.x;
  int center2_y = pos->c.y * 2 + size.c.y;

  int delta2_x = target.c.x * 2 - center2_x;
  int delta2_y = target.c.y * 2 - center2_y;

  int absdx = delta2_x;
  if (absdx < 0) absdx = -absdx;
  int absdy = delta2_y;
  if (absdy < 0) absdy = -absdy;

  int bound_w = pos->c.x;
  int bound_e = pos->c.x + size.c.x - 1;
  int bound_n = pos->c.y;
  int bound_s = pos->c.y + size.c.y - 1;

  int dir;
  if (absdx > absdy) {
    pos->c.y = (center2_y + size.c.x * delta2_y / delta2_x) / 2;
    if (delta2_x <= 0) {
      dir = W;
      pos->c.x = bound_w;
    } else {
      dir = E;
      pos->c.x = bound_e;
    }
  } else {
    pos->c.x = (center2_x + size.c.y * delta2_x / delta2_y) / 2;
    if (delta2_y <= 0) {
      dir = N;
      pos->c.y = bound_n;
    } else {
      dir = S;
      pos->c.y = bound_s;
    }
  }

  position ccw;
  position cw;
  int dir_ccw;
  int dir_cw;
  while (steps-- > 0) {
    bound_w--;
    bound_e++;
    bound_n--;
    bound_s++;
    ADVANCE(*pos, dir, 0);
    if (BOUND_CHECK(*pos) && callback(&pos->xy)) {
      return 1;
    }
    ccw.xy = pos->xy;
    dir_ccw = (dir + 1) % 4;
    cw.xy = pos->xy;
    dir_cw = (dir - 1 + 0x100) % 4;
    for (;;) {
      ADVANCE(ccw, dir_ccw, 1);
      if (ccw.xy == cw.xy) break;
      if (BOUND_CHECK(ccw) && callback(&ccw.xy)) {
        pos->xy = ccw.xy;
        return 1;
      }
      ADVANCE(cw, dir_cw, -1);
      if (ccw.xy == cw.xy) break;
      if (BOUND_CHECK(cw) && callback(&cw.xy)) {
        pos->xy = cw.xy;
        return 1;
      }
    }
  }
  return 0;
}
