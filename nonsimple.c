#include "nonsimple.h"

#include <bitbox.h>
#include <stdlib.h> // abs
#include <string.h> // memset

void_fn* graph_line_callback;

uint8_t superpixel[Ny+2][Nx+2] CCM_MEMORY;
uint8_t super_row_0[Nx+2] CCM_MEMORY;
uint8_t super_row_1[Nx+2] CCM_MEMORY;
uint8_t super_row_2[Nx+2] CCM_MEMORY;
uint8_t *super_above, *super_row, *super_below;

int wind_y, wind_x;

uint16_t palette[256] CCM_MEMORY;

int graph_debug;

void graph_frame() 
{
    super_above = super_row_0;
    super_row = super_row_1;
    super_below = super_row_2;

    // modify superpixel directly, use super_row/above/below to calculate
    // what superpixel should be, but don't modify them (apart from these
    // copies).
    memcpy(super_above, superpixel[0], (Nx+2));
    memcpy(super_row, superpixel[1], (Nx+2));
    memcpy(super_below, superpixel[2], (Nx+2));
}

void graph_line() 
{
    if (vga_odd)
        return;
    
    int j = vga_line/4+1;
    if (vga_line % 4 < 2) 
    {
        uint32_t *dst = (uint32_t *)draw_buffer;
        for (int i=1; i<=Nx; ++i) 
        {
            uint32_t color = palette[super_row[i]];
            color |= color<<16;
            *dst++ = color;
            *dst++ = color;
        }
    } 
    else if (vga_line % 4 == 2)
    {
        // keep superpixel row fixed, work off of that to determine what 
        // super row should be.
        if (graph_line_callback) 
        {
            graph_line_callback();
        }
    }
    else
    {
        if (j < Ny) 
        {
            // move around the row pointers to avoid copying all rows again:
            uint8_t *intermediate = super_above;
            super_above = super_row;
            super_row = super_below;
            super_below = intermediate;
            
            // copy in the new row (bottom row):
            memcpy(super_below, superpixel[j+2], (Nx+2));
            // superpixel[Ny+1] is the last row 
            // superpixel[3] is the first row needed to be copied in
        }
    }
}

