// Carlos D. Martinez                                Red ID: 827940172
// Bryan D. Zavala Velasco                      Red ID: 130177824

#include "tlb.h"
#include <algorithm>
#include "pagetable.h"
#include <iostream>
#include <algorithm>
#include <iostream>

// Constructor: Initializes the TLB cache with the specified maximum size
TLBCache::TLBCache(int size) : maxSize(size) {}

// Checks if a virtual page number (VPN) is in the TLB cache
// If found, returns true and updates physicalAddress with the cached value
bool TLBCache::contains(unsigned int vpn, unsigned int &physicalAddress) {
    auto it = cacheMap.find(vpn); // Search for VPN in cache map
    if (it != cacheMap.end()) {
        physicalAddress = it->second->second; // Set physical address from cached entry
        
        // Move accessed entry to the front (LRU)
        cache.splice(cache.begin(), cache, it->second);
        return true; // TLB hit 
    }
    return false; // TLB miss
}

// Inserts a new VPN to physical address mapping into the TLB
void TLBCache::insert(unsigned int vpn, unsigned int physicalAddress) {
    
    // Remove existing entry if VPN is already in cache
    auto it = cacheMap.find(vpn);
    if (it != cacheMap.end()) {
        
        cache.erase(it->second);
        cacheMap.erase(it);
    }

    // If the cache is full, remove the least recently used (LRU) entry
    if (cache.size() >= maxSize) {
        auto last = cache.back();
        cacheMap.erase(last.first); // Remove last item from cache map
        cache.pop_back();           // Remove last item from list (LRU)         
    }

    // Insert new VPN-physical address pair at the front of the cache
    cache.push_front({vpn, physicalAddress});
    cacheMap[vpn] = cache.begin();
}

// Alternative lookup function: checks if VPN is in TLB and returns the physical frame number
bool TLBCache::lookup(unsigned int vpn, unsigned int& pfn) {
    auto it = cacheMap.find(vpn);
    if (it != cacheMap.end()) {
       
        // Move accessed entry to the front (LRU)
        cache.splice(cache.begin(), cache, it->second);
        pfn = it->second->second; // Set physical frame number
        return true; // TLB hit
    }
    return false; // TLB miss
}






