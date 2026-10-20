/* Windows x64 GUI; C99 integer widths, explicit ABI declarations, no SDK. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
typedef void *HANDLE;
typedef unsigned long DWORD;
typedef unsigned long long WPARAM;
typedef long long LPARAM;
typedef long long LRESULT;
typedef LRESULT (*WNDPROC)(HANDLE, unsigned int, WPARAM, LPARAM);

typedef struct {
    long left, top, right, bottom;
} RECT;

typedef struct {
    long x, y;
} POINT;

typedef struct {
    unsigned int size, style;
    WNDPROC proc;
    int cls_extra, wnd_extra;
    HANDLE instance, icon, cursor, background;
    const char *menu, *name;
    HANDLE small_icon;
} WNDCLASS;

typedef struct {
    HANDLE window;
    unsigned int message;
    WPARAM wparam;
    LPARAM lparam;
    DWORD time;
    POINT point;
    DWORD private_data;
} MSG;

typedef struct {
    HANDLE dc;
    int erase;
    RECT rect;
    int restore, update;
    unsigned char reserved[32];
} PAINT;

HANDLE GetModuleHandleA(const char *);
DWORD GetTickCount(void);
unsigned short RegisterClassExA(const WNDCLASS *);
HANDLE CreateWindowExA(DWORD, const char *, const char *, DWORD, int, int, int, int, HANDLE, HANDLE,
                       HANDLE, void *);
LRESULT DefWindowProcA(HANDLE, unsigned int, WPARAM, LPARAM);
int ShowWindow(HANDLE, int);
int UpdateWindow(HANDLE);
int GetMessageA(MSG *, HANDLE, unsigned int, unsigned int);
int TranslateMessage(const MSG *);
LRESULT DispatchMessageA(const MSG *);
void PostQuitMessage(int);
int DestroyWindow(HANDLE);
int UnregisterClassA(const char *, HANDLE);
HANDLE LoadCursorA(HANDLE, const char *);
int AdjustWindowRect(RECT *, DWORD, int);
int SetWindowPos(HANDLE, HANDLE, int, int, int, int, unsigned int);
int GetClientRect(HANDLE, RECT *);
HANDLE BeginPaint(HANDLE, PAINT *);
int EndPaint(HANDLE, const PAINT *);
int FillRect(HANDLE, const RECT *, HANDLE);
int FrameRect(HANDLE, const RECT *, HANDLE);
int InvalidateRect(HANDLE, const RECT *, int);
int SetWindowTextA(HANDLE, const char *);
int MessageBoxA(HANDLE, const char *, const char *, unsigned int);
LRESULT SendMessageA(HANDLE, unsigned int, WPARAM, LPARAM);
HANDLE CreateSolidBrush(DWORD);
int DeleteObject(HANDLE);
int SetBkMode(HANDLE, int);
DWORD SetTextColor(HANDLE, DWORD);
int TextOutA(HANDLE, int, int, const char *, int);
HANDLE CreateFontA(int, int, int, int, int, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD, DWORD,
                   const char *);
HANDLE SelectObject(HANDLE, HANDLE);
int DrawTextW(HANDLE, const unsigned short *, int, RECT *, unsigned int);

/* A self-contained, procedural-art World 1-1 platform game. No SDK/assets. */
HANDLE GetDC(HANDLE);
int ReleaseDC(HANDLE, HANDLE);
HANDLE CreateCompatibleDC(HANDLE);
HANDLE CreateCompatibleBitmap(HANDLE, int, int);
int DeleteDC(HANDLE);
int BitBlt(HANDLE, int, int, int, int, HANDLE, int, int, DWORD);
DWORD GetPixel(HANDLE, int, int);
int GetDIBits(HANDLE, HANDLE, unsigned int, unsigned int, void *, void *, unsigned int);
unsigned long long SetTimer(HANDLE, unsigned long long, unsigned int, void *);
int KillTimer(HANDLE, unsigned long long);
#define WIDTH 960
#define HEIGHT 544
#define TILE 32
#define COLS 216
#define ROWS 15
#define SCALE 16
#define PW 22
#define MAX_ENEMIES 32

/* tiles: 0 air, 1 ground, 2 brick, 3 coin block, 4 used, 5 pipe, 6 coin, 7 mushroom block, 8 stairs */
typedef struct {
    int x, y, dx, dy, alive, kind, grace, wake;
} ACTOR;

/* kind: 0 goomba, 1 walking turtle, 2 stationary shell, 3 sliding shell. */
typedef struct {
    int v_map[ROWS][COLS];
    ACTOR v_enemies[MAX_ENEMIES], v_mushroom;
    int v_keys[256];
    int v_enemy_count;
    int v_px;
    int v_py;
    int v_vx;
    int v_vy;
    int v_grounded;
    int v_big;
    int v_invincible;
    int v_score;
    int v_coins;
    int v_lives;
    int v_ticks;
    int v_state;
    int v_death_ticks;
    int v_camera;
    int v_paused;
    int v_checkpoint;
    int v_jump_was_down;
    int v_frame;
    int v_burst_x;
    int v_burst_y;
    int v_burst_ticks;
    int v_hit_boxes;
    int v_mushroom_pickups;
    int v_goomba_stomps;
    int v_turtle_stomps;
    int v_shell_kicks;
    int v_shell_kills;
    int v_loose_coins;
    int v_hurts;
} GAME;

