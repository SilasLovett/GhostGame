
#include <stdlib.h>
#include <conio.h>
#include <dos.h>
#include <stdio.h>
#include <string.h>

#define NUM_COLORS 256

#define SET_MODE 0x00
#define VIDEO_INT 0x10
#define VGA_256_COLOR_MODE 0x13
#define TEXT_MODE 0x03

#define SCREEN_HEIGHT 200
#define SCREEN_WIDTH 320

#define PALETTE_INDEX 0x3CB
#define PALETTE_DATA 0x3c9
#define INPUT_STATUS 0x3DA
#define VRETRACE_BIT 0x08

#define NUM_KEYS 256

typedef int boolean;
#define true 1
#define false 0

typedef unsigned char byte;

byte far *VGA=(byte far *)0xA0000000L;

#define SETPIX(x,y,c) *(VGA+(x)+(y)*SCREEN_WIDTH)=c
#define GETPIX(x,y) *(VGA+(x)+(y)*SCREEN_WIDTH)
#define MAX(x,y) ((x) > (y) ? (x) : (y))
#define MIN(x,y) ((x) < (y) ? (x) : (y))
#define DUMMY_WRITE_CONDITIONAL(i, j) \
    do { \
        if (((i) + (j)) % 1000 == 0) { \
            volatile byte *dummy = VGA; \
            *dummy = *dummy; \
        } \
    } while (0)


byte playerBitmap[16 * 16] = {
    0, 0, 0, 0, 3, 3, 3, 3, 3, 3, 3, 3, 0, 0, 0, 0,
    0, 0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0,
    0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 3, 0, 0,
    0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 3, 0, 0,
    0, 0, 3, 0, 0, 0, 3, 0, 0, 0, 3, 0, 0, 3, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0, 3, 0, 0, 0, 0, 3, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0,
    0, 0, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 3, 0, 0,
    0, 0, 3, 0, 0, 3, 3, 0, 0, 3, 3, 0, 0, 3, 0, 0,
    0, 0, 3, 3, 3, 0, 0, 3, 3, 0, 0, 3, 3, 3, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
  };

typedef struct {
  int x, y;
  int width, height;
  byte *bitmap;
  byte *background;
  boolean physics;
  boolean falling;
  double jumping;
  double gravity;
} Sprite;

Sprite create_sprite(int x, int y, int width, int height, byte *bitmap, boolean physics) {
  Sprite sprite;
  sprite.x = x;
  sprite.y = y;
  sprite.width = width;
  sprite.height = height;
  sprite.bitmap = bitmap;
  sprite.background = (byte *)malloc(width * height * sizeof(byte));
  sprite.physics = physics;
  sprite.falling = false;
  sprite.jumping = 0;
  sprite.gravity = 0;
  return sprite;
}

#define MAX_SPRITES 100

Sprite *sprites[MAX_SPRITES];
int sprite_count = 0;

typedef struct {
  int x, y;
  int width, height;
  byte color;
  byte *background;
} Wall;


Wall create_wall(int x, int y, int width, int height, byte color) {
  Wall wall;
  wall.x = x;
  wall.y = y;
  wall.width = width;
  wall.height = height;
  wall.color = color;
  wall.background = (byte *)malloc(width * height * sizeof(byte));
  return wall;
}




#define MAX_WALLS 100

Wall walls[MAX_WALLS];
int wall_count = 0;

void add_sprite(Sprite *sprite) {
  if (sprite_count < MAX_SPRITES) {
    sprites[sprite_count++] = sprite;
  }
}


void drawBitmap(byte far *bitmap, int x, int y, int w, int h)
{
  int i, j;
  byte far *src;
  byte far *dst;

  /* Adjust the starting point and dimensions if the bitmap is partially off-screen */
  int startX = MAX(0, x);
  int startY = MAX(0, y);
  int endX = MIN(SCREEN_WIDTH, x + w);
  int endY = MIN(SCREEN_HEIGHT, y + h);

  src = bitmap + (startX - x) + (startY - y) * w;

  for (j = startY; j < endY; ++j) {
    for (i = startX; i < endX; ++i) {
      if(src[i - startX + (j - startY) * w] != (byte)0) {
        dst = VGA + i + j * SCREEN_WIDTH;
        *dst = src[i - startX + (j - startY) * w];
      }
    }
  }
}

