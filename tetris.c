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
#include <GL/glut.h>
#include <math.h>

// 3d constants
#define ROTATE_X_DEG 15
#define ROTATE_Y_DEG 15
#define ZOOM_MAX_DEG 5.0f
#define ZOOM_MIN_DEG 0.005f
#define ZOOM_DEG 1.2f
#define MOVE 0.1f
#define MOVE_MAX 1.0f

// game constants
#define MOVE_EVERY 300 // how many ms to move down by one
#define GAME_I 20 // board height
#define GAME_J 10 // board width

// pieces
#include "./tetrominos.h"
#define RANDOM_FILE "/dev/urandom" // file to read random bytes from for rng
//#define FORCE_PIECE ID_I // uncomment and set to ID_<piece> to force a piece to spawn instead of reading from RANDOM_FILE

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
FILE* rng;

// 3d variables
uint8_t lighting = 0;
float zoom;
float rotateX;
float rotateY;
float moveX;
float moveY;
int w, h, s, ow, oh;

void sleep_(unsigned long ms) {
	struct timespec r = { ms/1e3, (ms%1000)*1e6 }; // 1 millisecond = 1e6 nanoseonds
	nanosleep(&r, NULL);
}

char* score_string;
size_t score_len;

void cube(float r, float g, float b, float a, float x, float y, float z) {
	glColor4f(r, g, b, a);
	glBegin(GL_POLYGON);
	glVertex3f(x+0.0f, y+0.0f, z+0.0f);
	glVertex3f(x+0.0f, y+1.0f, z+0.0f);
	glVertex3f(x+1.0f, y+1.0f, z+0.0f);
	glVertex3f(x+1.0f, y+0.0f, z+0.0f);
	glEnd();
	glBegin(GL_POLYGON);
	glVertex3f(x+0.0f, y+0.0f, z+1.0f);
	glVertex3f(x+0.0f, y+1.0f, z+1.0f);
	glVertex3f(x+1.0f, y+1.0f, z+1.0f);
	glVertex3f(x+1.0f, y+0.0f, z+1.0f);
	glEnd();
	glBegin(GL_POLYGON);
	glVertex3f(x+0.0f, y+0.0f, z+0.0f);
	glVertex3f(x+0.0f, y+0.0f, z+1.0f);
	glVertex3f(x+1.0f, y+0.0f, z+1.0f);
	glVertex3f(x+1.0f, y+0.0f, z+0.0f);
	glEnd();
	glBegin(GL_POLYGON);
	glVertex3f(x+0.0f, y+1.0f, z+0.0f);
	glVertex3f(x+0.0f, y+1.0f, z+1.0f);
	glVertex3f(x+1.0f, y+1.0f, z+1.0f);
	glVertex3f(x+1.0f, y+1.0f, z+0.0f);
	glEnd();
	glBegin(GL_POLYGON);
	glVertex3f(x+0.0f, y+0.0f, z+0.0f);
	glVertex3f(x+0.0f, y+0.0f, z+1.0f);
	glVertex3f(x+0.0f, y+1.0f, z+1.0f);
	glVertex3f(x+0.0f, y+1.0f, z+0.0f);
	glEnd();
	glBegin(GL_POLYGON);
	glVertex3f(x+1.0f, y+0.0f, z+0.0f);
	glVertex3f(x+1.0f, y+0.0f, z+1.0f);
	glVertex3f(x+1.0f, y+1.0f, z+1.0f);
	glVertex3f(x+1.0f, y+1.0f, z+0.0f);
	glEnd();
}

void render() {
	w  = glutGet(GLUT_WINDOW_WIDTH);
	h  = glutGet(GLUT_WINDOW_HEIGHT);
	s  = w > h ? h : w;
	ow = w > h ? (w - s) / 2 : 0;
	oh = w < h ? (h - s) / 2 : 0;
	glViewport(ow, oh, s, s);
	glClearColor(0.0, 0.0, 0.0, 1.0);
	glPointSize(1.0);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_DEPTH_TEST);
	glPushMatrix();
	glTranslatef(moveX, moveY, 0);
	glScalef(zoom, zoom, zoom);
	glRotatef(rotateY, 1.0f, 0.0f, 0.0f);
	glRotatef(rotateX, 0.0f, 1.0f, 0.0f);
	glTranslatef((float)GAME_J/-2.0f, (float)GAME_I/2.0f-1.0f, 0.f);
	if (score == 0) sprintf(score_string, " 0 %c", '\0');
	else sprintf(score_string, " %li00 %c", score, '\0');
	score_len = strlen(score_string);
	for (uint8_t i = 0; i < GAME_I; ++i) {
		for (uint8_t j = 0; j < GAME_J; ++j) {
			float r;
			float g;
			float b;
			float a = 1.0f;
			if      (get_color(board          [i][j], &r, &g, &b));
			else if (get_color(place          [i][j], &r, &g, &b));
			else if (get_color(place_indicator[i][j], &r, &g, &b)) a = 0.5f;
			else continue;
			cube(r, g, b, a, j, -i, 0);
		}
	}
	glPopMatrix();
	glFlush();
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
		t_ = fgetc(rng);
		if (t_ == EOF) {
			fprintf(stderr, "RNG file has ended\n");
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
	while (move_down());
}
void reset() {
	random_next_piece();
	add_next_piece();
	random_next_piece();
	memset(board, 0, sizeof(place));
	memset(place, 0, sizeof(place));
	move_indicator();
	game_over = 0;
	score = 0;
	move_down();
}

