#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/poll.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#include <unistd.h>

#define LFLAGS ECHO|ISIG|ICANON // flags for tcsetattr
#define RANDOM_FILE "/dev/urandom" // file to read random bytes from for rng
#define DELAY 5 // how many ms every tick is
#define MOVE_EVERY 60 // how many delays to move down by one
#define GAME_I 20 // board height
#define GAME_J 10 // board width
#define BELL // comment out if you don't want a bell
long score = 0;
uint8_t game_over = 0; // flag for game over
FILE* rng;
uint8_t place_indicator[GAME_I][GAME_J]; // indicator for when dropping, also used as a temp for rotate
uint8_t temp[GAME_I][GAME_J]; // temp for rotate
uint8_t board[GAME_I][GAME_J]; // board
uint8_t place[GAME_I][GAME_J]; // tiles to be placed
uint8_t next_piece_id; // the id of the next piece to be placed
uint8_t** next_piece; // the array of the next piece to be placed
uint8_t next_piece_i; // height of next piece to be placed
uint8_t next_piece_j; // width of next piece to be placed
#include "./tetrominos.h"
#define CLEAR_BORDER 7 // needs to be at least the maximum j of all tetrominos
#define COLOR_BORDER "\x1b[38;5;8m" // the color sequence of the border
#define RESET "\x1b[0m" // reset sequence
#define BOARD_CHAR "██"
#define PLACE_CHAR "██"
#define NEXT_CHAR "██"
#define INDICATOR_CHAR "▒▒"
#define BORDER_CHAR "▒▒"
#define BORDER_CHAR_ONE "▒"
#define EMPTY_CHAR "  "
#define BORDER RESET COLOR_BORDER BORDER_CHAR
#define BORDER_ONE RESET COLOR_BORDER BORDER_CHAR_ONE

void bell() {
#ifdef BELL
	fprintf(stderr, "\a"); // \a is bell character, stderr is usually non buffered so we can bell after end()
#endif
}
void sleep_(unsigned long ms) {
	struct timespec r = { ms/1e3, (ms%1000)*1e6 }; // 1 millisecond = 1e6 nanoseonds
	nanosleep(&r, NULL);
}

char* score_string;
size_t score_len;

void render(uint8_t end) {
	if (!end) printf("\x1b[H"); // goto 0,0
	for (uint8_t j = 0; j < GAME_J+3+next_piece_j;       ++j) printf(BORDER);
	printf(RESET);
	for (uint8_t j = 0; j < CLEAR_BORDER-next_piece_j-1; ++j) printf(EMPTY_CHAR);
	printf("\r\n");
	sprintf(score_string, " %li %c", score, '\0');
	score_len = strlen(score_string);
	for (uint8_t i = 0; i < GAME_I; ++i) {
		printf(BORDER RESET);
		for (uint8_t j = 0; j < GAME_J; ++j) {
			char* col = NULL;
			printf(RESET);
			if        (col = get_color(board[i][j])) {
				printf(col);
				printf(BOARD_CHAR);
			} else if (col = get_color(place[i][j])) {
				printf(col);
				printf(PLACE_CHAR);
			} else if (col = get_color(place_indicator[i][j])) {
				printf(col);
				printf(INDICATOR_CHAR);
			} else {
				printf(EMPTY_CHAR);
			}
		}
		printf(BORDER);
		if (i == GAME_I - 1) {
			printf(RESET "%s" BORDER, score_string); // print score
		} else if (i == GAME_I - 2) {
			printf(BORDER);
			for (uint8_t j = 0; j < score_len; ++j)
				printf(BORDER_ONE);
		} else if (i == next_piece_i) {
			for (uint8_t j = 0; j < next_piece_j+1; ++j) printf(BORDER);
			printf(RESET);
			for (uint8_t j = 0; j < CLEAR_BORDER-next_piece_j-1; ++j) printf(EMPTY_CHAR);
		} else if (i > next_piece_i) {
			printf(RESET);
			for (uint8_t j = 0; j < CLEAR_BORDER;                ++j) printf(EMPTY_CHAR);
		} else {
			printf(RESET);
			printf(get_color(next_piece_id));
			for (uint8_t j = 0; j < next_piece_j; ++j) {
				printf(next_piece[i][j] ? NEXT_CHAR : EMPTY_CHAR); // print the tile
			}
			printf(BORDER RESET);
			for (uint8_t j = 0; j < CLEAR_BORDER-next_piece_j-1; ++j) printf(EMPTY_CHAR);
		}
		printf("\r\n");
	}
	for     (uint8_t j = 0; j < GAME_J+3; ++j) printf(BORDER);
	for     (uint8_t j = 0; j < score_len; ++j) printf(BORDER_ONE);
	printf(RESET "\r\n");
	if (!end) {
		printf("\r\n\
\x1b[7mA\x1b[27m to move left\r\n\
\x1b[7mD\x1b[27m to move right\r\n\
\x1b[7mS\x1b[27m to move down quicker\r\n\
\x1b[7mQ\x1b[27m to rotate -90°\r\n\
\x1b[7mW\x1b[27m to rotate 180°\r\n\
\x1b[7mE\x1b[27m to rotate 90°\r\n\
\x1b[7mSpace\x1b[27m to drop instantly\r\n\
");
	}
}