void draw_rectangle(int x, int y, int w, int h, byte c) {
    int i, j;
    for (j = y; j < y + h; ++j) {
        for (i = x; i < x + w; ++i) {
            SETPIX(i, j, c);
        }
    }
}


void set_pixel(int x, int y, byte color) {
  VGA[x + y * SCREEN_WIDTH] = color;
}

void draw_background() {
  draw_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, 2);
}

void draw_sprite(Sprite *sprite) {
  drawBitmap(sprite->bitmap, sprite->x, sprite->y, sprite->width, sprite->height);
}

void move_sprite(Sprite *sprite, int dx, int dy) {
  sprite->x += dx;
  sprite->y += dy;
}

void move_sprite_collision(Sprite *sprite, int dx, int dy) {
    int collisionX = detectCollisionX(sprite, abs(dx));
    int collisionY = detectCollisionY(sprite, abs(dy));

    if (dx > 0) {
        if (collisionX > 0) {
            sprite->x = sprite->x + collisionX - sprite->width - 1;
            dx = 0;
        } else {
            sprite->x += dx;
        }
    } else if (dx < 0) {
        if (collisionX < 0) {
            sprite->x = sprite->x + collisionX + sprite->width - 1;
            dx = 0;
        } else {
            sprite->x += dx;
        }
    }

    if (dy > 0) {
        if (collisionY > 0) {
            sprite->y = sprite->y + collisionY - sprite->height - 1;
            dy = 0;
        } else {
            sprite->y += dy;
        }
    } else if (dy < 0) {
        if (collisionY < 0) {
            sprite->y = sprite->y + collisionY + sprite->height - 1;
            dy = 0;
        } else {
            sprite->y += dy;
        }
    }
}


void save_sprite_background(Sprite *sprite) {
    if (sprite->x >= 0 && sprite->x + sprite->width <= SCREEN_WIDTH &&
        sprite->y >= 0 && sprite->y + sprite->height <= SCREEN_HEIGHT) {
        int i, j;
        for (j = 0; j < sprite->height; ++j) {
            volatile byte far *dummy = VGA;
            *dummy = *dummy;
            for (i = 0; i < sprite->width; ++i) {
                int x = sprite->x + i;
                int y = sprite->y + j;
                sprite->background[i + j * sprite->width] = GETPIX(x, y);
            }
        }
    }
}




void load_sprite_background(Sprite *sprite) {
  int i, j;
  for (j = 0; j < sprite->height; ++j) {
    for (i = 0; i < sprite->width; ++i) {
      SETPIX(sprite->x + i, sprite->y + j, sprite->background[i + j * sprite->width]);
    }
  }
}



void save_sprite_backgrounds() {
  int i;
  for (i = 0; i < sprite_count; ++i) {
    save_sprite_background(sprites[i]);
  }
}

void load_sprite_backgrounds() {
  int i;
  for (i = 0; i < sprite_count; ++i) {
    load_sprite_background(sprites[i]);
  }
}

void draw_sprites() {
  int i;
  for (i = 0; i < sprite_count; ++i) {
    draw_sprite(sprites[i]);
  }
}

void remove_sprite(int index) {
  int i;

  if (index < 0 || index >= sprite_count) {
    return;
  }

  free(sprites[index]->background);
  free(sprites[index]);

  for (i = index; i < sprite_count - 1; ++i) {
    sprites[i] = sprites[i + 1];
  }

  sprite_count--;
}

void add_wall(Wall wall) {
  if (wall_count < MAX_WALLS) {
    walls[wall_count++] = wall;
  }
}

void draw_wall(Wall *wall) {
  draw_rectangle(wall->x, wall->y, wall->width, wall->height, wall->color);
}

void move_wall(Wall *wall, int dx, int dy) {
  wall->x += dx;
  wall->y += dy;
}

void remove_wall(int index) {
  int i;
  if (index < 0 || index >= wall_count) {
    return;
  }
  for (i = index; i < wall_count - 1; ++i) {
    walls[i] = walls[i + 1];
  }
  wall_count--;
}

