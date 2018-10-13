#include "../vfs/vfs.h"
#include "../vfs/vnode.h"
#include "../uio.h"
#include <nfsc/libnfs.h>
#include <string.h>
#include "../network.h"
#include <sos.h>
#include "../picoro/picoro.h"

struct vnode_nfs_data {
	char * path;
	struct vnode * dev_list;
	struct vnode * next;
	struct nfsfh *  nfsfh;
};

struct nfs_data {
	struct vnode * vn;
    char * path;
	struct nfsfh *nfsfh;
	struct uio * u;
	void * statbuf;
	int ret;
	size_t i;
	coro co;
};

struct vnode * nfs_create_vnode();

void nfs_dirent_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	struct nfsdir *nfsdir = data;
	struct nfsdirent *nfsdirent;
	d->path = NULL;
	d->ret = -1;
	if (status < 0) {
		printf("opendir failed with \"%s\"\n", (char *)data);
		exit(10);
	}
	printf("opendir successful\n");
	size_t i = d->i;
	while((nfsdirent = nfs_readdir(nfs, nfsdir)) != NULL && i <= d->u->len) {
		printf("i %d, Inode:%d Name:%s\n", i,   (int)nfsdirent->inode, nfsdirent->name);
		if(i == d->u->len){
			d->path = malloc(strlen(nfsdirent->name));
			strcpy(d->path, nfsdirent->name);
			d->ret = 0;
			break;
		}
		i++;
	}
	nfs_closedir(nfs, nfsdir);
	resume(d->co, NULL);
}

void nfs_read_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	int i;

	if (status < 0) {
		printf("read failed with \"%s\"\n", (char *)data);
		exit(10);
	}

	printf("read successful with %d bytes of data\n", status);
	sos_copyin(data, status);
	d->ret = status;
	resume(d->co, NULL);
}

void nfs_write_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	char *read_data;

	if (status < 0) {
		printf("write failed with \"%s\"\n", (char *)data);
		exit(10);
	}
	printf("write successful with %d bytes of data\n", status);
	
	d->ret = status;
	resume(d->co, NULL);
}

void nfs_close_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	printf("CLOSE\n");
	struct nfs_data *d = private_data;
	struct vnode_nfs_data * vnode_data = d->vn->vn_data;
	struct nfsfh *nfsfh;

	if (status < 0) {
		printf("close call failed with \"%s\"\n", (char *)data);
		exit(10);
	}

	nfsfh = data;
	vnode_data->nfsfh = NULL;
	d->ret = 0;
	resume(d->co, NULL);
}

void nfs_open_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	struct vnode_nfs_data * vnode_data = d->vn->vn_data;
	struct nfsfh *nfsfh;

	if (status < 0) {
		printf("open call failed with \"%s\"\n", (char *)data);
		exit(10);
	}

	nfsfh = data;
	vnode_data->nfsfh = nfsfh;
	d->ret = 0;
	resume(d->co, NULL);
}

void nfs_stat64_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	struct nfs_stat_64 *st;
	st = (struct nfs_stat_64 *)data;
	if (status < 0) {
		d->ret = -1;
	} else {
		sos_stat_t * buf = d->statbuf;
		buf->st_atime = st->nfs_atime;
		buf->st_ctime = st->nfs_ctime;
		buf->st_fmode = st->nfs_mode;
		buf->st_size = st->nfs_size;
		buf->st_type = 0;
		
		printf("Mode %04o, Size: %x, a_time %x, c_time %x\n", 
			(unsigned int) st->nfs_mode, st->nfs_size , st->nfs_atime, st->nfs_ctime);
		
		d->ret = 0;
	}
	resume(d->co, NULL);
}

void nfs_lookup_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	struct nfsdir *nfsdir = data;
	struct nfsdirent *nfsdirent;
	d->ret = -1;
	if (status < 0) {
		printf("opendir failed with \"%s\"\n", (char *)data);
		exit(10);
	}
	bool found = false;
	struct vnode * result = NULL;
	printf("opendir successful\n");
	while((nfsdirent = nfs_readdir(nfs, nfsdir)) != NULL) {
		printf("Inode:%d Name:%s\n", (int)nfsdirent->inode, nfsdirent->name);
		if(!strcmp(nfsdirent->name, d->path)){
			VOP_CREAT(d->vn, d->path, FM_READ | FM_WRITE, FM_READ | FM_WRITE, &result);
			printf("MAKING %s\n", d->path);
			d->vn = result;
			found = true;
			break;
		}
	}
	nfs_closedir(nfs, nfsdir);
	if(found)
		d->ret = 0;
	resume(d->co, NULL);
	
}

