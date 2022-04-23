#include <os.h>
#include <devices.h>

#define NEVENTS 128
static sem_t sem_kbdirq;
static char keymap[][2];

static struct input_event event(int ctrl, int alt, int data) {
  return (struct input_event) {
    .ctrl = ctrl,
    .alt  = alt,
    .data = data,
  };
}

static int is_empty(input_t *in) {
  return in->rear == in->front;
}

static void push_event(input_t *in, struct input_event ev) {
  kmt->spin_lock(&in->lock);
  in->events[in->rear] = ev;
  in->rear = (in->rear + 1) % NEVENTS;
  panic_on(is_empty(in), "input queue full");
  kmt->spin_unlock(&in->lock);
  kmt->sem_signal(&in->event_sem);
}

static struct input_event pop_event(input_t *in) {
  kmt->sem_wait(&in->event_sem);
  kmt->spin_lock(&in->lock);
  panic_on(is_empty(in), "input queue empty");
  int idx = in->front;
  in->front = (in->front + 1) % NEVENTS;
  struct input_event ret = in->events[idx];
  kmt->spin_unlock(&in->lock);
  return ret;
}

static void input_keydown(device_t *dev, AM_INPUT_KEYBRD_T key) {
  input_t *in = dev->ptr;
  int ch;

  if (key.keydown) {
    // keydown
    switch (key.keycode) {
      case AM_KEY_CAPSLOCK: in->capslock     ^= 1; break; 
      case AM_KEY_LCTRL:    in->ctrl_down[0]  = 1; break; 
      case AM_KEY_RCTRL:    in->ctrl_down[1]  = 1; break; 
      case AM_KEY_LALT:     in->alt_down[0]   = 1; break; 
      case AM_KEY_RALT:     in->alt_down[1]   = 1; break; 
      case AM_KEY_LSHIFT:   in->shift_down[0] = 1; break; 
      case AM_KEY_RSHIFT:   in->shift_down[1] = 1; break; 
      default:
        ch = keymap[key.keycode][0];
        if (ch) {
          int shift = in->shift_down[0] || in->shift_down[1];
          int ctrl  = in->ctrl_down[0]  || in->ctrl_down[1];
          int alt   = in->alt_down[0]   || in->alt_down[1];

          if (ctrl || alt) {
            push_event(in, event(ctrl, alt, ch));
          } else {
            if (ch >= 'a' && ch <= 'z') {
              shift ^= in->capslock;
            }
            if (shift) {
              push_event(in, event(0, 0, keymap[key.keycode][1]));
            } else {
              push_event(in, event(0, 0, ch));
            }
          }
        }
    }
  } else {
    // keyup
    switch (key.keycode) {
      case AM_KEY_LCTRL:  in->ctrl_down[0]  = 0; break; 
      case AM_KEY_RCTRL:  in->ctrl_down[1]  = 0; break; 
      case AM_KEY_LALT:   in->alt_down[0]   = 0; break; 
      case AM_KEY_RALT:   in->alt_down[1]   = 0; break; 
      case AM_KEY_LSHIFT: in->shift_down[0] = 0; break; 
      case AM_KEY_RSHIFT: in->shift_down[1] = 0; break; 
    }
  }
}

static Context *input_notify(Event ev, Context *context) {
  kmt->sem_signal(&sem_kbdirq);
  return NULL;
}

static int input_init(device_t *dev) {
  input_t *in = dev->ptr;
  *in = (input_t) {};
  in->events = pmm->alloc(sizeof(in->events[0]) * NEVENTS);

  kmt->spin_init(&in->lock, "/dev/input lock");
  kmt->sem_init(&in->event_sem, "events in queue", 0);
  kmt->sem_init(&sem_kbdirq, "keyboard-interrupt", 0);

  os->on_irq(0, EVENT_IRQ_IODEV, input_notify);
  os->on_irq(0, EVENT_IRQ_TIMER, input_notify);
  return 0;
}

static int input_read(device_t *dev, int offset, void *buf, int count) {
  struct input_event ev = pop_event(dev->ptr);
  if (count >= sizeof(ev)) {
    memcpy(buf, &ev, sizeof(ev));
    return sizeof(ev);
  } else {
    return 0;
  }
}

