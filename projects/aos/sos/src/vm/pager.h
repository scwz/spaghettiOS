#pragma once
#include "frametable.h"

int pageout(seL4_Word page, seL4_Word entry);
int pagein(seL4_Word entry, seL4_Word vaddr);
