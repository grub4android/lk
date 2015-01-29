#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include <board.h>
#include <libfdt.h>
#include <2ndstage_tools.h>

struct nameval {
	char *name;
	char *value;
	int written;
};

struct dt_entry_v1
{
	uint32_t platform_id;
	uint32_t variant_id;
	uint32_t soc_rev;
	uint32_t offset;
	uint32_t size;
};

static int str2nameval(char* str, struct nameval *nameval) {
	char *c;
	int index;

	// get index of delimiter
	c = strchr(str, '=');
	if(c==NULL) {
		nameval->name = strdup(str);
		return -1;
	}
	index = (int)(c - str);

	// get name
	nameval->name = malloc(index+1);
	memcpy(nameval->name, str, index);
	nameval->name[index] = 0;

	// get value
	nameval->value = strdup(str+index+1);

	return 0;
}

static int cmdline2array(char* cmdline, struct nameval **ext_nv) {
	struct nameval *namevals = NULL;
	char* sep = " ";
	unsigned count = 0;
	
	char *pch = strtok(cmdline, sep);
	while (pch != NULL) {
		namevals = (void*) realloc (namevals, ++count * sizeof(struct nameval));
		struct nameval* nameval = &namevals[count-1];
		memset(nameval, 0, sizeof(struct nameval));

		str2nameval(pch, nameval);

		pch = strtok(NULL, sep);
	}

	*ext_nv = namevals;
	return count;
}

static void free_namevals(struct nameval *namevals, int count) {
	int i;
	if(namevals==NULL) return;

	for(i=0; i<count; i++) {
		free(namevals[i].name);
		free(namevals[i].value);
	}

	free(namevals);
}

static int namevals_get(struct nameval *namevals, int count, const char *name) {
	int i;

	for(i=0; i<count; i++) {
		if(!strcmp(name, namevals[i].name))
			return i;
	}

	return -1;
}

static void add_nameval_to_cmdline(struct nameval *nv, char **cmdline, int *len) {
	int has_value = nv->value!=NULL;
	int new_len = *len;

	// calculate new length
	new_len += *len==1 ? 0 : 1;  // ' '
	new_len += strlen(nv->name); // name
	new_len += has_value? 1 : 0; // '='
	new_len += has_value?strlen(nv->value):0; // value

	// realloc
	*cmdline = realloc(*cmdline, new_len);

	// append namevalue to cmdline
	sprintf(*cmdline + *len -1, "%s%s%s%s", *len==1?"":" ", nv->name, has_value?"=":"", has_value?nv->value:"");
	
	// update length
	*len = new_len;
}

char* sndstage_extend_cmdline(char* cmdline) {
	struct nameval *nv_orig = NULL;
	struct nameval *nv_bootimg = NULL;
	int num_orig = 0;
	int num_bootimg = 0;
	int i;
	char* cmdline_new = calloc(1, 1);
	int cmdline_len = 1;

	// get cmdlines
	struct original_atags_info *atags_info = board_get_original_atags_info();
	char* cmdline_orig = strdup(atags_info->cmdline);
	char* cmdline_bootimg = strdup(cmdline);

	// generate arrays
	num_orig = cmdline2array(strdup(cmdline_orig), &nv_orig);
	num_bootimg = cmdline2array(strdup(cmdline_bootimg), &nv_bootimg);

	// free cmdlines
	free(cmdline_orig);
	free(cmdline_bootimg);

	// write bootimg args but prefer orignal ones
	for(i=0; i<num_bootimg; i++) {
		// choose nv
		struct nameval *nv = &nv_bootimg[i];
		int pos = namevals_get(nv_orig, num_orig, nv_bootimg[i].name);
		if(pos>=0) nv = &nv_orig[pos];

		//printf("IMG=[%s]=[%s]\n", nv->name, nv->value);

		// add nv
		add_nameval_to_cmdline(nv, &cmdline_new, &cmdline_len);
		nv->written = 1;
	}

	// write the rest of original args
	for(i=0; i<num_orig; i++) {
		struct nameval *nv = &nv_orig[i];

		if(nv->written) continue;

		//printf("ORIG=[%s]=[%s]\n", nv->name, nv->value);

		// add nv
		add_nameval_to_cmdline(nv, &cmdline_new, &cmdline_len);
		nv->written = 1;
	}

	// free arrays
	free_namevals(nv_orig, num_orig);
	free_namevals(nv_bootimg, num_bootimg);

	return cmdline_new;
}

