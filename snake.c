#include "snake.h"
#include "glyphs.c"
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <stdbool.h>
#include <SDL2/SDL.h>

#if 0
//FULLSCREEN
#define WINDOW_X 0
#define WINDOW_Y 0
#define WINDOW_WIDTH 1920
#define WINDOW_HEIGHT 1080
#else
//STREAM
#define WINDOW_WIDTH 1770
#define WINDOW_HEIGHT 1405
#define WINDOW_X 10
#define WINDOW_Y -20
#endif
#define GRID_SIZE 28
#define GRID_DIM 1000
#define DELAY 30

enum {
    SNAKE_UP,
    SNAKE_DOWN,
    SNAKE_LEFT,
    SNAKE_RIGHT,
};

typedef struct {
    int x;
    int y;
    int score;
    int top_score;
} apple;
apple Apple;

struct snake {
    int x;
    int y;
    int dir;
    struct snake *next;
};
typedef struct snake Snake;
Snake *head;
Snake *tail;


#define NUM_STATES 1024
#define ALPHA      0.1f
#define GAMMA      0.99f
double qtable[512][3] = {0};
double epsilon = 0.3;

int episode = 0;
int prev_state = 0, prev_action = 0;
static int last_dist = 999;
static int diag_counter = 0;

void init_snake()
{
    Snake *new = malloc(sizeof(Snake));
    new->x = rand() % (GRID_SIZE / 2) + (GRID_SIZE / 4);
    new->y = rand() % (GRID_SIZE / 2) + (GRID_SIZE / 4);
    new->dir = SNAKE_UP;
    new->next = NULL;
   
    head = new;
    tail = new;
    return;
}

void increase_snake()
{
    Snake *new = malloc(sizeof(Snake));
   
    switch(tail->dir) {
        case SNAKE_UP:
            new->x = tail->x;
            new->y = tail->y +1;
            break;
        case SNAKE_DOWN:
            new->x = tail->x;
            new->y = tail->y - 1;
            break;
        case SNAKE_LEFT:
            new->x = tail->x + 1;
            new->y = tail->y;
            break;
        case SNAKE_RIGHT:
            new->x = tail->x - 1;
            new->y = tail->y;
            break;
    }
    new->dir = tail->dir;
    new->next = NULL;
    tail->next = new;
    tail = new;
    return;
}

void move_snake()
{
    int prev_x = head->x;
    int prev_y = head->y;
    int prev_dir = head->dir;
    switch(head->dir) {
        case SNAKE_UP:
            head->y--;
            break;
        case SNAKE_DOWN:
            head->y++;
            break;
        case SNAKE_LEFT:
            head->x--;
            break;
        case SNAKE_RIGHT:
            head->x++;
            break;
    }
    Snake *track = head;
    if(track->next != NULL) {
        track = track->next;
    }
    while(track != NULL) {
        int save_x = track->x;
        int save_y = track->y;
        int save_dir = track->dir;
        track->x = prev_x;
        track->y = prev_y;
        track->dir = prev_dir;
        track = track->next;
        prev_x = save_x;
        prev_y = save_y;
        prev_dir = save_dir;
    }
    return;
}

void reset_snake()
{
    Snake *track = head;
    Snake *temp;
    while(track != NULL) {
        temp = track;
        track = track->next;
        free(temp);
    }
    init_snake();
    increase_snake();
    increase_snake();
    increase_snake();
    if(Apple.score > Apple.top_score) {
        Apple.top_score = Apple.score;
    }
    Apple.score = 0;

	if (episode < 200) {
        // spawn apple 8 steps straight ahead (wraps safely)
        Apple.x = (head->x + 8 * ((head->dir == SNAKE_RIGHT) - (head->dir == SNAKE_LEFT) + GRID_SIZE)) % GRID_SIZE;
        Apple.y = (head->y + 8 * ((head->dir == SNAKE_DOWN)  - (head->dir == SNAKE_UP)    + GRID_SIZE)) % GRID_SIZE;
    }

    episode++;
    if(episode % 50 == 0)
        printf("Episode %d | Top score: %d\n", episode, Apple.top_score);
}

