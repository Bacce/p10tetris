// Original code source:
// https://github.com/MarginallyClever/ArduinoStarterKit/tree/master/LED8x8tetris


//------------------------------------------------------------------------------
// 8x8 single color LED Tetris for Arduino UNO and a joystick breakout.
// dan@marginallycelver.com 2015-01-22
//------------------------------------------------------------------------------
// Copyright at end of file.
// please see http://www.github.com/MarginallyClever/ArduinoStarterKit for more information.
//
// The LED grid is a red 8x8.
// The Arduino is an UNO.
// The joystick is a xinda ps3/xbox style clickable stick on a breakout board.
// 
// Place the grid so the lights are facing you and the nub on one edge is on the
// bottom.  On the back should be two horizontal rows of pins.  the top row, from
// left to right, connects to arduino digital pins 9-2.  The bottom row of pins,
// from left to right, connect to digital pins 13-10 and analog pins 2-5.
// 
// The joystick has 5 pins.  joystick 5v and GND go to the same pins on arduino.
// VRx goes to A0.  VRy goes to A1.  SW goes to arduino digital pin 1.
// 
// While the SW pin is connected you will not be able to use Serial.*, because
// Serial *also* uses pins 0 and 1.

//--------------------------------------------------------------------------------
// CONSTANTS
//--------------------------------------------------------------------------------

#include <SPI.h>
#include <DMD2.h>
// size of the LED grid
#define GRID_W           (16)
#define GRID_H           (20)
// max size of each tetris piece
#define PIECE_W          (4)
#define PIECE_H          (4)

// how many kinds of pieces
#define NUM_PIECE_TYPES  (7)

#define JOYSTICK_DEAD_ZONE  (30)

#define JOYSTICK_PIN     (0)
#define PIEZO_PIN        (1)

// 1 color drawings of each piece in each rotation.
// Each piece is max 4 wide, 4 tall, and 4 rotations.
const char piece_I[] = {
  0,0,0,0,
  0,0,0,0,
  0,0,0,0,
  1,1,1,1,

  0,1,0,0,
  0,1,0,0,
  0,1,0,0,
  0,1,0,0,
  
  0,0,0,0,
  0,0,0,0,
  0,0,0,0,
  1,1,1,1,

  0,1,0,0,
  0,1,0,0,
  0,1,0,0,
  0,1,0,0,
};

const char piece_L1[] = {
  0,1,0,0,
  0,1,0,0,
  0,1,1,0,
  0,0,0,0,

  0,0,0,0,
  1,1,1,0,
  1,0,0,0,
  0,0,0,0,
  
  1,1,0,0,
  0,1,0,0,
  0,1,0,0,
  0,0,0,0,

  0,0,1,0,
  1,1,1,0,
  0,0,0,0,
  0,0,0,0,
};

const char piece_L2[] = {
  0,1,0,0,
  0,1,0,0,
  1,1,0,0,
  0,0,0,0,

  0,0,0,0,
  1,0,0,0,
  1,1,1,0,
  0,0,0,0,
  
  0,1,1,0,
  0,1,0,0,
  0,1,0,0,
  0,0,0,0,

  0,0,0,0,
  1,1,1,0,
  0,0,1,0,
  0,0,0,0,
};

const char piece_T[] = {
  0,0,0,0,
  1,1,1,0,
  0,1,0,0,
  0,0,0,0,

  0,1,0,0,
  1,1,0,0,
  0,1,0,0,
  0,0,0,0,

  0,0,0,0,
  0,1,0,0,
  1,1,1,0,
  0,0,0,0,

  0,1,0,0,
  0,1,1,0,
  0,1,0,0,
  0,0,0,0,
};

const char piece_S1[] = {
  1,0,0,0,
  1,1,0,0,
  0,1,0,0,
  0,0,0,0,

  0,0,0,0,
  0,1,1,0,
  1,1,0,0,
  0,0,0,0,

  1,0,0,0,
  1,1,0,0,
  0,1,0,0,
  0,0,0,0,

  0,0,0,0,
  0,1,1,0,
  1,1,0,0,
  0,0,0,0,
};

const char piece_S2[] = {
  0,1,0,0,
  1,1,0,0,
  1,0,0,0,
  0,0,0,0,

  0,0,0,0,
  1,1,0,0,
  0,1,1,0,
  0,0,0,0,
  
  0,1,0,0,
  1,1,0,0,
  1,0,0,0,
  0,0,0,0,

  0,0,0,0,
  1,1,0,0,
  0,1,1,0,
  0,0,0,0,
};

