
#include <string.h>
#include <serial/serial.h>

#include "vfs.h"
#include "vnode.h"
#include "device_vops.h"
#include "../dev/console.h"
#include "../fs/libnfs_vops.h"

struct device_entry {
    char name[20];
    struct vnode * vn;
    struct device_entry * next;
};

static struct device_entry * device_list;

static struct vnode * root;

static struct vnode * find_device(char * name){
    struct device_entry * curr = device_list;
    for(; curr != NULL; curr = curr->next){
        if(!strcmp(name, curr->name)){
            return curr->vn;
        }
    }
    return NULL;
}

int vfs_lookup(char *path, struct vnode **retval) {
    printf("LOOKUP %s\n", path);
    struct vnode *startvn;
    int result;

    startvn = find_device(path);
    if(startvn!= NULL){
        *retval = startvn;
        return 0;
    }

    startvn = root;

    if(path[0] != '\0'){
        result = VOP_LOOKUP(startvn, path, retval);
    } else {
        *retval = startvn;
    }
    printf("vn : %lx \n", startvn);
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
    return 0;
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

    return 0;
}

void vfs_close(struct vnode *vn) {
    VOP_DECREF(vn);
}

void vfs_bootstrap(void) {
    device_list = malloc(sizeof(struct device_entry));

    strcpy(device_list[0].name, "console");
    device_list[0].next = NULL;
    struct device* dev = malloc(sizeof(struct device));
    dev->close = console_close;
    dev->open = console_open;
    dev->write = console_write;
    dev->read = console_read;
    dev->data = malloc(sizeof(struct console));
    struct console * c = dev->data;
    c->reader = NULL;
    device_list[0].vn = dev_create_vnode(dev);
    console_init();
    root = nfs_bootstrap();
    printf("vn : %lx \n", root);
}
