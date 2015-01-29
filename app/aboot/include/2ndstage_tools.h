#ifndef _2NDSTAGE_TOOLS_H_
#define _2NDSTAGE_TOOLS_H_

struct original_fdt_property {
	const char *name;
	void *data;
	int len;
};

int parse_original_devtree(void *fdt);
char* sndstage_extend_cmdline(char* cmdline);

struct original_atags_info {
	char* cmdline;
	uint32_t platform_id;
	uint32_t variant_id;
	uint32_t soc_rev;
#if DEVICE_TREE
	struct original_fdt_property* chosen_props;
	uint32_t num_chosen_props;
#endif
};

// parsed atag info
struct original_atags_info* board_get_original_atags_info(void);
int board_has_original_atags_info(void);
void board_parse_original_atags(void);

// original tags
extern void* original_atags;

#endif
