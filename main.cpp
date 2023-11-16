/*
Main program for the virtual memory project.
Make all of your modifications to this file.
You may add or rearrange any code or data as you need.
The header files page_table.h and disk.h explain
how to use the page table and disk interfaces.
*/

#include "page_table.h"
#include "disk.h"
#include "program.h"

#include <cassert>
#include <iostream>
#include <string.h>

using namespace std;

// Prototype for test program
typedef void (*program_f)(char *data, int length);

// Number of physical frames
int nframes;
// Pointer to disk for access from handlers
struct disk *disk = nullptr;
const char *algorithm;

program_f program;

constexpr static int invalid_frame = -1;

void evict_page(page_table *pt, int page, int selected_page, int selected_frame) {
    if (pt->page_bits[selected_page]==(PROT_READ|PROT_WRITE)) {
        printf("RW\n");
        disk_write(disk,selected_page,&pt->physmem[selected_frame*PAGE_SIZE]);
    }

    disk_read(disk,page,&pt->physmem[selected_frame*PAGE_SIZE]);
    page_table_set_entry(pt,page,selected_frame,PROT_READ);
    page_table_set_entry(pt,selected_page,selected_frame, 0);
    pt->page_mapping[selected_page] = invalid_frame;
}

bool try_select_avaliable_frame(page_table *pt, int page) {
    static int mapped_frame = 0;

    if (mapped_frame >= nframes) return false;

    printf("insdier non-full non-resident\n");
    page_table_set_entry(pt, page, mapped_frame, PROT_READ);
    printf("before disk read\n");
    disk_read(disk,page,&pt->physmem[mapped_frame * PAGE_SIZE]);
    mapped_frame += 1;

    return true;
}

// Simple handler for pages == frames
void page_fault_handler_example(struct page_table *pt, int page)
{
    cout << "page fault on page #" << page << endl;

    // Print the page table contents
    cout << "Before ---------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;

    // Map the page to the same frame number and set to read/write
    // TODO - Disable exit and enable page table update for example
    exit(1);
    // page_table_set_entry(pt, page, page, PROT_READ | PROT_WRITE);

    // Print the page table contents
    cout << "After ----------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;
}

void page_fault_handler_rand(struct page_table *pt, int page) {
    cout << "page fault on page #" << page << endl;

    // Print the page table contents
    cout << "Before ---------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;

    if (pt->page_bits[page] == 0) {
        if (!try_select_avaliable_frame(pt, page)) {
            printf("inside none\n");
            int selected_frame = rand() % nframes;
            cout<<"selected frame:"<<selected_frame<<endl;
            int selected_page = page_table_get_page(pt, selected_frame);
            cout<<"selected page:"<<selected_page<<endl;

            evict_page(pt, page, selected_page, selected_frame);
        }
    }
    else if (pt->page_bits[page] == PROT_READ){
         page_table_set_entry(pt, page, pt->page_mapping[page], PROT_READ |PROT_WRITE);
    }
    else {
        cout << "Unexpected page bits " << pt->page_bits[page] << " for page " << page;
        exit(1);
    }

    // Print the page table contents
    cout << "After ----------------------------" << endl;
    page_table_print(pt);
    cout << "----------------------------------" << endl;
}

void page_fault_handler_fifo(struct page_table *pt, int page) {
    //TODO
    exit(1);
}

void page_fault_handler_custom(struct page_table *pt, int page) {
    //TODO
    exit(1);
}

int main(int argc, char *argv[])
{
    // Check argument count
    if (argc != 5)
    {
        cerr << "Usage: virtmem <npages> <nframes> <rand|fifo|custom> <sort|scan|focus>" << endl;
        exit(1);
    }

    // Parse command line arguments
    int npages = atoi(argv[1]);
    nframes = atoi(argv[2]);
    algorithm = argv[3];
    const char *program_name = argv[4];

    // Validate the algorithm specified
    if ((strcmp(algorithm, "rand") != 0) &&
        (strcmp(algorithm, "fifo") != 0) &&
        (strcmp(algorithm, "custom") != 0))
    {
        cerr << "ERROR: Unknown algorithm: " << algorithm << endl;
        exit(1);
    }

    // Validate the program specified
    program = NULL;
    if (!strcmp(program_name, "sort"))
    {
        if (nframes < 2)
        {
            cerr << "ERROR: nFrames >= 2 for sort program" << endl;
            exit(1);
        }

        program = sort_program;
    }
    else if (!strcmp(program_name, "scan"))
    {
        program = scan_program;
    }
    else if (!strcmp(program_name, "focus"))
    {
        program = focus_program;
    }
    else
    {
        cerr << "ERROR: Unknown program: " << program_name << endl;
        exit(1);
    }

    // TODO - Any init needed

    // Create a virtual disk
    disk = disk_open("myvirtualdisk", npages);
    if (!disk)
    {
        cerr << "ERROR: Couldn't create virtual disk: " << strerror(errno) << endl;
        return 1;
    }

    //srand(static_cast<unsigned int>(time(0)));
    
    // Create a page table
    //struct page_table *pt = page_table_create(npages, nframes, page_fault_handler_rand /* TODO - Replace with your handler(s)*/);

    struct page_table *pt = NULL;
    if (strcmp(algorithm,"rand") == 0) {
        pt = page_table_create(npages, nframes, page_fault_handler_rand);
    }
    if (!pt)
    {
        cerr << "ERROR: Couldn't create page table: " << strerror(errno) << endl;
        return 1;
    }

    // Run the specified program
    char *virtmem = page_table_get_virtmem(pt);
    program(virtmem, npages * PAGE_SIZE);

    // Clean up the page table and disk
    page_table_delete(pt);
    disk_close(disk);

    return 0;
}
