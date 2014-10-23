#ifndef _APP_MENU_H_
#define _APP_MENU_H_

#define MENU_TEXT_COLOR 0, 115, 255
#define NORMAL_TEXT_COLOR 255, 255, 255
#define DIVIDER_COLOR 6,140,158
#define LOG_COLOR_NORMAL 255,255,0
#define LOG_COLOR_RED 255,0,0
#define LOG_COLOR_BLUE 0,0,255

void menu_putc(char c);
void menu_set_color(uint8_t r, uint8_t g, uint8_t b);

#endif
