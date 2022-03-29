#include <os.h>
#include <devices.h>

#define DEVICES(_) \
  _(0, input_t, "input",    1, &input_ops) \
  _(1, fb_t,    "fb",       1, &fb_ops) \
  _(2, tty_t,   "tty1",     1, &tty_ops) \
  _(3, tty_t,   "tty2",     2, &tty_ops) \
  _(4, sd_t,    "sda",      1, &sd_ops) \

#define DEV_CNT(...) + 1
device_t *devices[0 DEVICES(DEV_CNT)];

static device_t *dev_lookup(const char *name) {
  for (int i = 0; i < LENGTH(devices); i++) 
    if (strcmp(devices[i]->name, name) == 0)
      return devices[i];
  panic("lookup device failed.");
  return NULL;
}

static device_t *dev_create(int size, const char* name, int id, devops_t *ops) {
  device_t *dev = pmm->alloc(sizeof(device_t));
  *dev = (device_t) {
    .name = name,
    .ptr  = pmm->alloc(size),
    .id   = id,
    .ops  = ops,
  };
  return dev;
}

void dev_input_task();
void dev_tty_task();

static void dev_init() {
#define INIT(id, device_type, dev_name, dev_id, dev_ops) \
  devices[id] = dev_create(sizeof(device_type), dev_name, dev_id, dev_ops); \
  devices[id]->ops->init(devices[id]);

  DEVICES(INIT);

  kmt->create(pmm->alloc(sizeof(task_t)), "input-task", dev_input_task, NULL);
  kmt->create(pmm->alloc(sizeof(task_t)), "tty-task",   dev_tty_task,   NULL);
}

MODULE_DEF(dev) = {
  .init   = dev_init,
  .lookup = dev_lookup,
};
