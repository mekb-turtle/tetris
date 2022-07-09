#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <3ds.h>
#include <string.h>
#include <time.h>

// game constants
#define DELAY 30 // how many ms every tick is
#define GAME_OVER_DELAY 2000 // how many ms every tick is
#define MOVE_EVERY 10 // how many delays to move down by one
#define GAME_I 20 // board height
#define GAME_J 10 // board width

// pieces
#include "./tetrominos.h"
//#define FORCE_PIECE ID_I // uncomment and set to ID_<piece> to force a piece to spawn instead of reading from RANDOM_FILE

// rendering
#define CLEAR_BORDER    7 // needs to be at least the maximum j of all tetrominos
#define COLOR_BORDER    "\x1b[48;5;0m" // the color sequence of the border
#define RESET           "\x1b[0m" // reset sequence
#define BOARD_CHAR      "#"
#define PLACE_CHAR      "#"
#define NEXT_CHAR       "#"
#define INDICATOR_CHAR  "-"
#define BORDER_CHAR     " "
#define BORDER_CHAR_ONE " "
#define EMPTY_CHAR      " "
#define BORDER RESET COLOR_BORDER BORDER_CHAR
#define BORDER_ONE RESET COLOR_BORDER BORDER_CHAR_ONE

// game variables
long score = 0;
uint8_t game_over = 0; // flag for game over
uint8_t place_indicator[GAME_I][GAME_J]; // indicator for when dropping, also used as a temp for rotate
uint8_t temp[GAME_I][GAME_J]; // temp for rotate
uint8_t board[GAME_I][GAME_J]; // board
uint8_t place[GAME_I][GAME_J]; // tiles to be placed
uint8_t next_piece_id; // the id of the next piece to be placed
uint8_t** next_piece; // the array of the next piece to be placed
uint8_t next_piece_i; // height of next piece to be placed
uint8_t next_piece_j; // width of next piece to be placed

void sleep_(unsigned long ms) {
	struct timespec r = { ms/1e3, (ms%1000)*1e6 }; // 1 millisecond = 1e6 nanoseonds
	nanosleep(&r, NULL);
}

char* score_string;
size_t score_len;

void render() {
	printf("\x1b[0;0H");
	for (uint8_t j = 0; j < GAME_J+3+next_piece_j;       ++j) printf(BORDER);
	printf("%s", RESET);
	for (uint8_t j = 0; j < CLEAR_BORDER-next_piece_j-1; ++j) printf(EMPTY_CHAR);
	printf("\n");
	if (score == 0) sprintf(score_string, " 0 %c", '\0');
	else sprintf(score_string, " %li00 %c", score, '\0');
	score_len = strlen(score_string);
	for (uint8_t i = 0; i < GAME_I; ++i) {
		printf("%s", BORDER RESET);
		for (uint8_t j = 0; j < GAME_J; ++j) {
			char* col = NULL;
			printf("%s", RESET);
			if        (col = get_color(board[i][j])) {
				printf("\x1b[%sm%s", col, BOARD_CHAR);
			} else if (col = get_color(place[i][j])) {
				printf("\x1b[%sm%s", col, PLACE_CHAR);
			} else if (col = get_color(place_indicator[i][j])) {
				printf("\x1b[%sm%s", col, INDICATOR_CHAR);
			} else {
				printf("%s", EMPTY_CHAR);
			}
		}
		printf("%s", BORDER);
		if (i == GAME_I - 1) {
			printf("%s%s%s", RESET, score_string, BORDER); // print score
		} else if (i == GAME_I - 2) {
			printf("%s", BORDER);
			for (uint8_t j = 0; j < score_len; ++j)
				printf("%s", BORDER_ONE);
		} else if (i == next_piece_i) {
			for (uint8_t j = 0; j < next_piece_j+1; ++j) printf("%s", BORDER);
			printf("%s", RESET);
			for (uint8_t j = 0; j < CLEAR_BORDER-next_piece_j-1; ++j) printf("%s", EMPTY_CHAR);
		} else if (i > next_piece_i) {
			printf("%s", RESET);
			for (uint8_t j = 0; j < CLEAR_BORDER;                ++j) printf("%s", EMPTY_CHAR);
		} else {
			printf("%s", RESET);
			printf("\x1b[%sm", get_color(next_piece_id));
			for (uint8_t j = 0; j < next_piece_j; ++j) {
				printf("%s", next_piece[i][j] ? NEXT_CHAR : EMPTY_CHAR); // print the tile
			}
			printf("%s", BORDER RESET);
			for (uint8_t j = 0; j < CLEAR_BORDER-next_piece_j-1; ++j) printf("%s", EMPTY_CHAR);
		}
		printf("\n");
	}
	for (uint8_t j = 0; j < GAME_J+3; ++j) printf("%s", BORDER);
	for (uint8_t j = 0; j < score_len; ++j) printf("%s", BORDER_ONE);
	printf("%s\n", RESET);
}

