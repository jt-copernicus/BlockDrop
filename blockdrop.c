/*
 * BlockDrop - A color-matching falling blocks puzzle game
 *
 * by Jonathan Torres
 *
 * This program is free software: you can redistribute it and/or modify it under the terms of the 
 * GNU General Public License as published by the Free Software Foundation, either version 3 of 
 * the License, or (at your option) any later version.
 */

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <ncurses.h>
#include <signal.h>
#include <sys/stat.h>

#define GRID_W 12
#define GRID_H 18
#define NUM_COLORS 5

//from here on out, leaving out some troublesome colors -- using unique names to avoid conflict

/* color IDs */
#define NTC_RED       1
#define NTC_YELLOW    3
#define NTC_LBLUE     4
#define NTC_PINK      5
#define NTC_WHITE     7

/* piece IDs */
#define PIECE_LINEAR_2  1
#define PIECE_LINEAR_3  2
#define PIECE_SQUARE    3
#define PIECE_LSHAPE    4
#define PIECE_TSHAPE    5

/* game states */
#define STATE_MENU      0
#define STATE_PLAYING   1
#define STATE_PAUSED    2
#define STATE_EXIT_WARN 3

/* flood fill */
#define DIR_UP          0
#define DIR_RIGHT       1
#define DIR_DOWN        2
#define DIR_LEFT        3

/* block structure */
typedef struct {
    int color;      
    int piece_id;   /* 0 = emptee */
} Block;

/* active piece structure */
typedef struct {
    int x, y;           
    int type;
    int color; 
    int blocks[5][2]; 
    int num_blocks;
    int rotation;
    int piece_id;
} ActivePiece;

/* game state */
typedef struct {
    Block grid[GRID_H][GRID_W];
    ActivePiece current;
    int score;
    int high_score;
    int state;
    int fall_delay_ms;
    int next_piece_type;
    int next_piece_color;
    int exit_warning_shown;
    int next_piece_id;
} GameState;

/* color names for display */
const char* color_names[NUM_COLORS + 1] = {
    "",
    "Red",
/*    "Orange",*/
    "Yellow",
/*    "Green",*/
    "Blue",
    "Pink",
    "White"
};

/* ncurses color pairs (offset by 1) */
const int color_pairs[NUM_COLORS + 1] = {
    0,
    1,  /* Red */
/*    2,*/  /* Orange */
    3,  /* Yellow */
/*    4,*/  /* Green */
    5,  /* Light Blue */
    6,  /* Pink */
    7   /* White */
};

GameState game;
int running = 1;
const char* score_file = ".blockdrop_highscore";
char* score_path = NULL;

/* Function prototypes */
/* pero pero pero */
void init_game(void);
void cleanup(void);
void init_colors(void);
void draw_border(int start_y, int start_x);
void draw_grid(void);
void draw_ui(void);
void draw_piece(ActivePiece* p, int ghost);
void draw_menu(void);
void draw_pause(void);
void draw_exit_warning(void);
void generate_piece(void);
void rotate_piece_clockwise(void);
int can_move(int dx, int dy);
void move_piece(int dx, int dy);
void lock_piece(void);
void clear_matched_blocks(void);
void apply_gravity(void);
void check_matches_after_gravity(void);
void flood_fill(int r, int c, int color, int piece_id, int** visited, int** to_clear, int* count);
void blink_and_clear(int** to_clear, int count);
void lock_piece(void);
void update_fall_speed(void);
void load_high_score(void);
void save_high_score(void);
void handle_input(void);
void game_loop(void);
void get_piece_shape(int type, int rotation, int blocks[][2], int* num_blocks);
int get_piece_size(int type);
void get_screen_center(int* start_y, int* start_x);


