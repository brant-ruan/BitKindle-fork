/* Demo controller. It chooses only input keys. Lookahead runs on copies of
   the ordinary game state; every displayed step uses the unmodified tick(). */
#define BEAM 14
#define LOOK_STEPS 8
#define LOOK_DEPTH 7
static GAME demo_root;

typedef struct {
    GAME world;
    int value, first;
} PLAN;

static PLAN frontier[BEAM], following[BEAM];
static int demo_action, demo_left, demo_goal = -1;

static int objective(void) {
    if (map[9][16] != 4)
        return 0;
    if (map[9][21] != 4)
        return 1;
    if (!mushroom_pickups)
        return 2;
    if (!goomba_stomps)
        return 3;
    if (!turtle_stomps)
        return 4;
    if (!shell_kicks)
        return 5;
    if (!loose_coins)
        return 6;
    return 7;
}

static int target_enemy(int kind) {
    int i, best = -1, d = 999999;
    for (i = 0; i < enemy_count; i++)
        if (enemies[i].alive && enemies[i].kind == kind &&
            abs(enemies[i].x / SCALE - px / SCALE) < d) {
            d = abs(enemies[i].x / SCALE - px / SCALE);
            best = i;
        }
    return best;
}

static int target_coin(void) {
    int x, y, best = -1, d = 999999;
    for (y = 0; y < ROWS; y++)
        for (x = 0; x < COLS; x++)
            if (map[y][x] == 6 && abs(x * TILE - px / SCALE) < d) {
                d = abs(x * TILE - px / SCALE);
                best = y * COLS + x;
            }
    return best;
}

static void action_keys(int action) {
    memset(keys, 0, sizeof(keys));
    keys[16] = 1;
    if (action % 3 == 0)
        keys['A'] = 1;
    if (action % 3 == 2)
        keys['D'] = 1;
    keys[32] = action >= 3;
}

static int value_state(int goal, int target) {
    int tx = 198 * TILE, ty = 390, done = 0, value;
    if (state && state != 2)
        return -2000000000;
    if (py / SCALE > 13 * TILE + 12)
        return -2000000000;
    if (goal < 2) {
        tx = (goal == 0 ? 16 : 21) * TILE + 5;
        ty = 320;
        done = map[9][goal == 0 ? 16 : 21] == 4;
    }
    if (goal == 2) {
        tx = mushroom.x / SCALE;
        ty = mushroom.y / SCALE;
        done = mushroom_pickups > 0;
    }
    if (goal >= 3 && goal <= 5) {
        if (target >= 0) {
            tx = enemies[target].x / SCALE;
            ty = enemies[target].y / SCALE - (goal == 5 ? 0 : player_height());
        }
        done = goal == 3 ? goomba_stomps : goal == 4 ? turtle_stomps : shell_kicks;
    }
    if (goal == 6) {
        if (target >= 0) {
            tx = target % COLS * TILE;
            ty = target / COLS * TILE;
        }
        done = loose_coins > 0;
    }
    value = -abs(px / SCALE - tx) * 16 - (goal == 7 ? 0 : abs(py / SCALE - ty) * 3);
    value += hit_boxes * 800 + mushroom_pickups * 2000 + goomba_stomps * 400 + turtle_stomps * 600 +
             shell_kicks * 200 + loose_coins * 200;
    value -= hurts * 12000;
    if (done)
        value += 100000;
    if (state == 2)
        value += 200000;
    return value;
}

static int same_bucket(const GAME *g) {
    return g->v_px / (SCALE * 6) == px / (SCALE * 6) && g->v_py / (SCALE * 6) == py / (SCALE * 6) &&
           g->v_vx / 16 == vx / 16 && g->v_vy / 24 == vy / 24 &&
           g->v_jump_was_down == jump_was_down && g->v_score == score && g->v_big == big;
}

