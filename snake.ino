#include <LiquidCrystal.h>

#define SECTOR_WIDTH 5
#define SECTOR_HEIGHT 8
#define SECTORS_PER_ROW 16
#define SECTORS_PER_COLUMN 2
#define BOARD_WIDTH (SECTORS_PER_ROW * SECTOR_WIDTH)
#define BOARD_HEIGHT (SECTORS_PER_COLUMN * SECTOR_HEIGHT)

LiquidCrystal lcd(8, 9, 4, 5, 6, 7);

int lcd_key     = 0;
int adc_key_in  = 0;

#define btnNONE   0
#define btnRIGHT  1
#define btnUP     2
#define btnDOWN   3
#define btnLEFT   4
#define btnSELECT 5
#define NUM_KEYS 6


const short MIN_DRAW_WAIT = 60;

struct point_t {
  char x;
  char y;
};

struct snake_node_t {
  struct snake_node_t *next;
  struct snake_node_t *prev;
  struct point_t pos;
};

enum Direction {
  LEFT = 1,
  DOWN = LEFT << 1,
  RIGHT = LEFT << 2,
  UP = LEFT << 3,
};

struct snake_node_t* snake_head;
Direction curr_direction;
struct point_t apple_pos;

void generate_apple() {
  apple_pos.x = random(BOARD_WIDTH);
  apple_pos.y = random(BOARD_HEIGHT);
}

void init_character(byte* character) {
  for(int i = 0; i < SECTOR_HEIGHT; i++) {
  // Add a zero byte to every row
  character[i] = B00000;
  }
}

struct snake_node_t* add_snake_part() {
  struct snake_node_t* new_part = (struct snake_node_t*) malloc(sizeof(struct snake_node_t));
  struct snake_node_t* last = snake_head->next;
  snake_head->next = new_part;
  last->prev = new_part;
  new_part->next = last;
  new_part->prev = snake_head;
  return new_part;
}


void move_snake() {
  struct point_t last_pos = snake_head->pos;
  struct snake_node_t* curr_node = snake_head->prev;

  //Move head
  switch(curr_direction) {
    case LEFT:
      if(snake_head->pos.x == 0) {
        snake_head->pos.x = BOARD_WIDTH - 1;
      } else {
        snake_head->pos.x--;
      }
      break;
    case DOWN:
      if(snake_head->pos.y == (BOARD_HEIGHT - 1)) {
        snake_head->pos.y = 0;
      } else {
        snake_head->pos.y++;
      }
      break;
    case RIGHT:
      if(snake_head->pos.x == (BOARD_WIDTH - 1)) {
        snake_head->pos.x = 0;
      } else {
        snake_head->pos.x++;
      }
      break;
    case UP:
      if(snake_head->pos.y == 0) {
        snake_head->pos.y = BOARD_HEIGHT - 1;
      } else {
        snake_head->pos.y--;
      }
      break;
  }

  //Move body
  while(curr_node != snake_head) {
    struct point_t temp = curr_node->pos;
    curr_node->pos = last_pos;
    last_pos = temp;
    curr_node = curr_node->prev;
  }
}

void draw_snake() {
  byte sectors[SECTORS_PER_ROW][SECTORS_PER_COLUMN][SECTOR_HEIGHT];
  for(int i = 0; i < SECTORS_PER_COLUMN; i++) {
    for(int j = 0; j < SECTORS_PER_ROW; j++) {
      init_character(sectors[j][i]);
    }
  }

  //Apple drawing
  byte sec_x = apple_pos.x / SECTOR_WIDTH;
  byte sec_y = apple_pos.y / SECTOR_HEIGHT;
  sectors[sec_x][sec_y][apple_pos.y - (sec_y * SECTOR_HEIGHT)] |= (B10000 >> (apple_pos.x - (sec_x * SECTOR_WIDTH)));

  struct snake_node_t* curr_node = snake_head;

  do{
    struct point_t p = curr_node->pos;
    sec_x = p.x / SECTOR_WIDTH;
    sec_y = p.y / SECTOR_HEIGHT;
    sectors[sec_x][sec_y][p.y - (sec_y * SECTOR_HEIGHT)] |= (B10000 >> (p.x - (sec_x * SECTOR_WIDTH)));
    curr_node = curr_node->prev;
  }while(curr_node != snake_head);

  byte curr_sec = 0;

  for(int i = 0; i < SECTORS_PER_COLUMN; i++) {
    for(int j = 0; j < SECTORS_PER_ROW; j++) {
      bool draw_sec = false;
      for(int k = 0; k < SECTOR_HEIGHT; k++) {
        if(sectors[j][i][k]) {
          draw_sec = true;
          break;
        }
      }

      if(draw_sec) {
        lcd.createChar(curr_sec, sectors[j][i]);
        lcd.setCursor(j, i);
        lcd.write(byte(curr_sec));
        curr_sec++;
      } else {
        lcd.setCursor(j, i);
        lcd.write(" ");
      }
    }
  }
}


int read_LCD_buttons() {
  adc_key_in = analogRead(0);
  
  if(adc_key_in < 1000) {
    Serial.println(adc_key_in);
  }
  
  if (adc_key_in > 1000) return btnNONE; 
  if (adc_key_in < 50)   return btnRIGHT;  
  if (adc_key_in < 195)  return btnUP; 
  if (adc_key_in < 380)  return btnDOWN; 
  if (adc_key_in < 555)  return btnLEFT; 
  if (adc_key_in < 790)  return btnSELECT;  

  return btnNONE;  // when all others fail, return this...
}

void setup() {
  byte character[SECTOR_HEIGHT];
  lcd.begin(16, 2);
  Serial.begin(9600);
  Serial.println("starting snek!");
  snake_head = (struct snake_node_t*) malloc(sizeof(struct snake_node_t));
  snake_head->pos.x = 5;
  snake_head->pos.y = 5;
  snake_head->prev = snake_head;
  snake_head->next = snake_head;
  struct snake_node_t* body1 = add_snake_part();
  struct snake_node_t* body2 = add_snake_part();
  body1->pos.x = 4;
  body1->pos.y = 5;
  body2->pos.x = 3;
  body2->pos.y = 5;
  curr_direction = RIGHT;
  generate_apple();
  draw_snake();
}

bool left_button_state = false;
bool right_button_state = false;
bool left_just_pressed = false;
bool right_just_pressed = false;

short time_since_last_draw = 0;
unsigned long last_update = 0;


void handle_input() {
  int buttons = read_LCD_buttons();

// btnRIGHT
  if(buttons == btnLEFT ) {
    curr_direction = RIGHT;
    Serial.println("RIGHT");
  }

  if(buttons == btnSELECT ) {
    curr_direction = LEFT;
    Serial.println("LEFT");
  }
  if(buttons == btnUP ) {
    curr_direction = UP;
    Serial.println("UP");
  }
  if(buttons == btnDOWN ) {
    curr_direction = DOWN;
    Serial.println("DOWN");
  }

}


void loop() {

  update_input();

  unsigned long time = millis();
  unsigned long elapsed = time - last_update;
  last_update = time;
  time_since_last_draw += elapsed;
  if(time_since_last_draw >= MIN_DRAW_WAIT) {
    move_snake();
    if(snake_head->pos.x == apple_pos.x && snake_head->pos.y == apple_pos.y) {
      generate_apple();
      add_snake_part();
    }
    draw_snake();
    time_since_last_draw = 0;
  }
}
