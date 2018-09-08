#include "../vfs/vfs.h"
#include "../vfs/vnode.h"
#include "../uio.h"
#include <nfsc/libnfs.h>
#include <string.h>

struct vnode_nfs_data {
	char * path;
	struct vnode * next;
};

static struct nfs_context *nfs = NULL;

struct nfs_data {
	struct vnode * vn;
    char * path;
	struct nfsfh *nfsfh;
	struct uio * u;
	void * statbuf;
};

struct vnode * nfs_create_vnode();

void nfs_dirent_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	struct nfsdir *nfsdir = data;
	struct nfsdirent *nfsdirent;

	if (status < 0) {
		printf("opendir failed with \"%s\"\n", (char *)data);
		exit(10);
	}

	printf("opendir successful\n");
	size_t i = 0;
	while((nfsdirent = nfs_readdir(nfs, nfsdir)) != NULL && i < d->u->len) {
		printf("Inode:%d Name:%s\n", (int)nfsdirent->inode, nfsdirent->name);
		i++;
	}
	printf("dirent: Inode:%d Name:%s\n", (int)nfsdirent->inode, nfsdirent->name);
	nfs_closedir(nfs, nfsdir);
}

void nfs_close_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;

	if (status < 0) {
		printf("close failed with \"%s\"\n", (char *)data);
		exit(10);
	}

	printf("close successful\n");
	
}


void nfs_read_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	char *read_data;
	int i;

	if (status < 0) {
		printf("read failed with \"%s\"\n", (char *)data);
		exit(10);
	}

	printf("read successful with %d bytes of data\n", status);
	read_data = data;
	for (i=0;i<16;i++) {
		printf("%02x ", read_data[i]&0xff);
	}
	printf("\n");
	
}

void nfs_open_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	struct nfsfh *nfsfh;

	if (status < 0) {
		printf("open call failed with \"%s\"\n", (char *)data);
		exit(10);
	}

	nfsfh         = data;
	
}

void nfs_stat64_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	struct nfs_stat_64 *st;
 
	if (status < 0) {
		printf("stat call failed with \"%s\"\n", (char *)data);
		exit(10);
	}

	st = (struct nfs_stat_64 *)data;
	printf("Mode %04o\n", (unsigned int) st->nfs_mode);
	printf("Size %d\n", (int)st->nfs_size);
	printf("Inode %04o\n", (int)st->nfs_ino);

	
}

void nfs_lookup_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	struct nfsdir *nfsdir = data;
	struct nfsdirent *nfsdirent;

	if (status < 0) {
		printf("opendir failed with \"%s\"\n", (char *)data);
		exit(10);
	}

	printf("opendir successful\n");
	while((nfsdirent = nfs_readdir(nfs, nfsdir)) != NULL) {
		printf("Inode:%d Name:%s\n", (int)nfsdirent->inode, nfsdirent->name);
		if(!strcmp(nfsdirent->name, d->path)){
			struct vnode * result;
			VOP_CREAT(d->vn, d->path, 0, 0, &result);
			break;
		}
	}
	nfs_closedir(nfs, nfsdir);

	
}

static int vnfs_eachopen(struct vnode *v, int flags)
{
	return 0;
}

static
int
vnfs_write(struct vnode *v, struct uio *uio)
{
	return 0;
}

static
int
vnfs_read(struct vnode *v, struct uio *uio)
{
	return 0;
}

static int vnfs_getdirent(struct vnode *dir, struct uio *u){
	struct nfs_data * d = malloc(sizeof(struct nfs_data));
	d->u = u;
	int res = nfs_opendir_async(nfs, "", nfs_dirent_cb, d);
	printf("hi %lx\n", res);
    return res;
}

static int vnfs_stat(struct vnode *object, void * statbuf){
	struct nfs_data * d = malloc(sizeof(struct nfs_data));
	d->statbuf = statbuf;
    return nfs_stat64_async(nfs, "", nfs_stat64_cb, d);
}

static int vnfs_lookup(struct vnode *dir, char *pathname, struct vnode **result){
	struct vnode * curr = dir;
	while(curr != NULL){
		struct vnode_nfs_data * vnode_data = curr->vn_data;
		if(!strcmp(pathname, vnode_data->path)){
			*result = curr;
			return 0;
		}
	}
	struct nfs_data * d = malloc(sizeof(struct nfs_data));
	d->path = pathname;
	d->vn = curr;
    return nfs_opendir_async(nfs, "", nfs_lookup_cb, d);
}

static int vnfs_lookparent(struct vnode *dir, char *pathname, struct vnode **result, char *buf, size_t len){
    return 0;
}

static int vnfs_creat(struct vnode *dir, const char *name, int excl, int mode, struct vnode **result){
	*result = nfs_create_vnode();
	struct vnode_nfs_data * vnode_data = dir->vn_data;
	vnode_data->next = *result;
	vnode_data = (*result)->vn_data;
	vnode_data->path = name;
	return 0;
}

static const struct vnode_ops nfs_vnode_ops = {
	.vop_magic = VOP_MAGIC,

	.vop_eachopen = vnfs_eachopen,

	.vop_read = vnfs_read,
    .vop_getdirentry = vnfs_getdirent,

	.vop_write = vnfs_write,
	.vop_creat = vnfs_creat,
	.vop_stat = vnfs_stat,
	.vop_lookup = vnfs_lookup,
	.vop_lookparent = vnfs_lookparent,
};

struct vnode * nfs_create_vnode()
{
	int result;
	struct vnode *v;
	v = malloc(sizeof(struct vnode));
	if (v==NULL) {
		return NULL;
	}

	void *fsdata = malloc(sizeof(struct vnode_nfs_data));
	struct vnode_nfs_data * d = v->vn_data;
	d->next = NULL;
	result = vnode_init(v, &nfs_vnode_ops,  fsdata);
	assert(v->vn_ops);
	if (result != 0) {
		ZF_LOGV("While creating vnode for device: vnode_init: %s\n",
		      strerror(result));
	}
	return v;
}

void
nfs_uncreate_vnode(struct vnode *vn)
{
	assert(vn->vn_ops == &nfs_vnode_ops);
	free(vn->vn_data);
	vnode_cleanup(vn);
	free(vn);
}


struct vnode * nfs_bootstrap(){
	nfs = nfs_init_context();
    ZF_LOGF_IF(nfs == NULL, "Failed to init NFS context");
	struct vnode * v = nfs_create_vnode();
	struct vnode_nfs_data * d = v->vn_data;
	d->path = malloc(1);
	d->path = '\0';
	return v;
}