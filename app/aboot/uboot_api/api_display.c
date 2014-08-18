/*
 * Copyright (c) 2011 The Chromium OS Authors.
 * SPDX-License-Identifier:	GPL-2.0+
 */

//#include <common.h>
#include <debug.h>
#include <api_public.h>
#include <dev/fbcon.h>
//#include <lcd.h>
//#include <video_font.h> /* Get font width and height */

/* TODO(clchiou): add support of video device */

int display_get_info(int type, struct display_info *di)
{
	struct fbcon_config* config = NULL;

	if (!di)
		return API_EINVAL;

	switch (type) {
	default:
		dprintf(INFO, "%s: unsupport display device type: %d\n",
				__FILE__, type);
		return API_ENODEV;

	case DISPLAY_TYPE_LCD:
		config = fbcon_display();

		di->pixel_width  = config->width;
		di->pixel_height = config->height;
		di->screen_rows = 0;
		di->screen_cols = 0;
#if DISPLAY_USE_RGB
		di->color_format = DISPLAY_COLOR_FORMAT_RGB888;
#else
		di->color_format = DISPLAY_COLOR_FORMAT_BGR888;
#endif
		break;
	}

	di->type = type;
	return 0;
}

int display_draw_bitmap(ulong bitmap, int x, int y)
{
	if (!bitmap)
		return API_EINVAL;

	return 0;
}

void display_clear(void)
{
	fbcon_clear();
}