void updateLighting(uint8_t l) {
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHT0);
	glLightfv(GL_LIGHT0, GL_AMBIENT,  (float[]){0.0f,0.0f,0.0f,1.0f});
	glLightfv(GL_LIGHT0, GL_DIFFUSE,  (float[]){1.0f,1.0f,1.0f,1.0f});
	glLightfv(GL_LIGHT0, GL_SPECULAR, (float[]){1.0f,1.0f,1.0f,1.0f});
	glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
	if (l) {
		glEnable (GL_LIGHTING);
	} else {
		glDisable(GL_LIGHTING);
	}
	glLightfv(GL_LIGHT0, GL_POSITION, (float[]){0.0f,0.0f,0.0f,1.0f});
}
void updateRotate() {
	while (rotateX >= 360) rotateX -= 360;
	while (rotateX <  0)   rotateX += 360;
	while (rotateY >= 180) rotateY -= 360;
	while (rotateY < -180) rotateY += 360;
}
void updateMove() {
	if (moveX > +MOVE_MAX) moveX = +MOVE_MAX;
	if (moveX < -MOVE_MAX) moveX = -MOVE_MAX;
	if (moveY > +MOVE_MAX) moveY = +MOVE_MAX;
	if (moveY < -MOVE_MAX) moveY = -MOVE_MAX;
}
void zoomIn() {
	if (zoom < ZOOM_MAX_DEG) zoom *= ZOOM_DEG;
}
void zoomOut() {
	if (zoom > ZOOM_MIN_DEG) zoom /= ZOOM_DEG;
}
void resetView() {
	zoom = 0.1f;
	rotateX = 0.0f;
	rotateY = 0.0f;
	moveX = 0.0f;
	moveY = 0.0f;
}
void special(int key, int x, int y) {
	switch (key) {
		case GLUT_KEY_LEFT:  rotateX += ROTATE_X_DEG; break;
		case GLUT_KEY_RIGHT: rotateX -= ROTATE_X_DEG; break;
		case GLUT_KEY_UP:    rotateY += ROTATE_Y_DEG; break;
		case GLUT_KEY_DOWN:  rotateY -= ROTATE_Y_DEG; break;
		default: return;
	}
	updateRotate();
	glutPostRedisplay();
}
void keyboard(unsigned char key, int x, int y) {
	switch (key) {
		case ' ': drop(); break;
		case 'a': case 'A': if (game_over) break; move(0, -1); move_indicator(); break;
		case 'd': case 'D': if (game_over) break; move(0,  1); move_indicator(); break;
		case 's': case 'S': if (game_over) break; move_down(); break;
		case 'q': case 'Q': if (game_over) break; rotate(-1);  move_indicator();break;
		case 'w': case 'W': if (game_over) break; rotate( 2);  move_indicator(); break;
		case 'e': case 'E': if (game_over) break; rotate( 1);  move_indicator(); break;
		case 'i': case 'I': zoomIn();  break;
		case 'o': case 'O': zoomOut(); break;
		case 'm': case 'M': exit(0); return;
		case 'n': case 'N': reset(); break;
		case 'r': case 'R': break;
		case 't': case 'T': resetView(); break;
		case 'l': case 'L': updateLighting(lighting = !lighting); break;
		default: return;
	}
	updateMove();
	glutPostRedisplay();
}
int ix, iy;
uint8_t down0;
uint8_t down1;
uint8_t down2;
void mouse(int button, int up, int x, int y) {
	ix = x; iy = y;
	down0 = down1 = down2 = 0;
	if (button == 0) down0 = !up;
	if (button == 1) down1 = !up;
	if (button == 2) down2 = !up;
	if (button == 3) { zoomIn();  glutPostRedisplay(); }
	if (button == 4) { zoomOut(); glutPostRedisplay(); }
}
void motion(int x, int y) {
	int dx = x-ix,
	    dy = y-iy;
	if (down2) {
		moveX += ((float)dx)/s;
		moveY -= ((float)dy)/s;
		updateMove();
		glutPostRedisplay();
	} else if (down0) {
		rotateX -= ((float)dx)/s*180;
		rotateY -= ((float)dy)/s*180;
		updateRotate();
		glutPostRedisplay();
	}
	ix = x; iy = y;
}
void timer(int bla) {
	move_down();
}

int main(int argc, char* argv[]) {
	if (argc > 1) {
		printf("\
Usage: %s\n\n\
Controls:\n\
 Arrow keys or drag left click to rotate\n\
 A to move left\n\
 D to move right\n\
 S to move down quicker\n\
 Q to rotate -90°\n\
 W to rotate 180°\n\
 E to rotate 90°\n\
 Space to drop down instantly\n\
 I,O or scroll wheel to zoom in/out\n\
 M to quit\n\
 N to reset game\n\
 R to re-render\n\
 T to reset view\n\
 L to toggle lighting\n\
", argv[0]);
		return 2;
	}
#ifndef FORCE_PIECE
	rng = fopen(RANDOM_FILE, "r"); // r = open file for reading
	if (!rng) {
		fprintf(stderr, "%s: %s", RANDOM_FILE, strerror(errno)); // error if can't open RANDOM_FILE
		return 1;
	}
#endif
	create_pieces();
	score_string = malloc(24);
	reset();
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
	glutCreateWindow("Tetris");
	updateLighting(lighting);
	resetView();
	glutDisplayFunc(render);
	glutSpecialFunc(special);
	glutKeyboardFunc(keyboard);
	glutTimerFunc(MOVE_EVERY, timer, 0);
	glutMouseFunc(mouse);
	glutMotionFunc(motion);
	glutMainLoop();
	return 0;
}