void draw_walls() {
  int i;
  for (i = 0; i < wall_count; ++i) {
    draw_wall(&walls[i]);
  }
}

void save_wall_background(Wall *wall) {
    int i, j, k, x, y;
    if (wall->x >= 0 && wall->x + wall->width <= SCREEN_WIDTH &&
        wall->y >= 0 && wall->y + wall->height <= SCREEN_HEIGHT) {
        
        for (j = 0; j < wall->height; ++j) {
            for (i = 0; i < wall->width; ++i) {
                x = wall->x + i;
                y = wall->y + j;
                wall->background[i + j * wall->width] = GETPIX(x, y);
                k = 0;
                while (k < 0) {
                    k++;
                }
            }
        }
    }
}





void load_wall_background(Wall *wall) {
  int i, j;
  for (j = 0; j < wall->height; ++j) {
    for (i = 0; i < wall->width; ++i) {
      SETPIX(wall->x + i, wall->y + j, wall->background[i + j * wall->width]);
    }
  }
}


void save_wall_backgrounds() {
  int i;
  for (i = 0; i < wall_count; ++i) {
    save_wall_background(&walls[i]);
  }
}

void load_wall_backgrounds() {
  int i;
  for (i = 0; i < wall_count; ++i) {
    load_wall_background(&walls[i]);
  }
}




void wait_for_retrace()
{
  while( inp( INPUT_STATUS ) & VRETRACE_BIT );
  while( ! (inp( INPUT_STATUS) & VRETRACE_BIT) );
}

void set_mode(byte mode)
{
  union REGS regs;
  regs.h.ah = SET_MODE;
  regs.h.al = mode;
  int86( VIDEO_INT, &regs, &regs );
}

byte *get_sky_palette()
{
  byte *pal = (byte *)malloc((NUM_COLORS * 3) * sizeof(byte));
  int i;

  /* Transparent color (index 0)  */
  i = 0;
  pal[i*3 + 0] = 0;
  pal[i*3 + 1] = 0;
  pal[i*3 + 2] = 0;

  i = 1;
  pal[ i*3 + 0 ] = 255;
  pal[ i*3 + 1 ] = 10;
  pal[ i*3 + 2 ] = 10;
  /* deep red */

  i = 2;
  pal[ i*3 + 0 ] = 0;
  pal[ i*3 + 1 ] = 0;
  pal[ i*3 + 2 ] = 192;
  /* oxford blue */

  i = 3;
  pal[ i*3 + 0 ] = 0;
  pal[ i*3 + 1 ] = 255;
  pal[ i*3 + 2 ] = 255;
  /* aqua */

  i = 4;
  pal[ i*3 + 0 ] = 255;
  pal[ i*3 + 1 ] = 0;
  pal[ i*3 + 2 ] = 0;
  /* red */

  return pal;
}

void set_palette(byte *palette)
{
  int i;

  outp( PALETTE_INDEX, 0 );
  for( i = 0; i < NUM_COLORS * 3; ++i) {
    outp( PALETTE_DATA, palette[ i ] );
  }

}

int detectCollisionX(Sprite *sprite, int distance) {
    int i;
    int collisionDistance = 0;
    for (i = 0; i < wall_count; ++i) {
        Wall *wall = &walls[i];
        /* Check if the wall's x is within the range of the sprite's movement distance */
        if ((wall->x >= sprite->x - distance && wall->x <= sprite->x + sprite->width + distance) &&
            (wall->y < sprite->y + sprite->height && wall->y + wall->height > sprite->y)) {
            /* Calculate the collision distance */
            if (wall->x < sprite->x) {
                collisionDistance = sprite->x - wall->x - sprite->width;
            } else {
                collisionDistance = wall->x - sprite->x;
            }
            return collisionDistance;
        }
    }
    return 0;
}

int detectCollisionY(Sprite *sprite, int distance) {
    int i;
    int collisionDistance = 0;
    for (i = 0; i < wall_count; ++i) {
        Wall *wall = &walls[i];
        /* Check if the wall's y is within the range of the sprite's movement distance */
        if ((wall->y >= sprite->y - distance && wall->y <= sprite->y + sprite->height + distance) &&
            (wall->x < sprite->x + sprite->width && wall->x + wall->width > sprite->x)) {
            /* Calculate the collision distance */
            if (wall->y < sprite->y) {
                collisionDistance = sprite->y - wall->y - sprite->height;
            } else {
                collisionDistance = wall->y - sprite->y;
            }
            return collisionDistance;
        }
    }
    return 0;
}