const char piece_O[] = {
  1,1,0,0,
  1,1,0,0,
  0,0,0,0,
  0,0,0,0,
  
  1,1,0,0,
  1,1,0,0,
  0,0,0,0,
  0,0,0,0,
  
  1,1,0,0,
  1,1,0,0,
  0,0,0,0,
  0,0,0,0,
  
  1,1,0,0,
  1,1,0,0,
  0,0,0,0,
  0,0,0,0,
};


// An array of pointers!
const char *pieces[NUM_PIECE_TYPES] = {
  piece_S1,
  piece_S2,
  piece_L1,
  piece_L2,
  piece_O,
  piece_T,
  piece_I,
};


//--------------------------------------------------------------------------------
// GLOBALS
//--------------------------------------------------------------------------------

// this is how arduino remembers what the button was doing in the past,
// so arduino can tell when it changes.
int old_button=0;
// so arduino can tell when user moves sideways
int old_px = 0;
// so arduino can tell when user tries to turn
int old_i_want_to_turn=0;

// this is how arduino remembers the falling piece.
int next_piece_id;
int piece_id;
int piece_rotation;
int piece_x;
int piece_y;

char piece_sequence[NUM_PIECE_TYPES];
char sequence_i=NUM_PIECE_TYPES;

// this controls how fast the player can move.
long last_move;
long move_delay=100;  // 100ms = 5 times a second

// this controls when the piece automatically falls.
long last_drop;
long drop_delay=500;

// this is how arduino remembers where pieces are on the grid.
char grid[GRID_W * GRID_H];
char gridPrev[GRID_W * GRID_H];

int points = 0;
int level = 0;

bool gameStarted = false;
bool introRendered = false;
bool gameOver = false;
bool overRendered = false;

SoftDMD dmd(1, 1);
//--------------------------------------------------------------------------------
// METHODS
//--------------------------------------------------------------------------------

// Function to copy 'len' elements from 'src' to 'dst'
// https://forum.arduino.cc/t/copying-array-into-another-array/264828
void copyChar(char* src, char* dst, int len) {
  memcpy(dst, src, sizeof(src[0])*len);
}


// grid contains the arduino's memory of the game board, including the piece that is falling.
// Only update the pixels what change
void draw_grid() {
  int x, y;
  for(y=0;y<GRID_H;++y) {
    for(x=0;x<GRID_W;++x) {
      if( grid[y*GRID_W+x] > gridPrev[y * GRID_W + x] ) {
        dmd.setPixel(y, x); // Turn pixel on
      }
      else if ( grid[y * GRID_W + x] < gridPrev[y * GRID_W + x] ) {
        dmd.setPixel(y, x, GRAPHICS_OFF); //turn pixel off
      }
      else {
        //No change, nothing to do
      }
    }
  }
  copyChar(grid, gridPrev, 16 * 20);
}

// choose a new piece from the sequence.
// the sequence is a random list that contains one of each piece.
// that way you're guaranteed an even number of pieces over time,
// tho the order is random.
void choose_new_piece() {
  if( sequence_i >= NUM_PIECE_TYPES ) {
    // list exhausted
    int i,j, k;
    for(i=0;i<NUM_PIECE_TYPES;++i) {
      do {
        // pick a random piece
        j = rand() % NUM_PIECE_TYPES;
        // make sure it isn't already in the sequence.
        for(k=0;k<i;++k) {
          if(piece_sequence[k]==j) break;  // already in sequence
        }
      } while(k<i);
      // not in sequence.  Add it.
      piece_sequence[i] = j;
    }
    // rewind sequence counter
    sequence_i=0;
  }
  
  next_piece_id = piece_sequence[sequence_i++];
  draw_gui();
}

void use_next_piece(){
  // get the next piece in the sequence.
  piece_id = next_piece_id;
  // always start the piece top center.
  piece_y=-4;  // -4 squares off the top of the screen.
  piece_x=7;
  // always start in the same orientation.
  piece_rotation=0;
}


void erase_piece_from_grid() {
  int x, y;
  
  const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);
  
  for(y=0;y<PIECE_H;++y) {
    for(x=0;x<PIECE_W;++x) {
      int nx=piece_x+x;
      int ny=piece_y+y;
      if(ny<0 || ny>GRID_H) continue;
      if(nx<0 || nx>GRID_W) continue;
      if(piece[y*PIECE_W+x]==1) {
        grid[ny*GRID_W+nx]=0;  // zero erases the grid location.
      }
    }
  }
}