static struct dt_entry* get_dt_entry(void *dtb, uint32_t dtb_size)
{
	int root_offset;
	const void *prop = NULL;
	const char *plat_prop = NULL;
	const char *board_prop = NULL;
	const char *pmic_prop = NULL;
	char *model = NULL;
	struct dt_entry *cur_dt_entry = NULL;
	struct dt_entry *dt_entry_array = NULL;
	struct board_id *board_data = NULL;
	struct plat_id *platform_data = NULL;
	struct pmic_id *pmic_data = NULL;
	int len;
	int len_board_id;
	int len_plat_id;
	int min_plat_id_len = 0;
	int len_pmic_id;
	uint32_t dtb_ver;
	uint32_t num_entries = 0;
	uint32_t i, j, k, n;
	uint32_t msm_data_count;
	uint32_t board_data_count;
	uint32_t pmic_data_count;

	root_offset = fdt_path_offset(dtb, "/");
	if (root_offset < 0)
		return NULL;

	prop = fdt_getprop(dtb, root_offset, "model", &len);
	if (prop && len > 0) {
		model = (char *) malloc(sizeof(char) * len);
		ASSERT(model);
		strlcpy(model, prop, len);
	} else {
		dprintf(INFO, "model does not exist in device tree\n");
	}
	/* Find the pmic-id prop from DTB , if pmic-id is present then
	* the DTB is version 3, otherwise find the board-id prop from DTB ,
	* if board-id is present then the DTB is version 2 */
	pmic_prop = (const char *)fdt_getprop(dtb, root_offset, "qcom,pmic-id", &len_pmic_id);
	board_prop = (const char *)fdt_getprop(dtb, root_offset, "qcom,board-id", &len_board_id);
	if (pmic_prop && (len_pmic_id > 0) && board_prop && (len_board_id > 0)) {
		if ((len_pmic_id % PMIC_ID_SIZE) || (len_board_id % BOARD_ID_SIZE))
		{
			dprintf(CRITICAL, "qcom,pmic-id(%d) or qcom,board-id(%d) in device tree is not a multiple of (%d %d)\n",
				len_pmic_id, len_board_id, PMIC_ID_SIZE, BOARD_ID_SIZE);
			return NULL;
		}
		dtb_ver = DEV_TREE_VERSION_V3;
		min_plat_id_len = PLAT_ID_SIZE;
	} else if (board_prop && len_board_id > 0) {
		if (len_board_id % BOARD_ID_SIZE)
		{
			dprintf(CRITICAL, "qcom,board-id in device tree is (%d) not a multiple of (%d)\n",
				len_board_id, BOARD_ID_SIZE);
			return NULL;
		}
		dtb_ver = DEV_TREE_VERSION_V2;
		min_plat_id_len = PLAT_ID_SIZE;
	} else {
		dtb_ver = DEV_TREE_VERSION_V1;
		min_plat_id_len = DT_ENTRY_V1_SIZE;
	}

	/* Get the msm-id prop from DTB */
	plat_prop = (const char *)fdt_getprop(dtb, root_offset, "qcom,msm-id", &len_plat_id);
	if (!plat_prop || len_plat_id <= 0) {
		dprintf(INFO, "qcom,msm-id entry not found\n");
		return NULL;
	} else if (len_plat_id % min_plat_id_len) {
		dprintf(INFO, "qcom,msm-id in device tree is (%d) not a multiple of (%d)\n",
			len_plat_id, min_plat_id_len);
		return NULL;
	}