void clear_screen() 
{
    // set top and bottom to indestructible (144), center to zero:
    memset(superpixel[0], 144, Nx+2);
    memset(superpixel[1], 0, (Nx+2)*Ny);
    memset(superpixel[Ny+1], 144, Nx+2);

    // from 0 to 127, make rocket clouds / flame:
    palette[0] = 0;
    for (int i=1; i<4; ++i) {
        palette[i] = ((1+i*2+rand()%2)<<10)|((1+i*2+rand()%2)<<5)|(1+i*2+rand()%2); // grayscale cloud
    }
    for (int i=4; i<8; ++i) {
        palette[i] = ((i*4+rand()%4)<<10)|((i*4+rand()%4)<<5)|(i*4+rand()%4); // grayscale cloud
    }
    for (int i=8; i<20; ++i) {
        uint8_t gray = 10+rand()%16;
        palette[i] = ((gray+rand()%6)<<10)|((gray + rand()%6)<<5)|(gray+rand()%6); // grayscale cloud
    }
    for (int i=20; i<30; ++i) { 
        // yellow
        palette[i] = (31<<10)|((31-rand()%2)<<5)|(rand()%4);
    }
    for (int i=30; i<50; ++i) {
        // orange
        palette[i] = (31<<10)|((21+rand()%4)<<5)|(rand()%4);
    }
    for (int i=50; i<127; ++i) {
        uint8_t gray = 10+rand()%16;
        palette[i] = ((gray+rand()%6)<<10)|((gray + rand()%6)<<5)|(gray+rand()%6); // grayscale cloud
    }
    palette[127] = 31<<10; // bright red flame 
    // 128 to 143 --> explosive material, expands in any direction, destroys destructible stuff
    for (int i=128; i<144; ++i) {
        // orange / red / yellow
        palette[i] = ((31-rand()%4)<<10)|((21+rand()%8)<<5)|(rand()%4);
    }
    // 144 on up don't fade, but may do other things
    palette[144] = (31<<10)|(31<<5)|31; // indestructible white terrain
    palette[145] = (21<<10)|(21<<5)|21; // indestructible gray terrain

    palette[146] = (31<<10); // player 0
    palette[147] = (20<<10); // player 0 edge
    palette[148] = (5<<5)|(31); // player 1
    palette[149] = (4<<5)|(21); // player 1 edge 
    palette[150] = (25<<10)|(29<<5); // purple walls
    palette[151] = (21<<10)|(25<<5); // dark purple walls
    palette[152] = (31<<10)|(20<<5); // orange walls
    palette[153] = (23<<10)|(15<<5); // dark orange walls
    palette[154] = (31<<5)|20; // sea green walls 
    palette[155] = (23<<10)|15; // dark sea green walls

    // still some stuff you could fit in between 156 and 157

    palette[158] = (31<<5); // plants, destructible
    palette[159] = (20<<5); // darker plants, destructible

    // player 0 rocket from 160 to 175, with 4 bits directional info,
    // player 1 rocket from 176 to 191, with 4 bits directional info:
    // 0000 -> moving slow forward
    // 0001 -> moving fast forward
    // 0010 -> moving slow down
    // 0100 -> moving fast down
    // 0110 -> moving very fast down
    // 1010 -> moving slow up
    // 1100 -> moving fast up
    // 1110 -> moving very fast up
    // 1000 -> "negative zero" for up/down, moves somewhat randomly
    for (int i=160; i<176; ++i) {
        palette[i] = (20<<10)|(20<<5)|20; 
    }
    for (int i=176; i<192; ++i) {
        palette[i] = (28<<10)|(28<<5)|28;
    }

    // 192 to 192+63=255 are balls.
    // can check a pixel for being a ball by (pixel&((1<<7)|(1<<6)))
    // 3 bits for vertical movement, 3 bits for horizontal.
    // sign bit and 2 magnitude bits:
    // 000 -> zero, no movement in this component
    // 100 -> "negative zero" - random movement in this component
    // 001 -> positive, slow movement
    // 010 -> positive, more movement
    // 011 -> positive, fastest movement
    // 101 -> negative, slow movement
    // 110 -> negative, more movement
    // 111 -> negative, fastest movement
    for (int i=192; i<256; ++i) {
        palette[i] = (31<<10)|(31<<5)|31;
    }
}