void add_piece_to_grid() {
  int x, y;
  
  const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);
  
  for(y=0;y<PIECE_H;++y) {
    for(x=0;x<PIECE_W;++x) {
      int nx=piece_x+x;
      int ny=piece_y+y;
      if(ny<0 || ny>GRID_H) continue;
      if(nx<0 || nx>GRID_W) continue;
      if(piece[y*PIECE_W+x]==1) {
        grid[ny*GRID_W+nx]=1;  // zero erases the grid location.
      }
    }
  }
}


// Move everything down 1 space, destroying the old row number y in the process.
void delete_row(int y) {
  int x;
  for(;y>0;--y) {
    for(x=0;x<GRID_W;++x) {
      grid[y*GRID_W+x] = grid[(y-1)*GRID_W+x];
    }
  }
  // everything moved down 1, so the top row must be empty or the game would be over.
  for(x=0;x<GRID_W;++x) {
    grid[x]=0;
  }
}


void remove_full_rows() {
  int count=0;
  int linepoint = 15;
  int x, y, c;
  for(y=0;y<GRID_H;++y) {
    // count the full spaces in this row
    c = 0;
    for(x=0;x<GRID_W;++x) {
      if( grid[y*GRID_W+x] > 0 ) c++;
    }
    if(c==GRID_W) {
      // row full!
      delete_row(y);
      count++;
    }
  }

  if(count > 0) {
    // count == 1 is the default value
    if(count == 2) {linepoint = 30;}
    if(count == 3) {linepoint = 90;}
    if(count == 4) {linepoint = 150;}

    points = points + linepoint * (level + 1);

    // level 0 - points:0-50
    if(points > 50  && points<=120 ) { level = 1; drop_delay=300; }
    else if(points > 120 && points<=360 ) { level = 2; drop_delay=250; }
    else if(points > 360 && points<=720 ) { level = 3; drop_delay=200; }
    else if(points > 720 ) { level = 4; drop_delay=150; }

    draw_gui();
  }
}


void try_to_move_piece_sideways() {
  int left = digitalRead(4);
  int right = digitalRead(5);
  
  int new_px = 0;

  if(!left) { new_px=1;}
  if(!right) { new_px=-1;}

  if (piece_can_fit(piece_x + new_px, piece_y, piece_rotation) == 1) {
    piece_x += new_px;
  }
  old_px = new_px;
}


void try_to_rotate_piece() {
  int i_want_to_turn=0;
  int dy = digitalRead(A0);
  if (!dy) i_want_to_turn = 1;
  
  if (i_want_to_turn == 1 && i_want_to_turn != old_i_want_to_turn) {
    // figure out what it will look like at that new angle
    int new_pr = ( piece_rotation + 1 ) % 4;
    // if it can fit at that new angle (doesn't bump anything)
    if(piece_can_fit(piece_x,piece_y,new_pr)) {
      // then make the turn.
      piece_rotation = new_pr;
    }
  }
  old_i_want_to_turn = i_want_to_turn;
}


// can the piece fit in this new location?
int piece_can_fit(int px,int py,int pr) {
  if( piece_off_edge(px,py,pr) ) return 0;
  if( piece_hits_rubble(px,py,pr) ) return 0;
  return 1;
}


int piece_off_edge(int px,int py,int pr) {
  int x,y;
  const char *piece = pieces[piece_id] + (pr * PIECE_H * PIECE_W);
  
  for(y=0;y<PIECE_H;++y) {
    for(x=0;x<PIECE_W;++x) {
      int nx=px+x;
      int ny=py+y;
      if(ny<0) continue;  // off top, don't care
      if(piece[y*PIECE_W+x]>0) {
        if(nx<0) return 1;  // yes: off left side
        if(nx>=GRID_W ) return 1;  // yes: off right side
      }
    }
  }
  
  return 0;  // inside limits
}


int piece_hits_rubble(int px,int py,int pr) {
  int x,y;
  const char *piece = pieces[piece_id] + (pr * PIECE_H * PIECE_W);

  for(y=0;y<PIECE_H;++y) {
    int ny=py+y;
    if(ny<0) continue;
    for(x=0;x<PIECE_W;++x) {
      int nx=px+x;
      if(piece[y*PIECE_W+x]>0) {
        if(ny>=GRID_H ) return 1;  // yes: goes off bottom of grid
        if(grid[ny*GRID_W+nx]==1 ) return 1;  // yes: grid already full in this space
      }
    }
  }
  
  return 0;  // doesn't hit
}


