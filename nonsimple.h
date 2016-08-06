#ifndef NONSIMPLE_H
#define NONSIMPLE_H
#include "bitbox.h"

#define SCREEN_W 320 // number of regular pixels
#define SCREEN_H 240
#define Nx 80
#define Ny 60

typedef void (void_fn)(void);

void clear_screen();

extern uint8_t superpixel[Ny+2][Nx+2];
extern int graph_debug;
extern uint8_t graph_not_ready;
extern int wind_y, wind_x;

extern void_fn* graph_line_callback;

void propagate();

#endif