void render_snake(SDL_Renderer *renderer, int x, int y)
{
    int seg_size = GRID_DIM / GRID_SIZE;
    SDL_Rect seg;
    seg.w = seg_size - 2;
    seg.h = seg_size - 2;
    SDL_Rect seg_out;
    seg_out.w = seg_size;
    seg_out.h = seg_size;
    Snake *track = head;
    int bright = 255;
    int b_dir = 0;
    while(track != NULL) {
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, bright, 255);
        seg_out.x = x + track->x * seg_size;
        seg_out.y = y + track->y * seg_size;
        SDL_RenderFillRect(renderer, &seg_out);
        SDL_SetRenderDrawColor(renderer, 0x00, bright, 0x00, 255);
        seg.x = x + track->x * seg_size + 1;
        seg.y = y + track->y * seg_size + 1;
        SDL_RenderFillRect(renderer, &seg);
        track = track->next;
        if(b_dir == 0) {
            bright -= 5;
            if(bright < 150) b_dir = 1;
        }
        if(b_dir == 1) {
            bright += 5;
            if(bright > 250) b_dir = 0;
        }
    }
    return;
}

void flash_snake(SDL_Renderer *renderer, int x, int y)
{
    int seg_size = GRID_DIM / GRID_SIZE;
    SDL_Rect seg;
    seg.w = seg_size - 2;
    seg.h = seg_size - 2;
    SDL_Rect seg_out;
    seg_out.w = seg_size;
    seg_out.h = seg_size;
    Snake *track = head;
    int bright = 255;
    int b_dir = 0;
    int r = rand() % 255;
    int g = rand() % 255;
    int b = rand() % 255;
    while(track != NULL) {
        SDL_SetRenderDrawColor(renderer, 0x00, 0x00, bright, 255);
        seg_out.x = x + track->x * seg_size;
        seg_out.y = y + track->y * seg_size;
        SDL_RenderFillRect(renderer, &seg_out);
        SDL_SetRenderDrawColor(renderer, r, g, b, 255);
        seg.x = x + track->x * seg_size + 1;
        seg.y = y + track->y * seg_size + 1;
        SDL_RenderFillRect(renderer, &seg);
        track = track->next;
        if(b_dir == 0) {
            bright -= 5;
            if(bright < 150) b_dir = 1;
        }
        if(b_dir == 1) {
            bright += 5;
            if(bright > 250) b_dir = 0;
        }
    }
    return;
}

void render_grid(SDL_Renderer *renderer, int x, int y)
{
    SDL_SetRenderDrawColor(renderer, 0x55, 0x55, 0xff, 255);
    for(int i = 0; i < 255; i++) {
        SDL_SetRenderDrawColor(renderer, 0, 0, 255 - i, 255 - i);
        SDL_Rect outline;
        outline.x = x - i;
        outline.y = y - i;
        outline.w = GRID_DIM + i + i;
        outline.h = GRID_DIM + i + i;
        SDL_RenderDrawRect(renderer, &outline);
    }
    return;
}

void gen_apple()
{
    bool in_snake;
    do {
        in_snake = false;
        Apple.x = rand() % GRID_SIZE;
        Apple.y = rand() % GRID_SIZE;
        Snake *track = head;
        while(track != NULL) {
            if(track->x == Apple.x && track->y == Apple.y) {
                in_snake = true;
            }
            track = track->next;
        }
    }
    while(in_snake);
    Apple.score++;
    return;
}

void render_apple(SDL_Renderer *renderer, int x, int y)
{
    SDL_SetRenderDrawColor(renderer, 0xff, 0x00, 0x00, 255);
    int apple_size = GRID_DIM / GRID_SIZE;
    SDL_Rect app;
    app.w = apple_size;
    app.h = apple_size;
    app.x = x + Apple.x * apple_size;
    app.y = y + Apple.y * apple_size;
    SDL_RenderFillRect(renderer, &app);
}

void detect_apple()
{
    if(head->x == Apple.x && head->y == Apple.y) {
        gen_apple();
        increase_snake();
    }
    return;
}

void detect_crash()
{
    if(head->x < 0 || head->x >= GRID_SIZE || head->y < 0 || head->y >= GRID_SIZE) {
        reset_snake();
    }
    Snake *track = head;
    if(track->next != NULL) {
        track = track->next;
    }
    while(track != NULL) {
        if(track->x == head->x && track->y == head->y) {
            reset_snake();
        }
        track = track->next;
    }
    return;
}

void turn_left()
{
    switch(head->dir) {
        case SNAKE_UP:    head->dir = SNAKE_LEFT;  break;
        case SNAKE_DOWN:  head->dir = SNAKE_RIGHT; break;
        case SNAKE_LEFT:  head->dir = SNAKE_DOWN;  break;
        case SNAKE_RIGHT: head->dir = SNAKE_UP;    break;
    }
    return;
}