void init_colors(void) {
    start_color();
    use_default_colors();

    /* Initialize color pairs */
    init_pair(1, COLOR_RED, -1);
/* orange was really yellow and i got a bunch of stacked yellow blocks that werent' clearing.  so it's goan! */
    init_pair(3, COLOR_YELLOW, -1);
/* green was really kinda blue and giving me the same problem as orange, so its gooooaaaannn! */
    init_pair(5, COLOR_CYAN, -1);
    init_pair(6, COLOR_MAGENTA, -1);
    init_pair(7, COLOR_WHITE, -1);

    /* bold versions for active pieces */
    init_pair(8, COLOR_RED, -1);
/* here lies norange */
    init_pair(10, COLOR_YELLOW, -1);
/* may green rest in pieces */
    init_pair(12, COLOR_CYAN, -1);
    init_pair(13, COLOR_MAGENTA, -1);
    init_pair(14, COLOR_WHITE, -1);
}

/* get screen center to position the grid */
void get_screen_center(int* start_y, int* start_x) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);
    *start_y = (rows - GRID_H) / 2;
    *start_x = (cols - GRID_W * 2) / 2;
}

/* draw dem borderses preciouss */
void draw_border(int start_y, int start_x) {
    int i;

    /* topses */
    mvaddch(start_y - 1, start_x - 1, ACS_ULCORNER);
    for (i = 0; i < GRID_W * 2; i++) {
        mvaddch(start_y - 1, start_x + i, ACS_HLINE);
    }
    mvaddch(start_y - 1, start_x + GRID_W * 2, ACS_URCORNER);

    /* sideses */
    for (i = 0; i < GRID_H; i++) {
        mvaddch(start_y + i, start_x - 1, ACS_VLINE);
        mvaddch(start_y + i, start_x + GRID_W * 2, ACS_VLINE);
    }

    /* bottomses yes yes precious! */
    mvaddch(start_y + GRID_H, start_x - 1, ACS_LLCORNER);
    for (i = 0; i < GRID_W * 2; i++) {
        mvaddch(start_y + GRID_H, start_x + i, ACS_HLINE);
    }
    mvaddch(start_y + GRID_H, start_x + GRID_W * 2, ACS_LRCORNER);
}

/* draw the grid */
void draw_grid(void) {
    int r, c;
    int start_y, start_x;
    get_screen_center(&start_y, &start_x);

    erase();  /* using erase cuz clear was causing tty to flicker */
    draw_border(start_y, start_x);

    for (r = 0; r < GRID_H; r++) {
        for (c = 0; c < GRID_W; c++) {
            int color = game.grid[r][c].color;
            if (color > 0) {
                attron(COLOR_PAIR(color_pairs[color]));
                mvaddstr(start_y + r, start_x + c * 2, "[]");
                attroff(COLOR_PAIR(color_pairs[color]));
            } else {
                mvaddstr(start_y + r, start_x + c * 2, "  ");
            }
        }
    }

    /* draw current piece */
    draw_piece(&game.current, 0);

    draw_ui();
}

/* draw the UI  */
void draw_ui(void) {
    int start_y, start_x;
    get_screen_center(&start_y, &start_x);

    int ui_x = start_x + GRID_W * 2 + 4;
    int ui_y = start_y;

    mvprintw(ui_y, ui_x, "BlockDrop");
    mvprintw(ui_y + 1, ui_x, "Falling Blocks Game");
    mvprintw(ui_y + 3, ui_x, "Score: %d", game.score);
    mvprintw(ui_y + 4, ui_x, "High:  %d", game.high_score);


    float speed = 1000.0 / game.fall_delay_ms;
    mvprintw(ui_y + 6, ui_x, "Speed: %.1fx", speed);

    mvprintw(ui_y + 8, ui_x, "Next:");

    int px = ui_x;
    int py = ui_y + 10;

    mvprintw(py, px, "Piece: %d", game.next_piece_type);
}

/* draw the active piece */
void draw_piece(ActivePiece* p, int ghost) {
    int i;
    int start_y, start_x;
    get_screen_center(&start_y, &start_x);

    if (ghost) {

        return;
    }

    for (i = 0; i < p->num_blocks; i++) {
        int r = p->y + p->blocks[i][1];
        int c = p->x + p->blocks[i][0];
        if (r >= 0 && r < GRID_H && c >= 0 && c < GRID_W) {
            attron(COLOR_PAIR(color_pairs[p->color]) | A_BOLD);
            mvaddstr(start_y + r, start_x + c * 2, "[]");
            attroff(COLOR_PAIR(color_pairs[p->color]) | A_BOLD);
        }
    }
}

