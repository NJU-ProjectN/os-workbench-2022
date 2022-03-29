#include <os.h>
#include <devices.h>

static int sd_init(device_t *dev) {
  sd_t *sd = dev->ptr;
  if (!io_read(AM_DISK_CONFIG).present) {
    dev->ptr = NULL;
  } else {
    sd->blkcnt = io_read(AM_DISK_CONFIG).blkcnt;
    sd->blksz  = io_read(AM_DISK_CONFIG).blksz;
    sd->buf    = pmm->alloc(sd->blksz);
  }
  return 0;
}

static void blk_read(void *buf, int blkno, int blkcnt) {
  io_write(AM_DISK_BLKIO, false, buf, blkno, blkcnt);
  while (!io_read(AM_DISK_STATUS).ready) ;
}

static void blk_write(void *buf, int blkno, int blkcnt) {
  io_write(AM_DISK_BLKIO, true, buf, blkno, blkcnt);
  while (!io_read(AM_DISK_STATUS).ready) ;
}

static int sd_read(device_t *dev, int offset, void *buf, int count) {
  sd_t *sd = dev->ptr;
  panic_on(!sd, "no disk");
  uint32_t pos = 0;
  for (uint32_t st = ROUNDDOWN(offset, sd->blksz); pos < count; st = offset) {
    uint32_t n = sd->blksz - (offset - st);
    if (n > count - pos) n = count - pos;
    blk_read(sd->buf, st / sd->blksz, 1);
    memcpy((char *)buf + pos, sd->buf + offset - st, n);
    pos   += n;
    offset = st + sd->blksz;
  }
  return pos;
}

static int sd_write(device_t *dev, int offset, const void *buf, int count) {
  sd_t *sd = dev->ptr;
  panic_on(!sd, "no disk");
  uint32_t pos = 0;
  for (uint32_t st = ROUNDDOWN(offset, sd->blksz); pos < count; st = offset) {
    uint32_t n = sd->blksz - (offset - st);
    if (n > count - pos) n = count - pos;
    if (n < sd->blksz) {
      blk_read(sd->buf, st / sd->blksz, 1);
    }
    memcpy(sd->buf + offset - st, (char *)buf + pos, n);
    blk_write(sd->buf, st / sd->blksz, 1);
    pos   += n;
    offset = st + sd->blksz;
  }
  return pos;
}

devops_t sd_ops = {
  .init  = sd_init,
  .read  = sd_read,
  .write = sd_write,
};
