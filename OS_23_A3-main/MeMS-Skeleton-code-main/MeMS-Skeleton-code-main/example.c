#include <stdio.h>
#include <stdlib.h>
#include "mems.h" // Include your MeMS header file here

int main(int argc, char const *argv[])
{
    // Initialize the MeMS system
    mems_init();

    int *ptr[10];

    // Allocate 10 arrays of 250 integers each
    printf("\n------- Allocated virtual addresses [mems_malloc] -------\n");
    for (int i = 0; i < 10; i++)
    {
        ptr[i] = (int *)mems_malloc(sizeof(int) * 250);
        printf("Virtual address: %lu\n", (unsigned long)ptr[i]);
    }

    // Assign a value to the 1st index of the first array and access it via 0th index
    printf("\n------ Assigning value to Virtual address [mems_get] -----\n");
    int *phy_ptr = (int *)mems_get(&ptr[0][1]);  // Get the address of index 1
    phy_ptr[0] = 200;                            // Put value at index 1
    int *phy_ptr2 = (int *)mems_get(&ptr[0][0]); // Get the address of index 0
    printf("Virtual address: %lu\tPhysical Address: %lu\n", (unsigned long)ptr[0], (unsigned long)phy_ptr2);
    printf("Value written: %d\n", phy_ptr2[1]); // Print the value at index 1

    // Print MeMS system statistics
    printf("\n--------- Printing Stats [mems_print_stats] --------\n");
    mems_print_stats();

    // Free up memory and reallocate
    printf("\n--------- Freeing up the memory [mems_free] --------\n");
    mems_free(ptr[3]);
    mems_print_stats();
    ptr[3] = (int *)mems_malloc(sizeof(int) * 250);
    mems_print_stats();

    // Finish and clean up the MeMS system
    mems_finish();
    
mems_init();

    printf("Allocated virtual addresses [mems_malloc]\n");
    void* mem1 = mems_malloc(1000);
    void* mem2 = mems_malloc(2000);
    void* mem3 = mems_malloc(1000);
    void* mem4 = mems_malloc(1000);
    void* mem5 = mems_malloc(600);

    printf("Virtual address: %lu\n", (unsigned long)mem1);
    printf("Virtual address: %lu\n", (unsigned long)mem2);
    printf("Virtual address: %lu\n", (unsigned long)mem3);
    printf("Virtual address: %lu\n", (unsigned long)mem4);
    printf("Virtual address: %lu\n", (unsigned long)mem5);

    printf("------ Assigning value to Virtual address [mems_get] -----\n");
    // how to write to the virtual address of the MeMS (this is given to show that the system works on arrays as well)
    //int* phy_ptr = (int*)mems_get(mem1 + 4); // get the address of index 1
    phy_ptr[0] = 200; // put value at index 1
    //int* phy_ptr2 = (int*)mems_get(mem1); // get the address of index 0
    printf("Virtual address: %lu\tPhysical Address: %lu\n", (unsigned long)mem1, (unsigned long)phy_ptr2);
    printf("Value written: %d\n", phy_ptr2[1]); // print the address of index 1

    printf("Printing Stats [mems_print_stats]\n");
    mems_print_stats();

    printf("Freeing up the memory [mems_free]\n");
    mems_free(mem2);
    mems_free(mem4);

    mems_print_stats();

    printf("Unmapping all memory [mems_finish]\n");
    mems_finish();

    return 0;
    
    
    

    
}
