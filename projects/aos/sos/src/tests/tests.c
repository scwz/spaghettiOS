/*
 * Copyright 2018, Data61
 * Commonwealth Scientific and Industrial Research Organisation (CSIRO)
 * ABN 41 687 119 230.
 *
 * This software may be distributed and modified according to the terms of
 * the GNU General Public License version 2. Note that NO WARRANTY is provided.  * See "LICENSE_GPLv2.txt" for details.
 *
 * @TAG(DATA61_GPL)
 */
#define ZF_LOG_LEVEL ZF_LOG_INFO
#include <cspace/cspace.h>
#include <utils/page.h>
#include <clock/clock.h>
#include "../dma.h"
#include "../bootstrap.h"
#include "../vm/frametable.h"

static void test_bf_bit(unsigned long bit)
{
    ZF_LOGV("%lu", bit);
    seL4_Word bitfield[2] = {0};
    assert(bf_first_free(2, bitfield) == 0);

    bf_set_bit(bitfield, bit);
    assert(bf_get_bit(bitfield, bit));
    seL4_Word ff = bit == 0 ? 1 : 0;
    assert(bf_first_free(2, bitfield) == ff);

    bf_clr_bit(bitfield, bit);
    assert(!bf_get_bit(bitfield, bit));
}

static void test_bf(void)
{
    test_bf_bit(0);

    test_bf_bit(1);
    test_bf_bit(63);
    test_bf_bit(64);
    test_bf_bit(65);
    test_bf_bit(127);

    seL4_Word bitfield[2] = {0};
    for (unsigned int i = 0; i < 127; i++) {
        assert(bf_get_bit(bitfield, i) == 0);
        bf_set_bit(bitfield, i);
        assert(bf_get_bit(bitfield, i));
        assert(bf_first_free(2, bitfield) == i+1);
    }
}

static void test_cspace(cspace_t *cspace)
{
    ZF_LOGI("Test cspace");
   /* test we can alloc a cptr */
   ZF_LOGV("Test allocating cslot");
   seL4_CPtr cptr = cspace_alloc_slot(cspace);
   assert(cptr != 0);

   ZF_LOGV("Test freeing cslot");
   /* test we can free the cptr */
   cspace_free_slot(cspace, cptr);

   ZF_LOGV("Test free slot is returned");
   /* test we get the same cptr back if we alloc again */
   seL4_CPtr cptr_new = cspace_alloc_slot(cspace);
   assert(cptr == cptr_new);

   cspace_free_slot(cspace, cptr_new);

   /* test allocating and freeing a large amount of slots */
   int nslots = CNODE_SLOTS(CNODE_SIZE_BITS) / 2;
   if (cspace->two_level) {
       nslots = MIN(CNODE_SLOTS(cspace->top_lvl_size_bits) * CNODE_SLOTS(CNODE_SIZE_BITS) - 4,
                    CNODE_SLOTS(CNODE_SIZE_BITS) * BOT_LVL_PER_NODE + 1);
   }
   seL4_CPtr *slots = malloc(sizeof(seL4_CPtr) * nslots);
   assert(slots != NULL);

   ZF_LOGV("Test allocating and freeing %d slots", nslots);

   for (int i = 0; i < nslots; i++) {
       slots[i] = cspace_alloc_slot(cspace);
       if (slots[i] == seL4_CapNull) {
           nslots = i;
           break;
       }
   }

   ZF_LOGV("Allocated %lu <-> %lu slots\n", slots[0], slots[nslots - 1]);

   for (int i = 0; i < nslots; i++) {
       cspace_free_slot(cspace, slots[i]);
   }

   free(slots);
}

static void test_dma(void)
{
    dma_addr_t dma = sos_dma_malloc(PAGE_SIZE_4K, PAGE_SIZE_4K);
    char *blah = (char *) dma.vaddr;
    for (size_t i = 0; i < PAGE_SIZE_4K; i++) {
        blah[i] = 'a' + i % 25;
    }

    for (size_t i = 0; i < PAGE_SIZE_4K; i++) {
        assert(blah[i] == 'a' + i % 25);
    }
}