/* menu screen */
void draw_menu(void) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    erase();

    mvprintw(rows / 2 - 4, (cols - 20) / 2, "BlockDrop");
    mvprintw(rows / 2 - 3, (cols - 20) / 2, "Falling Blocks Game");
    mvprintw(rows / 2 - 1, (cols - 20) / 2, "High Score: %d", game.high_score);
    mvprintw(rows / 2 + 1, (cols - 20) / 2, "Press ENTER to start");
    mvprintw(rows / 2 + 3, (cols - 20) / 2, "Controls:");
    mvprintw(rows / 2 + 4, (cols - 20) / 2, "Arrows: Move");
    mvprintw(rows / 2 + 5, (cols - 20) / 2, "Space:  Rotate");
    mvprintw(rows / 2 + 6, (cols - 20) / 2, "Enter:  Pause");
    mvprintw(rows / 2 + 7, (cols - 20) / 2, "Esc:    Exit");
}

/* pause screen */
void draw_pause(void) {
    int rows, cols;
    getmaxyx(stdscr, cols, rows);  
    getmaxyx(stdscr, rows, cols);

    attron(A_BOLD);
    mvprintw(rows / 2, (cols - 10) / 2, "P A U S E D");
    attroff(A_BOLD);
    mvprintw(rows / 2 + 2, (cols - 24) / 2, "Press ENTER to resume");
    mvprintw(rows / 2 + 3, (cols - 24) / 2, "Press ESC twice to exit");
}

/* exit warning */
void draw_exit_warning(void) {
    int rows, cols;
    getmaxyx(stdscr, rows, cols);

    attron(A_BOLD | COLOR_PAIR(1));
    mvprintw(rows / 2, (cols - 24) / 2, "Press ESC again to exit");
    attroff(A_BOLD | COLOR_PAIR(1));
}

/* piece size */
int get_piece_size(int type) {
    switch (type) {
        case PIECE_LINEAR_2: return 2;
        case PIECE_LINEAR_3: return 3;
        case PIECE_SQUARE:   return 4;
        case PIECE_LSHAPE:   return 4;
        case PIECE_TSHAPE:   return 5;
        default: return 2;
    }
}

