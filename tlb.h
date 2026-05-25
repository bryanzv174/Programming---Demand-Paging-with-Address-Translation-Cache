
#ifndef TLB_H
#define TLB_H

#include <unordered_map>
#include <list>
#include <utility> 
#include <memory>

// TLBCache class
class TLBCache {
public:
    TLBCache(size_t size); // Constructor that sets maximum TLB size 
    bool contains(unsigned int vpn, unsigned int &physicalAddress); // Checks if VPN exists and retrieves physical address
    explicit TLBCache(int size); // Alternative constructor, initializes with int size
    void insert(unsigned int vpn, unsigned int physicalAddress); // Adds a new VPN to physical address mapping
    bool lookup(unsigned int vpn, unsigned int& pfn); // Looks up VPN and retrieves the physical frame number

private:
    size_t maxSize; // Maximum number of entries in the TLB cache
    std::list<std::pair<unsigned int, unsigned int>> cache; // List to maintain LRU order; stores {VPN, physical address} 

    // Unordered map for fast VPN lookup, pointing to list iterators for efficient LRU updates
    std::unordered_map<unsigned int, std::list<std::pair<unsigned int, unsigned int>>::iterator> cacheMap, map;
};

// External unique pointer to TLB cache for use in other files
extern std::unique_ptr<TLBCache> tlbCache;

#endif

