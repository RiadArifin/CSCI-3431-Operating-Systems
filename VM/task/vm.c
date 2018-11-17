/**
 * Virtual Memory Manager.
 *
 * Virtual memory manager where physical memory is less than
 * the size of logical memory.
 *
 * This implementation uses a simple FIFO page replacement algorithm.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define PAGE_NUMBER_MASK    0x0000FF00
#define OFFSET_MASK         0x000000FF

#define PAGE_SIZE           256
#define FRAME_SIZE          256

#define NUMBER_OF_FRAMES    64
#define PAGE_TABLE_SIZE     256
#define TLB_SIZE            16

#define BUFFER_SIZE         10

//#define ARRAYS
#define POINTERS

#define FIFO
//#define LRU

// function prototypes
void    init_page_table();
int     get_page_number(int);
int     get_offset(int);
int     get_next_free_frame();
int     init_physical_memory();
void    flush_tlb();
int     check_tlb(int);
void    update_tlb(int, int);
int     check_page_table(int);
int     invalidate_tlb(int);
int     invalidate_page_table(int);
int     FIFO_page_replacement();
int     LRU_page_replacement();
int     noop_page_replacement();

// an entry in the tlb
struct tlb_entry
{
    int page_number;
    int frame_number;
    int valid;
};

typedef struct tlb_entry tlb_entry_t;


FILE    *address_file;
FILE    *backing_store;

// buffer for reading logical addresses
char    buffer[BUFFER_SIZE];

// representation of physical memory
#ifdef POINTERS
signed char     *physical_memory;
#elif defined ARRAYS
signed char     physical_memory[NUMBER_OF_FRAMES][FRAME_SIZE];
#endif

// page table
int     page_table[PAGE_TABLE_SIZE];

// the logical address
int     logical_address;

// the translated physical address
int     physical_address;

// paging stuff
int     page_number;
int     frame_number;
int     offset;

// statistics
int     page_faults = 0;
int     tlb_hits = 0;

// list of free frames
int     free_frame_list[NUMBER_OF_FRAMES];

// number of free frames
int     number_of_free_frames;

// index of next fifo frame
int     next_fifo_frame;

// index of next free tlb entry
int     next_free_tlb_index;

// the tlb
tlb_entry_t tlb[TLB_SIZE];

// the value stored at the physical address
signed char     value;

// temporary frame we read from the backing store
signed char     temp_frame[FRAME_SIZE];

/**
 * Returns the page number from the logical address
 */
int get_page_number(int logical_address) 
{
        return (logical_address & PAGE_NUMBER_MASK) >> 8;
}

/**
 * Returns the offset from the logical address
 */
int get_offset(int logical_address) 
{
        return (logical_address & OFFSET_MASK);
}


/**
 * Initializes the page table
 */
void init_page_table()
{
    int i;
    
    for (i = 0; i < PAGE_TABLE_SIZE; i++)
        page_table[i] = -1;
}

/**
 * Initialize and allocate physical memory
 */

int init_physical_memory()
{
    int i;
    int rv;

#ifdef POINTERS
    physical_memory = (signed char *) malloc(FRAME_SIZE * NUMBER_OF_FRAMES);
#endif

    if (physical_memory == NULL) {
        rv = -1;
    }
    else {
        // initially record all frames as free
        for (i = 0; i < NUMBER_OF_FRAMES; i++) {
            free_frame_list[i] = 1;
        }

        number_of_free_frames = NUMBER_OF_FRAMES;
        
        // index of fifo frame - is used for fifo page replacement
        next_fifo_frame = 0;
        
        init_page_table();

        rv = 1;
    }

    return rv;
}

/**
 * Page replacement algorithm.
 *
 * A simple FIFO algorithm.
 */

int FIFO_page_replacement()
{
    int next_free_frame = -1;
    
    invalidate_tlb(next_fifo_frame);
    invalidate_page_table(next_fifo_frame);
    
    next_free_frame = next_fifo_frame;
    
    next_fifo_frame = (next_fifo_frame + 1) % NUMBER_OF_FRAMES;
    
    return next_free_frame;
}

/**
 * Page replacement algorithm--To be done!!! 
 *
 * LRU
 */
int LRU_page_replacement()
{
    /*Your task begins here
     * 
     *  (1) rearranging the list with the LRU page at the head of the list.
     *
     *  (2) putting a time stamp and finding the LRU (old stamp) page using O(n) algorithm.
     *
     *  
     */

    
    return 0;
}



/**
 * A simple noop page replacement algorithm.
 *
 * This should result in program failure!
 */
int noop_page_replacement()
{
    return -99;
}

/**
 * returns the index of the next free page frame.
 */
int get_next_free_frame()
{
    int next_free_frame = -1;
    int i = 0;
    
    if (number_of_free_frames > 0) {
        // search through the free frame list for a free frame
        while (free_frame_list[i] == -1)
            i++;
        
        next_free_frame = i;
        
        free_frame_list[i] = -1;
        
        number_of_free_frames--;
    }
    else
    {
        // select a frame for replacement
        
        #ifdef FIFO
            next_free_frame = FIFO_page_replacement();
        #elif defined LRU
            next_free_frame = LRU_page_replacement();
        #else
            next_free_frame = noop_page_replacement();
        #endif
    }
    
    return next_free_frame;
}



/**
 * Invalidates the tlb with the specified frame number.
 *
 * Return 0 if no tlb entry
 * Return 1 if tlb entry invalidated
 */