/* get piece shape */
void get_piece_shape(int type, int rotation, int blocks[][2], int* num_blocks) {
    int i;

    /* all to 0 */
    for (i = 0; i < 5; i++) {
        blocks[i][0] = 0;
        blocks[i][1] = 0;
    }

    switch (type) {
        case PIECE_LINEAR_2:

            *num_blocks = 2;
            blocks[0][0] = 0; blocks[0][1] = 0;
            if (rotation % 2 == 0) {

                blocks[1][0] = 1; blocks[1][1] = 0;
            } else {

                blocks[1][0] = 0; blocks[1][1] = 1;
            }
            break;

        case PIECE_LINEAR_3:

            *num_blocks = 3;
            blocks[0][0] = 0; blocks[0][1] = 0;
            if (rotation % 2 == 0) {

                blocks[1][0] = 1; blocks[1][1] = 0;
                blocks[2][0] = 2; blocks[2][1] = 0;
            } else {

                blocks[1][0] = 0; blocks[1][1] = 1;
                blocks[2][0] = 0; blocks[2][1] = 2;
            }
            break;

        case PIECE_SQUARE:
/* square pieces don't rotate */
            *num_blocks = 4;
            blocks[0][0] = 0; blocks[0][1] = 0;
            blocks[1][0] = 1; blocks[1][1] = 0;
            blocks[2][0] = 0; blocks[2][1] = 1;
            blocks[3][0] = 1; blocks[3][1] = 1;
            break;

        case PIECE_LSHAPE:

            *num_blocks = 4;
            switch (rotation % 4) {
                case 0:

                    blocks[0][0] = 0; blocks[0][1] = 0;
                    blocks[1][0] = 0; blocks[1][1] = 1;
                    blocks[2][0] = 0; blocks[2][1] = 2;
                    blocks[3][0] = 1; blocks[3][1] = 2;
                    break;
                case 1:

                    blocks[0][0] = 0; blocks[0][1] = 0;
                    blocks[1][0] = 1; blocks[1][1] = 0;
                    blocks[2][0] = 2; blocks[2][1] = 0;
                    blocks[3][0] = 0; blocks[3][1] = 1;
                    break;
                case 2:

                    blocks[0][0] = 0; blocks[0][1] = 0;
                    blocks[1][0] = 1; blocks[1][1] = 0;
                    blocks[2][0] = 1; blocks[2][1] = 1;
                    blocks[3][0] = 1; blocks[3][1] = 2;
                    break;
                case 3:

                    blocks[0][0] = 2; blocks[0][1] = 0;
                    blocks[1][0] = 0; blocks[1][1] = 1;
                    blocks[2][0] = 1; blocks[2][1] = 1;
                    blocks[3][0] = 2; blocks[3][1] = 1;
                    break;
            }
            break;

        case PIECE_TSHAPE:

            *num_blocks = 5;
            switch (rotation % 4) {
                case 0:

                    blocks[0][0] = 1; blocks[0][1] = 0;
                    blocks[1][0] = 0; blocks[1][1] = 1;
                    blocks[2][0] = 1; blocks[2][1] = 1;
                    blocks[3][0] = 2; blocks[3][1] = 1;
                    blocks[4][0] = 1; blocks[4][1] = 2;
                    break;
                case 1:

                    blocks[0][0] = 0; blocks[0][1] = 1;
                    blocks[1][0] = 1; blocks[1][1] = 0;
                    blocks[2][0] = 1; blocks[2][1] = 1;
                    blocks[3][0] = 1; blocks[3][1] = 2;
                    blocks[4][0] = 2; blocks[4][1] = 1;
                    break;
                case 2:

                    blocks[0][0] = 0; blocks[0][1] = 0;
                    blocks[1][0] = 1; blocks[1][1] = 0;
                    blocks[2][0] = 2; blocks[2][1] = 0;
                    blocks[3][0] = 1; blocks[3][1] = 1;
                    blocks[4][0] = 1; blocks[4][1] = 2;
                    break;
                case 3:

                    blocks[0][0] = 0; blocks[0][1] = 1;
                    blocks[1][0] = 1; blocks[1][1] = 0;
                    blocks[2][0] = 1; blocks[2][1] = 1;
                    blocks[3][0] = 1; blocks[3][1] = 2;
                    blocks[4][0] = 2; blocks[4][1] = 1;

                    blocks[0][0] = 2; blocks[0][1] = 0;
                    blocks[1][0] = 2; blocks[1][1] = 1;
                    blocks[2][0] = 2; blocks[2][1] = 2;
                    blocks[3][0] = 1; blocks[3][1] = 1;
                    blocks[4][0] = 0; blocks[4][1] = 1;
                    break;
            }
            break;

        default:
            *num_blocks = 2;
            blocks[0][0] = 0; blocks[0][1] = 0;
            blocks[1][0] = 1; blocks[1][1] = 0;
    }
}

/* new piece */
void generate_piece(void) {

    if (game.next_piece_type == 0) {

        game.next_piece_type = 1 + (rand() % 5);
        game.next_piece_color = 1 + (rand() % NUM_COLORS);
    }

    game.current.type = game.next_piece_type;
    game.current.color = game.next_piece_color;
    game.current.rotation = 0;
    game.current.piece_id = game.next_piece_id++;


    game.next_piece_type = 1 + (rand() % 5);
    game.next_piece_color = 1 + (rand() % NUM_COLORS);


    get_piece_shape(game.current.type, game.current.rotation,
                    game.current.blocks, &game.current.num_blocks);


    game.current.x = GRID_W / 2 - 1;
    game.current.y = 0;


    if (!can_move(0, 0)) {

        game.state = STATE_MENU;
        if (game.score > game.high_score) {
            game.high_score = game.score;
            save_high_score();
        }
        memset(game.grid, 0, sizeof(game.grid));
        game.score = 0;
        game.fall_delay_ms = 1000;
        game.next_piece_id = 1;
    }
}