	/*
	 * If DTB version is '1' look for <x y z> pair in the DTB
	 * x: platform_id
	 * y: variant_id
	 * z: SOC rev
	 */
	if (dtb_ver == DEV_TREE_VERSION_V1) {
		cur_dt_entry = (struct dt_entry *)
				malloc(sizeof(struct dt_entry));

		if (!cur_dt_entry) {
			dprintf(CRITICAL, "Out of memory\n");
			return NULL;
		}
		memset(cur_dt_entry, 0, sizeof(struct dt_entry));

		cur_dt_entry->platform_id = fdt32_to_cpu(((const struct dt_entry_v1 *)plat_prop)->platform_id);
		cur_dt_entry->variant_id = fdt32_to_cpu(((const struct dt_entry_v1 *)plat_prop)->variant_id);
		cur_dt_entry->soc_rev = fdt32_to_cpu(((const struct dt_entry_v1 *)plat_prop)->soc_rev);
		cur_dt_entry->board_hw_subtype =
			fdt32_to_cpu(((const struct dt_entry_v1 *)plat_prop)->variant_id) >> 0x18;
		cur_dt_entry->pmic_rev[0] = board_pmic_target(0);
		cur_dt_entry->pmic_rev[1] = board_pmic_target(1);
		cur_dt_entry->pmic_rev[2] = board_pmic_target(2);
		cur_dt_entry->pmic_rev[3] = board_pmic_target(3);
		cur_dt_entry->offset = (uint32_t)dtb;
		cur_dt_entry->size = dtb_size;
	}
	/*
	 * If DTB Version is '3' then we have split DTB with board & msm data & pmic
	 * populated saperately in board-id & msm-id & pmic-id prop respectively.
	 * Extract the data & prepare a look up table
	 */
	else if (dtb_ver == DEV_TREE_VERSION_V2 || dtb_ver == DEV_TREE_VERSION_V3) {
		board_data_count = (len_board_id / BOARD_ID_SIZE);
		msm_data_count = (len_plat_id / PLAT_ID_SIZE);
		/* If dtb version is v2.0, the pmic_data_count will be <= 0 */
		pmic_data_count = (len_pmic_id / PMIC_ID_SIZE);

		/* If we are using dtb v3.0, then we have split board, msm & pmic data in the DTB
		*  If we are using dtb v2.0, then we have split board & msmdata in the DTB
		*/
		board_data = (struct board_id *) malloc(sizeof(struct board_id) * (len_board_id / BOARD_ID_SIZE));
		ASSERT(board_data);
		platform_data = (struct plat_id *) malloc(sizeof(struct plat_id) * (len_plat_id / PLAT_ID_SIZE));
		ASSERT(platform_data);
		if (dtb_ver == DEV_TREE_VERSION_V3) {
			pmic_data = (struct pmic_id *) malloc(sizeof(struct pmic_id) * (len_pmic_id / PMIC_ID_SIZE));
			ASSERT(pmic_data);
		}
		i = 0;

		/* Extract board data from DTB */
		for(i = 0 ; i < board_data_count; i++) {
			board_data[i].variant_id = fdt32_to_cpu(((struct board_id *)board_prop)->variant_id);
			board_data[i].platform_subtype = fdt32_to_cpu(((struct board_id *)board_prop)->platform_subtype);
			/* For V2/V3 version of DTBs we have platform version field as part
			 * of variant ID, in such case the subtype will be mentioned as 0x0
			 * As the qcom, board-id = <0xSSPMPmPH, 0x0>
			 * SS -- Subtype
			 * PM -- Platform major version
			 * Pm -- Platform minor version
			 * PH -- Platform hardware CDP/MTP
			 * In such case to make it compatible with LK algorithm move the subtype
			 * from variant_id to subtype field
			 */
			if (board_data[i].platform_subtype == 0)
				board_data[i].platform_subtype =
					fdt32_to_cpu(((struct board_id *)board_prop)->variant_id) >> 0x18;

			len_board_id -= sizeof(struct board_id);
			board_prop += sizeof(struct board_id);
		}

		/* Extract platform data from DTB */
		for(i = 0 ; i < msm_data_count; i++) {
			platform_data[i].platform_id = fdt32_to_cpu(((struct plat_id *)plat_prop)->platform_id);
			platform_data[i].soc_rev = fdt32_to_cpu(((struct plat_id *)plat_prop)->soc_rev);
			len_plat_id -= sizeof(struct plat_id);
			plat_prop += sizeof(struct plat_id);
		}

		if (dtb_ver == DEV_TREE_VERSION_V3 && pmic_prop) {
			/* Extract pmic data from DTB */
			for(i = 0 ; i < pmic_data_count; i++) {
				pmic_data[i].pmic_version[0]= fdt32_to_cpu(((struct pmic_id *)pmic_prop)->pmic_version[0]);
				pmic_data[i].pmic_version[1]= fdt32_to_cpu(((struct pmic_id *)pmic_prop)->pmic_version[1]);
				pmic_data[i].pmic_version[2]= fdt32_to_cpu(((struct pmic_id *)pmic_prop)->pmic_version[2]);
				pmic_data[i].pmic_version[3]= fdt32_to_cpu(((struct pmic_id *)pmic_prop)->pmic_version[3]);
				len_pmic_id -= sizeof(struct pmic_id);
				pmic_prop += sizeof(struct pmic_id);
			}

			/* We need to merge board & platform data into dt entry structure */
			num_entries = msm_data_count * board_data_count * pmic_data_count;
		} else {
			/* We need to merge board & platform data into dt entry structure */
			num_entries = msm_data_count * board_data_count;
		}

		if ((((uint64_t)msm_data_count * (uint64_t)board_data_count * (uint64_t)pmic_data_count) !=
			msm_data_count * board_data_count * pmic_data_count) ||
			(((uint64_t)msm_data_count * (uint64_t)board_data_count) != msm_data_count * board_data_count)) {

			free(board_data);
			free(platform_data);
			if (pmic_data)
				free(pmic_data);
			if (model)
				free(model);
			return NULL;
		}

		dt_entry_array = (struct dt_entry*) malloc(sizeof(struct dt_entry) * num_entries);
		ASSERT(dt_entry_array);

		/* If we have '<X>; <Y>; <Z>' as platform data & '<A>; <B>; <C>' as board data.
		 * Then dt entry should look like
		 * <X ,A >;<X, B>;<X, C>;
		 * <Y ,A >;<Y, B>;<Y, C>;
		 * <Z ,A >;<Z, B>;<Z, C>;
		 */
		i = 0;
		k = 0;
		n = 0;
		for (i = 0; i < msm_data_count; i++) {
			for (j = 0; j < board_data_count; j++) {
				if (dtb_ver == DEV_TREE_VERSION_V3 && pmic_prop) {
					for (n = 0; n < pmic_data_count; n++) {
						dt_entry_array[k].platform_id = platform_data[i].platform_id;
						dt_entry_array[k].soc_rev = platform_data[i].soc_rev;
						dt_entry_array[k].variant_id = board_data[j].variant_id;
						dt_entry_array[k].board_hw_subtype = board_data[j].platform_subtype;
						dt_entry_array[k].pmic_rev[0]= pmic_data[n].pmic_version[0];
						dt_entry_array[k].pmic_rev[1]= pmic_data[n].pmic_version[1];
						dt_entry_array[k].pmic_rev[2]= pmic_data[n].pmic_version[2];
						dt_entry_array[k].pmic_rev[3]= pmic_data[n].pmic_version[3];
						dt_entry_array[k].offset = (uint32_t)dtb;
						dt_entry_array[k].size = dtb_size;
						k++;
					}

				} else {
					dt_entry_array[k].platform_id = platform_data[i].platform_id;
					dt_entry_array[k].soc_rev = platform_data[i].soc_rev;
					dt_entry_array[k].variant_id = board_data[j].variant_id;
					dt_entry_array[k].board_hw_subtype = board_data[j].platform_subtype;
					dt_entry_array[k].pmic_rev[0]= board_pmic_target(0);
					dt_entry_array[k].pmic_rev[1]= board_pmic_target(1);
					dt_entry_array[k].pmic_rev[2]= board_pmic_target(2);
					dt_entry_array[k].pmic_rev[3]= board_pmic_target(3);
					dt_entry_array[k].offset = (uint32_t)dtb;
					dt_entry_array[k].size = dtb_size;
					k++;
				}
			}
		}

		if (num_entries>0) {
			dprintf(SPEW, "Found an appended flattened device tree (%s - %u %u %u 0x%x)\n",
				*model ? model : "unknown",
				dt_entry_array[i].platform_id, dt_entry_array[i].variant_id, dt_entry_array[i].board_hw_subtype, dt_entry_array[i].soc_rev);

			// allocate dt entry
			cur_dt_entry = (struct dt_entry *) malloc(sizeof(struct dt_entry));
			if (!cur_dt_entry) {
				dprintf(CRITICAL, "Out of memory\n");
				return NULL;
			}

			// copy entry
			memcpy(cur_dt_entry, &dt_entry_array[i], sizeof(struct dt_entry));
		}

		free(board_data);
		free(platform_data);
		if (pmic_data)
			free(pmic_data);
		free(dt_entry_array);
	}
	if (model)
		free(model);