int invalidate_tlb(int frame_number)
{
    int i;
    int rv = 0;
    
    for (i = 0; i < TLB_SIZE; i++) {
        if (tlb[i].frame_number == frame_number) {
            tlb[i].valid = -1;
            tlb[i].page_number = -1;
            tlb[i].frame_number = -1;
            rv = 1;
            
            break;
        }
    }
    
    return rv;
}

/**
 * Invalidates the page table with the specified frame number.
 *
 * Return -1 error condition (no mapping in page table.)
 * Return 1 if successfully removes mapping of page to frame.
 */
int invalidate_page_table(int frame)
{
    int i;
    int rv = -1;
    
    for (i = 0; i < PAGE_TABLE_SIZE; i++) {
        if (page_table[i] == frame) {
            page_table[i] = -1;
            
            rv = 1;
            
            break;
        }
    }
    return rv;
}

/**
 * flush the tlb by invalidating all entries
 */
void flush_tlb() 
{
    int i;

    for (i = 0; i < TLB_SIZE; i++) {
        tlb[i].valid = -1;
        tlb[i].page_number = -1;
        tlb[i].frame_number = -1;
    }
    
    next_free_tlb_index = 0;
}

/**
 * checks if the associated page number is present in the tlb.
 *
 * Returns -1 if tlb-miss, >= 0 representing frame number if tlb-hit.
 */
int check_tlb(int page_number) 
{
    /* This is an O(n) linear search of the tlb. A dictionary-style lookup would be preferable. */
    int i;
    int frame_number = -1;

    for (i = 0; i < TLB_SIZE; i++) {
        if ( (tlb[i].page_number == page_number) && (tlb[i].valid == 1) ) {
            frame_number = tlb[i].frame_number;
            ++tlb_hits;

            break;
        }
    }

    return frame_number;
}

/**
 * Updates the tlb so that it now contains a mapping of the specified
 * page number to frame number.
 *
 */

void update_tlb(int page_number, int frame_number) 
{
    /* simple FIFO updating algorithm */
    tlb[next_free_tlb_index].page_number = page_number;
    tlb[next_free_tlb_index].frame_number = frame_number;
    tlb[next_free_tlb_index].valid = 1;

    next_free_tlb_index = (next_free_tlb_index + 1) % TLB_SIZE;
}

/**
 * checks if the associated page number is present in the page table.
 *
 * Returns -1 if page fault, >= 0 representing frame number if table hit.
 */
int check_page_table(int page_number) {
        int frame_number = -1;

        frame_number = page_table[page_number];

        if (frame_number == -1) { /* page fault */
            //printf("*");
            ++page_faults;

            // first seek to the appropriate page in the backing store
            if (fseek(backing_store, (page_number * PAGE_SIZE), SEEK_SET) != 0) {
                fprintf(stderr, "Error seeking in backing store\n");
                return -1;
            }

            // now read the page from the backing store to physical memory
            if (fread(temp_frame, sizeof(signed char), FRAME_SIZE, backing_store) == 0) {
                fprintf(stderr, "Error reading from backing store\n");
                return -1;
            }

            // update the page table
            frame_number = get_next_free_frame();
            page_table[page_number] = frame_number;

            // now copy from the temporary frame into physical memory
#ifdef POINTERS
            memcpy( (physical_memory + (frame_number * FRAME_SIZE)), temp_frame, FRAME_SIZE);
#elif defined ARRAYS
            memcpy(&physical_memory[frame_number], temp_frame, FRAME_SIZE);
#endif

            // update the tlb
            update_tlb(page_number, frame_number);
        }

        return frame_number;
}

int main(int argc, char *argv[])
{

    // initial error checking
    if (argc != 3) {
        fprintf(stderr,"Usage: ./a.out [backing store] [input file]\n");
        return -1;
    }

    // open the file containing the backing store
    backing_store = fopen(argv[1], "rb");
    
    if (backing_store == NULL) { 
        fprintf(stderr, "Error opening %s\n",argv[1]);
        return -1;
    }

    // open the file containing the logical addresses
    address_file = fopen(argv[2], "r");

    if (address_file == NULL) {
        fprintf(stderr, "Error opening %s\n",argv[2]);
        return -1;
    }

    // allocate physical memory

    if (init_physical_memory() == -1) {
        fprintf(stderr, "Error allocating physical memory\n");
        return -1;
    }

    // initialize the page table
    init_physical_memory();

    // initially flush the tlb
    flush_tlb();

    /**
     * The big show ....
     *
     * Read through the input file and translate each logical address
     * to its corresponding physical address, and extract the byte value
     * (represented as a signed char) at the physical address.
     */
    while ( fgets(buffer, BUFFER_SIZE, address_file) != NULL) {
        // read in a logical address
        logical_address = atoi(buffer);

        // extract the page number and offset from the logical address
        page_number = get_page_number(logical_address); 
        offset = get_offset(logical_address);

        // first try to get the frame from the tlb
        frame_number = check_tlb(page_number);

        if (frame_number == -1) { // tlb-miss - get frame number from page table
            frame_number = check_page_table(page_number);
        }
        // extract the value from the frame
#ifdef POINTERS
        value = *(physical_memory + (frame_number * FRAME_SIZE) + offset);
#elif defined ARRAYS
        value = physical_memory[frame_number][offset];
#endif

        //printf("%d \t %d\n",logical_address, value);
        printf("%d\n",value);
    }

    //printf("Page Faults = %d\n",page_faults);
    //printf("TLB Hits = %d\n",tlb_hits);

    // close file resources
    fclose(address_file);
    fclose(backing_store);

    return 0;
}

