#include <stdlib.h> // rand
#include <math.h>
#include "nonsimple.h"
#include <stdio.h>
#include <string.h>

struct player {
    int position;
    int up;
    int down;
    int rockets;
    int speed;
} player[2];

uint8_t player_palette[2] = {146, 148};
uint8_t player_edge_palette[2] = {147, 149};

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

void move_player_relative(int p, int y) {
    if (y < 0) { // move up
        if (player[p].position > player[p].up+1) {
            int px = p*(Nx-1) + 1;
            superpixel[player[p].position+player[p].down][px] = 0;
            superpixel[player[p].position-player[p].up][px] = player_palette[p];
            --player[p].position;
            superpixel[player[p].position+player[p].down][px] = player_edge_palette[p];
            superpixel[player[p].position-player[p].up][px] = player_edge_palette[p];
        }
    } 
    else { // move down
        if (player[p].position + player[p].down < Ny) {
            int px = p*(Nx-1) + 1;
            superpixel[player[p].position+player[p].down][px] = player_palette[p];
            superpixel[player[p].position-player[p].up][px] = 0;
            ++player[p].position;
            superpixel[player[p].position+player[p].down][px] = player_edge_palette[p];
            superpixel[player[p].position-player[p].up][px] = player_edge_palette[p];
        }
    }
}

void player_init() {
    for (int p=0; p<2; ++p) {
        int i = 1 + p*(Nx-1);
        player[p].position = Ny/2;
        player[p].up = 5;
        player[p].down = 6;
        int j=player[p].position-player[p].up;
        superpixel[j][i] = player_edge_palette[p];
        ++j;
        for (; j<player[p].position+player[p].down; ++j) {
            superpixel[j][i] = player_palette[p];
        }
        superpixel[j][i] = player_edge_palette[p];
        player[p].speed = 2;
    }
}

void game_init()
{ 
    graph_debug = 0;
    clear_screen();
    player_init();
    graph_line_callback = propagate;
}

void game_frame()
{
    kbd_emulate_gamepad();

    if (GAMEPAD_PRESSED(0, X))
    {
        if (vga_frame % player[1].speed == 0)
            move_player_relative(1, -1);
    }
    if (GAMEPAD_PRESSED(0, Y))
    {
        if (vga_frame % player[1].speed == 0)
            move_player_relative(1, 1);
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
        //if (GAMEPAD_PRESSED(p, left))
        //{
        //}
        //else if (GAMEPAD_PRESSED(p, right))
        //{
        //}
        if (GAMEPAD_PRESSED(p, down))
        {
            if (vga_frame % player[p].speed == 0)
                move_player_relative(p, 1);
        }
        else if (GAMEPAD_PRESSED(p, up))
        {
            if (vga_frame % player[p].speed == 0)
                move_player_relative(p, -1);
        }
        if (GAMEPAD_PRESSED(p, B))
        {
            //if (!(superpixel[Ny/2][Nx/2] & 128)) {
            //    superpixel[Ny/2][Nx/2] = 127;
            //}
            int ry = player[p].position, rx = 2 + (Nx-3)*p;
            if (superpixel[ry][rx] < 128 && (vga_frame % 4 == 0))
                superpixel[ry][rx] = 160+16*p;
        }
    }
    if (GAMEPAD_PRESSED(0, select))
    {
    }
    
    if (GAMEPAD_PRESSED(0, start))
    {
    }
}
