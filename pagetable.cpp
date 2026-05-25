// Carlos D. Martinez                                Red ID: 827940172
// Bryan D. Zavala Velasco                      Red ID: 130177824


#include "pagetable.h"
#include "log.h"
#include <iostream>
#include <iomanip>
#include <unordered_map>

// Constructor: initializes the PageTable based on the levels specified
PageTable::PageTable(const std::vector<int>& levels, int cacheSize, int tlbSize)
    : levelBits(levels), maxCacheSize(cacheSize) {
    numLevels = levels.size();
    bitMaskAry = new int[numLevels];
    shiftAry = new int[numLevels];
    entryCountAry = new int[numLevels];

    // Calculate the offset bits by subtracting each level's bits from 32
    offsetBits = 32; 
    for (int bits : levels) {
        offsetBits -= bits;  
    }

    // Set up the bit masks, shifts, and entry counts for each level
    unsigned int shift = 32;
    for (int i = 0; i < numLevels; ++i) {
        shift -= levels[i];
        bitMaskAry[i] = ((1U << levels[i]) - 1) << shift;
        shiftAry[i] = shift;
        entryCountAry[i] = 1U << levels[i];
    }
    
    // Initialize the root level
    root = new Level(entryCountAry[0]);
    
    // Initialize the TLB cache if needed
    if (tlbSize > 0) {
        tlbCache = std::make_unique<TLBCache>(tlbSize);
    }
}

// Destructor: cleans up dynamically allocated arrays and root level
PageTable::~PageTable() {
    delete root;
    delete[] bitMaskAry;
    delete[] shiftAry;
    delete[] entryCountAry;
}

// Lookup function to retrieve the physical frame from the virtual address
unsigned int PageTable::pageTableLookup(unsigned int virtualAddress) {
    Level* current = root;
    unsigned int physicalFrame = 0;
    
    // Traverse each level in the page table to find the page entry
    for (int i = 0; i < numLevels; ++i) {
        unsigned int pageIndex = (virtualAddress & bitMaskAry[i]) >> shiftAry[i];

        if (pageIndex >= current->numOfEntries) {
            std::cerr << "Error: Índice fuera de los límites en el nivel " << i << std::endl;
            return 0;
        }

        if (current->nextLevelPtr[pageIndex] == nullptr) {
            std::cerr << "Error: Entrada de página no encontrada" << std::endl;
            return 0;
        }

        current = current->nextLevelPtr[pageIndex];
    }

    // Calculate the final physical address with offset
    physicalFrame = current->frame;
    unsigned int offset = virtualAddress & ((1U << shiftAry[numLevels - 1]) - 1);
    return (physicalFrame << shiftAry[numLevels - 1]) | offset;
}

// Extract page number from address using a mask and shift
unsigned int PageTable::extractPageNumberFromAddress(unsigned int virtualAddress, unsigned int mask, unsigned int shift) {
    return (virtualAddress & mask) >> shift;
}

// Look up the mapping in the page table; returns nullptr if not found
Map* PageTable::lookup_vpn2pfn(unsigned int virtualAddress) {
    Level* currentLevel = root;
    unsigned int shiftAmount = 32;

    // Traverse through each level in the page table
    for (size_t i = 0; i < levelBits.size(); ++i) {
        shiftAmount -= levelBits[i];
       unsigned int vpnPart = extractPageNumberFromAddress(virtualAddress, ((1U << levelBits[i]) - 1) << shiftAmount, shiftAmount);

        if (currentLevel->nextLevelPtr[vpnPart] == nullptr) {
            return nullptr; // Return null if VPN not found
        }
        currentLevel = currentLevel->nextLevelPtr[vpnPart];
    }

    
    return currentLevel->map; // Return map containing the frame
}

// Insert a new VPN to PFN mapping if it doesn't exist
void PageTable::insert_vpn2pfn(unsigned int virtualAddress, unsigned int frame) {
    Level* currentLevel = root;
    unsigned int shiftAmount = 32;

    // Traverse each level and create entries as needed
    for (size_t i = 0; i < levelBits.size(); ++i) {
        shiftAmount -= levelBits[i];
       unsigned int vpnPart = extractPageNumberFromAddress(virtualAddress, ((1U << levelBits[i]) - 1) << shiftAmount, shiftAmount);


        if (currentLevel->nextLevelPtr[vpnPart] == nullptr) {
            currentLevel->nextLevelPtr[vpnPart] = new Level(i + 1 < levelBits.size() ? entryCountAry[i + 1] : 0);
        }
        currentLevel = currentLevel->nextLevelPtr[vpnPart];
    }

    currentLevel->map = new Map{frame}; // Set the map to contain the frame
}