static int input_write(device_t *dev, int offset, const void *buf, int count) {
  return 0;
}

devops_t input_ops = {
  .init  = input_init,
  .read  = input_read,
  .write = input_write,
};

static char keymap[256][2] = {
  [AM_KEY_0] = { '0', ')' },
  [AM_KEY_1] = { '1', '!' }, 
  [AM_KEY_2] = { '2', '@' },
  [AM_KEY_3] = { '3', '#' },
  [AM_KEY_4] = { '4', '$' },
  [AM_KEY_5] = { '5', '%' },
  [AM_KEY_6] = { '6', '^' },
  [AM_KEY_7] = { '7', '&' },
  [AM_KEY_8] = { '8', '*' },
  [AM_KEY_9] = { '9', '(' },
  [AM_KEY_A] = { 'a', 'A' },
  [AM_KEY_B] = { 'b', 'B' },
  [AM_KEY_C] = { 'c', 'C' },
  [AM_KEY_D] = { 'd', 'D' },
  [AM_KEY_E] = { 'e', 'E' },
  [AM_KEY_F] = { 'f', 'F' },
  [AM_KEY_G] = { 'g', 'G' },
  [AM_KEY_H] = { 'h', 'H' },
  [AM_KEY_I] = { 'i', 'I' },
  [AM_KEY_J] = { 'j', 'J' },
  [AM_KEY_K] = { 'k', 'K' },
  [AM_KEY_L] = { 'l', 'L' },
  [AM_KEY_M] = { 'm', 'M' },
  [AM_KEY_N] = { 'n', 'N' },
  [AM_KEY_O] = { 'o', 'O' },
  [AM_KEY_P] = { 'p', 'P' },
  [AM_KEY_Q] = { 'q', 'Q' },
  [AM_KEY_R] = { 'r', 'R' },
  [AM_KEY_S] = { 's', 'S' },
  [AM_KEY_T] = { 't', 'T' },
  [AM_KEY_U] = { 'u', 'U' },
  [AM_KEY_V] = { 'v', 'V' },
  [AM_KEY_W] = { 'w', 'W' },
  [AM_KEY_X] = { 'x', 'X' },
  [AM_KEY_Y] = { 'y', 'Y' },
  [AM_KEY_Z] = { 'z', 'Z' },
  [AM_KEY_GRAVE]        = { '`', '~' },
  [AM_KEY_MINUS]        = { '-', '_' },
  [AM_KEY_EQUALS]       = { '=', '+' },
  [AM_KEY_LEFTBRACKET]  = { '[', '{' },
  [AM_KEY_RIGHTBRACKET] = { ']', '}' },
  [AM_KEY_BACKSLASH]    = { '\\', '|' },
  [AM_KEY_SEMICOLON]    = { ';', ':' },
  [AM_KEY_APOSTROPHE]   = { '\'', '"' },
  [AM_KEY_RETURN]       = { '\n', '\n' },
  [AM_KEY_COMMA]        = { ',', '<' },
  [AM_KEY_PERIOD]       = { '.', '>' },
  [AM_KEY_SLASH]        = { '/', '?' },
  [AM_KEY_SPACE]        = { ' ', ' ' },
  [AM_KEY_BACKSPACE]    = { '\b', '\b' },
};  

// input daemon
// ------------------------------------------------------------------

void dev_input_task(void *args) {
  device_t *in = dev->lookup("input");
  uint32_t known_time = io_read(AM_TIMER_UPTIME).us;

  while (1) {
    uint32_t time;
    AM_INPUT_KEYBRD_T key;
    while ((key = io_read(AM_INPUT_KEYBRD)).keycode != 0) {
      input_keydown(in, key);
    }
    time = io_read(AM_TIMER_UPTIME).us;
    if ((time - known_time) / 1000 > 100 && is_empty(in->ptr)) {
      push_event(in->ptr, event(0, 0, 0));
      known_time = time;
    }
    kmt->sem_wait(&sem_kbdirq);
  }
}