/* rot clockwise */
void rotate_piece_clockwise(void) {
    int old_rotation = game.current.rotation;
    int old_blocks[5][2];
    int i;


    memcpy(old_blocks, game.current.blocks, sizeof(old_blocks));


    game.current.rotation = (game.current.rotation + 1) % 4;
    get_piece_shape(game.current.type, game.current.rotation,
                    game.current.blocks, &game.current.num_blocks);


    if (!can_move(0, 0)) {

        int kicks[] = {-1, 1, -2, 2};
        int kick_success = 0;

        for (i = 0; i < 4; i++) {
            game.current.x += kicks[i];
            if (can_move(0, 0)) {
                kick_success = 1;
                break;
            }
            game.current.x -= kicks[i];
        }

        if (!kick_success) {

            game.current.rotation = old_rotation;
            memcpy(game.current.blocks, old_blocks, sizeof(old_blocks));
        }
    }
}

/* check if can move */
int can_move(int dx, int dy) {
    int i;
    int new_x = game.current.x + dx;
    int new_y = game.current.y + dy;

    for (i = 0; i < game.current.num_blocks; i++) {
        int c = new_x + game.current.blocks[i][0];
        int r = new_y + game.current.blocks[i][1];


        if (c < 0 || c >= GRID_W || r >= GRID_H) {
            return 0;
        }

        /* check collision with blocks */
        if (r >= 0 && game.grid[r][c].color != 0) {
            return 0;
        }
    }

    return 1;
}


void move_piece(int dx, int dy) {
    if (can_move(dx, dy)) {
        game.current.x += dx;
        game.current.y += dy;
    }
}


void lock_piece(void) {
    int i;

    for (i = 0; i < game.current.num_blocks; i++) {
        int c = game.current.x + game.current.blocks[i][0];
        int r = game.current.y + game.current.blocks[i][1];

        if (r >= 0 && r < GRID_H && c >= 0 && c < GRID_W) {
            game.grid[r][c].color = game.current.color;
            game.grid[r][c].piece_id = game.current.piece_id;
        }
    }
}


void flood_fill(int r, int c, int color, int piece_id, int** visited, int** to_clear, int* count) {
    int dr[] = {-1, 0, 1, 0};  /* ain't matching corners */
    int dc[] = {0, 1, 0, -1};
    int i;


    if (r < 0 || r >= GRID_H || c < 0 || c >= GRID_W) {
        return;
    }


    if (visited[r][c]) {
        return;
    }


    if (game.grid[r][c].color == color && game.grid[r][c].piece_id != piece_id) {
        visited[r][c] = 1;
        to_clear[*count][0] = r;
        to_clear[*count][1] = c;
        (*count)++;


        for (i = 0; i < 4; i++) {
            flood_fill(r + dr[i], c + dc[i], color, piece_id, visited, to_clear, count);
        }
    }
}


void blink_and_clear(int** to_clear, int count) {
    int i, b;
    int start_y, start_x;
    get_screen_center(&start_y, &start_x);

    /* blink 3 times */
    for (b = 0; b < 3; b++) {

        for (i = 0; i < count; i++) {
            int r = to_clear[i][0];
            int c = to_clear[i][1];
            mvaddstr(start_y + r, start_x + c * 2, "  ");
        }
        refresh();
        usleep(250000);


        attron(A_BOLD);
        for (i = 0; i < count; i++) {
            int r = to_clear[i][0];
            int c = to_clear[i][1];
            int color = game.grid[r][c].color;
            attron(COLOR_PAIR(color_pairs[color]));
            mvaddstr(start_y + r, start_x + c * 2, "XX");
            attroff(COLOR_PAIR(color_pairs[color]));
        }
        attroff(A_BOLD);
        refresh();
        usleep(250000);
    }

    /* clear blocks */
    for (i = 0; i < count; i++) {
        int r = to_clear[i][0];
        int c = to_clear[i][1];
        game.grid[r][c].color = 0;
        game.grid[r][c].piece_id = 0;
        game.score += 25;
    }
}

