#include "vfs.h"
#include "vnode.h"
#include <string.h>
#include "device_vops.h"

static struct device_entry{
	char* name[20];
	struct vnode * vn;
	struct device_entry * next;
};

static struct device_entry * device_list;

static struct vnode * find_device(char * name){
	struct device_entry * curr = device_list;
	for(; curr != NULL; curr = curr->next){
		if(strcmp(name, curr->name)){
			return curr->vn;
		}
	}
	return NULL;
}

int vfs_lookup(char *path, struct vnode **retval) {
    struct vnode *startvn;
	int result;

	startvn = find_device(path);
	if(startvn!= NULL){
		*retval = startvn;
		return 0;
	}


	result = VOP_LOOKUP(startvn, path, retval);

	VOP_DECREF(startvn);
    return result;
}

int vfs_lookparent(char *path, struct vnode **retval, char *buf, size_t buflen) {
    struct vnode *startvn;
	int result;

    // idk this does; i think if console no path
	if(strlen(path) == 0){
        *retval = startvn;
        return 0;
    }


	result = VOP_LOOKPARENT(startvn, path, retval, buf, buflen);

	VOP_DECREF(startvn);
}

int vfs_open(char *path, int openflags, int mode, struct vnode **ret) {
    int how;
	int result;
	int canwrite;
	struct vnode *vn = NULL;

    
	result = vfs_lookup(path, &vn);

	if (result) {
		return result;
	}

	assert(vn != NULL);

	result = VOP_EACHOPEN(vn, openflags);
	if (result) {
		VOP_DECREF(vn);
		return result;
	}

	*ret = vn;

return 0;
}

void vfs_close(struct vnode *vn) {
    VOP_DECREF(vn);
}

void open_console(){
	
}


void vfs_bootstrap(void) {
	device_list = malloc(sizeof(struct device_entry));
	
	strcpy(device_list->name, "console");
	device_list->next = NULL;
	device_list->vn = dev_create_vnode();
}
