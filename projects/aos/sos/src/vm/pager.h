#pragma once
#include "frametable.h"

void pager_bootstrap(void);
int pageout(seL4_Word page, seL4_Word entry);
int pagein(seL4_Word entry, seL4_Word vaddr);