/* check and clear all matched blocks */
void clear_matched_blocks(void) {
    int** visited;
    int** to_clear;
    int count;
    int i;
    int cleared_anything = 0;
    int has_match = 0;
    int dr[] = {-1, 0, 1, 0};
    int dc[] = {0, 1, 0, -1};
    int d;

    /* allocate 2D arrays */
    visited = malloc(GRID_H * sizeof(int*));
    to_clear = malloc(GRID_H * GRID_W * sizeof(int*));
    for (i = 0; i < GRID_H; i++) {
        visited[i] = calloc(GRID_W, sizeof(int));
    }
    for (i = 0; i < GRID_H * GRID_W; i++) {
        to_clear[i] = malloc(2 * sizeof(int));
    }


    count = 0;
    has_match = 0;


    for (i = 0; i < game.current.num_blocks; i++) {
        int pc = game.current.x + game.current.blocks[i][0];
        int pr = game.current.y + game.current.blocks[i][1];

        if (pr < 0 || pr >= GRID_H || pc < 0 || pc >= GRID_W) {
            continue;
        }

        int color = game.current.color;
        int piece_id = game.current.piece_id;


        visited[pr][pc] = 1;

        /* start flood fill from adjacent blocks */
        for (d = 0; d < 4; d++) {
            int nr = pr + dr[d];
            int nc = pc + dc[d];
            if (nr >= 0 && nr < GRID_H && nc >= 0 && nc < GRID_W) {
                if (game.grid[nr][nc].color == color &&
                    game.grid[nr][nc].piece_id != piece_id &&
                    !visited[nr][nc]) {
                    flood_fill(nr, nc, color, piece_id, visited, to_clear, &count);
                    has_match = 1;
                }
            }
        }
    }

    /* now add all blocks from the current piece that have matches */
    if (has_match) {
        for (i = 0; i < game.current.num_blocks; i++) {
            int pc = game.current.x + game.current.blocks[i][0];
            int pr = game.current.y + game.current.blocks[i][1];

            if (pr >= 0 && pr < GRID_H && pc >= 0 && pc < GRID_W) {

                for (d = 0; d < 4; d++) {
                    int nr = pr + dr[d];
                    int nc = pc + dc[d];
                    if (nr >= 0 && nr < GRID_H && nc >= 0 && nc < GRID_W) {
                        if (visited[nr][nc] &&
                            game.grid[nr][nc].color == game.current.color &&
                            game.grid[nr][nc].piece_id != game.current.piece_id) {

                            to_clear[count][0] = pr;
                            to_clear[count][1] = pc;
                            count++;
                            break;  
                        }
                    }
                }
            }
        }
    }

    /* clear everything if matches */
    if (count > 0) {
        blink_and_clear(to_clear, count);
        cleared_anything = 1;
    }

    /* free memory */
    for (i = 0; i < GRID_H; i++) {
        free(visited[i]);
    }
    free(visited);
    for (i = 0; i < GRID_H * GRID_W; i++) {
        free(to_clear[i]);
    }
    free(to_clear);

    /* apply gravity */
    if (cleared_anything) {
        apply_gravity();
    }
}

/* apply individual block gravity after clearing */
void apply_gravity(void) {
    int c, r;
    int moved;
    int chain = 1;

    do {
        moved = 0;


        for (c = 0; c < GRID_W; c++) {
            for (r = GRID_H - 1; r > 0; r--) {
                if (game.grid[r][c].color == 0) {

                    int above_r;
                    for (above_r = r - 1; above_r >= 0; above_r--) {
                        if (game.grid[above_r][c].color != 0) {
                            break;
                        }
                    }

                    if (above_r >= 0) {

                        game.grid[r][c] = game.grid[above_r][c];
                        game.grid[above_r][c].color = 0;
                        game.grid[above_r][c].piece_id = 0;
                        moved = 1;
                    }
                }
            }
        }

        /* redraw and pause */
        if (moved) {
            draw_grid();
            refresh();
            usleep(100000);  


            check_matches_after_gravity();
        }

        chain++;
    } while (moved && chain < 10);  
}

