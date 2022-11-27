/*
 * todo:
 *  - add ball/bat angles
 *  - add sound?
 *  
 * Built this on a breadboard, using a bluepill (STM32F103C8T6) with an ILI9163 display and a 100k Ohm potentiometer with the wiper connected to pin A0
 * as a controller. THe display is 3.3V, as well as the controller, so no level conversion was needed.
 * 
 */
 
#include <SPI.h>
#include <Adafruit_GFX.h>
#include <TFT_ILI9163C.h>

// pin definitions
#define RST PB5
#define DC PB6
#define CS PB7
#define PADDLE_PIN PA0


// Color definitions
#define	BLACK   0x0000
#define	BLUE    0x001F
#define	RED     0xF800
#define	GREEN   0x07E0
#define CYAN    0x07FF
#define MAGENTA 0xF81F
#define YELLOW  0xFFE0
#define WHITE   0xFFFF

#define VISIBLE 1
#define CLEAR   0

#define ROWS  5
#define BLOCKS 10
#define TOP_OFFSET 16
#define LEFT_OFFSET 4
#define RIGHT_OFFSET 4

#define BLOCK_WIDTH  12
#define BLOCK_HEIGHT  6

#define BAT_Y 115
#define BAT_HEIGHT 2

#define MAX_LIVES 3

// Globals
TFT_ILI9163C tft = TFT_ILI9163C(CS, DC, RST);
uint8_t errorCode = 0;
int16_t row,block;
int16_t colours[] = { RED, BLUE, YELLOW, GREEN, CYAN };
uint8_t block_status[ROWS][BLOCKS];
int difficulty = 6;  // controls speed and bat width (min 1, max 5)
int bat_width = 20;
int bat_x = 50;
int16_t paddle_pos;
int blocks_left;
int8_t lives_left;
bool game_over;
uint16_t score;

int16_t get_paddle_pos()
{
  int16_t reading;

  reading = analogRead(PADDLE_PIN);
  reading = constrain(reading,150,4050);

  return map(reading, 200,4050, LEFT_OFFSET, 128 - RIGHT_OFFSET - bat_width);
  
}

bool test_block_hit(int ball_x, int ball_y)
{
    int row,block;

    row = ( ball_y - TOP_OFFSET ) / BLOCK_HEIGHT;
    block = ( ball_x - LEFT_OFFSET ) / BLOCK_WIDTH;

    if ( row >= 0 and row < ROWS and 
         block >= 0 and block < BLOCKS and
         block_status [row][block] == VISIBLE ) {
      block_status [row][block] = CLEAR;
      return true;
    } else
      return false;
}

bool test_bat_hit(int ball_x, int ball_y)
{
  return ball_y >= BAT_Y and ball_y < BAT_Y + BAT_HEIGHT and ball_x >= bat_x and ball_x < bat_x + bat_width;
}

void draw_score(uint16_t score)
{
  char buffer[7];

  sprintf(buffer, "%6u", score);
  tft.setCursor(90,120);
  tft.fillRect(90,120,8*5,8,BLACK);
  tft.println(buffer);
}

void draw_lives(int8_t lives)
{
  int8_t i;

  for ( i=0; i< MAX_LIVES; i++ ) {
    tft.fillCircle(4+i*5,125,1,( lives > i ? WHITE : BLACK ) );
  }
}

void draw_game_over()
{
  tft.setTextSize(3);
  tft.setCursor(30,55);
  tft.println("GAME");
  tft.setCursor(30,90);
  tft.println("OVER");
}

void redraw_blocks()
{
  int colour;
  int border;
  
  // field of blocks
  for ( row=0; row<ROWS; row++ ) {
    for ( block=0; block<BLOCKS; block++ ) {
      if ( block_status[row][block] == CLEAR ) {
        colour = BLACK;
        border = 0;
      } else {
        colour = colours[row];
        border = 1;
      }
              
      tft.fillRect(LEFT_OFFSET+block*BLOCK_WIDTH, TOP_OFFSET+(row*BLOCK_HEIGHT), BLOCK_WIDTH-border, BLOCK_HEIGHT-border, colour);
    }
  }
  
}


void setup() {
  Serial.begin(38400);
  tft.begin();
  //the following it's mainly for Teensy
  //it will help you to understand if you have choosed the 
  //wrong combination of pins!
  errorCode = tft.errorCode();
  if (errorCode != 0) {
    Serial.print("Init error! ");
    if (bitRead(errorCode, 0)) Serial.print("MOSI or SCLK pin mismach!\n");
    if (bitRead(errorCode, 1)) Serial.print("CS or DC pin mismach!\n");
  }

  tft.fillScreen();
  tft.setFont();
  tft.setTextSize(1);
  tft.setTextColor(WHITE);

  // game init
  game_over = false;
  score = 0;
  lives_left  = MAX_LIVES;

}

void loop(void) {   // play a single game

  if ( game_over ) return;

  // level init
  int ball_dx = 2;
  int ball_dy = -3;

  int ball_x = 60;
  int ball_y = BAT_Y - 2;

  bat_width = 15 - (5 - difficulty);
  blocks_left = ROWS * BLOCKS;

  difficulty--;

  // field of blocks
  for(row=0;row<ROWS;row++) {
    for(block=0;block<BLOCKS;block++) {
      tft.fillRect(LEFT_OFFSET+block*BLOCK_WIDTH, TOP_OFFSET+(row*BLOCK_HEIGHT), BLOCK_WIDTH-1, BLOCK_HEIGHT-1, colours[row]);
      block_status[row][block] = VISIBLE;
    }
  }

  bat_x = get_paddle_pos();
  draw_lives(lives_left);
  draw_score(score);
  
  // play loop
  for(;;) {
    tft.fillCircle(ball_x,ball_y,1,WHITE);  // ball
    tft.fillRect(bat_x,BAT_Y,bat_width,BAT_HEIGHT,WHITE); // bat
    draw_lives(lives_left);
    if ( test_block_hit(ball_x, ball_y)) {
      redraw_blocks();
      blocks_left--;
      score++;
      draw_score(score);
      if ( blocks_left == 0 ) break;
      ball_dy *= -1;
    }
    if ( test_bat_hit (ball_x, ball_y) ) {
      // TODO: Add logic to change ball_dx,ball_dy based on where the bat was hit 
      ball_dy *= -1;
    }
    delay(difficulty * 10);
    tft.fillCircle(ball_x,ball_y,1,BLACK);  // ball
    tft.fillRect(bat_x,BAT_Y,bat_width,BAT_HEIGHT,BLACK); // bat

    // temp: move the bath back and forth
    bat_x = get_paddle_pos();

    if (ball_x >= 127 - RIGHT_OFFSET or ball_x <= LEFT_OFFSET) ball_dx *= -1;
    if (ball_y >= 127 or ball_y <= 1 ) ball_dy *= -1;

    if(ball_y >= 127) {
      lives_left--;
      if ( lives_left < 0 ) {
        game_over = true;
        draw_game_over();
        break;
      }
      draw_lives(lives_left);
      ball_x = 60;
      ball_y = BAT_Y -2;
      delay(500);
    }

    ball_x += ball_dx;
    ball_y += ball_dy;
  }
}
