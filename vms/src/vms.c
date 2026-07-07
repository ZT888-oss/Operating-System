#include "vms.h"

#include "mmu.h"
#include "pages.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* A debugging helper that will print information about the pointed to PTE
   entry. */
static void print_pte_entry(uint64_t *entry);

// Assuming a fixed maximum number of physical pages
#define MAX_PHYS_PAGES 1024

// Global array to track references for each physical page
int refcount[MAX_PAGES] = {0};

// void page_fault_handler(void *virtual_address, int level, void *page_table) {
//   // We only handle COW at L0
//   if (level != 0)
//     return;
// printf("l0 arrvie");
//   uint64_t *entry = vms_page_table_pte_entry(page_table, virtual_address, 0);

//   // Not COW ? fix it
//   if (!vms_pte_custom(entry)){

//     return;
//   }

//   uint64_t old_ppn = vms_pte_get_ppn(entry);

//   if (refcount[old_ppn] > 1) {

//     void *old_page = vms_ppn_to_page(old_ppn);
//     void *new_page = vms_new_page();

//     memcpy(new_page, old_page, PAGE_SIZE);

//     uint64_t new_ppn = vms_page_to_ppn(new_page);

//     refcount[old_ppn]--;
//     refcount[new_ppn] = 1;

//     vms_pte_set_ppn(entry, new_ppn);
//     vms_pte_set_ppn(entry, new_ppn);
//     // update page table entry is currently point to

//   }

// restore write permission
//   vms_pte_write_set(entry);
//   vms_pte_custom_clear(entry);
// }

void page_fault_handler(void *virtual_address, int level, void *page_table) {
  uint64_t *entry =
      vms_page_table_pte_entry(page_table, virtual_address, level);
  if (level > 0) {
    printf("allocate new page table for l2 and l1");
    void *new_table = vms_new_page();
    memset(new_table, 0, PAGE_SIZE);

    uint64_t new_ppn = vms_page_to_ppn(new_table);

    vms_pte_set_ppn(entry, new_ppn);
    vms_pte_valid_set(entry);

    return;
  }
  
if (level == 0 &&
    vms_pte_valid(entry) &&
    vms_pte_custom(entry) &&
    !vms_pte_write(entry))
{
    uint64_t old_ppn = vms_pte_get_ppn(entry);

    if (refcount[old_ppn] > 1)
    {
        void *old_page = vms_ppn_to_page(old_ppn);
        void *new_page = vms_new_page();

        memcpy(new_page, old_page, PAGE_SIZE);

        uint64_t new_ppn = vms_page_to_ppn(new_page);

        refcount[old_ppn]--;
        refcount[new_ppn] = 1;

        vms_pte_set_ppn(entry, new_ppn);
    }

    vms_pte_write_set(entry);
    vms_pte_custom_clear(entry);

    return;
}



}

void *vms_fork_copy(void) {
  void *parent_l2 = vms_get_root_page_table();
  void *child_l2 = vms_new_page();
  // process should be page table -> page table entry -> ppn -> next physical
  // page(page table) pppn pointing to
  for (int i = 0; i < 512; i++) {

    uint64_t *parent_l2_entry =
        vms_page_table_pte_entry_from_index(parent_l2, i);

    if (!vms_pte_valid(parent_l2_entry))
      continue;

    // Allocate child L1
    void *parent_l1 = vms_ppn_to_page(vms_pte_get_ppn(
        parent_l2_entry)); // ppn of l2 pointing to l1 page table
    void *child_l1 = vms_new_page();

    uint64_t *child_l2_entry = vms_page_table_pte_entry_from_index(child_l2, i);

    // Copy flags and set PPN
    vms_pte_set_ppn(child_l2_entry, vms_page_to_ppn(child_l1));
    vms_pte_valid_set(child_l2_entry);

    // allocate l1
    for (int j = 0; j < 512; j++) {
      uint64_t *parent_l1_entry =
          vms_page_table_pte_entry_from_index(parent_l1, j);

      if (!vms_pte_valid(parent_l1_entry))
        continue;

      void *parent_l0 = vms_ppn_to_page(vms_pte_get_ppn(parent_l1_entry));
      void *child_l0 = vms_new_page();

      uint64_t *child_l1_entry =
          vms_page_table_pte_entry_from_index(child_l1, j);

      vms_pte_set_ppn(child_l1_entry, vms_page_to_ppn(child_l0));
      vms_pte_valid_set(child_l1_entry);

      // allocate l0
      for (int k = 0; k < 512; k++) {

        uint64_t *parent_l0_entry =
            vms_page_table_pte_entry_from_index(parent_l0, k);

        if (!vms_pte_valid(parent_l0_entry))
          continue;

        void *parent_data = vms_ppn_to_page(vms_pte_get_ppn(parent_l0_entry));

        void *child_data = vms_new_page();

        memcpy(child_data, parent_data, PAGE_SIZE);

        uint64_t *child_l0_entry =
            vms_page_table_pte_entry_from_index(child_l0, k);

        vms_pte_set_ppn(child_l0_entry, vms_page_to_ppn(child_data));

        vms_pte_valid_set(child_l0_entry);

        if (vms_pte_read(parent_l0_entry))
          vms_pte_read_set(child_l0_entry);

        if (vms_pte_write(parent_l0_entry))
          vms_pte_write_set(child_l0_entry);
      }
    }
  }

  return child_l2;
}