void propagate() {
    // modify superpixel directly, use super_row/above/below to calculate
    // what superpixel should be, but don't modify them.
    int j = vga_line/4+1;
    for (int i=1+(vga_frame+j)%2; i<=Nx; i+=2) {
        if (super_row[i] <= 128) {
            // permeable stuff; clouds and what not.
            int cloud = super_row[i]; // default value, if nothing else is moving in
            
            // check if there are any balls coming in. 
            int explosion = 0; // value which will override cloud, if necessary
            int plants = 0;
            int rocket = 0;

            // from left
            if (super_row[i-1] <= 128) {
                cloud += super_row[i-1]/2;
            } 
            else if (super_row[i-1] < 144) {
                // explosions 
                explosion += super_row[i-1] - 128;
            }
            else if (super_row[i-1] < 158) {} // walls
            else if ((super_row[i-1] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_row[i-1] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 (and higher stuff, filtered above)
                // rocket
                if (super_row[i-1] < 176) {
                    // player 0 rocket, going to the right
                    rocket = 160;
                    superpixel[j][i-1] = 127;
                }
            }
            else {
                // plants; 158 and 159
                if ((vga_frame/2) % 2)
                    plants += super_row[i-1]-157;
            }
            
            // from right
            if (super_row[i+1] <= 128) {
                cloud += super_row[i+1]/2;
            } 
            else if (super_row[i+1] < 144) {
                // explosion
                explosion += super_row[i+1] - 128;
            } 
            else if (super_row[i+1] < 158) {} // immovable stuff, don't bother updating current spot
            else if ((super_row[i+1] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_row[i+1] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 (and higher stuff, filtered above)
                // rocket
                if (super_row[i+1] >= 176) {
                    // player 1 rocket, going to the left
                    if (rocket) {
                        explosion = 300;
                        rocket = 0;
                    }
                    else {
                        superpixel[j][i+1] = 127;
                        rocket = 176;
                    }
                }
            }
            else {
                // plants; 158 and 159
                if ((vga_frame/2) % 2)
                    plants += super_row[i+1]-157;
            }
            
            // from above
            if (super_above[i] <= 128) {
                cloud += super_above[i]/2;
            } 
            else if (super_above[i] < 144) {
                // explosion
                explosion += super_above[i] - 128;
            } 
            else if (super_above[i] < 158) {} // immovable stuff, don't bother updating current spot
            else if ((super_above[i] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_above[i] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 (and higher stuff, filtered above)
                // rocket
            }
            else {
                // plants; 158 and 159
                if ((vga_frame/2) % 2)
                    plants += super_above[i]-157;
            } 
            
            // from below
            if (super_below[i] <= 128) {
                cloud += super_below[i]/2;
            } 
            else if (super_below[i] < 144) {
                // explosion
                explosion += super_below[i] - 128;
            } 
            else if (super_below[i] < 158) {} // immovable stuff, don't bother updating current spot
            else if ((super_below[i] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_below[i] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 
                // rocket
                
            }
            else {
                // plants; 158 and 159
                if ((vga_frame/2) % 2)
                    plants += super_below[i]-157;
            } 
            

            if (explosion) {
                if (rocket || explosion/4 > 15) {
                    superpixel[j][i] = 143;
                }
                else if (explosion == 1) { 
                    // create some smoke
                    superpixel[j][i] = 127;
                }
                else {
                    superpixel[j][i] = 128 + explosion/4;
                }
            }
            else if (rocket) {
                superpixel[j][i] = rocket;
            }
            else {
                superpixel[j][i] = cloud/4;
            }
        }
        else if (super_row[i] < 144) {
            // explosion happening here
            // TODO:
            // check if there are any balls coming in, reverse them
            int explosion = super_row[i] - 1;
            // from left
            if ((super_row[i-1] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_row[i-1] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 (and higher stuff, filtered above)
                // rocket
                // player 0 rocket, going to the right
                explosion += 16;
                superpixel[j][i-1] = 143; // explode this guy, too
            }
            
            // from right
            if ((super_row[i+1] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_row[i+1] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 (and higher stuff, filtered above)
                // rocket
                // player 1 rocket, going to the left
                explosion += 16;
                superpixel[j][i+1] = 143; // explode this guy, too
            }
            
            if (explosion > 143) {
                // explode the warhead
                superpixel[j][i] = 143;
            }
            else {
                superpixel[j][i] = explosion;
            }
        }
        else if (super_row[i] < 158) {
            // player walls, etc. 
            
            // check if there are any balls coming in. 
            int explosion = 0; // or explosions.
            int direction = 0; // direction of incoming rocket

            // from left
            if (super_row[i-1] <= 128) {} 
            else if (super_row[i-1] < 144) {
                // explosions 
                explosion += super_row[i-1] - 128;
            }
            else if (super_row[i-1] < 158) {} // walls
            else if ((super_row[i-1] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_row[i-1] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 (and higher stuff, filtered above)
                // rocket
                if (super_row[i-1] < 176) {
                    // player 0 rocket, going to the right
                    explosion += 300;
                    superpixel[j][i-1] = 127;
                    direction |= 1;
                }
            }
            
            // from right
            if (super_row[i+1] <= 128) {} 
            else if (super_row[i+1] < 144) {
                // explosion
                explosion += super_row[i+1] - 128;
            } 
            else if (super_row[i+1] < 158) {} // immovable stuff, don't bother updating current spot
            else if ((super_row[i+1] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_row[i+1] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 (and higher stuff, filtered above)
                // rocket
                if (super_row[i+1] >= 176) {
                    // player 1 rocket, going to the left
                    explosion += 300;
                    superpixel[j][i+1] = 127;
                    direction |= 2;
                }
            }
            
            // from above
            if (super_above[i] <= 128) {} 
            else if (super_above[i] < 144) {
                // explosion
                explosion += super_above[i] - 128;
            } 
            else if (super_above[i] < 158) {} // immovable stuff, don't bother updating current spot
            else if ((super_above[i] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_above[i] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 (and higher stuff, filtered above)
                // rocket
            }
            
            // from below
            if (super_below[i] <= 128) {} 
            else if (super_below[i] < 144) {
                // explosion
                explosion += super_below[i] - 128;
            } 
            else if (super_below[i] < 158) {} // immovable stuff, don't bother updating current spot
            else if ((super_below[i] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_below[i] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 
                // rocket
            }

            if (explosion > 2) {
                if (super_row[i]/2 == 144/2) {
                    // indestructible wall, only explode rockets
                    if (explosion >= 300)
                    {
                        if (direction&1) {
                            superpixel[j][i-1] = 143;
                        }
                        if (direction&2) {
                            superpixel[j][i+1] = 143;
                        }
                    }

                } 
                else {
                    if (explosion/4 > 15)
                        superpixel[j][i] = 143;
                    else {
                        superpixel[j][i] = 128 + explosion/4;
                    }
                }
            }
        }
        else if ((super_row[i] & ((1<<7)|(1<<6))) == 192) {}
        else if ((super_row[i] & ((1<<7)|(1<<5))) == 160) {
            if (super_row[i] < 176) {
                // player 0 rocket
                if (i == Nx) {
                    superpixel[j][i] = 127;
                    continue;
                }
            } 
            else {
                // player 1 rocket
                if (i == 1) {
                    superpixel[j][i] = 127;
                    continue;
                }
            }
            // rockets, this is called usually only once, near beginning.
            
            // check if there are any balls coming in, or explosions
            int explosion = 0; 

            // from left
            if (super_row[i-1] <= 128) {} 
            else if (super_row[i-1] < 144) {
                // explosions 
                explosion += super_row[i-1] - 128;
            }
            else if (super_row[i-1] < 158) {} // walls
            else if ((super_row[i-1] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_row[i-1] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 (and higher stuff, filtered above)
                // rocket
                // player 0 rocket, going to the right
                explosion += 300;
                superpixel[j][i-1] = 143; // explode this guy, too
            }
            
            // from right
            if (super_row[i+1] <= 128) {} 
            else if (super_row[i+1] < 144) {
                // explosion
                explosion += super_row[i+1] - 128;
            } 
            else if (super_row[i+1] < 158) {} // immovable stuff, don't bother updating current spot
            else if ((super_row[i+1] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_row[i+1] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 (and higher stuff, filtered above)
                // rocket
                // player 1 rocket, going to the left
                explosion += 300;
                superpixel[j][i+1] = 143; // explode this guy, too
            }
            
            // from above
            if (super_above[i] <= 128) {} 
            else if (super_above[i] < 144) {
                // explosion
                explosion += super_above[i] - 128;
            } 
            else if (super_above[i] < 158) {} // immovable stuff, don't bother updating current spot
            else if ((super_above[i] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_above[i] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 (and higher stuff, filtered above)
                // rocket
            }
            
            // from below
            if (super_below[i] <= 128) {} 
            else if (super_below[i] < 144) {
                // explosion
                explosion += super_below[i] - 128;
            } 
            else if (super_below[i] < 158) {} // immovable stuff, don't bother updating current spot
            else if ((super_below[i] & ((1<<7)|(1<<6))) == 192) { // 192 - 255
                // ball
                
            } 
            else if ((super_below[i] & ((1<<7)|(1<<5))) == 160) { // 160 - 191 
                // rocket
            }

            if (explosion > 2) {
                // explode the warhead
                superpixel[j][i] = 143;
            }
        }
    }
}

