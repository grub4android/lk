#include <string.h>
#include <stdlib.h>
#include <printf.h>
#include <board.h>

struct nameval {
	char *name;
	char *value;
	int written;
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
