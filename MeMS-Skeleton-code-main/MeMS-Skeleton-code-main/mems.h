#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/mman.h>

// Macros for page size (you can adjust this if needed)
#define PAGE_SIZE 4096

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

// Constants for node types
#define PROCESS 1
#define HOLE 0

// Function to initialize MeMS
void mems_init()
{
    // Initialize necessary parameters
    free_list_head = NULL;
    mems_start_address = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    // Initialize other global variables
}

// Function to finish MeMS and deallocate memory
void mems_finish()
{
    // Unmap allocated memory
    if (mems_start_address != NULL)
    {
        munmap(mems_start_address, PAGE_SIZE);
    }

    // Traverse and free all nodes in the free list
    MainChainNode *main_node = free_list_head;
    while (main_node != NULL)
    {
        SubChainNode *sub_node = main_node->sub_chain;
        while (sub_node != NULL)
        {
            SubChainNode *temp = sub_node;
            sub_node = sub_node->next;
            free(temp);
        }
        MainChainNode *temp_main = main_node;
        main_node = main_node->next;
        free(temp_main);
    }
}

// Function to allocate memory
void *mems_malloc(size_t size)
{
    // Search free list for a suitable segment, else request memory from OS
    // Update free list accordingly
    void *mem_ptr = NULL;

    // Iterate through the free list to find a suitable segment
    MainChainNode *main_node = free_list_head;
    while (main_node != NULL)
    {
        SubChainNode *sub_node = main_node->sub_chain;
        while (sub_node != NULL)
        {
            if (sub_node->type == HOLE && sub_node->size >= size)
            {
                // Found a suitable HOLE
                if (sub_node->size > size)
                {
                    // Create a new HOLE with the remaining space
                    SubChainNode *new_hole = (SubChainNode *)malloc(sizeof(SubChainNode));
                    new_hole->start = (char *)sub_node->start + size;
                    new_hole->size = sub_node->size - size;
                    new_hole->type = HOLE;
                    new_hole->prev = sub_node;
                    new_hole->next = sub_node->next;
                    if (sub_node->next)
                    {
                        sub_node->next->prev = new_hole;
                    }
                    sub_node->next = new_hole;
                    sub_node->size = size;
                }
                sub_node->type = PROCESS;
                mem_ptr = sub_node->start;
                break;
            }
            sub_node = sub_node->next;
        }
        if (mem_ptr != NULL)
        {
            break;
        }
        main_node = main_node->next;
    }

    if (mem_ptr == NULL)
    {
        // If a suitable segment wasn't found, request memory from the OS
        void *new_memory = mmap(NULL, PAGE_SIZE, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

        // Update the free list with the new main chain node
        MainChainNode *new_main_node = (MainChainNode *)malloc(sizeof(MainChainNode));
        new_main_node->sub_chain = (SubChainNode *)malloc(sizeof(SubChainNode));
        new_main_node->sub_chain->start = new_memory;
        new_main_node->sub_chain->size = PAGE_SIZE;
        new_main_node->sub_chain->type = HOLE;
        new_main_node->sub_chain->next = NULL;
        new_main_node->sub_chain->prev = NULL;
        new_main_node->next = free_list_head;
        new_main_node->prev = NULL;
        if (free_list_head)
        {
            free_list_head->prev = new_main_node;
        }
        free_list_head = new_main_node;

        // Try allocating again
        mem_ptr = mems_malloc(size);
    }

    return mem_ptr;
}

// Function to deallocate memory
void mems_free(void *ptr)
{
    // Mark the corresponding sub-chain node as HOLE
    SubChainNode *sub_node = free_list_head->sub_chain;
    while (sub_node != NULL)
    {
        if (sub_node->start == ptr)
        {
            sub_node->type = HOLE;
            break;
        }
        sub_node = sub_node->next;
    }
}

// Function to print memory statistics
void mems_print_stats()
{
    int total_mapped_pages = 0;
    size_t unused_memory = 0;

    MainChainNode *main_node = free_list_head;
    while (main_node != NULL)
    {
        total_mapped_pages++;
        SubChainNode *sub_node = main_node->sub_chain;
        while (sub_node != NULL)
        {
            if (sub_node->type == HOLE)
            {
                unused_memory += sub_node->size;
            }
            sub_node = sub_node->next;
        }
        main_node = main_node->next;
    }

    printf("Total Mapped Pages: %d\n", total_mapped_pages);
    printf("Unused Memory: %zu bytes\n", unused_memory);
}

// Function to get physical address mapped to