uint8_t began = 0;
void flush_all() {
	fflush(stdin); fflush(stdout); fflush(stderr);
}

uint8_t is_placing() { // returns 1 if there are tiles in place
	for (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			if (place[i][j] != 0) return 1;
	return 0;
}
void add_block(uint8_t id, uint8_t** piece, uint8_t I, uint8_t J) { // puts a piece on place
	uint8_t i_ = 0;
	uint8_t j_ = (GAME_J-J)/2;
	for (uint8_t i = 0; i < I; ++i) {
		for (uint8_t j = 0; j < J; ++j) {
			if (piece[i][j]) {
				if (board[i_+i][j_+j]) {
					game_over = 1; // game over if there's already a tile in the board there
					memset(place, 0, sizeof(place));
					return;
				}
				place[i_+i][j_+j] = id;
			}
		}
	}
}
void add_next_piece() {
	add_block(next_piece_id, next_piece, next_piece_i, next_piece_j);
}
void set_next_piece(uint8_t id, uint8_t** piece, uint8_t I, uint8_t J) {
	next_piece_id = id;
	next_piece    = piece;
	next_piece_i  = I;
	next_piece_j  = J;
}
uint8_t random_next_piece() {
#ifndef FORCE_PIECE
	uint8_t t_;
	while (1) {
		t_ = rand();
		if (t_ == EOF) {
			exit(2);
			return 0;
		}
		if (t_ < (UINT8_MAX - (UINT8_MAX % NUM_PIECES))) break; // avoid bias
	}
	uint8_t t = (t_ % NUM_PIECES) + 1;
#else
	uint8_t t = FORCE_PIECE;
#endif
	if (t == ID_I) set_next_piece(ID_I, piece_i, I_I, J_I);
	if (t == ID_J) set_next_piece(ID_J, piece_j, I_J, J_J);
	if (t == ID_L) set_next_piece(ID_L, piece_l, I_L, J_L);
	if (t == ID_O) set_next_piece(ID_O, piece_o, I_O, J_O);
	if (t == ID_S) set_next_piece(ID_S, piece_s, I_S, J_S);
	if (t == ID_T) set_next_piece(ID_T, piece_t, I_T, J_T);
	if (t == ID_Z) set_next_piece(ID_Z, piece_z, I_Z, J_Z);
	return 1;
}

uint8_t move(int8_t move_i, int8_t move_j) { // move in nplace
	if (!is_placing()) return 0;
	if (move_i == 0 && move_j == 0) return 1; // not moving anywhere
	if (move_i < -1 || move_i > 1 || move_j < -1 || move_j > 1) return 0; // invalid number
	for (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			if (place[i][j]) {
				if (move_i < 0 && i == 0) return 0;
				if (move_i > 0 && i >= GAME_I-1) return 0;
				if (move_j < 0 && j == 0) return 0;
				if (move_j > 0 && j >= GAME_J-1) return 0;
				if (board[i+move_i][j+move_j]) return 0; // check there aren't tiles there on the board already
			}
	// find which way to loop
	for (uint8_t i = move_i >= 0 ? GAME_I - 1 : 0; move_i >= 0 ? (i >= 0 && i != UINT8_MAX) : i < GAME_I; i += move_i >= 0 ? -1 : 1)
		for (uint8_t j = move_j >= 0 ? GAME_J - 1 : 0; move_j >= 0 ? (j >= 0 && j != UINT8_MAX) : j < GAME_J; j += move_j >= 0 ? -1 : 1)
			if (place[i][j]) {
				place[i+move_i][j+move_j] = place[i][j];
				place[i][j] = 0;
			}
	return 1;
}
void rotate_flip_ij(uint8_t si1, uint8_t sj1, uint8_t si2, uint8_t sj2) { // flip the i and j around
	memset(temp, 0, sizeof(temp));
	for (uint8_t i = si1; i <= si2; ++i)
		for (uint8_t j = sj1; j <= sj2; ++j)
			temp[j-sj1+si1][i-si1+sj1] = place_indicator[i][j];
	memcpy(place_indicator, temp, sizeof(temp));
}
void rotate_reverse(uint8_t si1, uint8_t sj1, uint8_t si2, uint8_t sj2) { // reverse
	memset(temp, 0, sizeof(temp));
	for (uint8_t i = si1; i <= si2; ++i)
		for (uint8_t j = sj1; j <= sj2; ++j)
			temp[((si2-si1)-(i-si1))+si1][j] = place_indicator[i][j];
	memcpy(place_indicator, temp, sizeof(temp));
}
uint8_t rotate(int8_t a) {
	if (!is_placing()) return 0;
	if (a == 0) return 1;
	if (a == -2) a = 2;
	if (a != -1 && a != 1 && a != 2) return 0;
	uint8_t si1 = UINT8_MAX;
	uint8_t sj1 = UINT8_MAX;
	uint8_t si2 = UINT8_MAX;
	uint8_t sj2 = UINT8_MAX;
	for (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			if (place[i][j]) {
				if (si1 == UINT8_MAX || i < si1) si1 = i; // find lowest i
				if (sj1 == UINT8_MAX || j < sj1) sj1 = j; // find lowest j
				if (si2 == UINT8_MAX || i > si2) si2 = i; // find highest i
				if (sj2 == UINT8_MAX || j > sj2) sj2 = j; // find highest j
			}
	if (si1 == UINT8_MAX || sj1 == UINT8_MAX || si2 == UINT8_MAX || sj2 == UINT8_MAX) return 1;
	if (a < 2) {
		if (si1 + (sj2 - sj1) >= GAME_I) return 0; // check if in bounds
		if (sj1 + (si2 - si1) >= GAME_J) return 0;
	}
	memcpy(place_indicator, place, sizeof(place));
	for (uint8_t r = 0; r < (a == 2 ? 2 : 1); ++r) {
		if (a > 0) rotate_reverse(si1, sj1, si2, sj2);
		rotate_flip_ij(si1, sj1, si2, sj2);
		uint8_t tmp = (sj2-sj1)+si1; // update i and j
		sj2 = (si2-si1)+sj1;
		si2 = tmp;
	}
	if (a == -1) rotate_reverse(si1, sj1, si2, sj2);
	for (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			if (place_indicator[i][j] && board[i][j]) return 0; // check there aren't tiles there on the board already 
	memcpy(place, place_indicator, sizeof(place));
	return 0;
}
uint8_t move_indicator() { // move place_indicator down until it hits something
	if (!is_placing()) return 0;
	memcpy(place_indicator, place, sizeof(place));
	uint8_t break_ = 0;
	while (!break_) {
		for (uint8_t i = 0; !break_ && i < GAME_I; ++i)
			for (uint8_t j = 0; !break_ && j < GAME_J; ++j)
				if (place_indicator[i][j]) {
					if (i >= GAME_I-1) { break_ = 1; break; }
					if (board[i+1][j]) { break_ = 1; break; }
				}
		if (break_) break;
		for (uint8_t i = GAME_I - 1; i >= 0 && i != UINT8_MAX; --i)
			for (uint8_t j = 0; j < GAME_J; ++j)
				if (place_indicator[i][j]) {
					place_indicator[i+1][j] = place_indicator[i][j];
					place_indicator[i][j] = 0;
				}
	}
	return 1;
}

uint8_t place_to_board() { // apply place to board
	uint8_t g = 0;
	for (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			if (place[i][j]) {
				board[i][j] = place[i][j];
				place[i][j] = 0;
				g = 1;
			}
	return g;
}
uint8_t check_full_lines() {
	uint8_t full;
	uint8_t lines_cleared = 0;
	for (uint8_t i = 0; i < GAME_I; ++i) {
		full = 1;
		for (uint8_t j = 0; j < GAME_J; ++j) {
			if (!board[i][j]) { full = 0; break; }
		}
		if (!full) continue;
		++lines_cleared;
		for (uint8_t k = i; k >= 0 && k != UINT8_MAX; --k) {
			for (uint8_t j = 0; j < GAME_J; ++j) {
				if (k == 0)
					board[k][j] = 0;
				else
					board[k][j] = board[k-1][j];
			}
		}
	}
	return lines_cleared;
}
void add_score(uint8_t lines_cleared) {
	if (lines_cleared == 0) return;
	else if (lines_cleared == 1) score += 1;
	else if (lines_cleared == 2) score += 3;
	else if (lines_cleared == 3) score += 5;
	else score += 8;
}

uint8_t move_timer = 0;
uint8_t move_down() { // move place down
	if (!move(1, 0)) { // if it hits something
		move_timer = 0;
		place_to_board();
		add_score(check_full_lines());
		add_next_piece();
		if (!game_over) {
			random_next_piece();
			move_indicator();
		}
		return 0;
	}
	return 1;
}
void drop() { // move_down until it hits something
	uint8_t res;
	do {
		res = move_down();
	} while (res);
}

void reset_game() {
	game_over = 0;
	score = 0;
	memset(board, 0, sizeof(board));
	memset(place, 0, sizeof(place));
	random_next_piece();
	add_next_piece();
	random_next_piece();
	move_indicator();
}


int main() {
	gfxInitDefault();
	consoleInit(GFX_TOP, NULL);
	create_pieces();
	reset_game();
	score_string = malloc(24);
	while (aptMainLoop()) {
		gspWaitForVBlank();
		gfxSwapBuffers();
		hidScanInput();
		u32 kDown = hidKeysDown();
		u32 kHeld = hidKeysHeld();
		u32 kUp   = hidKeysUp();
		if (kDown & KEY_START) {
			consoleClear();
			break;
		}
		if (kDown & KEY_SELECT) {
			consoleClear();
			printf("%s", "\
\x1b[7mLeft\x1b[27m    to move left\n\
\x1b[7mRight\x1b[27m   to move right\n\
\x1b[7mDown\x1b[27m    to move down quicker\n\
\x1b[7mL\x1b[27m       to rotate -90\n\
\x1b[7mZL\x1b[27m \x1b[7mZR\x1b[27m   to rotate 180\n\
\x1b[7mR\x1b[27m       to rotate 90\n\
\x1b[7mX\x1b[27m       to drop instantly\n\
\x1b[7mStart\x1b[27m   to quit\n\
");
		} else if (kUp & KEY_SELECT) {
			consoleClear();
		} else if (!(kHeld & KEY_SELECT)) {
			if (++move_timer >= MOVE_EVERY) {
				move_timer = 0;
				move_down();
			}
			if (kDown & KEY_X)                        { drop(); }
			if (kDown & KEY_LEFT)                     { move(0, -1); move_indicator(); }
			if (kDown & KEY_RIGHT)                    { move(0,  1); move_indicator(); }
			if (kDown & KEY_DOWN)                     { move_down(); } // no need to move_indicator here
			if (kDown & KEY_L)                        { rotate(-1); move_indicator(); }
			if ((kDown & KEY_ZL) || (kDown & KEY_ZR)) { rotate( 2); move_indicator(); }
			if (kDown & KEY_R)                        { rotate( 1); move_indicator(); }
			render();
			if (game_over) {
				printf("Game Over!        \n");
				sleep_(GAME_OVER_DELAY);
				reset_game();
			} else {
				printf("%s", "\
\x1b[7mSelect\x1b[27m  for help\n\
");
			}
		}
		sleep_(DELAY);
	}
	gfxExit();
	return 0;
}