void nfs_create_cb(int status, struct nfs_context *nfs, void *data, void *private_data)
{
	struct nfs_data *d = private_data;
	struct vnode_nfs_data * vnode_dat = d->vn->vn_data;
	vnode_dat->nfsfh = data;
	VOP_INCREF(d->vn);
	
	d->ret = 0;
	resume(d->co, NULL);
}

static struct vnode * find_device_name(struct vnode *dir, char * name){

	struct vnode_nfs_data * vnode_dat = dir->vn_data;
	struct device_entry * curr = vnode_dat->dev_list;
	while(curr != NULL){
		if(!strcmp(name, curr->name)){
			return curr->vn;
		}
		curr = curr->next;
	}
	return NULL;
}

static char * find_device_pos(struct vnode *dir, size_t * i, struct uio * u){

	struct vnode_nfs_data * vnode_dat = dir->vn_data;
	struct device_entry * curr = vnode_dat->dev_list;
	while(curr != NULL && *i <= u->len){
		if (*i == u->len){
			return curr->name;
		}
		curr = curr->next;
		(*i)++;
	}
	
	return NULL;
}

static int vnfs_eachopen(struct vnode *v, int flags)
{
	printf("OPEN1\n");
	struct nfs_data * d = malloc(sizeof(struct nfs_data));
	VOP_INCREF(v);
	struct vnode_nfs_data * data = v->vn_data;
	d->vn = v;
	printf("OPEN2\n");
	if(nfs_open_async(nfs, data->path, flags, nfs_open_cb, d)){
		return -1;
	}
	printf("OPEN3\n");
	d->co = get_running();
	yield(NULL);
	int ret = d->ret;
	free(d);
	return ret;
}

static
int
vnfs_write(struct vnode *v, struct uio *uio)
{
	printf("WRITE CALLED\n");
	struct nfs_data * d = malloc(sizeof(struct nfs_data));
	printf("WRITE CALLED\n");
	struct vnode_nfs_data * vnode_dat = v->vn_data;
	printf("nfs %d, vnode_dat %d, offset %d, len %d, shared_buf %x, nfs_write_cb %x, d %x\n", nfs, vnode_dat->nfsfh,uio->offset, uio->len, shared_buf, nfs_write_cb, d);
	if(uio->len > 1 << 14){
		uio->len = 1 << 14;
	}
	if(nfs_pwrite_async(nfs, vnode_dat->nfsfh,uio->offset, uio->len, shared_buf, nfs_write_cb, d)){
		printf("fail\n");
		free(d);
		return -1;
	}
	d->co = get_running();
	yield(NULL);
	int ret = d->ret;
	free(d);
	return ret;
}

static
int
vnfs_read(struct vnode *v, struct uio *uio)
{
	struct nfs_data * d = malloc(sizeof(struct nfs_data));
	struct vnode_nfs_data * vnode_dat = v->vn_data;
	printf("uio: %lu\n", uio->len);
	if(nfs_pread_async(nfs, vnode_dat->nfsfh, uio->offset, uio->len, nfs_read_cb, d)){
		free(d);
		printf("working2?:\n");
		return -1;
	}
	printf("working?:\n");
	d->co = get_running();
	yield(NULL);
	int ret = d->ret;
	free(d);
	return ret;
}

static int vnfs_getdirent(struct vnode *dir, struct uio *u){
	assert(dir);
	
	size_t i = 0;
	char * name = find_device_pos(dir, &i, u);
	if(name){
		sos_copyin(name, strlen(name) + 1);
		return strlen(name)+1;
	}
	struct nfs_data * d = malloc(sizeof(struct nfs_data));
	d->u = u;
	d->i = i;
	if(nfs_opendir_async(nfs, "", nfs_dirent_cb, d)){
		free(d);
		return -1;
	}
    d->co = get_running();
	yield(NULL);
	
	int ret = d->ret;
	if(ret){
		free(d);
		return 0;
	}
	size_t pathlen = strlen(d->path)+1;
	sos_copyin(d->path, pathlen);
	free(d->path);
	free(d);
	return pathlen;
}

