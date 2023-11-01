#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

// Macros for page size (you can adjust this if needed)
#define PAGE_SIZE 4096

// Constants to represent memory types
#define PROCESS 1
#define HOLE 0

// Structure for a sub-chain node
typedef struct SubChainNode
{
    void *start; // Start address of the segment
    size_t size; // Size of the segment
    int type;    // Type: PROCESS or HOLE
    struct SubChainNode *next;
    struct SubChainNode *prev;
} SubChainNode;

// Structure for a main chain node
typedef struct MainChainNode
{
    SubChainNode *sub_chain;
    struct MainChainNode *next;
    struct MainChainNode *prev;
} MainChainNode;

MainChainNode *free_list_head = NULL; // Head of the main chain
void *mems_start_address = NULL;      // Starting MeMS virtual address

// Function to initialize MeMS
void mems_init()
{
    // Initialize necessary parameters
    free_list_head = (MainChainNode *)malloc(sizeof(MainChainNode));
    free_list_head->sub_chain = NULL;
    free_list_head->next = free_list_head;
    free_list_head->prev = free_list_head;

    mems_start_address = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    // Initialize other global variables
}

// Function to finish MeMS and deallocate memory
void mems_finish()
{
    // Unmap allocated memory
    MainChainNode *current_main_chain = free_list_head->next;
    while (current_main_chain != free_list_head)
    {
        SubChainNode *current_sub_chain = current_main_chain->sub_chain;
        while (current_sub_chain != NULL)
        {
            SubChainNode *temp_sub_chain = current_sub_chain;
            current_sub_chain = current_sub_chain->next;
            free(temp_sub_chain);
        }
        MainChainNode *temp_main_chain = current_main_chain;
        current_main_chain = current_main_chain->next;
        free(temp_main_chain);
    }
    free_list_head = NULL;

    munmap(mems_start_address, PAGE_SIZE);
    // Cleanup any other resources
}

// Function to allocate memory
void *mems_malloc(size_t size)
{
    // Search free list for a suitable segment, else request memory from OS
    // Update free list accordingly
    // Return MeMS virtual address
    void *mems_virtual_address = NULL;

    MainChainNode *current = free_list_head->next;
    while (current != free_list_head)
    {
        SubChainNode *sub_chain_current = current->sub_chain;
        while (sub_chain_current != NULL)
        {
            if (sub_chain_current->type == HOLE && sub_chain_current->size >= size)
            {
                // Allocate from this hole
                sub_chain_current->type = PROCESS;
                mems_virtual_address = sub_chain_current->start;
                // Split the hole if there's any remaining space
                if (sub_chain_current->size > size)
                {
                    SubChainNode *new_hole = (SubChainNode *)malloc(sizeof(SubChainNode));
                    new_hole->start = sub_chain_current->start + size;
                    new_hole->size = sub_chain_current->size - size;
                    new_hole->type = HOLE;
                    new_hole->next = sub_chain_current->next;
                    new_hole->prev = sub_chain_current;
                    sub_chain_current->next = new_hole;
                    if (new_hole->next != NULL)
                    {
                        new_hole->next->prev = new_hole;
                    }
                }
                return mems_virtual_address;
            }
            sub_chain_current = sub_chain_current->next;
        }
        current = current->next;
    }

    // No suitable segment found in the free list, request memory from OS
    void *new_memory = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    SubChainNode *new_process = (SubChainNode *)malloc(sizeof(SubChainNode));
    new_process->start = new_memory;
    new_process->size = size;
    new_process->type = PROCESS;
    new_process->next = NULL;
    new_process->prev = NULL;

    MainChainNode *new_main_chain_node = (MainChainNode *)malloc(sizeof(MainChainNode));
    new_main_chain_node->sub_chain = new_process;
    new_main_chain_node->next = free_list_head;
    new_main_chain_node->prev = free_list_head->prev;
    free_list_head->prev->next = new_main_chain_node;
    free_list_head->prev = new_main_chain_node;

    return new_memory;
}

// Function to deallocate memory
void mems_free(void *ptr)
{
    // Mark the corresponding sub-chain node as HOLE
    SubChainNode *current = free_list_head->next;
    while (current != free_list_head)
    {
        SubChainNode *sub_chain_current = current->sub_chain;
        while (sub_chain_current != NULL)
        {
            if (sub_chain_current->start == ptr)
            {
                sub_chain_current->type = HOLE;
                return;
            }
            sub_chain_current = sub_chain_current->next;
        }
        current = current->next;
    }
}

// Function to print memory statistics
void mems_print_stats()
{
    // Print total mapped pages, unused memory, main chain nodes, and sub-chain nodes
    int total_pages = 0;
    size_t unused_memory = 0;

    MainChainNode *current_main_chain = free_list_head->next;
    while (current_main_chain != free_list_head)
    {
        SubChainNode *current_sub_chain = current_main_chain->sub_chain;
        while (current_sub_chain != NULL)
        {
            total_pages += current_sub_chain->size / PAGE_SIZE;
            if (current_sub_chain->type == HOLE)
            {
                unused_memory += current_sub_chain->size;
            }
            current_sub_chain = current_sub_chain->next;
        }
        current_main_chain = current_main_chain->next;
    }

    printf("Total Mapped Pages: %d\n", total_pages);
    printf("Unused Memory: %lu bytes\n", unused_memory);
    printf("Main Chain Nodes: %d\n", total_pages);

    // Print Sub-Chain Nodes
    current_main_chain = free_list_head->next;
    int main_chain_node_count = 0;
    while (current_main_chain != free_list_head)
    {
        SubChainNode *current_sub_chain = current_main_chain->sub_chain;
        printf("Sub-Chain %d:\n", main_chain_node_count);
        while (current_sub_chain != NULL)
        {
            printf("  Type: %s, Size: %lu bytes\n", (current_sub_chain->type == PROCESS) ? "PROCESS" : "HOLE", current_sub_chain->size);
            current_sub_chain = current_sub_chain->next;
        }
        main_chain_node_count++;
        current_main_chain = current_main_chain->next;
    }
}

// Function to get physical address mapped to MeMS virtual address
void *mems_get(void *v_ptr)
{
    // Calculate physical address (which is the same for MeMS)
    return v_ptr;
}

int main()
{
    mems_init();

    void *mem1 = mems_malloc(1000);
    void *mem2 = mems_malloc(2000);

    printf("Memory 1: %p\n", mem1);
    printf("Memory 2: %p\n", mem2);

    mems_print_stats();

    mems_free(mem1);
    mems_free(mem2);

    mems_print_stats();

    mems_finish();
    return 0;
}