void updatePhysicsSprites() {
    int i;

    for (i = 0; i < sprite_count; ++i) {
        if (sprites[i]->physics) {

            int collisionY = detectCollisionY(sprites[i], 1);
            if (collisionY <= 0) {
                sprites[i]->falling = true;
            } else {
                sprites[i]->falling = false;
                sprites[i]->gravity = 0;
            }

            if (sprites[i]->falling) {
                if(sprites[i]->gravity < 7) {
                    int newY = sprites[i]->y + sprites[i]->gravity;
                    int collisionY = detectCollisionY(sprites[i], newY - sprites[i]->y);
                    if (collisionY > 0) {
                        sprites[i]->y = sprites[i]->y + collisionY - sprites[i]->height;
                        sprites[i]->gravity = 0;
                    } else {
                        sprites[i]->gravity += 0.2;
                        move_sprite_collision(sprites[i], 0, sprites[i]->gravity);
                    }
                }
            }

            if(sprites[i]->jumping < 0) {
              sprites[i]->jumping += 0.2;
              move_sprite_collision(sprites[i], 0, sprites[i]->jumping);
            }

        }
    }
}




int main() {
    int kc = 0;
    char s[255];
    byte *pal;
    int keyPressed[NUM_KEYS] = {0}; /* Array to track key presses */
    int i; /* Variable for the for loop */

    Sprite *sprite = (Sprite *)malloc(sizeof(Sprite));
    Sprite *spritea = (Sprite *)malloc(sizeof(Sprite));

    Wall wall1 = create_wall(0, 166, SCREEN_WIDTH, 1, 3);
    Wall wall2 = create_wall(250, 91, 1, 75, 3);
    Wall wall3 = create_wall(100, 130, 75, 1, 3);

    *sprite = create_sprite(50, 100, 16, 16, playerBitmap, true);
    *spritea = create_sprite(100, 150, 16, 16, playerBitmap, false);

    add_sprite(sprite);
    add_sprite(spritea);

    add_wall(wall1);
    add_wall(wall2);
    add_wall(wall3);

    set_mode(VGA_256_COLOR_MODE);

    pal = get_sky_palette();
    set_palette(pal);

    clrscr();
    draw_background();

    save_sprite_backgrounds();
    save_wall_backgrounds();

    /* Loop until ESC pressed, main game loop */
    while (kc != 0x1b) {
        /* Wait for screen retrace */
        wait_for_retrace();

        /* Restore the background after moving the sprite */
        load_wall_backgrounds();
        load_sprite_backgrounds();

        updatePhysicsSprites();

        /* Keyboard Handling */
        if (kbhit()) {
            kc = getch();
            if (kc == 0 || kc == 224) { /* Handle extended keys */
                int ext_kc = getch();
                kc = ext_kc; /* Use only the second byte */
            }

            /* Update key state */
            if (kc < NUM_KEYS) {
                keyPressed[kc] = 1;
            }
        }

        /* Handle key states */
        if (keyPressed[0x4B] || keyPressed['a']) { /* Left arrow key or 'a' key */
            move_sprite_collision(sprite, -3, 0);
        }
        if (keyPressed[0x48] || keyPressed['w']) { /* Up arrow key or 'w' key */
            sprite -> jumping = -5;
            sprite -> gravity = 0;
        }
        if (keyPressed[0x4D] || keyPressed['d']) { /* Right arrow key or 'd' key */
            move_sprite_collision(sprite, 3, 0);
        }

        /* Save the background before drawing the sprite */
        save_wall_backgrounds();
        save_sprite_backgrounds();

        /* Draw the sprite */
        draw_walls();
        draw_sprites();

        /* Reset key state for the key that was pressed */
        if (kc < NUM_KEYS) {
            keyPressed[kc] = 0;
        }
    }

    set_mode(TEXT_MODE);

    free(pal);
    free(sprite);
    return 0;
}