void turn_right()
{
    switch(head->dir) {
        case SNAKE_UP:    head->dir = SNAKE_RIGHT; break;
        case SNAKE_DOWN:  head->dir = SNAKE_LEFT;  break;
        case SNAKE_LEFT:  head->dir = SNAKE_UP;    break;
        case SNAKE_RIGHT: head->dir = SNAKE_DOWN;  break;
    }
    return;
}

void render_score(SDL_Renderer *renderer, int x, int y)
{
    SDL_SetRenderDrawColor(renderer, 0x00, 0xFF, 0x00, 255);
    int cell_size = 11;
    SDL_Rect cell;
    cell.w = cell_size - 2;
    cell.h = cell_size - 2;
    char buff[10];
    snprintf(buff, sizeof(buff), "%4d", Apple.score);
    for(int k = 0; k < 4; k++) {
        for(int i = 0; i < 9; i++) {
            for(int j = 0; j < 9; j++) {
                if(glyphs[buff[k]][j][i]) {
                    cell.x = x + cell_size * i + (cell_size * 9 * k);
                    cell.y = y + cell_size * j;
                    SDL_RenderFillRect(renderer, &cell);
                }
            }
        }
    }
    return;
}

void render_top_score(SDL_Renderer *renderer, int x, int y)
{
    SDL_SetRenderDrawColor(renderer, 0xFF, 0xFF, 0x00, 255);
    int cell_size = 11;
    SDL_Rect cell;
    cell.w = cell_size - 2;
    cell.h = cell_size - 2;
    char buff[10];
    snprintf(buff, sizeof(buff), "%4d", Apple.top_score);
    for(int k = 0; k < 4; k++) {
        for(int i = 0; i < 9; i++) {
            for(int j = 0; j < 9; j++) {
                if(glyphs[buff[k]][j][i]) {
                    cell.x = x + cell_size * i + (cell_size * 9 * k);
                    cell.y = y + cell_size * j;
                    SDL_RenderFillRect(renderer, &cell);
                }
            }
        }
    }
    return;
}

int get_state()
{
    int hx = head->x, hy = head->y, dir = head->dir;
    int ax = Apple.x, ay = Apple.y;

    int df = 0, dl = 0, dr = 0;

    // front
    int fx = hx, fy = hy;
    switch(dir) {
        case SNAKE_UP: fy--; break;
        case SNAKE_DOWN: fy++; break;
        case SNAKE_LEFT: fx--; break;
        case SNAKE_RIGHT: fx++; break;
    }
    if(fx<0||fx>=GRID_SIZE||fy<0||fy>=GRID_SIZE) df = 1;
    for(Snake *t=head->next; t; t=t->next) if(t->x==fx && t->y==fy) df = 1;

    // left
    int ld = (dir + 1) % 4;
    int lx = hx, ly = hy;
    switch(ld) {
        case SNAKE_UP: ly--; break;
        case SNAKE_DOWN: ly++; break;
        case SNAKE_LEFT: lx--; break;
        case SNAKE_RIGHT: lx++; break;
    }
    if(lx<0||lx>=GRID_SIZE||ly<0||ly>=GRID_SIZE) dl = 1;
    for(Snake *t=head->next; t; t=t->next) if(t->x==lx && t->y==ly) dl = 1;

    // right
    int rd = (dir + 3) % 4;
    int rx = hx, ry = hy;
    switch(rd) {
        case SNAKE_UP: ry--; break;
        case SNAKE_DOWN: ry++; break;
        case SNAKE_LEFT: rx--; break;
        case SNAKE_RIGHT: rx++; break;
    }
    if(rx<0||rx>=GRID_SIZE||ry<0||ry>=GRID_SIZE) dr = 1;
    for(Snake *t=head->next; t; t=t->next) if(t->x==rx && t->y==ry) dr = 1;

    // apple direction (relative)
    int dx = ax - hx, dy = ay - hy;
    int local_x, local_y;
    switch(dir) {
        case SNAKE_UP:    local_x = dx;  local_y = -dy; break;
        case SNAKE_DOWN:  local_x = -dx; local_y = dy;  break;
        case SNAKE_LEFT:  local_x = -dy; local_y = -dx; break;
        default:          local_x = dy;  local_y = dx;  break;
    }
    int apple_bin = (abs(local_x) >= abs(local_y)) ? (local_x > 0 ? 1 : 2) : (local_y > 0 ? 3 : 4);

    return (df<<0)|(dl<<1)|(dr<<2)|((apple_bin&7)<<3);
}

