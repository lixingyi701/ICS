#include "common.h"

#define NAME(key) \
  [_KEY_##key] = #key,


extern void getScreen(int* p_width, int* p_height);

static const char *keyname[256] __attribute__((used)) = {
  [_KEY_NONE] = "NONE",
  _KEYS(NAME)
};

size_t events_read(void *buf, size_t len) {
  char buffer[40];
  int key = _read_key();
  int down = 0;
  if(key & 0x8000) {
      key ^= 0x8000;
      down = 1;
  }
  if(key != _KEY_NONE) {
     sprintf(buffer, "%s %s\n", down ? "kd": "ku", keyname[key]);
  }
  else {
      sprintf(buffer,"t %d\n", _uptime());
  }
  if(strlen(buffer) <= len) { 
    strncpy((char*)buf, buffer,strlen(buffer));
	  return strlen(buffer);
  } 
  return 0;
}

static char dispinfo[128] __attribute__((used));

void dispinfo_read(void *buf, off_t offset, size_t len) {
  strncpy(buf, dispinfo + offset, len);
}

void fb_write(const void *buf, off_t offset, size_t len) {
  assert(offset % 4 == 0 && len % 4 == 0);
  int index, screen_x, screen_y;
  int w = 0; 
  int h = 0;
  getScreen(&w, &h);
  for(int i = 0; i < len / 4; i++) {
    index = offset / 4 + i;
    screen_y = index / w;
    screen_x = index % w;
    _draw_rect(buf + i *4, screen_x, screen_y, 1, 1);
  }

}

void init_device() {
  _ioe_init();

  int w = 0, h = 0;
  getScreen(&w, &h);
  sprintf(dispinfo, "WIDTH:%d\nHEIGHT:%d\n", w,h);
}
