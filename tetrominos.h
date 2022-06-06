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
uint8_t piece_i[I_I][J_I] = {{1},{1},{1},{1}};
uint8_t piece_j[I_J][J_J] = {{0,1},{0,1},{1,1}};
uint8_t piece_l[I_L][J_L] = {{1,0},{1,0},{1,1}};
uint8_t piece_o[I_O][J_O] = {{1,1},{1,1}};
uint8_t piece_s[I_S][J_S] = {{1,0},{1,1},{0,1}};
uint8_t piece_t[I_T][J_T] = {{1,0},{1,1},{1,0}};
uint8_t piece_z[I_Z][J_Z] = {{0,1},{1,1},{1,0}};
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
