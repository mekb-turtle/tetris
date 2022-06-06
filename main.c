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
#include "./tetrominos.h"
#define RESET "\x1b[0m"
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
uint8_t is_placing() {
	for (size_t i = 0; i < GAME_H; ++i) {
		for (size_t j = 0; j < GAME_W; ++j) {
			if (place[i][j] != 0) return 1;
		}
	}
	return 0;
}
uint8_t place_block(uint8_t id, uint8_t** piece, uint8_t I, uint8_t J) {
	if (is_placing()) return 1;
	size_t i_ = 0;
	size_t j_ = (GAME_W-J)/2;
	for (size_t i = 0; i < I; ++i) {
		for (size_t j = 0; j < J; ++j) {
			if (piece[i][j])
				place[i_+i][j_+j] = id;
		}
	}
	return 0;
}
uint8_t place_random_block() {
	uint8_t t = rand() % 7;
	place_block(
		t==0?ID_I:t==1?ID_J:t==2?ID_L:t==3?ID_O:t==4?ID_S:t==5?ID_T:ID_Z,
		t==0?piece_i:t==1?piece_j:t==2?piece_l:t==3?piece_o:t==4?piece_s:t==5?piece_t:piece_z,
		t==0?I_I:t==1?I_J:t==2?I_L:t==3?I_O:t==4?I_S:t==5?I_T:I_Z,
		t==0?J_I:t==1?J_J:t==2?J_L:t==3?J_O:t==4?J_S:t==5?J_T:J_Z
	);
}
void clear() {
	printf("\x1b[H\x1b[J\x1b[2J");
}
void logic() {
	
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
	place_random_block();
	clear();
	logic();
	render();
	return 0;
}