static GAME game;
#define map game.v_map
#define enemies game.v_enemies
#define mushroom game.v_mushroom
#define keys game.v_keys
#define enemy_count game.v_enemy_count
#define px game.v_px
#define py game.v_py
#define vx game.v_vx
#define vy game.v_vy
#define grounded game.v_grounded
#define big game.v_big
#define invincible game.v_invincible
#define score game.v_score
#define coins game.v_coins
#define lives game.v_lives
#define ticks game.v_ticks
#define state game.v_state
#define death_ticks game.v_death_ticks
#define camera game.v_camera
#define paused game.v_paused
#define checkpoint game.v_checkpoint
#define jump_was_down game.v_jump_was_down
#define frame game.v_frame
#define burst_x game.v_burst_x
#define burst_y game.v_burst_y
#define burst_ticks game.v_burst_ticks
#define hit_boxes game.v_hit_boxes
#define mushroom_pickups game.v_mushroom_pickups
#define goomba_stomps game.v_goomba_stomps
#define turtle_stomps game.v_turtle_stomps
#define shell_kicks game.v_shell_kicks
#define shell_kills game.v_shell_kills
#define loose_coins game.v_loose_coins
#define hurts game.v_hurts
static int paint_count;
static HANDLE brushes[16], font, back_dc, back_bitmap, old_bitmap;
static int terrain_mode, terrain_steps, terrain_done, terrain_wait = 180;
static DWORD last_tick;
static unsigned long accumulator;
static const DWORD palette[16] = {0xE9A05C, 0xFFFFFF, 0x22252E, 0x273DDC, 0xCC6B24, 0x80BCF5,
                                  0x185594, 0x329BEF, 0x388F27, 0x59D74F, 0x30D9FF, 0xB0F4FF,
                                  0x705040, 0xBBDBE7, 0x746CA0, 0x244F20};

static int min_i(int a, int b) {
    return a < b ? a : b;
}

static int max_i(int a, int b) {
    return a > b ? a : b;
}

static int player_height(void) {
    return big ? 44 : 26;
}

static int tile_at(int x, int y) {
    if (x < 0)
        return 1;
    if (x >= COLS || y < 0 || y >= ROWS)
        return 0;
    return map[y][x];
}

static int solid(int t) {
    return t != 0 && t != 6;
}

static int overlap(int x, int y, int w, int h, int a, int b, int c, int d) {
    return x < a + c && x + w > a && y < b + d && y + h > b;
}

static int blocked(int x, int y, int w, int h) {
    int a, b, left, right, top, bottom;
    if (x < 0)
        return 1;
    left = x / TILE;
    right = (x + w - 1) / TILE;
    top = max_i(0, y / TILE);
    bottom = (y + h - 1) / TILE;
    for (b = top; b <= bottom && b < ROWS; b++)
        for (a = left; a <= right; a++)
            if (solid(tile_at(a, b)))
                return 1;
    return 0;
}

static void row(int a, int b, int y, int t) {
    int x;
    for (x = a; x <= b; x++)
        map[y][x] = t;
}

static void pipe_at(int x, int height) {
    int y;
    for (y = 13 - height; y < 13; y++) {
        map[y][x] = 5;
        map[y][x + 1] = 5;
    }
}

static void enemy_at(int x) {
    ACTOR *e;
    if (enemy_count >= MAX_ENEMIES)
        return;
    e = &enemies[enemy_count++];
    e->x = x * TILE * SCALE;
    e->y = (13 * TILE - 24) * SCALE;
    e->dx = -16;
    e->dy = 0;
    e->alive = 1;
}

static void make_level(void) {
    int x, y, h;
    memset(map, 0, sizeof(map));
    memset(enemies, 0, sizeof(enemies));
    memset(&mushroom, 0, sizeof(mushroom));
    enemy_count = 0;
    row(0, COLS - 1, 13, 1);
    row(0, COLS - 1, 14, 1);
    row(69, 70, 13, 0);
    row(69, 70, 14, 0);
    row(86, 88, 13, 0);
    row(86, 88, 14, 0);
    row(153, 154, 13, 0);
    row(153, 154, 14, 0);
    map[9][16] = 3;
    row(20, 24, 9, 2);
    map[9][21] = 7;
    map[9][23] = 3;
    map[5][22] = 3;
    pipe_at(28, 2);
    pipe_at(38, 3);
    pipe_at(46, 4);
    pipe_at(57, 4);
    row(77, 79, 9, 2);
    map[9][78] = 7;
    row(80, 87, 5, 2);
    row(91, 93, 5, 2);
    map[5][94] = 3;
    map[9][94] = 2;
    map[9][100] = 2;
    map[9][101] = 3;
    map[9][106] = 3;
    map[5][109] = 7;
    map[9][109] = 3;
    map[9][112] = 3;
    row(118, 120, 9, 2);
    row(121, 123, 5, 2);
    row(128, 131, 5, 2);
    map[5][129] = 3;
    map[5][130] = 3;
    for (x = 0; x < 4; x++)
        for (y = 12 - x; y < 13; y++) {
            map[y][134 + x] = 8;
            map[y][143 - x] = 8;
        }
    for (x = 0; x < 4; x++)
        for (y = 12 - x; y < 13; y++)
            map[y][148 + x] = 8;
    for (x = 0; x < 4; x++)
        for (y = 9 + x; y < 13; y++)
            map[y][155 + x] = 8;
    pipe_at(163, 2);
    row(168, 171, 9, 2);
    map[9][170] = 3;
    pipe_at(179, 2);
    for (x = 0; x < 8; x++) {
        h = x + 1;
        for (y = 13 - h; y < 13; y++)
            map[y][185 + x] = 8;
    }
    row(193, 193, 5, 8);
    for (y = 6; y < 13; y++)
        map[y][193] = 8;
    for (x = 60; x < 65; x++)
        map[8][x] = 6;
    for (x = 80; x < 86; x++)
        map[4][x] = 6;
    for (x = 114; x < 118; x++)
        map[8][x] = 6;
    enemy_at(22);
    enemy_at(40);
    enemy_at(51);
    enemy_at(53);
    enemy_at(65);
    enemy_at(78);
    enemy_at(82);
    enemy_at(96);
    enemy_at(98);
    enemy_at(107);
    enemy_at(115);
    enemy_at(117);
    enemy_at(125);
    enemy_at(127);
    enemy_at(169);
    enemy_at(172);
    enemy_at(176);
    enemies[1].kind = 1;
    enemies[9].kind = 1;
    enemies[14].kind = 1;
}

