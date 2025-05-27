#include <stdio.h>
#include <stdlib.h>

#define PAGE_TABLE_SIZE 256
#define TLB_SIZE 16
#define FRAME_SIZE 256
#define PHYSICAL_MEMORY_SIZE (PAGE_TABLE_SIZE * FRAME_SIZE)

signed char physical_memory[PHYSICAL_MEMORY_SIZE];
int page_table[PAGE_TABLE_SIZE];
int tlb[TLB_SIZE][2];
int tlb_index = 0;
int next_free_frame = 0;

int tlb_hits = 0;
int page_faults = 0;
int total_addresses = 0;

FILE *backing_store;

void initialize() {
    for (int i = 0; i < PAGE_TABLE_SIZE; i++)
        page_table[i] = -1;
    for (int i = 0; i < TLB_SIZE; i++)
        tlb[i][0] = -1;
}

int search_TLB(int page_number) {
    for (int i = 0; i < TLB_SIZE; i++) {
        if (tlb[i][0] == page_number) {
            tlb_hits++;
            return tlb[i][1];
        }
    }
    return -1;
}

void add_to_TLB(int page_number, int frame_number) {
    tlb[tlb_index][0] = page_number;
    tlb[tlb_index][1] = frame_number;
    tlb_index = (tlb_index + 1) % TLB_SIZE;
}

void page_fault_handler(int page_number) {
    if (next_free_frame >= PAGE_TABLE_SIZE) {
        printf("Ошибка: физ. память переполнена, замещение страниц не выполнено\n");
        exit(1);
    }

    fseek(backing_store, page_number * FRAME_SIZE, SEEK_SET);
    size_t read_bytes = fread(&physical_memory[next_free_frame * FRAME_SIZE], sizeof(signed char), FRAME_SIZE, backing_store);
    if (read_bytes != FRAME_SIZE) {
        printf("Ошибка чтения из BACKING_STORE.bin на странице №%d\n", page_number);
        exit(1);
    }

    page_table[page_number] = next_free_frame;
    add_to_TLB(page_number, next_free_frame);
    next_free_frame++;
    page_faults++;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Не указан файл, по которому будут анализироваться адреса");
        return 1;
    }

    initialize();

    backing_store = fopen("BACKING_STORE.bin", "rb");
    if (!backing_store) {
        perror("Не удалость открыть BACKING_STORE.bin");
        return 1;
    }

    FILE *address_file = fopen(argv[1], "r");
    if (!address_file) {
        perror("Ошибка открытия файла с адресами");
        fclose(backing_store);
        return 1;
    }

    int logical_address;
    while (fscanf(address_file, "%d", &logical_address) != EOF) {
        total_addresses++;

        int page_number = (logical_address >> 8) & 0xFF;
        int offset = logical_address & 0xFF;

        int frame_number = search_TLB(page_number);

        if (frame_number == -1) {
            frame_number = page_table[page_number];

            if (frame_number == -1) {
                page_fault_handler(page_number);
                frame_number = page_table[page_number];
            } else {
                add_to_TLB(page_number, frame_number);
            }
        }

        int physical_address = frame_number * FRAME_SIZE + offset;
        signed char value = physical_memory[physical_address];

        printf("Логический адрес: %d, Физический адрес: %d, Значение: %d\n",
               logical_address, physical_address, value);
    }

    printf("Частота ошибок страниц: %.2f%%\n", 100.0 * page_faults / total_addresses);
    printf("Частота попаданий в TLB: %.2f%%\n", 100.0 * tlb_hits / total_addresses);

    fclose(address_file);
    fclose(backing_store);

    return 0;
}