void *vms_fork_copy_on_write(void)
{
    void *parent_l2 = vms_get_root_page_table();
    void *child_l2  = vms_new_page();
    memset(child_l2, 0, PAGE_SIZE);

    for (int i = 0; i < NUM_PTE_ENTRIES; i++)
    {
        uint64_t *parent_l2_entry =
            vms_page_table_pte_entry_from_index(parent_l2, i);

        if (!vms_pte_valid(parent_l2_entry))
            continue;

        void *parent_l1 =
            vms_ppn_to_page(vms_pte_get_ppn(parent_l2_entry));

        void *child_l1 = vms_new_page();
        memset(child_l1, 0, PAGE_SIZE);

        uint64_t *child_l2_entry =
            vms_page_table_pte_entry_from_index(child_l2, i);

        vms_pte_set_ppn(child_l2_entry,
                        vms_page_to_ppn(child_l1));
        vms_pte_valid_set(child_l2_entry);

        // --------------------------------
        // L1
        // --------------------------------
        for (int j = 0; j < NUM_PTE_ENTRIES; j++)
        {
            uint64_t *parent_l1_entry =
                vms_page_table_pte_entry_from_index(parent_l1, j);

            if (!vms_pte_valid(parent_l1_entry))
                continue;

            void *parent_l0 =
                vms_ppn_to_page(vms_pte_get_ppn(parent_l1_entry));

            void *child_l0 = vms_new_page();
            memset(child_l0, 0, PAGE_SIZE);

            uint64_t *child_l1_entry =
                vms_page_table_pte_entry_from_index(child_l1, j);

            vms_pte_set_ppn(child_l1_entry,
                            vms_page_to_ppn(child_l0));
            vms_pte_valid_set(child_l1_entry);

            // --------------------------------
            // L0 (actual pages)
            // --------------------------------
            for (int k = 0; k < NUM_PTE_ENTRIES; k++)
            {
                uint64_t *parent_l0_entry =
                    vms_page_table_pte_entry_from_index(parent_l0, k);

                if (!vms_pte_valid(parent_l0_entry))
                    continue;

                uint64_t *child_l0_entry =
                    vms_page_table_pte_entry_from_index(child_l0, k);

                uint64_t ppn =
                    vms_pte_get_ppn(parent_l0_entry);

                // Share physical page
                vms_pte_set_ppn(child_l0_entry, ppn);

                // Copy read flag
                if (vms_pte_read(parent_l0_entry))
                    vms_pte_read_set(child_l0_entry);

                // If writable ? convert to COW
                if (vms_pte_write(parent_l0_entry))
                {
                    vms_pte_write_clear(parent_l0_entry);
                    vms_pte_write_clear(child_l0_entry);
                }

                // Mark COW in both
                vms_pte_custom_set(parent_l0_entry);
                vms_pte_custom_set(child_l0_entry);

                vms_pte_valid_set(child_l0_entry);

                // Increment refcount
                refcount[ppn]+=2;
            }
        }
    }

    return child_l2;
}

static void print_pte_entry(uint64_t *entry) {
  const char *dash = "-";
  const char *custom = dash;
  const char *write = dash;
  const char *read = dash;
  const char *valid = dash;
  if (vms_pte_custom(entry)) {
    custom = "C";
  }
  if (vms_pte_write(entry)) {
    write = "W";
  }
  if (vms_pte_read(entry)) {
    read = "R";
  }
  if (vms_pte_valid(entry)) {
    valid = "V";
  }

  printf("PPN: 0x%lX Flags: %s%s%s%s\n", vms_pte_get_ppn(entry), custom, write,
         read, valid);
}
// --- Helper to walk and print all active entries in a 3-level SV-39 page table
// ---