static void spawn_player(void) {
    px = (checkpoint ? 104 : 2) * TILE * SCALE;
    py = (13 * TILE - 26) * SCALE;
    vx = vy = 0;
    big = 0;
    grounded = 1;
    invincible = 0;
    camera = max_i(0, px / SCALE - 220);
    jump_was_down = 0;
    state = 0;
}

static void new_game(void) {
    memset(&game, 0, sizeof(game));
    make_level();
    score = coins = checkpoint = frame = 0;
    lives = 3;
    ticks = 400 * 60;
    paused = 0;
    memset(keys, 0, sizeof(keys));
    burst_ticks = 0;
    spawn_player();
}

static void collect_coin(void) {
    score += 200;
    coins++;
    if (coins == 100) {
        coins = 0;
        lives++;
    }
}

static void die(void) {
    if (state)
        return;
    state = 1;
    death_ticks = 75;
    vy = -120;
    vx = 0;
}

static void hurt(void) {
    if (invincible || state)
        return;
    hurts++;
    if (big) {
        big = 0;
        py += 18 * SCALE;
        invincible = 120;
    } else
        die();
}

static void hit_block(int x, int y) {
    int t = tile_at(x, y);
    if (x < 0 || x >= COLS || y < 0 || y >= ROWS)
        return;
    if (t == 3 || t == 7) {
        hit_boxes++;
        map[y][x] = 4;
        burst_x = x * TILE;
        burst_y = y * TILE;
        burst_ticks = 24;
        if (t == 7 && !big) {
            mushroom.x = x * TILE * SCALE;
            mushroom.y = (y * TILE - 26) * SCALE;
            mushroom.dx = 24;
            mushroom.dy = 0;
            mushroom.alive = 1;
        } else
            collect_coin();
    } else if (t == 2 && big) {
        map[y][x] = 0;
        score += 50;
        burst_x = x * TILE;
        burst_y = y * TILE;
        burst_ticks = 16;
    }
}

/* Pixel-sized substeps prevent tunnelling through thin terrain at high speed. */
static void move_player(void) {
    int next, dir, h = player_height(), old_y = py;
    next = px + vx;
    dir = vx > 0 ? 1 : -1;
    while (px != next) {
        int step = min_i(SCALE, abs(next - px)) * dir;
        if (blocked((px + step) / SCALE, py / SCALE, PW, h)) {
            vx = 0;
            break;
        }
        px += step;
    }
    next = py + vy;
    dir = vy > 0 ? 1 : -1;
    grounded = 0;
    while (py != next) {
        int step = min_i(SCALE, abs(next - py)) * dir;
        if (blocked(px / SCALE, (py + step) / SCALE, PW, h)) {
            if (dir > 0)
                grounded = 1;
            else {
                int ty = ((py + step) / SCALE) / TILE;
                hit_block((px / SCALE + 2) / TILE, ty);
                if ((px / SCALE + PW - 2) / TILE != (px / SCALE + 2) / TILE)
                    hit_block((px / SCALE + PW - 2) / TILE, ty);
            }
            vy = 0;
            break;
        }
        py += step;
    }
    if (py / SCALE > ROWS * TILE + 40)
        die();
    /* Collect tiles touched by either the small or grown player. */
    {
        int x, y;
        for (y = max_i(0, py / SCALE / TILE); y <= min_i(ROWS - 1, (py / SCALE + h - 1) / TILE);
             y++)
            for (x = max_i(0, px / SCALE / TILE);
                 x <= min_i(COLS - 1, (px / SCALE + PW - 1) / TILE); x++)
                if (map[y][x] == 6) {
                    map[y][x] = 0;
                    loose_coins++;
                    collect_coin();
                }
    }
    {
        int i;
        for (i = 0; i < enemy_count; i++) {
            ACTOR *e = &enemies[i];
            if (!e->alive)
                continue;
            if (overlap(px / SCALE, py / SCALE, PW, h, e->x / SCALE, e->y / SCALE, 26, 24)) {
                if (e->grace)
                    continue;
                if (py >= old_y && old_y / SCALE + h <= e->y / SCALE + 10) {
                    if (e->kind == 0) {
                        e->alive = 0;
                        goomba_stomps++;
                    } else if (e->kind == 1) {
                        e->kind = 2;
                        e->dx = 0;
                        e->wake = 600;
                        turtle_stomps++;
                    } else if (e->kind == 3) {
                        e->kind = 2;
                        e->dx = 0;
                        e->wake = 600;
                    } else {
                        e->kind = 3;
                        e->dx = px < e->x ? 112 : -112;
                        shell_kicks++;
                    }
                    e->grace = 10;
                    vy = -112;
                    score += 100;
                } else if (e->kind == 2) {
                    e->kind = 3;
                    e->dx = px < e->x ? 112 : -112;
                    e->grace = 12;
                    shell_kicks++;
                    score += 100;
                } else
                    hurt();
            }
        }
    }
    if (mushroom.alive &&
        overlap(px / SCALE, py / SCALE, PW, h, mushroom.x / SCALE, mushroom.y / SCALE, 26, 26)) {
        mushroom.alive = 0;
        mushroom_pickups++;
        score += 1000;
        if (!big && !blocked(px / SCALE, py / SCALE - 18, PW, 44)) {
            py -= 18 * SCALE;
            big = 1;
        }
        invincible = 60;
    }
}

