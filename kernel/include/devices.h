typedef struct devops {
  int (*init)(device_t *dev);
  int (*read) (device_t *dev, int offset, void *buf, int count);
  int (*write)(device_t *dev, int offset, const void *buf, int count);
} devops_t;
extern devops_t tty_ops, fb_ops, sd_ops, input_ops;

struct device {
  const char *name;
  int id;
  void *ptr;
  devops_t *ops;
};

// Input
// -------------------------------------------------------------------

struct input_event {
  uint32_t ctrl: 1, alt: 1;
  uint32_t data: 16;
};

typedef struct {
  spinlock_t lock;
  sem_t event_sem;
  struct input_event *events;
  int front, rear;
  int capslock, shift_down[2], ctrl_down[2], alt_down[2];
} input_t;

// -------------------------------------------------------------------

// Sprite-based virtual graphic accelerator
// +--------------+---------------------------------+-------------
// |  Info (256B) | textures (65535 x 256B = 16M)   |  sprites
// +--------------+---------------------------------+-------------
//        |            |                            ^        |
//        |            v                        0x1000000    |
//        |          +---+                                   |
//        |          |   |  8 x 8 x 32bit (ARRGGBB) tiles    |
//        v          +---+                                   v
// struct display_info            set of {#texture, x, y, z, #display}
// -------------------------------------------------------------------

#define TEXTURE_W 8
#define TEXTURE_H 8
#define SPRITE_BRK 0x1000000

struct display_info {
  uint32_t width, height;
  uint32_t num_displays;
  uint32_t current;
  uint32_t num_textures, num_sprites;
};

struct texture {
  uint32_t pixels[TEXTURE_W * TEXTURE_H];
};

struct sprite {
  uint16_t texture, x, y;
  unsigned int display: 4;
  unsigned int z: 12;
} __attribute__((packed));

typedef struct {
  struct display_info *info;
  struct texture *textures;
  struct sprite *sprites;
} fb_t;

// -------------------------------------------------------------------
// Virtual console on fb

struct character {
  uint32_t metadata;
  unsigned char ch;
};

struct tty_queue {
  char *buf, *end, *front, *rear;
};

typedef struct {
  sem_t lock, cooked;
  device_t *fbdev; int display;
  int lines, columns, size;
  struct character *buf, *end, *cursor;
  struct tty_queue queue;
  uint8_t *dirty;
  struct sprite *sp_buf;
} tty_t;

// -------------------------------------------------------------------
// SCSI (Standard) Disk

typedef struct {
  uint32_t blkcnt, blksz;
  uint8_t *buf;
} sd_t;