void run_tests(cspace_t *cspace)
{
    /* test the cspace bitfield data structure */
    test_bf();

    /* test the root cspace */
    test_cspace(cspace);
    ZF_LOGI("Root CSpace test passed!");

    /* test a new, 1-level cspace */
    cspace_t dummy_cspace;
    int error = cspace_create_one_level(cspace, &dummy_cspace);
    assert(error == 0);
    test_cspace(&dummy_cspace);
    cspace_destroy(&dummy_cspace);
    ZF_LOGI("Single level cspace test passed!");

    /* test a new, 2 level cspace */
    cspace_alloc_t cspace_alloc = {
        .map_frame = bootstrap_cspace_map_frame,
        .alloc_4k_ut = bootstrap_cspace_alloc_4k_ut,
        .free_4k_ut = bootstrap_cspace_free_4k_ut,
        .cookie = NULL
    };
    error = cspace_create_two_level(cspace, &dummy_cspace, cspace_alloc);
    assert(error == 0);
    test_cspace(&dummy_cspace);
    cspace_destroy(&dummy_cspace);
    ZF_LOGI("Double level cspace test passed!");

    /* test DMA */
    test_dma();
    ZF_LOGI("DMA test passed!");
}

static void 
m2_1(void) 
{
	/* Allocate 10 pages and make sure you can touch them all */
	for (int i = 0; i < 10; i++) {
		/* Allocate a page */
		seL4_Word vaddr;
		frame_alloc(&vaddr);
		assert(vaddr);

		/* Test you can touch the page */
		*(seL4_Word *) vaddr = 0x37;
		assert(*(seL4_Word *) vaddr == 0x37);

		printf("Page #%d allocated at %p\n", i, (void *) vaddr);
	}
    printf("Milestone 2 test 1 passed!\n");
}

static void 
m2_2(void) 
{
	/* Test that you never run out of memory if you always free frames. */
    seL4_Word page;
	for (int i = 0; i < 10000; i++) {
		/* Allocate a page */
		seL4_Word vaddr;
		page = frame_alloc(&vaddr);
		assert(vaddr != 0);

		/* Test you can touch the page */
		*(seL4_Word *) vaddr = 0x37;
		assert(*(seL4_Word *) vaddr == 0x37);

		/* print every 1000 iterations */
		if (i % 1000 == 0) {
			printf("Page #%d allocated at %p\n", i, (void*) vaddr);
		}

		frame_free(page);
	}
    printf("Milestone 2 test 2 passed!\n");
}

static void 
m2_3(void) 
{
	/* Test that you eventually run out of memory gracefully,
	   and doesn't crash */
    //int i = 0;
	while (true) {
		/* Allocate a page */
		seL4_Word vaddr;
		frame_alloc(&vaddr);
		if (!vaddr) {
			printf("Out of memory!\n");
			break;
		}

		/* Test you can touch the page */
		*(seL4_Word *) vaddr = 0x37;
		assert(*(seL4_Word *) vaddr == 0x37);
        /*
		if (i++ % 1000 == 0) {
			printf("Page #%d allocated at %p\n", i, (void*) vaddr);
		}
        */
	}
    for (size_t i = 0; i < (ut_size() / PAGE_SIZE_4K) * 0.8; i++) {
        frame_free(i);
    }
    printf("Milestone 2 test 3 passed!\n");
}

void test_m1(void)
{
#if 0
    static void test_timer(void) {
        printf("CALLBACK FROM PERIODIC 100MS TIMER\n");
    }

    static void test_timer2(void) {
        printf("CALLBACK FROM PERIODIC 200MS TIMER\n");
    }

    static void test_timer3(void) {
        printf("CALLBACK FROM ONE SHOT 100S TIMER\n");
    }

    static void test_timer4(void) {
        printf("CALLBACK FROM ONE SHOT 200S TIMER\n");
    }

    static void test_timer5(void) {
        printf("CALLBACK FROM ONE SHOT 300S TIMER\n");
    }

    uint32_t timer1, timer2, timer3, timer4, timer5;
    timer1 = register_timer(100000, PERIODIC, &test_timer, NULL);
    timer2 = register_timer(200000, PERIODIC, &test_timer2, NULL);
    timer5 = register_timer(300000000, ONE_SHOT, &test_timer5, NULL);
    timer4 = register_timer(200000000, ONE_SHOT, &test_timer4, NULL);
    timer3 = register_timer(100000000, ONE_SHOT, &test_timer3, NULL);
#endif
}

void test_m2(void)
{
    m2_1();
    m2_2();
    m2_3();
}

