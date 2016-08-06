#include <stdlib.h> // rand
#include <math.h>
#include "nonsimple.h"
#include <stdio.h>
#include <string.h>

struct player {
    uint16_t y, x;
    int rockets;
    int speed;
} player[2];

uint8_t player_edge_palette[2] = {146, 148};
uint8_t player_palette[2] = {147, 149};

uint8_t ball(int vx, int vy) {
    uint8_t result = 192;
    if (vx >= 0) {
        if (vx > 3) {
            result |= (1<<3); // wobbly motion
        } else {
            result |= vx;
        }
    } else {
        if (vx < -3) {
            result |= (1<<3); // wobbly motion
        } else {
            result |= (-vx) | (1<<3);
        }
    }
    if (vy >= 0) {
        if (vy > 3) {
            result |= (1<<3)<<3; // wobbly motion
        } else {
            result |= vy<<3;
        }
    } else {
        if (vy < -3) {
            result |= (1<<3)<<3; // wobbly motion
        } else {
            result |= ((-vy) | (1<<3))<<3;
        }
    }
    return result;
}

void move_player_dy(int p, int dy) {
    if (dy < 0) { // move up
        if (player[p].y > 1) {
            superpixel[player[p].y][player[p].x] = player_palette[p];
            superpixel[--player[p].y][player[p].x] = player_edge_palette[p];
        }
    } 
    else { // move down
        if (player[p].y < Ny) {
            superpixel[player[p].y][player[p].x] = player_palette[p];
            superpixel[++player[p].y][player[p].x] = player_edge_palette[p];
        }
    }
}

void move_player_dx(int p, int dx) {
    if (dx < 0) { // move left
        if ((p==1) && (player[p].x <= Nx/2+1))
            return;
        if (player[p].x > 1) {
            superpixel[player[p].y][player[p].x] = player_palette[p];
            superpixel[player[p].y][--player[p].x] = player_edge_palette[p];
        }
    } 
    else { // move right 
        if ((p==0) && (player[p].x >= Nx/2-1))
            return;
        if (player[p].x < Nx) {
            superpixel[player[p].y][player[p].x] = player_palette[p];
            superpixel[player[p].y][++player[p].x] = player_edge_palette[p];
        }
    }
}

void player_init() {
    for (int p=0; p<2; ++p) {
        player[p].y = Ny/2;
        player[p].x = 1 + p*(Nx-1);
        superpixel[player[p].y][player[p].x] = player_edge_palette[p];
        player[p].speed = 2;
    }
    superpixel[Ny][Nx/2] = 145;
    superpixel[Ny-1][Nx/2] = 144;
    superpixel[Ny-2][Nx/2] = 144;
    superpixel[Ny-3][Nx/2] = 144;
    superpixel[Ny-4][Nx/2] = 144;
    superpixel[Ny-5][Nx/2] = 144;
    superpixel[Ny-6][Nx/2] = 144;
    superpixel[Ny-7][Nx/2] = 145;
}

void game_init()
{ 
    graph_not_ready = 1;
    graph_debug = 0;
    clear_screen();
    player_init();
    graph_line_callback = propagate;
    graph_not_ready = 0;
}

void game_frame()
{
    kbd_emulate_gamepad();

    if (GAMEPAD_PRESSED(0, X)) {
        if (vga_frame % player[1].speed == 0)
            move_player_dy(1, -1);
    }
    if (GAMEPAD_PRESSED(0, Y)) {
        if (vga_frame % player[1].speed == 0)
            move_player_dy(1, 1);
    }

    if (GAMEPAD_PRESSED(0, A)) { 
        superpixel[1][Nx/2] = 145;
        superpixel[2][Nx/2] = 144;
        superpixel[3][Nx/2] = 144;
        superpixel[4][Nx/2] = 144;
        superpixel[5][Nx/2] = 144;
        superpixel[6][Nx/2] = 144;
        superpixel[7][Nx/2] = 144;
        superpixel[8][Nx/2] = 145;
    }
    
    if (GAMEPAD_PRESSED(0, R))
    {
        wind_x = 1;
    }
    if (GAMEPAD_PRESSED(0, L))
    {
        wind_x = -1;
    }

    for (int p=0; p<2; ++p) {
        if (vga_frame % player[p].speed)
            continue;

        if (GAMEPAD_PRESSED(p, left))
            move_player_dx(p, -1);
        if (GAMEPAD_PRESSED(p, right))
            move_player_dx(p, 1);
        if (GAMEPAD_PRESSED(p, down))
            move_player_dy(p, 1);
        if (GAMEPAD_PRESSED(p, up))
            move_player_dy(p, -1);
        
        if (GAMEPAD_PRESSED(p, B))
        {
            //if (!(superpixel[Ny/2][Nx/2] & 128)) {
            //    superpixel[Ny/2][Nx/2] = 127;
            //}
            if ((vga_frame/player[p].speed)%player[p].speed == 0)
            {
                int ry = player[p].y, rx = player[p].x + 1 - 2*p;
                if (superpixel[ry][rx] < 128)
                    superpixel[ry][rx] = 160+16*p;
                else
                    superpixel[player[p].y][player[p].x] = 143;
            }
        }
    }
    if (GAMEPAD_PRESSED(0, select))
    {
    }
    
    if (GAMEPAD_PRESSED(0, start))
    {
    }
}
