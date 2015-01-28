#ifndef _APP_DISPLAY_SERVER_H_
#define _APP_DISPLAY_SERVER_H_

typedef void (*renderer_t)(int keycode);

void display_server_start(void);
void display_server_stop(void);
void display_server_set_renderer(renderer_t r);
void display_server_refresh(void);
void display_server_pause(void);
void display_server_unpause(void);

typedef struct {
	bool pressed;
	lk_time_t time;
	bool repeat;
} keymap_t;

#define CHECK_AND_REPORT_KEY(code, value) \
	keymap[code].time+=delta; \
	if(value) { \
		bool report = false; \
		if(!keymap[code].pressed) {keymap[code].pressed=true; report=true; keymap[code].time=0;} \
\
		if((keymap[code].repeat && keymap[code].time>=200) || keymap[code].time>=500) \
		{report=true; keymap[code].time=0; keymap[code].repeat=true;} \
\
		if(report) return code; \
	} else if(keymap[code].time>200) {keymap[code].pressed=false; keymap[code].time=0; keymap[code].repeat=false;}

#endif
