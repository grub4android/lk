#ifndef _APP_DISPLAY_SERVER_H_
#define _APP_DISPLAY_SERVER_H_

typedef void (*renderer_t)(int keycode);

void display_server_start(void);
void display_server_stop(void);
void display_server_set_renderer(renderer_t r);
void display_server_refresh(void);
void display_server_pause(void);
void display_server_unpause(void);

#endif