void game_over() {
  int x,y;

  while(1) {
    if(!overRendered)renderOver();
    // click the button?
    if(digitalRead(A0)==0) {
      // restart!
      overRendered = false;
      gameOver=false;
      setup();
      return;
    }
  }
}


void try_to_drop_piece() {
  erase_piece_from_grid();
  if(piece_can_fit(piece_x,piece_y+1,piece_rotation)) {
    piece_y++;  // move piece down
    add_piece_to_grid();
  } else {
    // hit something!
    // put it back
    add_piece_to_grid();
    remove_full_rows();
    if(game_is_over()==1) {
      game_over();
    }
    // game isn't over, choose a new piece
    points = points+1;
    use_next_piece();
    choose_new_piece();
  }
}


void try_to_drop_faster() {
  int y = digitalRead(3);
  if(!y) {
    try_to_drop_piece();
  }
}


void react_to_player() {
  erase_piece_from_grid();
  try_to_move_piece_sideways();
  try_to_rotate_piece();
  add_piece_to_grid();
  
  try_to_drop_faster();
}


// can the piece fit in this new location
int game_is_over() {
  int x,y;
  const char *piece = pieces[piece_id] + (piece_rotation * PIECE_H * PIECE_W);
  for(y=0;y<PIECE_H;++y) {
    for(x=0;x<PIECE_W;++x) {
      int ny=piece_y+y;
      int nx=piece_x+x;
      if(piece[y*PIECE_W+x]>0) {
        if(ny<0) {
          gameOver = true;
          return 1;  // yes: off the top!
        }
      }
    }
  }
  return 0;  // not over yet...
}

void draw_number(int x, int y, char number) {
  switch(number) {
    case '0':
      dmd.drawLine(x, y, x+4, y);
      dmd.drawLine(x, y+1, x+4, y+1);
      break;
    case '1':
      dmd.setPixel(x+1, y+1);
      dmd.drawLine(x, y, x+4, y);
      break;
    case '2':
      dmd.setPixel(x, y);
      dmd.setPixel(x, y+1);
      dmd.setPixel(x+1, y);
      
      dmd.setPixel(x+2, y+1);
      dmd.setPixel(x+3, y+1);
      dmd.setPixel(x+4, y+1);
      dmd.setPixel(x+4, y);
      break;
    case '3':
      dmd.drawLine(x, y, x+4, y);
      dmd.setPixel(x, y+1);
      dmd.setPixel(x+2, y+1);
      dmd.setPixel(x+4, y+1);
      break;
    case '4':
      dmd.drawLine(x+2, y, x+4, y);
      dmd.drawLine(x, y+1, x+2, y+1);
      break;
    case '5':
      dmd.drawLine(x+2, y, x+4, y);
      dmd.drawLine(x, y+1, x+2, y+1);
      dmd.setPixel(x, y);
      dmd.setPixel(x+4, y+1);
      break;
    case '6':
      dmd.drawLine(x+2, y, x+4, y);
      dmd.drawLine(x, y+1, x+4, y+1);
      dmd.setPixel(x, y);
      break;
    case '7':
      dmd.setPixel(x, y);
      dmd.setPixel(x, y+1);
      dmd.setPixel(x+1, y);
      dmd.drawLine(x+2,y+1, x+4, y+1);
      break;
    case '8':
      dmd.drawLine(x,y,x,y+1);
      dmd.drawLine(x+2,y,x+4,y);
      dmd.drawLine(x+2,y+1,x+4,y+1);
      break;
    case '9':
      dmd.drawLine(x, y, x+4, y);
      dmd.drawLine(x, y+1, x+1, y+1);
      dmd.setPixel(x+4, y+1);
      break;
  }
}

void draw_gui(){  
  dmd.drawFilledBox(21,0,31,15, GRAPHICS_OFF);
  char buffer[7];
  sprintf (buffer, "%i", points);

  draw_number(23, 2, buffer[4]); //x,y,number
  draw_number(23, 5, buffer[3]);
  draw_number(23, 8, buffer[2]);
  draw_number(23, 11, buffer[1]);
  draw_number(23, 14, buffer[0]);

  dmd.drawLine(GRID_H, 0, GRID_H, GRID_W-1);

  dmd.drawLine(31, 0, 31, level);

  // Draw next piece
  int posy=29;
  const char *piece = pieces[next_piece_id];

  // piece "I" should be positioned differently on the preview
  if(next_piece_id == 6) {
     posy=27;
  }
  for(int y=0;y<PIECE_H;++y) {
    for(int x=0;x<PIECE_W;++x) {
      if(y<0 || y>GRID_H) continue;
      if(x<0 || x>GRID_W) continue;
      if(piece[y*PIECE_W+x]==1) {
        dmd.setPixel(y+posy,x+7);
      }
    }
  }
}