static void move_actor(ACTOR *a, int height) {
    int next, dir;
    if (!a->alive)
        return;
    if (blocked((a->x + a->dx) / SCALE, a->y / SCALE, 26, height))
        a->dx = -a->dx;
    else
        a->x += a->dx;
    a->dy = min_i(144, a->dy + 7);
    next = a->y + a->dy;
    dir = a->dy > 0 ? 1 : -1;
    while (a->y != next) {
        int s = min_i(SCALE, abs(next - a->y)) * dir;
        if (blocked(a->x / SCALE, (a->y + s) / SCALE, 26, height)) {
            a->dy = 0;
            break;
        }
        a->y += s;
    }
    if (a->y > ROWS * TILE * SCALE)
        a->alive = 0;
}

static void tick(void) {
    int i, jump, run, speed;
    frame++;
    if (paused || state == 2 || state == 3)
        return;
    if (state == 1) {
        py += vy;
        vy += 7;
        if (--death_ticks == 0) {
            lives--;
            if (!lives)
                state = 3;
            else {
                make_level();
                spawn_player();
                ticks = 400 * 60;
            }
        }
        return;
    }
    if (--ticks <= 0) {
        die();
        return;
    }
    if (invincible)
        invincible--;
    if (burst_ticks)
        burst_ticks--;
    jump = keys[32] || keys[38] || keys['W'];
    run = keys[16];
    speed = run ? 72 : 48;
    if (keys[37] || keys['A'])
        vx = max_i(-speed, vx - 6);
    else if (keys[39] || keys['D'])
        vx = min_i(speed, vx + 6);
    else if (vx > 0)
        vx = max_i(0, vx - 7);
    else
        vx = min_i(0, vx + 7);
    if (jump && !jump_was_down && grounded) {
        vy = -184;
        grounded = 0;
    }
    if (!jump && vy < -70)
        vy = -70;
    jump_was_down = jump;
    vy = min_i(160, vy + 7);
    for (i = 0; i < enemy_count; i++)
        if (enemies[i].alive && abs(enemies[i].x / SCALE - px / SCALE) < WIDTH) {
            ACTOR *e = &enemies[i];
            if (e->grace)
                e->grace--;
            if (e->kind == 2 && --e->wake <= 0) {
                e->kind = 1;
                e->dx = -16;
            }
            move_actor(e, 24);
        }
    for (i = 0; i < enemy_count; i++)
        if (enemies[i].alive && enemies[i].kind == 3) {
            int j;
            for (j = 0; j < enemy_count; j++)
                if (j != i && enemies[j].alive &&
                    overlap(enemies[i].x / SCALE, enemies[i].y / SCALE, 26, 24,
                            enemies[j].x / SCALE, enemies[j].y / SCALE, 26, 24)) {
                    enemies[j].alive = 0;
                    shell_kills++;
                    score += 200;
                }
        }
    move_actor(&mushroom, 26);
    move_player();
    if (px / SCALE > 104 * TILE)
        checkpoint = 1;
    camera = min_i(COLS * TILE - WIDTH, max_i(0, px / SCALE - WIDTH / 3));
    if (!state && px / SCALE >= 198 * TILE) {
        state = 2;
        score += 5000 + ticks / 60 * 10;
    }
}

static void box(HANDLE dc, int x, int y, int w, int h, int color) {
    RECT r;
    r.left = x;
    r.top = y;
    r.right = x + w;
    r.bottom = y + h;
    FillRect(dc, &r, brushes[color]);
}

static void text_at(HANDLE dc, int x, int y, const char *s, int color) {
    SetTextColor(dc, palette[color]);
    TextOutA(dc, x, y, s, (int)strlen(s));
}

/* Original small pixel sprites, drawn from text tables at integer scale. */
static const char *hero[] = {"....RRRRR...", "...RRRRRRRR.", "...HHHSSKS..", "..HSHSSSKSS.",
                             "..HSHHSSSKSS", "..HHSSSSKKK.", "....SSSSSS..", "...RBRRBR...",
                             "..RRBRRBRRR.", ".RRRBBBBRRR.", ".SSBYBBYBSS.", ".SSBBBBBBSS.",
                             "...BBBBBB...", "...BB..BB...", "..HH....HH..", ".HHH....HHH."};
static const char *goomba[] = {"....HHHH....", "...HHHHHH...", "..HHHHHHHH..", ".HHKHHHHKHH.",
                               "HHHKKHHKKHHH", "HHHWSHH SWHH", "HHHHHHHHHHHH", "...SSSSSS...",
                               "..SSSSSSSS..", ".KKK....KKK.", "KKKK....KKKK", ".KK......KK."};

static int sprite_color(char c) {
    if (c == 'R')
        return 3;
    if (c == 'B')
        return 4;
    if (c == 'S')
        return 5;
    if (c == 'H')
        return 6;
    if (c == 'K')
        return 2;
    if (c == 'W')
        return 1;
    if (c == 'Y')
        return 10;
    return -1;
}

static void sprite(HANDLE dc, int x, int y, const char **p, int rows, int sy, int flip) {
    int a, b, c;
    for (b = 0; b < rows; b++)
        for (a = 0; p[b][a]; a++) {
            c = sprite_color(p[b][a]);
            if (c >= 0)
                box(dc, x + (flip ? 11 - a : a) * 2, y + b * sy, 2, sy, c);
        }
}