/* check for matches after gravity */
void check_matches_after_gravity(void) {
    int r, c;
    int** visited;
    int** to_clear;
    int count;
    int i, d;
    int cleared_anything = 0;
    int dr[] = {-1, 0, 1, 0};
    int dc[] = {0, 1, 0, -1};

    visited = malloc(GRID_H * sizeof(int*));
    to_clear = malloc(GRID_H * GRID_W * sizeof(int*));
    for (i = 0; i < GRID_H; i++) {
        visited[i] = calloc(GRID_W, sizeof(int));
    }
    for (i = 0; i < GRID_H * GRID_W; i++) {
        to_clear[i] = malloc(2 * sizeof(int));
    }

    count = 0;
    for (i = 0; i < GRID_H; i++) {
        memset(visited[i], 0, GRID_W * sizeof(int));
    }


    for (r = 0; r < GRID_H; r++) {
        for (c = 0; c < GRID_W; c++) {
            if (game.grid[r][c].color == 0) continue;
            if (visited[r][c]) continue;  

            int color = game.grid[r][c].color;
            int piece_id = game.grid[r][c].piece_id;
            int has_neighbor_match = 0;

            /* check adjacent for matching color, different piece_id */
            for (d = 0; d < 4; d++) {
                int nr = r + dr[d];
                int nc = c + dc[d];
                if (nr >= 0 && nr < GRID_H && nc >= 0 && nc < GRID_W) {
                    if (game.grid[nr][nc].color == color &&
                        game.grid[nr][nc].piece_id != piece_id) {
                        has_neighbor_match = 1;
                        flood_fill(nr, nc, color, piece_id, visited, to_clear, &count);
                    }
                }
            }


            if (has_neighbor_match) {
                to_clear[count][0] = r;
                to_clear[count][1] = c;
                count++;
            }
        }
    }


    if (count > 0) {
        blink_and_clear(to_clear, count);
        cleared_anything = 1;
    }


    for (i = 0; i < GRID_H; i++) {
        free(visited[i]);
    }
    free(visited);
    for (i = 0; i < GRID_H * GRID_W; i++) {
        free(to_clear[i]);
    }
    free(to_clear);


    if (cleared_anything) {
        apply_gravity();
    }
}

/* update fall speed based on score */
void update_fall_speed(void) {
    int milestones = game.score / 500;
    float speed_multiplier = 1.0;
    int i;

    for (i = 0; i < milestones; i++) {
        speed_multiplier *= 1.3;
    }

    game.fall_delay_ms = (int)(1000.0 / speed_multiplier);
    if (game.fall_delay_ms < 100) {
        game.fall_delay_ms = 100;  /* oguri cap at 100ms */
    }
}

/* load high score from file */
void load_high_score(void) {
    FILE* f;
    const char* home = getenv("HOME");

    if (home) {
        size_t len = strlen(home) + strlen(score_file) + 2;
        score_path = malloc(len);
        snprintf(score_path, len, "%s/%s", home, score_file);
    } else {
        score_path = strdup(score_file);
    }

    f = fopen(score_path, "r");
    if (f) {
        if (fscanf(f, "%d", &game.high_score) != 1) {
            game.high_score = 0;
        }
        fclose(f);
    } else {
        game.high_score = 0;
    }
}

/* Ssve high score to file */
void save_high_score(void) {
    FILE* f = fopen(score_path, "w");
    if (f) {
        fprintf(f, "%d\n", game.high_score);
        fclose(f);
        chmod(score_path, 0600);
    }
}

