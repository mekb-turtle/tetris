#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <signal.h>
#include <termios.h>
#define RANDOM_FILE "/dev/urandom"
#define DELAY 10
#define MOVE_EVERY 30
#define GAME_I 20
#define GAME_J 10
#define BELL // comment out if you don't want a bell
uint8_t game_over = 0;
FILE* rng;
uint8_t place_indicator[GAME_I][GAME_J];
uint8_t board[GAME_I][GAME_J];
uint8_t place[GAME_I][GAME_J];
#include "./tetrominos.h"
#define COLOR_BORDER "\x1b[38;5;8m"
#define RESET "\x1b[0m"
#define BOARD_CHAR "██"
#define PLACE_CHAR "██"
#define INDICATOR_CHAR "▒▒"
#define BORDER_CHAR "▒▒"
#define EMPTY_CHAR "  "
#define BORDER RESET COLOR_BORDER BORDER_CHAR
uint8_t print_color(uint8_t col) {
	if      (col == ID_I) printf(COLOR_I);
	else if (col == ID_J) printf(COLOR_J);
	else if (col == ID_L) printf(COLOR_L);
	else if (col == ID_O) printf(COLOR_O);
	else if (col == ID_S) printf(COLOR_S);
	else if (col == ID_T) printf(COLOR_T);
	else if (col == ID_Z) printf(COLOR_Z);
	else return 0;
	return 1;
}
void render() {
	printf("\x1b[H");
	for     (uint8_t j = 0; j < GAME_J+2; ++j) printf(BORDER);
	printf("\r\n");
	for     (uint8_t i = 0; i < GAME_I; ++i) {
		printf(BORDER RESET);
		for (uint8_t j = 0; j < GAME_J; ++j) {
			if (print_color(board[i][j])) {
				printf(BOARD_CHAR);
			} else if (print_color(place[i][j])) {
				printf(PLACE_CHAR);
			} else if (print_color(place_indicator[i][j])) {
				printf(INDICATOR_CHAR);
			} else {
				printf(EMPTY_CHAR);
			}
		}
		printf(BORDER "\r\n");
	}
	for     (uint8_t j = 0; j < GAME_J+2; ++j) printf(BORDER);
	printf("\r\n\r\n" RESET "\
\x1b[7mA\x1b[27m to move left\r\n\
\x1b[7mD\x1b[27m to move right\r\n\
\x1b[7mS\x1b[27m to move down quicker\r\n\
\x1b[7mQ\x1b[27m to rotate anti-clockwise\r\n\
\x1b[7mE\x1b[27m to rotate clockwise\r\n\
\x1b[7mSpace\x1b[27m to drop instantly\r\n\
");
}
uint8_t is_placing() {
	for     (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			if (place[i][j] != 0) return 1;
	return 0;
}
void add_block(uint8_t id, uint8_t** piece, uint8_t I, uint8_t J) {
	uint8_t i_ = 0;
	uint8_t j_ = (GAME_J-J)/2;
	for     (uint8_t i = 0; i < I; ++i)
		for (uint8_t j = 0; j < J; ++j)
			if (piece[i][j]) {
				if (board[i_+i][j_+j])
					game_over = 1;
				place[i_+i][j_+j] = id;
			}
}
void add_random_block() {
	uint8_t t = getc(rng) % 7;
	add_block(
		t==0?ID_I:t==1?ID_J:t==2?ID_L:t==3?ID_O:t==4?ID_S:t==5?ID_T:ID_Z,
		t==0?piece_i:t==1?piece_j:t==2?piece_l:t==3?piece_o:t==4?piece_s:t==5?piece_t:piece_z,
		t==0?I_I:t==1?I_J:t==2?I_L:t==3?I_O:t==4?I_S:t==5?I_T:I_Z,
		t==0?J_I:t==1?J_J:t==2?J_L:t==3?J_O:t==4?J_S:t==5?J_T:J_Z
	);
}
void sleep_(unsigned long ms) {
	struct timespec r = { ms/1e3, (ms%1000)*1e6 };
	nanosleep(&r, NULL);
}
unsigned char poll_() {
	struct pollfd fds;
	unsigned char ret;
	fds.fd = fileno(stdin);
	fds.events = POLLIN;
	ret = poll(&fds, 1, 0);
	return ret;
}
uint8_t move(int8_t move_i, int8_t move_j) {
	if (move_i == 0 && move_j == 0) return 1;
	if (move_i < -1 || move_i > 1 || move_j < -1 || move_j > 1) return 0;
	for     (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			if (place[i][j]) {
				if (move_i < 0 && i == 0) return 0;
				if (move_i > 0 && i >= GAME_I-1) return 0;
				if (move_j < 0 && j == 0) return 0;
				if (move_j > 0 && j >= GAME_J-1) return 0;
				if (board[i+move_i][j+move_j]) return 0;
			}
	for     (uint8_t i = move_i >= 0 ? GAME_I - 1 : 0; move_i >= 0 ? (i >= 0 && i != UINT8_MAX) : i < GAME_I; i += move_i >= 0 ? -1 : 1)
		for (uint8_t j = move_j >= 0 ? GAME_J - 1 : 0; move_j >= 0 ? (j >= 0 && j != UINT8_MAX) : j < GAME_J; j += move_j >= 0 ? -1 : 1)
			if (place[i][j]) {
				place[i+move_i][j+move_j] = place[i][j];
				place[i][j] = 0;
			}
	return 1;
}
uint8_t move_indicator() {
	for     (uint8_t i = 0; i < GAME_I; ++i)
		for (uint8_t j = 0; j < GAME_J; ++j)
			place_indicator[i][j] = place[i][j];
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
uint8_t place_to_board() {
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
uint8_t move_timer = 0;
void clear() {
	printf("\x1b[H\x1b[J\x1b[2J\x1b[0m");
}
void begin() {
	system("stty raw");
	printf("\x1b[s\x1b[?47h\x1b[?25l");
	clear();
    struct termios term;
    tcgetattr(fileno(stdin), &term);
    term.c_lflag &= ~(ECHO|ISIG);
    tcsetattr(fileno(stdin), 0, &term);
}
void end() {
    struct termios term;
    tcgetattr(fileno(stdin), &term);
    term.c_lflag |= ECHO|ISIG;
    tcsetattr(fileno(stdin), 0, &term);
	clear();
	printf("\x1b[?47l\x1b[?25h\x1b[u");
	system("stty sane");
}
void end_and_exit() {
	end();
	exit(0);
}
void bell() {
#ifdef BELL
	printf("\a");
#endif
}
int main() {
	rng = fopen(RANDOM_FILE, "r");
	if (!rng) {
		fprintf(stderr, "%s: %s", RANDOM_FILE, strerror(errno));
		return 1;
	}
	create_pieces();
	begin();
	signal(SIGINT,  end_and_exit);
	signal(SIGHUP,  end_and_exit);
	signal(SIGQUIT, end_and_exit);
	signal(SIGKILL, end_and_exit);
	signal(SIGTERM, end_and_exit);
	signal(SIGSTOP, end_and_exit);
	signal(SIGSEGV, end_and_exit);
	signal(SIGILL,  end_and_exit);
	add_random_block();
	move_indicator();
	bell();
	while (1) {
		if (++move_timer >= MOVE_EVERY) {
			move_timer = 0;
			if (!move(1, 0)) {
				bell();
				place_to_board();
				//add_random_block();
				move_indicator();
			}
		}
		while (poll_()) {
			int c_ = fgetc(stdin);
			if (c_ == EOF) {
				end();
				clear();
				fprintf(stderr, "EOF\n");
				return 2;
			}
			char c = c_;
			if (c == '\x1a' || c == '\x03' || c == '\x04') { end(); return 0; }
			else if (c == '\x1b') break;
			else if (c == 'a' || c == 'A') { move(0, -1); move_indicator(); }
			else if (c == 'd' || c == 'D') { move(0,  1); move_indicator(); }
			else if (c == 's' || c == 'S') { move(1,  0); }
		}
		render();
		if (game_over) {
			bell();
			end();
			clear();
			printf("Game Over!\n");
			return 0;
		}
		sleep_(DELAY);
	}
	end();
	return 0;
}