static int vnfs_stat(struct vnode *v, void * statbuf){
	struct nfs_data * d = malloc(sizeof(struct nfs_data));
	struct vnode_nfs_data * vnode_dat= v->vn_data;
	d->statbuf = statbuf;
    if(nfs_stat64_async(nfs, vnode_dat->path, nfs_stat64_cb, d)){
		free(d);
		return -1;
	}
	d->co = get_running();
	yield(NULL);
	int ret = d->ret;
	free(d);
	return ret;
}

static int vnfs_lookup(struct vnode *dir, char *pathname, struct vnode **result, bool create){
	struct vnode * curr = dir;
	*result = NULL;
	while(curr != NULL){
		struct vnode_nfs_data * vnode_data = curr->vn_data;
		if(!strcmp(pathname, vnode_data->path)){
			*result = curr;
			return 0;
		}
		curr = vnode_data->next;
	}
	struct nfs_data * d = malloc(sizeof(struct nfs_data));
	*result = find_device_name(dir, pathname);
	if(*result){
		*result = curr;
		return 0;
	}
	d->path = malloc(strlen(pathname));
	strcpy(d->path, pathname);
	d->vn = dir;
    if(nfs_opendir_async(nfs, "", nfs_lookup_cb, d)){
		free(d);
		free(d->path);
		return -1;
	}
	d->co = get_running();
	yield(NULL);
	int ret = d->ret;
	
	if(ret && create){
		struct vnode * new;
		VOP_CREAT(d->vn, d->path, FM_READ | FM_WRITE, FM_READ | FM_WRITE, &new);
		d->vn = new;
		int flags= (FM_READ | FM_WRITE) << 6 | (FM_READ | FM_WRITE) << 3 | (FM_READ | FM_WRITE) ;
		if (nfs_create_async(nfs, d->path, flags, flags, nfs_create_cb, d)){
			VOP_RECLAIM(new);
			free(d);
			return -1;
		}
		d->co = get_running();
		yield(NULL);
		VOP_RECLAIM(d->vn); //close the new vnode
		ret = d->ret;
	} else if (ret){
		free(d->path);
	}
	*result = d->vn;
	free(d);
	return ret;
}

static int vnfs_lookparent(struct vnode *dir, char *pathname, struct vnode **result, char *buf, size_t len){
    return 0;
}

static int vnfs_creat(struct vnode *dir, const char *name, int excl, int mode, struct vnode **result){
	*result = nfs_create_vnode();
	
	struct vnode_nfs_data * vnode_data = dir->vn_data;
	struct vnode *tmp = vnode_data->next;
	vnode_data->next = *result;
	vnode_data = (*result)->vn_data;
	vnode_data->path = name;
	vnode_data->next = tmp;
	return 0;
}

static int vnfs_reclaim(struct vnode *v){
	struct nfs_data * d = malloc(sizeof(struct nfs_data));
	struct vnode_nfs_data * vnode_dat = v->vn_data;
	d->vn = v;
	if( nfs_close_async(nfs, vnode_dat->nfsfh, nfs_close_cb, d)){
		free(d);
		return -1;	
	}
	d->co = get_running();
	yield(NULL);
	int ret = d->ret;
	free(d);
	return ret;
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
	.vop_reclaim = vnfs_reclaim,
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
	struct vnode_nfs_data * d = fsdata;
	d->next = NULL;
	result = vnode_init(v, &nfs_vnode_ops,  fsdata);
	assert(v->vn_ops);
	assert(v->vn_data);
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
    assert(nfs);
	struct vnode * v = nfs_create_vnode();
	struct vnode_nfs_data * d = v->vn_data;
	d->path = malloc(1);
	d->path[0] = '\0';
	assert(d->next == NULL);
	return v;
}

int nfs_get_statbuf(struct vnode * dir, char * path, struct stat_buf_t * statbuf){
	printf("hello\n");
	struct vnode * dev = find_device_name(dir, path);
	if(dev != NULL){
		VOP_STAT(dev, statbuf);
		return 0;
	}
	if(!strcmp(path, "..")){
		path[1] = '\0';
	}
	struct nfs_data * d = malloc(sizeof(struct nfs_data));
	d->statbuf = statbuf;
	printf("hello\n");
    if(nfs_stat64_async(nfs, path, nfs_stat64_cb, d)){
		free(d);
		return -1;
	}
	d->co = get_running();
	yield(NULL);
	int ret = d->ret;
	free(d);
	return ret;
}