/* handle user input */
void handle_input(void) {
    int ch = getch();

    if (ch == ERR) {
        return;  
    }

    switch (game.state) {
        case STATE_MENU:
            if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                game.state = STATE_PLAYING;
                game.score = 0;
                game.fall_delay_ms = 1000;
                game.next_piece_id = 1;
                memset(game.grid, 0, sizeof(game.grid));
                game.next_piece_type = 0;
                generate_piece();
            } else if (ch == 27) {  /* ESC */
                game.exit_warning_shown = 1;
                game.state = STATE_EXIT_WARN;
            }
            break;

        case STATE_PLAYING:
            switch (ch) {
                case KEY_LEFT:
                    move_piece(-1, 0);
                    break;
                case KEY_RIGHT:
                    move_piece(1, 0);
                    break;
                case KEY_DOWN:
                    move_piece(0, 1);
                    break;
                case KEY_UP:
                    /* no going up */
                    break;
                case ' ':
                    rotate_piece_clockwise();
                    break;
                case '\n':
                case '\r':
                case KEY_ENTER:
                    game.state = STATE_PAUSED;
                    break;
                case 27:  /* 1st esc warning*/
                    game.exit_warning_shown = 1;
                    game.state = STATE_EXIT_WARN;
                    break;
            }
            break;

        case STATE_PAUSED:
            if (ch == '\n' || ch == '\r' || ch == KEY_ENTER) {
                game.state = STATE_PLAYING;
            } else if (ch == 27) { 
                game.exit_warning_shown = 1;
                game.state = STATE_EXIT_WARN;
            }
            break;

        case STATE_EXIT_WARN:
            if (ch == 27) {  /* 2nd esc exit */
                running = 0;
                if (game.score > game.high_score) {
                    game.high_score = game.score;
                    save_high_score();
                }
            } else {
                /* cancel exit warning, return */
                if (game.exit_warning_shown) {
                    game.exit_warning_shown = 0;
                    game.state = STATE_PAUSED;  /* default to pause */
                }
            }
            break;
    }
}

/* main game loop */
void game_loop(void) {
    struct timespec last_fall, now;
    long elapsed_ms;

    clock_gettime(CLOCK_MONOTONIC, &last_fall);

    while (running) {

        handle_input();


        switch (game.state) {
            case STATE_MENU:
                draw_menu();
                break;
            case STATE_PLAYING:
                draw_grid();

                /* check if time to fall */
                clock_gettime(CLOCK_MONOTONIC, &now);
                elapsed_ms = (now.tv_sec - last_fall.tv_sec) * 1000 +
                            (now.tv_nsec - last_fall.tv_nsec) / 1000000;

                if (elapsed_ms >= game.fall_delay_ms) {
                    if (can_move(0, 1)) {
                        move_piece(0, 1);
                    } else {
                        /* eagle has landed */
                        lock_piece();
                        clear_matched_blocks();
                        update_fall_speed();
                        generate_piece();
                    }
                    last_fall = now;
                }
                break;

            case STATE_PAUSED:
                draw_grid();
                draw_pause();
                break;

            case STATE_EXIT_WARN:
                draw_grid();
                draw_exit_warning();
                break;
        }

        refresh();
        usleep(16666);  /* ~60 FPS */
    }
}

/* initialize game */
void init_game(void) {

    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    nodelay(stdscr, TRUE);  
    /* TTY optimizations to reduce flicker */
    leaveok(stdscr, TRUE);   /* don't restore cursor position - reduces flicker */
    idlok(stdscr, TRUE);     /* enable line insertion/deletion optimization */


    init_colors();


    memset(&game, 0, sizeof(game));
    game.state = STATE_MENU;
    game.fall_delay_ms = 1000;
    game.next_piece_id = 1;


    load_high_score();


    srand(time(NULL));
}


void cleanup(void) {
    endwin();
    if (score_path) {
        free(score_path);
    }
}


void signal_handler(int sig) {
    (void)sig;
    running = 0;
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;


    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    /* run forrest */
    init_game();
    game_loop();
    cleanup();

    return 0;
}
