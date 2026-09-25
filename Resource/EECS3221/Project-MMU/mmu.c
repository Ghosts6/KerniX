#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define VIRTUAL_ADDRESS_SPACE 65536
#define PAGE_SIZE 256
#define NUM_PAGES 256
#define TLB_SIZE 16
#define FRAME_SIZE 256
#define BACKING_STORE_SIZE 65536

typedef struct {
    int page_num;
    int frame_num;
    int valid;
    int last_used;
} TLBEntry;

typedef struct {
    int frame_num;
    int valid;
    int dirty;
} PageTableEntry;

typedef struct {
    int page_num;
    int frame_num;
    int last_used;
} FrameInfo;

TLBEntry tlb[TLB_SIZE];
PageTableEntry page_table[NUM_PAGES];
unsigned char physical_memory[VIRTUAL_ADDRESS_SPACE];
FrameInfo frame_info[NUM_PAGES];
int num_frames, tlb_entries = 0, page_faults = 0, tlb_hits = 0;
int total_addresses = 0, next_frame = 0, clock_counter = 0;

void init_mmu(int num_frames_arg) {
    num_frames = num_frames_arg;
    memset(tlb, 0, sizeof(tlb));
    memset(page_table, 0, sizeof(page_table));
    memset(physical_memory, 0, sizeof(physical_memory));
    memset(frame_info, 0, sizeof(frame_info));
}

int tlb_lookup(int page_num) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].valid && tlb[i].page_num == page_num) {
            tlb[i].last_used = clock_counter++;
            tlb_hits++;
            return tlb[i].frame_num;
        }
    }
    return -1;
}

void tlb_insert(int page_num, int frame_num) {
    int lru_idx = 0, lru_time = tlb[0].last_used;
    for (int i = 0; i < TLB_SIZE; i++) {
        if (!tlb[i].valid) {
            tlb[i].page_num = page_num;
            tlb[i].frame_num = frame_num;
            tlb[i].valid = 1;
            tlb[i].last_used = clock_counter++;
            tlb_entries++;
            return;
        }
        if (tlb[i].last_used < lru_time) {
            lru_time = tlb[i].last_used;
            lru_idx = i;
        }
    }
    tlb[lru_idx].page_num = page_num;
    tlb[lru_idx].frame_num = frame_num;
    tlb[lru_idx].valid = 1;
    tlb[lru_idx].last_used = clock_counter++;
}

int get_free_frame() {
    if (next_frame < num_frames) {
        return next_frame++;
    }
    return -1;
}

int evict_frame() {
    int lru_idx = 0, lru_time = frame_info[0].last_used;
    for (int i = 1; i < num_frames; i++) {
        if (frame_info[i].last_used < lru_time) {
            lru_time = frame_info[i].last_used;
            lru_idx = i;
        }
    }
    page_table[frame_info[lru_idx].page_num].valid = 0;
    return lru_idx;
}

void load_page(int page_num, FILE *backing_store) {
    int frame_num = get_free_frame();
    if (frame_num == -1) {
        if (num_frames != 128) {
            fprintf(stderr, "No free frames\n");
            exit(1);
        }
        frame_num = evict_frame();
    }
    fseek(backing_store, page_num * PAGE_SIZE, SEEK_SET);
    fread(&physical_memory[frame_num * FRAME_SIZE], 1, PAGE_SIZE, backing_store);
    page_table[page_num].frame_num = frame_num;
    page_table[page_num].valid = 1;
    frame_info[frame_num].page_num = page_num;
    frame_info[frame_num].last_used = clock_counter++;
    tlb_insert(page_num, frame_num);
    page_faults++;
}

unsigned char translate_address(int log_addr, FILE *backing_store) {
    int page_num = (log_addr >> 8) & 0xFF;
    int offset = log_addr & 0xFF;
    total_addresses++;

    int frame_num = tlb_lookup(page_num);
    if (frame_num == -1) {
        if (page_table[page_num].valid) {
            frame_num = page_table[page_num].frame_num;
            frame_info[frame_num].last_used = clock_counter++;
            tlb_insert(page_num, frame_num);
        } else {
            load_page(page_num, backing_store);
            frame_num = page_table[page_num].frame_num;
        }
    }
    return (unsigned char)physical_memory[frame_num * FRAME_SIZE + offset];
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <frames> <backing_store> <addresses>\n", argv[0]);
        return 1;
    }

    int num_frames_arg = atoi(argv[1]);
    const char *backing_store_file = argv[2];
    const char *address_file = argv[3];

    init_mmu(num_frames_arg);

    for (int i = 0; i < num_frames; i++) {
        frame_info[i].page_num = -1;
        frame_info[i].last_used = 0;
    }

    FILE *backing_store = fopen(backing_store_file, "rb");
    if (!backing_store) {
        perror("Error opening backing store");
        return 1;
    }

    FILE *addresses = fopen(address_file, "r");
    if (!addresses) {
        perror("Error opening addresses file");
        fclose(backing_store);
        return 1;
    }

    const char *output_file = (num_frames_arg == 256) ? "output256.csv" : "output128.csv";
    FILE *output = fopen(output_file, "w");
    if (!output) {
        perror("Error creating output file");
        fclose(backing_store);
        fclose(addresses);
        return 1;
    }

    fprintf(output, "logical_address,physical_address,byte_value\n");

    int logical_address;
    while (fscanf(addresses, "%d", &logical_address) == 1) {
        unsigned char byte_val = translate_address(logical_address, backing_store);
        int page_num = (logical_address >> 8) & 0xFF;
        int phys_addr = (page_table[page_num].frame_num * FRAME_SIZE) + (logical_address & 0xFF);
        fprintf(output, "%d,%d,%d\n", logical_address, phys_addr, (signed char)byte_val);
    }

    fprintf(output, "%.2f\n%.2f\n",
        (double)page_faults / total_addresses * 100.0,
        (double)tlb_hits / total_addresses * 100.0);

    fclose(backing_store);
    fclose(addresses);
    fclose(output);

    return 0;
}
