#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <fcntl.h>
#include <sys/poll.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#define GAME_W 10
#define GAME_H 30
uint8_t board[GAME_H][GAME_W];
uint8_t place[GAME_H][GAME_W];
uint8_t PIECE_I[][1] = {{1},{1},{1},{1}};
uint8_t PIECE_J[][2] = {{0,1},{0,1},{1,1}};
uint8_t PIECE_L[][2] = {{1,0},{1,0},{1,1}};
uint8_t PIECE_O[][2] = {{1,1},{1,1}};
uint8_t PIECE_S[][2] = {{1,0},{1,1},{0,1}};
uint8_t PIECE_T[][2] = {{1,0},{1,1},{1,0}};
uint8_t PIECE_Z[][2] = {{0,1},{1,1},{1,0}};
#define RESET "\x1b[0m"
#define COLOR_I "\x1b[48;5;14m"
#define COLOR_J "\x1b[48;5;12m"
#define COLOR_L "\x1b[48;5;3m"
#define COLOR_O "\x1b[48;5;11m"
#define COLOR_S "\x1b[48;5;10m"
#define COLOR_T "\x1b[48;5;13m"
#define COLOR_Z "\x1b[48;5;9m"
#define ID_I 1
#define ID_J 2
#define ID_L 3
#define ID_O 4
#define ID_S 5
#define ID_T 6
#define ID_Z 7
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
	for (size_t i = 0; i < GAME_H; ++i) {
		for (size_t j = 0; j < GAME_W; ++j) {
			printf(RESET);
			if (print_color(board[i][j]))
				print_color(place[i][j]);
			printf(" ");
		}
		printf("\n");
	}
}
void clear() {
	printf("\x1b[H\x1b[J\x1b[2J");
}
void logic() {
	board[0][2] = ID_J; // debugging
}
void sleep_(unsigned long ms) {
	struct timespec r = { ms/1e3, (ms%1000)*1e6 };
	nanosleep(&r, NULL);
}
unsigned char poll_() {
	struct pollfd fds;
	unsigned char ret;
	fds.fd = STDIN_FILENO;
	fds.events = POLLIN;
	ret = poll(&fds, 1, 0);
	return ret;
}
void step() {
	while (poll_()) {
		unsigned char c = getc(stdin);
	}
}
int main() {
	clear();
	logic();
	render();
	return 0;
}