	return cur_dt_entry;
}

int parse_original_devtree(void *fdt) {
	int ret = 0;
	int len = 0;
	uint32_t offset = 0;
	const char* str_prop = NULL;
	int prop;
	struct original_atags_info* info = board_get_original_atags_info();
	uint32_t dtb_size;

	/* Check the device tree header */
	ret = fdt_check_header(fdt);
	if (ret)
	{
		dprintf(CRITICAL, "Invalid device tree header \n");
		return ret;
	}

	/* get size */
	dtb_size = fdt_totalsize(fdt);

	/* Get offset of the chosen node */
	ret = fdt_path_offset(fdt, "/chosen");
	if (ret < 0)
	{
		dprintf(CRITICAL, "Could not find chosen node.\n");
		return ret;
	}
	offset = ret;

	/* backup chosen properties */
	info->chosen_props = NULL;
	info->num_chosen_props = 0;
	prop = fdt_first_property_offset(fdt, offset);
	for(;;) {
		if(prop<0) break;

		// get prop info
		const char *name;
		const void *data = fdt_getprop_by_offset(fdt, prop, &name, &len);

		// backup prop
		info->chosen_props = realloc(info->chosen_props, ++info->num_chosen_props*sizeof(struct original_fdt_property));
		info->chosen_props[info->num_chosen_props-1].data = malloc(len);
		memcpy(info->chosen_props[info->num_chosen_props-1].data, data, len);
		info->chosen_props[info->num_chosen_props-1].name = name?strdup(name):NULL;
		info->chosen_props[info->num_chosen_props-1].len = len;

		// next prop
		prop = fdt_next_property_offset(fdt, prop);
	}

	/* store cmdline */
	str_prop = (const char *)fdt_getprop(fdt, offset, "bootargs", &len);
	if (str_prop && len>0)
	{
		info->cmdline = strdup(str_prop);
	}

	/* get dt entry */
	struct dt_entry* dt_entry = get_dt_entry(fdt, dtb_size);
	if(!dt_entry) {
		dprintf(CRITICAL, "Could not find dt entry.\n");
		return -1;
	}

	/* store dt entry information */
	info->platform_id = (dt_entry->platform_id);
	info->variant_id = (dt_entry->variant_id);
	info->soc_rev = (dt_entry->soc_rev);

	return 0;
}

static struct original_atags_info original_atags_info = {0,0,0,0,0,0};
static bool has_original_atags_info = 0;

struct original_atags_info* board_get_original_atags_info(void) {
	return &original_atags_info;
}

int board_has_original_atags_info(void) {
	return has_original_atags_info;
}

void board_parse_original_atags(void)
{
	int err = 0;
#if DEVICE_TREE
	err = parse_original_devtree(original_atags);
#else
	dprintf(CRITICAL, "parsing atags isn't implemented!");
	err = -1;
#endif

	if(!err) {
		has_original_atags_info = 1;
		dprintf(INFO, "socinfo: platform=%u variant=0x%x soc_rev=0x%x\n",
			original_atags_info.platform_id, original_atags_info.variant_id,
			original_atags_info.soc_rev);
	}
	else dprintf(CRITICAL, "error parsing original atags info!");
}
