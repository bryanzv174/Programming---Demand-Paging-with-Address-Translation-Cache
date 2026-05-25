// Carlos D. Martinez                                Red ID: 827940172
// Bryan D. Zavala Velasco                      Red ID: 130177824


#include <iostream>
#include <vector>
#include <cstdlib>
#include <cstring>
#include <memory>
#include "pagetable.h"
#include "tracereader.h"
#include "log.h"
#include "tlb.h"

std::unique_ptr<TLBCache> tlbCache = nullptr; // Initialize TLB pointer to null
PageTable* pageTable = nullptr; // Pointer for the page table, starts uninitialized

// Function to initialize simulation parameters and validate input
void initializeSimulation(int argc, char* argv[]) {
    std::vector<int> levelBits; // Holds bits used for each page table level
    char* traceFile = nullptr; // Path to the trace file
    int tlbSize = 0; // Default TLB size
    int numAccesses = -1; // Number of memory accesses to process (-1 means all)
    std::string outputMode = "summary"; // Default output mode is summary

    // Parse command-line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0) {
            numAccesses = atoi(argv[++i]);
            if (numAccesses <= 0) { // Check for valid memory access count
                std::cerr << "Number of memory accesses must be a number, greater than 0" << std::endl;
                exit(1);
            }
        } else if (strcmp(argv[i], "-c") == 0) {
            tlbSize = atoi(argv[++i]);
            if (tlbSize < 0) { // Check for valid TLB size
                std::cerr << "Cache capacity must be a number, greater than or equal to 0" << std::endl;
                exit(1);
            }
        } else if (strcmp(argv[i], "-o") == 0) {
            outputMode = argv[++i]; // Set the specified output mode
        } else if (strstr(argv[i], ".tr") != nullptr) {
            traceFile = argv[i]; // Trace file path is identified
        } else {
            int bits = atoi(argv[i]);
            if (bits < 1) { // Ensure each level uses at least 1 bit
                std::cerr << "Level " << levelBits.size() << " page table must be at least 1 bit" << std::endl;
                exit(1);
            }
            levelBits.push_back(bits);
        }
    }

    // Error if missing required arguments (trace file or level bits)
    if (!traceFile || levelBits.empty()) {
        std::cerr << "Error: Missing required arguments." << std::endl;
        exit(1);
    }

    // Validate total bits used in the page table levels
    int totalBits = 0;
    for (int bits : levelBits) {
        totalBits += bits;
    }
    if (totalBits > 28) { // Max allowed bits for page table is 28
        std::cerr << "Too many bits used in page tables" << std::endl;
        exit(1);
    }

    // Confirm trace file can be opened
    FILE* traceFilePtr = fopen(traceFile, "rb");
    if (!traceFilePtr) {
        std::cerr << "Unable to open <<" << traceFile << ">>" << std::endl;
        exit(1);
    }
    fclose(traceFilePtr); // Close immediately, we just wanted to validate it

    // Initialize TLB cache if a size is specified; otherwise, TLB remains nullptr
    tlbCache = (tlbSize > 0) ? std::make_unique<TLBCache>(tlbSize) : nullptr;
    pageTable = new PageTable(levelBits, tlbSize, tlbSize); // Page table creation

    // If output mode is bitmasks, calculate and log bitmasks for each level
    if (outputMode == "bitmasks") {
        std::vector<uint32_t> masks;
        uint32_t shift = 32;
        for (int bits : levelBits) {
            shift -= bits;
            uint32_t mask = ((1U << bits) - 1) << shift;
            masks.push_back(mask);
        }
        log_bitmasks(levelBits.size(), masks.data());
    } else {
        processTraceFile(traceFile, numAccesses, outputMode); // Process the trace file
    }

    delete pageTable; // Cleanup after simulation
}
// Reads and processes the trace file based on mode and parameters
void processTraceFile(const char *traceFile, int numAccesses, std::string outputMode) {
    FILE *traceFilePtr = fopen(traceFile, "rb"); // Open trace file
    if (!traceFilePtr) {
        std::cerr << "Error: Unable to open trace file." << std::endl;
        return;
    }
    // Counters for simulation stats
    int addressCount = 0;
    int cacheHits = 0;
    int frames_used = 0;
    unsigned long int pgtableEntries = 0;
    int page_size = pageTable->getPageSize();
    int pageTableHits = 0;
    p2AddrTr traceAddress;

    // Process each address in the trace file up to the specified limit
    while ((numAccesses == -1 || addressCount < numAccesses) && NextAddress(traceFilePtr, &traceAddress)) {
        unsigned int virtualAddress = traceAddress.addr;
        unsigned int physicalAddress = 0;

        bool tlbHit = false;
        bool pthit = false;
        
        // Translate the virtual address and update hit/miss stats
        physicalAddress = pageTable->translateAddress(virtualAddress, tlbHit, pthit, outputMode);

        if (tlbHit) {
            cacheHits++;
        } else if (pthit) {
            pageTableHits++;
        } else {
            frames_used++;
        }

        addressCount++; // Count each address processed

        // Log address translation based on the selected output mode
        if (outputMode == "vpn2pfn") {
            pageTable->logVPN2PFN(virtualAddress);
        } else if (outputMode == "offset") {
            log_offset(virtualAddress);
        }
    }

    pgtableEntries = pageTable->getTotalEntries(); // Total page table entries
    if (outputMode == "summary") {
        // Log summary of cache/page hits, misses, and other details
        log_summary(page_size, cacheHits, pageTableHits, addressCount, frames_used, pgtableEntries);
    }

    fclose(traceFilePtr); // Close trace file after processing
}

int main(int argc, char *argv[]) {
    initializeSimulation(argc, argv); // Initialize simulation based on user input
    return 0;
}