static int plan_action(void) {
    int goal = objective(), target = -1, count = 1, next_count, depth, n, a, k, value, slot, worst,
        best, first;
    if (goal >= 3 && goal <= 5)
        target = target_enemy(goal == 3 ? 0 : goal == 4 ? 1 : 2);
    if (goal == 6)
        target = target_coin();
    memcpy(&demo_root, &game, sizeof(game));
    memcpy(&frontier[0].world, &game, sizeof(game));
    frontier[0].first = 2;
    frontier[0].value = 0;
    for (depth = 0; depth < LOOK_DEPTH; depth++) {
        next_count = 0;
        for (n = 0; n < count; n++)
            for (a = 0; a < 6; a++) {
                memcpy(&game, &frontier[n].world, sizeof(game));
                for (k = 0; k < LOOK_STEPS; k++) {
                    action_keys(a);
                    tick();
                    if (state)
                        break;
                }
                value = value_state(goal, target);
                if (value <= -2000000000)
                    continue;
                if (depth)
                    value += frontier[n].value;
                first = depth ? frontier[n].first : a;
                slot = -1;
                for (k = 0; k < next_count; k++)
                    if (same_bucket(&following[k].world)) {
                        slot = k;
                        break;
                    }
                if (slot >= 0) {
                    if (following[slot].value >= value)
                        continue;
                } else if (next_count < BEAM)
                    slot = next_count++;
                else {
                    worst = 0;
                    for (k = 1; k < next_count; k++)
                        if (following[k].value < following[worst].value)
                            worst = k;
                    if (value <= following[worst].value)
                        continue;
                    slot = worst;
                }
                memcpy(&following[slot].world, &game, sizeof(game));
                following[slot].value = value;
                following[slot].first = first;
            }
        if (!next_count)
            break;
        memcpy(frontier, following, next_count * sizeof(PLAN));
        count = next_count;
    }
    best = 0;
    for (n = 1; n < count; n++)
        if (frontier[n].value > frontier[best].value)
            best = n;
    a = frontier[best].first;
    memcpy(&game, &demo_root, sizeof(game));
    return a;
}

static void demo_input(void) {
    int goal = objective();
    if (!demo_left || goal != demo_goal) {
        demo_action = plan_action();
        demo_left = LOOK_STEPS;
        demo_goal = goal;
    }
    action_keys(demo_action);
    demo_left--;
}

static int demo_pass(void) {
    return state == 2 && hit_boxes >= 2 && coins > 0 && mushroom_pickups > 0 && goomba_stomps > 0 &&
           turtle_stomps > 0 && shell_kicks > 0 && loose_coins > 0;
}

static int demo_test(const char *path) {
    int i;
    FILE *f = path ? fopen(path, "w") : 0;
    new_game();
    demo_left = 0;
    demo_goal = -1;
    for (i = 0; i < 6000 && !state; i++) {
        demo_input();
        tick();
        if (f && i % 120 == 0) {
            fprintf(
                f,
                "tick=%d x=%d y=%d goal=%d coins=%d boxes=%d mushrooms=%d goombas=%d turtles=%d kicks=%d loose=%d hurts=%d\n",
                i, px / SCALE, py / SCALE, objective(), coins, hit_boxes, mushroom_pickups,
                goomba_stomps, turtle_stomps, shell_kicks, loose_coins, hurts);
            fflush(f);
        }
    }
    if (f) {
        fprintf(
            f,
            "%s state=%d ticks=%d x=%d coins=%d boxes=%d mushrooms=%d goombas=%d turtles=%d kicks=%d shellkills=%d loose=%d hurts=%d\n",
            demo_pass() ? "PASS" : "FAIL", state, i, px / SCALE, coins, hit_boxes, mushroom_pickups,
            goomba_stomps, turtle_stomps, shell_kicks, shell_kills, loose_coins, hurts);
        fclose(f);
    }
    return demo_pass() ? 0 : 80;
}
