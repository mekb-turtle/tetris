#define I_I 4
#define J_I 1
#define I_J 3
#define J_J 2
#define I_L 3
#define J_L 2
#define I_O 2
#define J_O 2
#define I_S 3
#define J_S 2
#define I_T 3
#define J_T 2
#define I_Z 3
#define J_Z 2
uint8_t** piece_i;
uint8_t** piece_j;
uint8_t** piece_l;
uint8_t** piece_o;
uint8_t** piece_s;
uint8_t** piece_t;
uint8_t** piece_z;

void create_pieces() {
	piece_i = malloc(sizeof(uint8_t*)*I_I);
	piece_j = malloc(sizeof(uint8_t*)*I_J);
	piece_l = malloc(sizeof(uint8_t*)*I_L);
	piece_o = malloc(sizeof(uint8_t*)*I_O);
	piece_s = malloc(sizeof(uint8_t*)*I_S);
	piece_t = malloc(sizeof(uint8_t*)*I_T);
	piece_z = malloc(sizeof(uint8_t*)*I_Z);

	for (size_t i = 0; i < I_I; ++i) { piece_i[i] = malloc(sizeof(uint8_t)*J_I); }
	for (size_t i = 0; i < I_J; ++i) { piece_j[i] = malloc(sizeof(uint8_t)*J_J); }
	for (size_t i = 0; i < I_L; ++i) { piece_l[i] = malloc(sizeof(uint8_t)*J_L); }
	for (size_t i = 0; i < I_O; ++i) { piece_o[i] = malloc(sizeof(uint8_t)*J_O); }
	for (size_t i = 0; i < I_S; ++i) { piece_s[i] = malloc(sizeof(uint8_t)*J_S); }
	for (size_t i = 0; i < I_T; ++i) { piece_t[i] = malloc(sizeof(uint8_t)*J_T); }
	for (size_t i = 0; i < I_Z; ++i) { piece_z[i] = malloc(sizeof(uint8_t)*J_Z); }

	piece_i[0][0] = piece_i[1][0] = piece_i[2][0] = piece_i[3][0] = 1;

	piece_j[0][0] = piece_j[1][0] = 0;
	piece_j[0][1] = piece_j[1][1] = piece_j[2][1] = piece_j[2][0] = 1;

	piece_l[0][1] = piece_l[1][1] = 0;
	piece_l[0][0] = piece_l[1][0] = piece_l[2][0] = piece_l[2][1] = 1;

	piece_o[0][0] = piece_o[0][1] = piece_o[1][0] = piece_o[1][1] = 1;

	piece_s[0][1] = piece_s[2][0] = 0;
	piece_s[0][0] = piece_s[1][0] = piece_s[1][1] = piece_s[2][1] = 1;

	piece_t[0][1] = piece_t[2][1] = 0;
	piece_t[0][0] = piece_t[1][0] = piece_t[1][1] = piece_t[2][0] = 1;

	piece_z[0][0] = piece_z[2][1] = 0;
	piece_z[0][1] = piece_z[1][0] = piece_z[1][1] = piece_z[2][0] = 1;
}

#define COLOR_I "\x1b[38;5;14m"
#define COLOR_J "\x1b[38;5;12m"
#define COLOR_L "\x1b[38;5;3m"
#define COLOR_O "\x1b[38;5;11m"
#define COLOR_S "\x1b[38;5;10m"
#define COLOR_T "\x1b[38;5;13m"
#define COLOR_Z "\x1b[38;5;9m"

// do not put 0, it is treated as no block
#define ID_I 1
#define ID_J 2
#define ID_L 3
#define ID_O 4
#define ID_S 5
#define ID_T 6
#define ID_Z 7