// called once when arduino reboots
void setup() {
  //Serial.begin(115200);
  dmd.setBrightness(10);
  dmd.begin();
  int i;

  // set up speaker pin
  pinMode(PIEZO_PIN, OUTPUT);
  
  // set up joystick button
  pinMode(4, INPUT_PULLUP);
  pinMode(5, INPUT_PULLUP);
  pinMode(A0, INPUT_PULLUP);
  // make sure arduino knows the grid is empty.
  for(i=0;i<GRID_W*GRID_H;++i) {
    grid[i]=0;
    gridPrev[i]=0;
  }

  // make the game a bit more random - pull a number from space and use it to 'seed' a crop of random numbers.
  randomSeed(analogRead(1));
  
  // Reset values for new game
  points = 0;
  level = 0;
  drop_delay=500;
}

void renderIntro(){
  int posx=7;
  int posy=3;
  dmd.drawFilledBox(0,0,31,15, GRAPHICS_OFF);
  
  bool image[17][10] = {
    {1,1,1,1,0,1,1,1,1},
    {0,1,1,0,0,1,1},
    {0,1,1,0,0,1,1,1},
    {0,1,1,0,0,1,1},
    {0,1,1,0,0,1,1,1,1},
    {0},
    {1,1,1,1,0,1,1,1,1},
    {0,1,1,0,0,1,1,0,1},
    {0,1,1,0,0,1,1,1},
    {0,1,1,0,0,1,1,0,1},
    {0,1,1,0,0,1,1,0,1},
    {0},
    {1,1,1,1,0,1,1,1,1},
    {0,1,1,0,0,1,1},
    {0,1,1,0,0,1,1,1,1},
    {0,1,1,0,0,0,0,1,1},
    {1,1,1,1,0,1,1,1,1},
  };
  
  // Render image
  for (int row=0; row<17; row++) {
    for(int col=0; col<10; col++) {
      if(image[row][col]){dmd.setPixel(row+posx, 15-col-posy);}
    }
  }
}

void renderOver() {
  int posx=4;
  int posy=3;
  dmd.drawFilledBox(0,0,19,15, GRAPHICS_OFF);

  bool image[17][10] = {
    {1,1,1,1,0,1,1,0,1},
    {1,1,0,1,0,1,1,0,1},
    {1,1,0,1,0,1,1,0,1},
    {1,1,0,1,0,0,1,0,1},
    {1,1,1,1,0,0,0,1,0},
    {0},
    {1,1,1,1,0,1,1,1,1},
    {1,1,0,0,0,1,1,0,1},
    {1,1,1,0,0,1,1,1},
    {1,1,0,0,0,1,1,0,1},
    {1,1,1,1,0,1,1,0,1},
  };


  // Render image
  for (int row=0; row<17; row++) {
    for(int col=0; col<10; col++) {
      if(image[row][col]){dmd.setPixel(row+posx, 15-col-posy);}
    }
  }
  overRendered=true;
}


// called over and over after setup()
void loop() {
  if(!gameStarted && !introRendered) {
    renderIntro();
    introRendered = true;
  }
  else if(gameStarted) {
    // the game plays at one speed,
    if(millis() - last_move > move_delay ) {
      last_move = millis();
      react_to_player();
      draw_grid();
    }
  
    // ...and drops the falling block at a different speed.
    if(millis() - last_drop > drop_delay ) {
      last_drop = millis();
      try_to_drop_piece();
    }
  }

  if(gameStarted == false) {
    if(digitalRead(A0) == 0) {
      dmd.drawFilledBox(0,0,31,15, GRAPHICS_OFF);
      // get ready to start the game.
      choose_new_piece();
      use_next_piece();
      choose_new_piece();
      // start the game clock
      last_move = millis();
      last_drop = last_move;
      gameStarted = true;
      draw_gui();
    }
  }
}


/**
* This file is part of ArduinoStarterKit.
*
* ArduinoStarterKit is free software: you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation, either version 3 of the License, or
* (at your option) any later version.
*
* ArduinoStarterKit is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with Arduino Timer Interrupt. If not, see <http://www.gnu.org/licenses/>.
*/