static void draw_scene(HANDLE dc) {
    int x, y, t, sx, sy, i, base;
    char buf[160];
    box(dc, 0, 0, WIDTH, HEIGHT, 0);
    /* Parallax clouds and stepped hills. */
    for (i = 0; i < 14; i++) {
        x = i * 210 - camera / 3;
        if (x < -160 || x > WIDTH + 160)
            continue;
        y = 110 + (i % 3) * 26;
        box(dc, x, y, 88, 22, 1);
        box(dc, x + 18, y - 14, 44, 20, 1);
        box(dc, x + 34, y - 25, 25, 20, 1);
    }
    for (i = 0; i < 30; i++) {
        x = i * 240 - camera / 2;
        for (y = 0; y < 4; y++)
            box(dc, x + y * 18, 480 - 18 * (y + 1), 150 - y * 36, 18, 8);
    }
    for (y = 0; y < ROWS; y++)
        for (x = max_i(0, camera / TILE); x < min_i(COLS, camera / TILE + 32); x++) {
            t = map[y][x];
            if (!t)
                continue;
            sx = x * TILE - camera;
            sy = y * TILE + 64;
            if (t == 6) {
                box(dc, sx + 11, sy + 6, 10, 20, 10);
                box(dc, sx + 15, sy + 9, 3, 14, 11);
                continue;
            }
            if (t == 5) {
                box(dc, sx, sy, 32, 32, 15);
                box(dc, sx + 3, sy, 26, 32, 8);
                box(dc, sx + 6, sy, 6, 32, 9);
                if (tile_at(x, y - 1) != 5) {
                    box(dc, sx - 2, sy, 36, 9, 15);
                    box(dc, sx, sy + 2, 32, 5, 9);
                }
                continue;
            }
            box(dc, sx, sy, 32, 32, 2);
            box(dc, sx + 1, sy + 1, 30, 30, t == 1 ? 6 : t == 4 ? 6 : t == 8 ? 7 : t == 2 ? 7 : 10);
            box(dc, sx + 2, sy + 2, 28, 2, t == 1 ? 7 : 11);
            if (t == 2 || t == 1) {
                box(dc, sx, sy + 16, 32, 2, 2);
                box(dc, sx + 16, sy, 2, 16, 2);
                box(dc, sx + 8, sy + 17, 2, 15, 2);
            }
            if (t == 3 || t == 7)
                text_at(dc, sx + 10, sy + 2, "?", 6);
            if (t == 4) {
                box(dc, sx + 5, sy + 5, 3, 3, 2);
                box(dc, sx + 24, sy + 24, 3, 3, 2);
            }
        }
    /* Pole and castle form the visible finish line. */
    sx = 198 * TILE - camera;
    box(dc, sx + 12, 128, 5, 352, 1);
    box(dc, sx + 7, 120, 15, 12, 10);
    sy = state == 2 ? 408 : 148;
    for (i = 0; i < 22; i++)
        box(dc, sx - 20 + i, sy + i, 32 - i, 1, 9);
    sx = 204 * TILE - camera;
    box(dc, sx, 384, 144, 96, 6);
    box(dc, sx + 24, 352, 96, 32, 7);
    for (i = 0; i < 5; i++)
        box(dc, sx + i * 30, 368, 22, 24, 7);
    box(dc, sx + 54, 428, 32, 52, 2);
    for (i = 0; i < enemy_count; i++)
        if (enemies[i].alive) {
            sx = enemies[i].x / SCALE - camera;
            sy = enemies[i].y / SCALE + 64;
            if (sx > -30 && sx < WIDTH) {
                if (enemies[i].kind == 0)
                    sprite(dc, sx, sy, goomba, 12, 2, (frame / 10) % 2);
                else {
                    box(dc, sx + 3, sy + 4, 20, 17, 15);
                    box(dc, sx + 5, sy + 5, 16, 13, 9);
                    box(dc, sx + 12, sy + 6, 2, 13, 8);
                    box(dc, sx + 5, sy + 11, 16, 2, 8);
                    box(dc, sx + 1, sy + 19, 24, 4, 1);
                    if (enemies[i].kind == 1) {
                        int head = enemies[i].dx < 0 ? sx - 4 : sx + 19;
                        box(dc, head, sy - 6, 10, 14, 10);
                        box(dc, head + 3, sy - 4, 3, 4, 2);
                        box(dc, sx + 2, sy + 23, 8, 3, 10);
                        box(dc, sx + 17, sy + 23, 8, 3, 10);
                    } else if (enemies[i].kind == 3)
                        box(dc, sx + ((frame / 3) % 2 ? 7 : 17), sy + 7, 3, 9, 11);
                }
            }
        }
    if (mushroom.alive) {
        sx = mushroom.x / SCALE - camera;
        sy = mushroom.y / SCALE + 64;
        box(dc, sx + 6, sy + 12, 15, 14, 5);
        box(dc, sx, sy + 4, 26, 12, 3);
        box(dc, sx + 6, sy, 14, 8, 3);
        box(dc, sx + 5, sy + 5, 6, 6, 1);
        box(dc, sx + 18, sy + 8, 5, 5, 1);
    }
    if (!invincible || (frame / 5) % 2) {
        sx = px / SCALE - camera;
        sy = py / SCALE + 64 - (big ? 4 : 6);
        sprite(dc, sx, sy, hero, 16, big ? 3 : 2, vx < 0);
        if (grounded && vx && (frame / 5) % 2)
            box(dc, sx + 3, sy + (big ? 44 : 26), 9, 4, 6);
    }
    if (burst_ticks) {
        sx = burst_x - camera;
        sy = burst_y + 40 - (24 - burst_ticks) * 2;
        box(dc, sx + 12, sy, 8, 16, 10);
    }
    box(dc, 0, 0, WIDTH, 64, 2);
    sprintf(buf, "MARIO   %06d     COINS %02d     WORLD 1-1     TIME %03d     LIVES %d", score,
            coins, ticks / 60, lives);
    text_at(dc, 18, 6, buf, 1);
    if (terrain_mode) {
        if (terrain_wait)
            text_at(dc, 18, 34, "GAMEPLAY DEMO: starts in 3 seconds / all enemies enabled", 10);
        else {
            sprintf(buf, "%s  BOX %d  MUSHROOM %d  GOOMBA %d  KOOPA %d  KICK %d  P:pause ESC:close",
                    terrain_done == 1   ? "PASS"
                    : terrain_done == 2 ? "FAIL"
                                        : "DEMO",
                    hit_boxes, mushroom_pickups, goomba_stomps, turtle_stomps, shell_kicks);
            text_at(dc, 18, 34, buf, 10);
        }
    } else
        text_at(dc, 18, 34,
                "Move: A/D or arrows   Jump: Space/W/Up   Run: Shift   Pause: P   Restart: R", 13);
    if (paused || state == 2 || state == 3) {
        base = paused ? 0 : state;
        box(dc, 185, 195, 590, 126, 2);
        text_at(dc, 260, 213,
                base == 2   ? "WORLD 1-1 COMPLETE!"
                : base == 3 ? "GAME OVER"
                            : "PAUSED",
                10);
        text_at(dc, 232, 258,
                terrain_mode
                    ? (paused ? "Press P to resume / Esc to close" : "Test finished / Esc to close")
                    : (paused ? "Press P to resume / R to restart"
                              : "Press R to play again / Esc to close"),
                1);
    }
}