void ai()
{
    int state = get_state();

    int action;
    if ((double)rand() / RAND_MAX < epsilon)
        action = rand() % 3;
    else {
        action = 0;
        for (int a = 1; a < 3; a++)
            if (qtable[state][a] > qtable[state][action])
                action = a;
    }

    if (action == 1) turn_left();
    if (action == 2) turn_right();

    prev_state = state;
    prev_action = action;
}


void update_q_and_diag(bool ate, bool died)
{
    int dist = abs(head->x - Apple.x) + abs(head->y - Apple.y);
    double reward = -0.1;

    if (died) reward = -100.0;
    else if (ate) reward = +3000.0;          // <<< THIS IS THE KEY

    else {
        if (dist < last_dist - 2) reward += 20.0;
        else if (dist < last_dist) reward += 5.0;
        else if (dist > last_dist + 2) reward -= 10.0;
        else if (dist > last_dist) reward -= 2.0;
    }
    if (ate) last_dist = 999;
    else last_dist = dist;

    int state = get_state();
    double max_next = died ? 0.0 : qtable[state][0];
    for (int a = 1; a < 3; a++)
        if (qtable[state][a] > max_next) max_next = qtable[state][a];

    qtable[prev_state][prev_action] += ALPHA * (reward + GAMMA * max_next - qtable[prev_state][prev_action]);

    //Diagnostics every 30 steps
    if (++diag_counter % 30 == 0) {
        const char *acts[] = {"FORWARD", "LEFT   ", "RIGHT  "};
        printf("[Step %4d | Ep %3d] State=%3d D=[%d%d%d] AppleDir=%d Q=[%6.1f %6.1f %6.1f] Act=%s Reward=%+6.1f %s%s Score=%d\n",
               diag_counter, episode, prev_state,
               (prev_state>>0)&1, (prev_state>>1)&1, (prev_state>>2)&1, (prev_state>>3)&7,
               qtable[prev_state][0], qtable[prev_state][1], qtable[prev_state][2],
               acts[prev_action], reward, ate?"ATE":"   ", died?"DIED":"    ", Apple.score);
    }
}


int main()
{
    srand(time(0));
    init_snake();
    increase_snake();
    increase_snake();
    increase_snake();
    gen_apple();
    Apple.score = 0;

    SDL_Window *window;
    SDL_Renderer *renderer;
    if(SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "ERROR: SDL_INIT_VIDEO");
        return 1;
    }
    window = SDL_CreateWindow("Snake", WINDOW_X, WINDOW_Y, WINDOW_WIDTH, WINDOW_HEIGHT, SDL_WINDOW_BORDERLESS);
    if(!window) { fprintf(stderr, "ERROR: !window"); return 1; }
    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if(!renderer) { fprintf(stderr, "!renderer"); return 1; }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    int grid_x = (WINDOW_WIDTH / 2) - (GRID_DIM / 2);
    int grid_y = (WINDOW_HEIGHT / 2) - (GRID_DIM / 2);
    int flash = 0;
    bool quit = false;
    SDL_Event event;

    while(!quit) {
        while(SDL_PollEvent(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    if(event.key.keysym.sym == SDLK_ESCAPE) quit = true;
                    break;
            }
        }

        SDL_RenderClear(renderer);

        ai();
        move_snake();
        detect_apple();
        detect_crash();

        bool ate = (head->x == Apple.x && head->y == Apple.y);
        bool died = (head->x < 0 || head->x >= GRID_SIZE || head->y < 0 || head->y >= GRID_SIZE);
        if(!died) {
            Snake *t = head->next;
            while(t) {
                if(t->x == head->x && t->y == head->y) { died = true; break; }
                t = t->next;
            }
        }

        update_q_and_diag(ate, died);

        render_grid(renderer, grid_x, grid_y);
        render_snake(renderer, grid_x, grid_y);
        if(Apple.score % 10 == 0 && Apple.score != 0) flash = 10;
        if(flash > 0) { flash_snake(renderer, grid_x, grid_y); flash--; }
        render_apple(renderer, grid_x, grid_y);
        render_score(renderer, WINDOW_WIDTH / 2 - 100, 50);
        render_top_score(renderer, WINDOW_WIDTH / 2 + 200, 50);

        SDL_SetRenderDrawColor(renderer, 0x11, 0x11, 0x11, 255);
        SDL_RenderPresent(renderer);
        SDL_Delay(DELAY);
    }

    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