// Translates a virtual address to a physical address using TLB and page table
unsigned int PageTable::translateAddress(unsigned int virtualAddress, bool &tlbhit, bool &pthit, const std::string &outputMode) {
    unsigned int physicalAddress = 0; 
    unsigned int offset = virtualAddress & ((1U << offsetBits) - 1);  
    unsigned int vpn = virtualAddress >> offsetBits;

    // Check for a TLB hit
    tlbhit = tlbCache && tlbCache->contains(vpn, physicalAddress);
    if (tlbhit) {
       
        physicalAddress = (physicalAddress << offsetBits) | offset;
    } else {
      
        // If no TLB hit, check the page table
        Map* mapping = lookup_vpn2pfn(virtualAddress);
        
        if (mapping) {
           
            pthit = true;
            physicalAddress = (mapping->frame << offsetBits) | offset;
        } else {
          
            insert_vpn2pfn(virtualAddress, nextFrame++);
            pthit = false;
            physicalAddress = ((nextFrame - 1) << offsetBits) | offset;
        }

        
        if (tlbCache) {
            tlbCache->insert(vpn, (physicalAddress >> offsetBits));  
        }
    }

    // Log output based on mode
    if (outputMode == "va2pa") {
        log_virtualAddr2physicalAddr(virtualAddress, physicalAddress);
    } else if (outputMode == "va2pa_atc_ptwalk") {
        log_va2pa_ATC_PTwalk(virtualAddress, physicalAddress, tlbhit, pthit);
    }

    return physicalAddress;
}

// Logs the VPN to PFN mapping for a given virtual address
void PageTable::logVPN2PFN(unsigned int virtualAddress) {
    std::vector<uint32_t> pages;
    unsigned int shiftAmount = 0;

    // Extract VPN parts for each level
    for (size_t currentLevel = 0; currentLevel < levelBits.size(); ++currentLevel) {
        int levelBitsCount = levelBits[currentLevel];
        unsigned int vpn = (virtualAddress >> (32 - shiftAmount - levelBitsCount)) & ((1 << levelBitsCount) - 1);
        pages.push_back(vpn);
        shiftAmount += levelBitsCount;
    }

    // Traverse the page table to find the frame
    Level* currentLevelPtr = root;
    for (auto vpnPart : pages) {
        if (!currentLevelPtr || vpnPart >= currentLevelPtr->numOfEntries || !currentLevelPtr->nextLevelPtr[vpnPart]) {
 
            log_pagemapping(pages.size(), pages.data(), 0);
            return;
        }
        currentLevelPtr = currentLevelPtr->nextLevelPtr[vpnPart];
    }

 
    uint32_t frame = (currentLevelPtr && currentLevelPtr->map) ? currentLevelPtr->map->frame : 0;
    log_pagemapping(pages.size(), pages.data(), frame);
}

// Recursively counts all entries in the page table
unsigned long PageTable::countEntriesRecursive(size_t currentLevel, Level* currentLevelPtr) const {
    if (currentLevelPtr == nullptr) {
        return 0;
    }

   
    unsigned long entriesAtCurrentLevel = 1UL << levelBits[currentLevel];

    
    if (currentLevel == levelBits.size() - 1) {
        return entriesAtCurrentLevel;
    }

    
    unsigned long totalEntries = entriesAtCurrentLevel;

    for (size_t i = 0; i < currentLevelPtr->numOfEntries; ++i) {
        if (currentLevelPtr->nextLevelPtr[i] != nullptr) {
            totalEntries += countEntriesRecursive(currentLevel + 1, currentLevelPtr->nextLevelPtr[i]);
        }
    }

    return totalEntries;
}




// Gets the total number of entries in the page table
unsigned long int PageTable::getTotalEntries() const {
    return countEntriesRecursive(0, root);  
}

// Calculates page size based on offset bits
int PageTable::getPageSize() const {
    int totalBits = 0;
    for (int bits : levelBits) {
        totalBits += bits;
    }
    int offsetBits = 32 - totalBits;  
    return 1 << offsetBits; 
}

// Logs a summary of the simulation results
void PageTable::logSummary(unsigned int page_size, unsigned int cacheHits, unsigned int pageTableHits, unsigned int addresses, unsigned int frames_used, unsigned long int pgtableEntries) {
    std::cout << "Cache hits: " << cacheHits << ", Page hits: " << pageTableHits
              << ", Addresses processed: " << addresses << std::endl;
}