static void terrain_input(void) {
    keys['D'] = keys[16] = 1;
    int ahead, need = 0;
    if (grounded) {
        for (ahead = 1; ahead <= 100; ahead++)
            if (blocked(px / SCALE + PW + ahead, py / SCALE + player_height() - 4, 1, 3))
                need = 1;
        if (!blocked(px / SCALE + PW + 56, py / SCALE + player_height() + 2, 1, 1))
            need = 1;
        if (jump_was_down)
            keys[32] = 0;
        else
            keys[32] = need;
    }
}

#include "src/examples/mario/demo.h"

static int turtle_tests(void) {
    int i;
    new_game();
    enemy_count = 1;
    enemies[0].kind = 1;
    enemies[0].x = px;
    enemies[0].y = py;
    py -= 24 * SCALE;
    vy = 100;
    grounded = 0;
    move_player();
    if (!enemies[0].alive || enemies[0].kind != 2 || enemies[0].dx || turtle_stomps != 1 ||
        vy >= 0 || state)
        return 90;
    /* Kick the stationary shell from the side, then verify it is dangerous. */
    px = enemies[0].x - 20 * SCALE;
    py = enemies[0].y;
    vx = vy = 0;
    enemies[0].grace = 0;
    move_player();
    if (enemies[0].kind != 3 || enemies[0].dx != 112 || shell_kicks != 1 || state)
        return 91;
    enemies[0].grace = 0;
    move_player();
    if (state != 1)
        return 92;
    /* Stomping a moving shell stops it and bounces the player. */
    new_game();
    enemy_count = 1;
    enemies[0].kind = 3;
    enemies[0].dx = 112;
    enemies[0].x = px;
    enemies[0].y = py;
    py -= 24 * SCALE;
    vy = 100;
    move_player();
    if (enemies[0].kind != 2 || enemies[0].dx || vy >= 0 || state)
        return 93;
    /* A shell reflects from terrain and strikes another live enemy. */
    new_game();
    enemy_count = 2;
    enemies[0].kind = 3;
    enemies[0].dx = 112;
    enemies[0].x = (28 * TILE - 26) * SCALE;
    move_actor(&enemies[0], 24);
    if (enemies[0].dx != -112)
        return 94;
    enemies[0].x = 300 * SCALE;
    enemies[0].dx = 112;
    enemies[1].x = 325 * SCALE;
    for (i = 0; i < 5; i++)
        tick();
    if (enemies[1].alive || shell_kills != 1 || state)
        return 95;
    new_game();
    enemy_count = 1;
    enemies[0].kind = 2;
    enemies[0].wake = 1;
    enemies[0].dx = 0;
    enemies[0].x = 300 * SCALE;
    tick();
    if (enemies[0].kind != 1 || enemies[0].dx != -16)
        return 96;
    return 0;
}

