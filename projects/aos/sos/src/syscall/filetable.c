
#include <sos.h>
#include "filetable.h"

struct filetable *fdt_create(void) {
    struct filetable *fdt = malloc(sizeof(struct filetable));

    for (int i = 0; i < PROCESS_MAX_FILES; i++) {
        fdt->openfiles[i] = NULL;
    }

    return fdt;
}

void fdt_destroy(struct filetable * fdt) {
    for (int i = 0; i < PROCESS_MAX_FILES; i++) {
        if(fdt->openfiles[i]){
            free(fdt->openfiles[i]);
        }
    }
    free(fdt);
}
