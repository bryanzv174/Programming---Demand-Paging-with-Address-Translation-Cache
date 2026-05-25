#ifndef PAGETABLE_H
#define PAGETABLE_H

#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include "tlb.h"  

// Forward declaration for processing the trace file
void processTraceFile(const char *traceFile, int numAccesses, std::string outputMode);

// Map structure for storing the mapping of a page to a frame
struct Map {
    unsigned int frame;
};

// Level class: represents each level in the multi-level page table
class Level {
public:
    unsigned int numOfAccesses = 0; // Tracks accesses at this level
    unsigned int numOfEntries; // Number of entries at this level
    unsigned int frame = 0; // Frame number if this is a leaf node 
    Map* map = nullptr; // Map structure to store page-frame mapping if this is a leaf node
    Level** nextLevelPtr; // Array of pointers to next level

    // Constructor initializes entries for the level and allocates next level pointers
    Level(int entries) : numOfEntries(entries), nextLevelPtr(new Level*[entries]) {
        
        for (int i = 0; i < entries; ++i) {
            nextLevelPtr[i] = nullptr;
        }
    }

 // Destructor cleans up the next level pointers and map pointer
~Level() {
    for (unsigned int i = 0; i < numOfEntries; ++i) {  
        if (nextLevelPtr[i]) {
            delete nextLevelPtr[i];
        }
    }
    delete[] nextLevelPtr;
    delete map;
}

};

// PageTable class: implements a multi-level page table with optional TLB support
class PageTable {
private:
    unsigned int offsetBits;    // Number of bits for the offset
    int numLevels;              // Number of levels in the page table                      
    std::vector<int> levelBits; // Bit distribution across levels           
    int* bitMaskAry;            // Bit masks for each level            
    int* shiftAry;              // Shift values for each level           
    int* entryCountAry;         // Entry counts for each level          
    unsigned int nextFrame = 0; // Frame counter for new mappings       
    Level* root;                // Root of the page table (top level)           
    unsigned int maxCacheSize;  // Maximum cache size (for TLB)            

    // Recursive function to count total entries in the page table
    unsigned long countEntriesRecursive(size_t currentLevel, Level* currentLevelPtr) const;

    // Map for additional page table entries if required (unique pointers for safety)
    std::map<unsigned int, std::unique_ptr<Level>> pageTableEntries;

public:
    // Utility to extract page number from a virtual address given a mask and shift
    unsigned int extractPageNumberFromAddress(unsigned int virtualAddress, unsigned int mask, unsigned int shift);
    
    // Lookup and insert functions for VPN to PFN mappings
    Map* lookup_vpn2pfn(unsigned int virtualAddress);
    void insert_vpn2pfn(unsigned int virtualAddress, unsigned int frame);
    
    // Constructor and destructor for setting up and cleaning up the page table
    PageTable(const std::vector<int>& levels, int cacheSize, int tlbSize);  
    ~PageTable();  

    // Lookup function for the page table and helper methods                    
    unsigned int pageTableLookup(unsigned int virtualAddress);
    int getPageSize() const;           
    int getOffsetBits()const;
    unsigned int translate(unsigned int virtualAddress); 

    // Returns the total count of entries in the page table 
    unsigned long int getTotalEntries() const; 

    // Logs the VPN to PFN mapping for a given virtual address 
    void logVPN2PFN(unsigned int virtualAddress); 

    // Logs a summary of page table statistics
    void logSummary(unsigned int page_size, unsigned int cacheHits, unsigned int pageTableHits, unsigned int addresses, unsigned int frames_used, unsigned long int pgtableEntries);  // Summarizes cache hits and misses
    
    // Translates a virtual address and returns the corresponding physical address, with TLB and page table hit flags
    unsigned int translateAddress(unsigned int virtualAddress, bool &tlbhit, bool &pthit, const std::string &outputMode);
};

#endif