static int self_test(void) {
    int i, old, peak;
    new_game();
    if (sizeof(WNDCLASS) != 80 || sizeof(MSG) != 48 || sizeof(PAINT) != 72)
        return 1;
    for (i = 0; i < 120; i++)
        tick();
    if (!grounded || py / SCALE != 390 || state)
        return 2;
    keys[32] = 1;
    peak = py;
    for (i = 0; i < 60; i++) {
        tick();
        if (py < peak)
            peak = py;
    }
    if (peak > 270 * SCALE || !grounded)
        return 3;
    keys[32] = 0;
    tick();
    keys['D'] = 1;
    for (i = 0; i < 100; i++)
        tick();
    if (px <= 2 * TILE * SCALE || state)
        return 4;
    new_game();
    px = (28 * TILE - PW) * SCALE;
    keys['D'] = 1;
    for (i = 0; i < 30; i++)
        tick();
    if (px / SCALE > 28 * TILE - PW)
        return 5;
    new_game();
    px = 16 * TILE * SCALE;
    keys[32] = 1;
    for (i = 0; i < 40; i++)
        tick();
    if (map[9][16] != 4 || coins != 1)
        return 6;
    new_game();
    px = 21 * TILE * SCALE;
    keys[32] = 1;
    for (i = 0; i < 25; i++)
        tick();
    if (map[9][21] != 4 || !mushroom.alive)
        return 7;
    mushroom.x = px;
    mushroom.y = py;
    move_player();
    if (!big || mushroom.alive)
        return 8;
    invincible = 0;
    old = lives;
    hurt();
    if (big || !invincible || state || lives != old)
        return 9;
    invincible = 0;
    hurt();
    for (i = 0; i < 75; i++)
        tick();
    if (lives != 2 || state)
        return 10;
    new_game();
    px = 69 * TILE * SCALE;
    for (i = 0; i < 80 && !state; i++)
        tick();
    if (state != 1)
        return 11;
    new_game();
    enemy_count = 1;
    enemies[0].x = px;
    enemies[0].y = py;
    py -= 24 * SCALE;
    vy = 100;
    grounded = 0;
    move_player();
    if (enemies[0].alive || vy >= 0 || state || score != 100)
        return 12;
    new_game();
    enemy_count = 1;
    enemies[0].x = px;
    enemies[0].y = py;
    move_player();
    if (state != 1)
        return 13;
    new_game();
    px = 198 * TILE * SCALE;
    tick();
    if (state != 2 || score < 5000)
        return 14;
    old = px;
    for (i = 0; i < 60; i++)
        tick();
    if (px != old)
        return 15;
    new_game();
    paused = 1;
    old = ticks;
    keys['D'] = 1;
    for (i = 0; i < 120; i++)
        tick();
    if (ticks != old || px != 64 * SCALE)
        return 16;
    new_game();
    ticks = 1;
    tick();
    if (state != 1)
        return 17;
    new_game();
    coins = 99;
    collect_coin();
    if (coins || lives != 4)
        return 18;
    /* Traverse an unobstructed lane via input, then jump a real level gap. */
    new_game();
    enemy_count = 0;
    px = 66 * TILE * SCALE;
    keys['D'] = keys[16] = 1;
    for (i = 0; i < 13; i++)
        tick();
    keys[32] = 1;
    for (i = 0; i < 50; i++)
        tick();
    if (state || px / SCALE < 71 * TILE)
        return 19;
    /* Walk the entire terrain using only input and normal physics, with enemies
    isolated out (stomp/damage are tested above). This catches unreachable jumps. */
    new_game();
    enemy_count = 0;
    keys['D'] = keys[16] = 1;
    for (i = 0; i < 12000 && !state; i++) {
        terrain_input();
        tick();
    }
    if (state != 2)
        return 50 + px / SCALE / TILE / 10;
    return turtle_tests();
}

