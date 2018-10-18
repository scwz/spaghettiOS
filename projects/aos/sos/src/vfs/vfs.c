
#include <string.h>
#include <serial/serial.h>

#include "vfs.h"
#include "vnode.h"
#include "device_vops.h"
#include "../dev/console.h"
#include "../fs/libnfs_vops.h"

static struct device_entry *device_list;

static struct vnode * 
find_device(char *name)
{
    struct device_entry * curr = device_list;
    for(; curr != NULL; curr = curr->next){
        if(!strcmp(name, curr->name)){
            return curr->vn;
        }
    }
    return NULL;
}

int 
vfs_lookup(char *path, struct vnode **retval, bool create, pid_t pid) 
{
    printf("LOOKUP %s\n", path);
    struct vnode *startvn;
    int result;
    printf("stat path %s\n", path);
    startvn = find_device(path);
    if (startvn!= NULL) {
        *retval = startvn;
        return 0;
    }
    
    startvn = root;

    if (path[0] != '\0') {
        result = VOP_LOOKUP(startvn, path, retval, create, pid);
    } 
    else {
        *retval = startvn;
        result = 0;
    }
    printf("vn : %lx \n", *retval);
    return result;
}

void 
vfs_close(struct vnode *vn, pid_t pid) 
{
    VOP_DECREF(vn, pid);
}

void 
vfs_bootstrap(void) 
{
    device_list = malloc(sizeof(struct device_entry));

    strcpy(device_list[0].name, "console");
    device_list[0].next = NULL;
    struct device *dev = malloc(sizeof(struct device));
    dev->close = console_close;
    dev->open = console_open;
    dev->write = console_write;
    dev->read = console_read;
    dev->data = malloc(sizeof(struct console));
    struct console *c = dev->data;
    c->reader = NULL;
    device_list[0].vn = dev_create_vnode(dev);
    console_init();
    root = nfs_bootstrap();
    struct vnode_nfs_data *nfs_dat = root->vn_data;
    nfs_dat->dev_list = device_list;
    printf("vn : %lx \n", root);
}