uint8_t is_placing() { // returns 1 if there are tiles in place
	for     (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			if (place[i][j] != 0) return 1;
	return 0;
}
void add_block(uint8_t id, uint8_t** piece, uint8_t I, uint8_t J) { // puts a piece on place
	uint8_t i_ = 0;
	uint8_t j_ = (GAME_J-J)/2;
	for     (uint8_t i = 0; i < I; ++i)
		for (uint8_t j = 0; j < J; ++j)
			if (piece[i][j]) {
				if (board[i_+i][j_+j])
					game_over = 1; // game over if there's already a tile in the board there
				place[i_+i][j_+j] = id;
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
void random_next_piece() {
	uint8_t t = getc(rng) % 7; // the only time rng is used
	if (t == 0) set_next_piece(ID_I, piece_i, I_I, J_I);
	if (t == 1) set_next_piece(ID_J, piece_j, I_J, J_J);
	if (t == 2) set_next_piece(ID_L, piece_l, I_L, J_L);
	if (t == 3) set_next_piece(ID_O, piece_o, I_O, J_O);
	if (t == 4) set_next_piece(ID_S, piece_s, I_S, J_S);
	if (t == 5) set_next_piece(ID_T, piece_t, I_T, J_T);
	if (t == 6) set_next_piece(ID_Z, piece_z, I_Z, J_Z);
}

uint8_t move(int8_t move_i, int8_t move_j) { // move in nplace
	if (move_i == 0 && move_j == 0) return 1; // not moving anywhere
	if (move_i < -1 || move_i > 1 || move_j < -1 || move_j > 1) return 0; // invalid number
	for     (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			if (place[i][j]) {
				if (move_i < 0 && i == 0) return 0;
				if (move_i > 0 && i >= GAME_I-1) return 0;
				if (move_j < 0 && j == 0) return 0;
				if (move_j > 0 && j >= GAME_J-1) return 0;
				if (board[i+move_i][j+move_j]) return 0; // check there aren't tiles there on the board already
			}
	// find which way to loop
	for     (uint8_t i = move_i >= 0 ? GAME_I - 1 : 0; move_i >= 0 ? (i >= 0 && i != UINT8_MAX) : i < GAME_I; i += move_i >= 0 ? -1 : 1)
		for (uint8_t j = move_j >= 0 ? GAME_J - 1 : 0; move_j >= 0 ? (j >= 0 && j != UINT8_MAX) : j < GAME_J; j += move_j >= 0 ? -1 : 1)
			if (place[i][j]) {
				place[i+move_i][j+move_j] = place[i][j];
				place[i][j] = 0;
			}
	return 1;
}
void copy_cells(uint8_t a[GAME_I][GAME_J], uint8_t b[GAME_I][GAME_J]) { // copy all cells from b to a
	for     (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			a[i][j] = b[i][j];
}
void set_cells(uint8_t a[GAME_I][GAME_J], uint8_t b) { // set all cells in a to b
	for     (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			a[i][j] = b;
}
void rotate_flip_ij(uint8_t si1, uint8_t sj1, uint8_t si2, uint8_t sj2) { // flip the i and j around
	set_cells(temp, 0);
	for     (uint8_t i = si1; i <= si2; ++i)
		for (uint8_t j = sj1; j <= sj2; ++j)
			temp[j-sj1+si1][i-si1+sj1] = place_indicator[i][j];
	copy_cells(place_indicator, temp);
}
void rotate_reverse(uint8_t si1, uint8_t sj1, uint8_t si2, uint8_t sj2) { // reverse
	set_cells(temp, 0);
	for     (uint8_t i = si1; i <= si2; ++i)
		for (uint8_t j = sj1; j <= sj2; ++j)
			temp[((si2-si1)-(i-si1))+si1][j] = place_indicator[i][j];
	copy_cells(place_indicator, temp);
}
uint8_t rotate(int8_t a) {
	if (a == 0) return 1;
	if (a == -2) a = 2;
	if (a != -1 && a != 1 && a != 2) return 0;
	uint8_t si1 = UINT8_MAX;
	uint8_t sj1 = UINT8_MAX;
	uint8_t si2 = UINT8_MAX;
	uint8_t sj2 = UINT8_MAX;
	for     (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			if (place[i][j]) {
				if (si1 == UINT8_MAX || i < si1) si1 = i; // find lowest i
				if (sj1 == UINT8_MAX || j < sj1) sj1 = j; // find lowest j
				if (si2 == UINT8_MAX || i > si2) si2 = i; // find highest i
				if (sj2 == UINT8_MAX || j > sj2) sj2 = j; // find highest j
			}
	if (si1 == UINT8_MAX || sj1 == UINT8_MAX || si2 == UINT8_MAX || sj2 == UINT8_MAX) return 1;
	if (si1 + (sj2 - sj1) >= GAME_I) return 0; // check if in bounds
	if (sj1 + (si2 - si1) >= GAME_J) return 0;
	copy_cells(place_indicator, place);
	for (uint8_t r = 0; r < (a == 2 ? 2 : 1); ++r) {
		if (a > 0) rotate_reverse(si1, sj1, si2, sj2);
		rotate_flip_ij(si1, sj1, si2, sj2);
		uint8_t tmp = (sj2-sj1)+si1; // update i and j
		sj2 = (si2-si1)+sj1;
		si2 = tmp;
	}
	if (a == -1) rotate_reverse(si1, sj1, si2, sj2);
	for     (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			if (place_indicator[i][j] && board[i][j]) return 0; // check there aren't tiles there on the board already 
	copy_cells(place, place_indicator);
	return 0;
}
uint8_t move_indicator() { // move place_indicator down until it hits something
	copy_cells(place_indicator, place);
	uint8_t break_ = 0;
	while (!break_) {
		for     (uint8_t i = 0; !break_ && i < GAME_I; ++i)
			for (uint8_t j = 0; !break_ && j < GAME_J; ++j)
				if (place_indicator[i][j]) {
					if (i >= GAME_I-1) { break_ = 1; break; }
					if (board[i+1][j]) { break_ = 1; break; }
				}
		if (break_) break;
		for     (uint8_t i = GAME_I - 1; i >= 0 && i != UINT8_MAX; --i)
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
	for     (uint8_t i = 0; i < GAME_I; ++i)
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
	uint8_t all_not_full = 0;
	uint8_t lines_cleared = 0;
	while (!all_not_full) {
		all_not_full = 1;
		for (uint8_t i = 0; i < GAME_I; ++i) {
			full = 1;
			for (uint8_t j = 0; j < GAME_J; ++j) {
				if (!board[i][j]) { full = 0; break; }
			}
			if (full) {
				++lines_cleared;
				all_not_full = 0;
				for (uint8_t k = i; k >= 0 && k != UINT8_MAX; --k) {
					for (uint8_t j = 0; j < GAME_J; ++j) {
						if (k == 0)
							board[k][j] = 0;
						else
							board[k][j] = board[k-1][j];
					}
				}
			}
		}
	}
	return lines_cleared;
}
void add_score(uint8_t lines_cleared) {
	if (lines_cleared == 0) return;
	else if (lines_cleared == 1) score += 100;
	else if (lines_cleared == 2) score += 300;
	else if (lines_cleared == 3) score += 500;
	else if (lines_cleared == 4) score += 800;
	else score += 1000;
	if (fork() == 0) {
		for (uint8_t i = 0; i < lines_cleared; ++i) {
			if (i > 0) sleep_(150); // do beep pattern
			bell();
		}
		exit(0);
	}
}

uint8_t move_timer = 0;
uint8_t move_down() { // move place down
	if (!move(1, 0)) { // if it hits something
		move_timer = 0;
		bell();
		place_to_board();
		add_score(check_full_lines());
		add_next_piece();
		random_next_piece();
		move_indicator();
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

unsigned char poll_() { // check if there is something buffered in stdin
	struct pollfd fds;
	unsigned char ret;
	fds.fd = fileno(stdin);
	fds.events = POLLIN;
	ret = poll(&fds, 1, 0);
	return ret;
}
uint8_t began = 0;
void clear() {
	printf("\x1b[H\x1b[J\x1b[2J\x1b[0m"); // clear sequence
}
void flush_all() 
{
	fflush(stdin); fflush(stdout); fflush(stderr);
}
void begin() {
	if (began) return;
	began = 1;
	system("stty raw"); // set to raw mode
	printf("\x1b[s\x1b[?47h\x1b[?25l"); // save the screen and cursor location
	clear();
    struct termios term;
    tcgetattr(fileno(stdin), &term);
    term.c_lflag &= ~(LFLAGS); // remove the LFLAGS
    tcsetattr(fileno(stdin), 0, &term);
	flush_all();
}
void end(uint8_t do_render) {
	if (!began) return;
	began = 0;
    struct termios term;
	clear();
	printf("\x1b[?47l\x1b[?25h\x1b[u"); // restore the screen and cursor location
	if (do_render) render(1);
    tcgetattr(fileno(stdin), &term);
    term.c_lflag |= LFLAGS; // add the LFLAGS
    tcsetattr(fileno(stdin), 0, &term);
	flush_all();
	system("stty sane"); // set to normal mode
}
void end_and_exit() {
	end(1);
	exit(0);
}
void tstp() {
	bell();
	end(0);
	raise(SIGSTOP); // raise SIGSTOP which cannot be caught and actually does the suspend
}
void cont() {
	bell();
	begin();
}

int main() {
	if (!isatty(fileno(stdout))) { fprintf(stderr, "stdout is not a tty\n"); return 9; }
	if (!isatty(fileno(stderr))) { fprintf(stderr, "stderr is not a tty\n"); return 9; }
	if (!isatty(fileno(stdin ))) { fprintf(stderr, "stdin is not a tty\n");  return 9; }
	rng = fopen(RANDOM_FILE, "r"); // r = open file for reading
	if (!rng) {
		fprintf(stderr, "%s: %s", RANDOM_FILE, strerror(errno)); // error if can't open RANDOM_FILE
		return 1;
	}
	create_pieces();
	begin();
	signal(SIGINT,  end_and_exit);
	signal(SIGHUP,  end_and_exit);
	signal(SIGQUIT, end_and_exit);
	signal(SIGKILL, end_and_exit); // cannot be caught but do it anyways
	signal(SIGTERM, end_and_exit);
	signal(SIGSEGV, end_and_exit);
	signal(SIGILL,  end_and_exit);
	signal(SIGTSTP, tstp); // suspend
	signal(SIGCONT, cont); // continue
	random_next_piece();
	add_next_piece();
	random_next_piece();
	move_indicator();
	bell();
	score_string = malloc(24);
	uint8_t ignore;
	while (1) {
		if (++move_timer >= MOVE_EVERY) {
			move_timer = 0;
			move_down();
		}
		ignore = 0;
		while (poll_()) {
			int c_ = fgetc(stdin);
			if (c_ == EOF) { // exit if EOF, otherwise poll_ will return true and will hang
				end(1);
				fprintf(stderr, "EOF\n");
				return 2;
			}
			char c = c_;
			if (c == '\x1a') { raise(SIGTSTP); } // ctrl+Z
			else if (c == '\x03' || c == '\x04') { end(1); return 0; } // ctrl+C ctrl+D
			else if (ignore);
			else if (c == '\x1b') ignore = 1; // ignore if escape code because ^[[A and ^[[D can be caught by WASD, too lazy to read the whole escape code for arrow keys
			else if (c == ' ') drop();
			else if (c == 'a' || c == 'A') { move(0, -1); move_indicator(); }
			else if (c == 'd' || c == 'D') { move(0,  1); move_indicator(); }
			else if (c == 's' || c == 'S') { move_down(); } // no need to move_indicator here
			else if (c == 'q' || c == 'Q') { rotate(-1); move_indicator(); }
			else if (c == 'w' || c == 'W') { rotate( 2); move_indicator(); }
			else if (c == 'e' || c == 'E') { rotate( 1); move_indicator(); }
			else ignore = 1;
		}
		render(0);
		if (game_over) {
			end(1);
			printf("Game Over!\n");
			if (fork() == 0) {
				for (uint8_t i = 0; i < 4; ++i) {
					if (i > 0) sleep_(150); // do beep pattern
					bell();
				}
				return 0;
			}
			return 0;
		}
		sleep_(DELAY);
	}
	// this shouldn't happen
	end(1);
	return 0;
}