static LRESULT window_proc(HANDLE window, unsigned int msg, WPARAM w, LPARAM l) {
    PAINT p;
    HANDLE dc;
    DWORD now;
    int n;
    char title[180];
    if (msg == 0x14)
        return 1;
    if (msg == 0x100) {
        if (terrain_mode) {
            if (w == 27)
                DestroyWindow(window);
            if (w == 'P' && !(l & 0x40000000LL))
                paused = !paused;
            return 0;
        }
        if (w < 256)
            keys[(int)w] = 1;
        if (!(l & 0x40000000LL)) {
            if (w == 'R')
                new_game();
            if (w == 'P')
                paused = !paused;
            if (w == 27)
                DestroyWindow(window);
        }
        InvalidateRect(window, 0, 0);
        return 0;
    }
    if (msg == 0x101) {
        if (w < 256)
            keys[(int)w] = 0;
        return 0;
    }
    if (msg == 8) {
        if (terrain_mode)
            return 0;
        memset(keys, 0, sizeof(keys));
        if (!state)
            paused = 1;
        return 0;
    }
    if (msg == 0x113) {
        now = GetTickCount();
        accumulator += min_i((int)(now - last_tick), 250) * 60;
        last_tick = now;
        n = 0;
        while (accumulator >= 1000 && n < 15) {
            if (terrain_mode) {
                if (!paused && !terrain_done) {
                    if (terrain_wait)
                        terrain_wait--;
                    else {
                        demo_input();
                        tick();
                        terrain_steps++;
                        if (demo_pass()) {
                            terrain_done = 1;
                            sprintf(
                                title,
                                "PASS - coins %d, boxes %d, mushrooms %d, goombas %d, koopas %d, kicks %d",
                                coins, hit_boxes, mushroom_pickups, goomba_stomps, turtle_stomps,
                                shell_kicks);
                            SetWindowTextA(window, title);
                        } else if (state || terrain_steps >= 6000) {
                            terrain_done = 2;
                            state = 3;
                            SetWindowTextA(window, "FAIL - C1 Mario terrain traversal");
                        }
                    }
                }
            } else
                tick();
            accumulator -= 1000;
            n++;
        }
        InvalidateRect(window, 0, 0);
        return 0;
    }
    if (msg == 0x318) {
        draw_scene((HANDLE)w);
        paint_count++;
        return 0;
    }
    if (msg == 15) {
        dc = BeginPaint(window, &p);
        draw_scene(back_dc);
        BitBlt(dc, 0, 0, WIDTH, HEIGHT, back_dc, 0, 0, 0x00CC0020);
        EndPaint(window, &p);
        paint_count++;
        return 0;
    }
    if (msg == 2) {
        KillTimer(window, 1);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcA(window, msg, w, l);
}

/* Optional smoke-test screenshot; generated by the example itself. */
static void put32(unsigned char *p, unsigned int n) {
    p[0] = (unsigned char)n;
    p[1] = (unsigned char)(n >> 8);
    p[2] = (unsigned char)(n >> 16);
    p[3] = (unsigned char)(n >> 24);
}

static int save_preview(const char *path) {
    unsigned char header[14], info[40];
    static unsigned char pixels[WIDTH * HEIGHT * 4];
    FILE *f;
    int ok;
    memset(header, 0, sizeof(header));
    memset(info, 0, sizeof(info));
    header[0] = 'B';
    header[1] = 'M';
    put32(header + 2, 54 + sizeof(pixels));
    put32(header + 10, 54);
    put32(info, 40);
    put32(info + 4, WIDTH);
    put32(info + 8, HEIGHT);
    info[12] = 1;
    info[14] = 32;
    SelectObject(back_dc, old_bitmap);
    ok = GetDIBits(back_dc, back_bitmap, 0, HEIGHT, pixels, info, 0);
    SelectObject(back_dc, back_bitmap);
    if (ok != HEIGHT)
        return 0;
    f = fopen(path, "wb");
    if (!f)
        return 0;
    ok = fwrite(header, 1, 14, f) == 14 && fwrite(info, 1, 40, f) == 40 &&
         fwrite(pixels, 1, sizeof(pixels), f) == sizeof(pixels);
    if (fclose(f))
        ok = 0;
    return ok;
}

int main(int argc, char **argv) {
    WNDCLASS cls;
    MSG msg;
    RECT rect;
    HANDLE instance, window, dc, old_font;
    int i, smoke, result = 0;
    unsigned long long timer;
    if (argc > 1 && !strcmp(argv[1], "--self-test"))
        return self_test();
    if (argc > 1 && !strcmp(argv[1], "--demo-test"))
        return demo_test(argc > 2 ? argv[2] : 0);
    smoke = argc > 1 && !strcmp(argv[1], "--smoke-test");
    new_game();
    terrain_mode = argc > 1 && !strcmp(argv[1], "--terrain-test-visible");
    for (i = 0; i < 16; i++) {
        brushes[i] = CreateSolidBrush(palette[i]);
        if (!brushes[i])
            return 30;
    }
    font = CreateFontA(-21, 0, 0, 0, 700, 0, 0, 0, 0, 0, 0, 0, 49, "Consolas");
    if (!font)
        return 31;
    instance = GetModuleHandleA(0);
    memset(&cls, 0, sizeof(cls));
    cls.size = sizeof(cls);
    cls.proc = window_proc;
    cls.instance = instance;
    cls.cursor = LoadCursorA(0, (const char *)32512);
    cls.name = "C1MarioFirstLevel";
    if (!RegisterClassExA(&cls))
        return 32;
    rect.left = rect.top = 0;
    rect.right = WIDTH;
    rect.bottom = HEIGHT;
    if (!AdjustWindowRect(&rect, 0x00CA0000, 0))
        return 33;
    window = CreateWindowExA(0, cls.name, "C1 Mario - World 1-1", 0x00CA0000, (int)0x80000000U,
                             (int)0x80000000U, rect.right - rect.left, rect.bottom - rect.top, 0, 0,
                             instance, 0);
    if (!window)
        return 34;
    back_dc = CreateCompatibleDC(0);
    if (!back_dc)
        return 35;
    /* A screen DC is necessary: a memory DC initially contains a monochrome bitmap. */
    dc = GetDC(window);
    if (!dc)
        return 36;
    back_bitmap = CreateCompatibleBitmap(dc, WIDTH, HEIGHT);
    ReleaseDC(window, dc);
    if (!back_bitmap)
        return 37;
    old_bitmap = SelectObject(back_dc, back_bitmap);
    old_font = SelectObject(back_dc, font);
    SetBkMode(back_dc, 1);
    last_tick = GetTickCount();
    accumulator = 0;
    timer = SetTimer(window, 1, 16, 0);
    if (!timer)
        return 38;
    if (smoke) {
        SendMessageA(window, 0x100, 'D', 0);
        for (i = 0; i < 20; i++)
            tick();
        SendMessageA(window, 0x101, 'D', 0);
        if (px <= 64 * SCALE)
            result = 40;
        SendMessageA(window, 0x318, (WPARAM)back_dc, 0);
        if (!paint_count || GetPixel(back_dc, 0, 0) != palette[2] ||
            GetPixel(back_dc, 0, 65) != palette[0])
            result = 41;
        last_tick -= 100;
        {
            int before = ticks;
            SendMessageA(window, 0x113, 1, 0);
            if (ticks >= before)
                result = 46;
        }
        if (argc > 2 && !save_preview(argv[2]))
            result = 45;
        SendMessageA(window, 0x100, 'P', 0);
        if (!paused)
            result = 42;
        SendMessageA(window, 0x100, 'R', 0);
        if (paused || score || px != 64 * SCALE)
            result = 43;
        DestroyWindow(window);
    } else {
        if (terrain_mode)
            SetWindowTextA(window, "C1 Mario - full gameplay demo (all enemies enabled)");
        ShowWindow(window, 5);
        UpdateWindow(window);
    }
    while ((i = GetMessageA(&msg, 0, 0, 0)) > 0) {
        TranslateMessage(&msg);
        DispatchMessageA(&msg);
    }
    if (i < 0)
        result = 44;
    SelectObject(back_dc, old_font);
    SelectObject(back_dc, old_bitmap);
    DeleteObject(back_bitmap);
    DeleteDC(back_dc);
    DeleteObject(font);
    for (i = 0; i < 16; i++)
        DeleteObject(brushes[i]);
    UnregisterClassA(cls.name, instance);
    return result;
}
