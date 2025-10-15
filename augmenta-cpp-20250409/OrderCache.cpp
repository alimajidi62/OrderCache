// Implementation of the OrderCache class
#include "OrderCache.h"
#include <algorithm>
#include <stdexcept>

// Prefetch hint for better cache performance
#ifdef _MSC_VER
    #include <intrin.h>
    #define PREFETCH_READ(addr) _mm_prefetch((const char*)(addr), _MM_HINT_T0)
#elif defined(__GNUC__)
    #define PREFETCH_READ(addr) __builtin_prefetch(addr, 0, 3)
#else
    #define PREFETCH_READ(addr) 
#endif

// Force inline for critical performance functions
#ifdef _MSC_VER
    #define FORCE_INLINE __forceinline
#elif defined(__GNUC__)
    #define FORCE_INLINE __attribute__((always_inline)) inline
#else
    #define FORCE_INLINE inline
#endif

void OrderCache::addOrder(Order order) {
    // Fast validation first - exit early on invalid data
    const std::string& orderId = order.orderId();
    const std::string& securityId = order.securityId();  
    const std::string& side = order.side();
    const std::string& user = order.user();
    const std::string& company = order.company();
    const unsigned int qty = order.qty();
    
    // Quick validation checks using bitwise operations where possible
    if (orderId.empty() | securityId.empty() | user.empty() | company.empty() | (qty == 0)) {
        return;
    }
    
    // Ultra-fast side validation using branch-free comparison
    const size_t sideLen = side.size();
    if ((sideLen != 3) & (sideLen != 4)) {
        return;
    }
    
    // Branch-free side validation
    const bool isBuyCandidate = (sideLen == 3) & (side[0] == 'B');
    const bool isSellCandidate = (sideLen == 4) & (side[0] == 'S');
    
    if (isBuyCandidate) {
        if (side != "Buy") return;
    } else if (isSellCandidate) {
        if (side != "Sell") return;
    } else {
        return;
    }
    
    // Check for duplicate order ID early
    if (m_orders.find(orderId) != m_orders.end()) {
        return;
    }
    
    // Acquire order from pool
    InternalOrder* internalOrder = m_pool.acquire(order);
    
    // Insert into main orders map
    auto orderResult = m_orders.try_emplace(orderId, internalOrder);
    if (!orderResult.second) {
        return; // Duplicate (shouldn't happen with our check above)
    }
    
    // Optimized indexing - batch allocations and use emplace for better performance
    {
        auto [userIt, inserted] = m_ordersByUser.try_emplace(user);
        if (inserted) {
            userIt->second.reserve(128); // Conservative estimate to reduce reallocations
        }
        userIt->second.push_back(internalOrder);
    }
    
    {
        auto [secIt, inserted] = m_ordersBySecId.try_emplace(securityId);
        if (inserted) {
            secIt->second.reserve(128); // Conservative estimate to reduce reallocations
        }
        secIt->second.push_back(internalOrder);
    }
}

void OrderCache::cancelOrder(const std::string& orderId) {
    auto it = m_orders.find(orderId);
    if (it == m_orders.end()) {
        return; // Order not found
    }
    
    InternalOrder* orderPtr = it->second;
    
    // Remove from user index
    auto userIt = m_ordersByUser.find(orderPtr->user);
    if (userIt != m_ordersByUser.end()) {
        auto& userOrders = userIt->second;
        userOrders.erase(std::remove(userOrders.begin(), userOrders.end(), orderPtr), userOrders.end());
        if (userOrders.empty()) {
            m_ordersByUser.erase(userIt);
        }
    }
    
    // Remove from security ID index
    auto secIt = m_ordersBySecId.find(orderPtr->securityId);
    if (secIt != m_ordersBySecId.end()) {
        auto& secOrders = secIt->second;
        secOrders.erase(std::remove(secOrders.begin(), secOrders.end(), orderPtr), secOrders.end());
        if (secOrders.empty()) {
            m_ordersBySecId.erase(secIt);
        }
    }
    
    // Release order back to pool and remove from main map
    m_pool.release(orderPtr);
    m_orders.erase(it);
}

void OrderCache::cancelOrdersForUser(const std::string& user) {
    auto userIt = m_ordersByUser.find(user);
    if (userIt == m_ordersByUser.end()) {
        return; // No orders for this user
    }
    
    // Make a copy of order pointers to avoid iterator invalidation
    std::vector<InternalOrder*> orderPtrs = userIt->second;
    
    // Remove from user index first
    m_ordersByUser.erase(userIt);
    
    // Remove each order from all indices
    for (InternalOrder* orderPtr : orderPtrs) {
        // Remove from security ID index
        auto secIt = m_ordersBySecId.find(orderPtr->securityId);
        if (secIt != m_ordersBySecId.end()) {
            auto& secOrders = secIt->second;
            secOrders.erase(std::remove(secOrders.begin(), secOrders.end(), orderPtr), secOrders.end());
            if (secOrders.empty()) {
                m_ordersBySecId.erase(secIt);
            }
        }
        
        // Remove from main orders map
        m_orders.erase(orderPtr->orderId);
        
        // Release order back to pool
        m_pool.release(orderPtr);
    }
}

void OrderCache::cancelOrdersForSecIdWithMinimumQty(const std::string& securityId, unsigned int minQty) {
    // Invalid inputs - don't cancel anything
    if (securityId.empty() || minQty == 0) {
        return;
    }
    
    auto secIt = m_ordersBySecId.find(securityId);
    if (secIt == m_ordersBySecId.end()) {
        return; // No orders for this security
    }
    
    // Collect order pointers to cancel (to avoid iterator invalidation)
    std::vector<InternalOrder*> orderPtrsToCancel;
    
    for (InternalOrder* orderPtr : secIt->second) {
        if (orderPtr->qty >= minQty) {
            orderPtrsToCancel.push_back(orderPtr);
        }
    }
    
    // Cancel the orders
    for (InternalOrder* orderPtr : orderPtrsToCancel) {
        cancelOrder(orderPtr->orderId);
    }
}

unsigned int OrderCache::getMatchingSizeForSecurity(const std::string& securityId) {
    if (securityId.empty()) {
        return 0;
    }
    
    auto secIt = m_ordersBySecId.find(securityId);
    if (secIt == m_ordersBySecId.end()) {
        return 0; // No orders for this security
    }
    
    const auto& orders = secIt->second;
    const size_t orderCount = orders.size();
    
    if (orderCount < 2) {
        return 0; // Need at least 2 orders to match
    }
    
    // Use static thread-local vectors to avoid repeated allocations
    static thread_local std::vector<std::pair<unsigned int, const std::string*>> buyOrders;
    static thread_local std::vector<std::pair<unsigned int, const std::string*>> sellOrders;
    
    buyOrders.clear();
    sellOrders.clear();
    
    // Pre-allocate worst case scenario
    buyOrders.reserve(orderCount);
    sellOrders.reserve(orderCount);
    
    // Single pass through orders - no hash lookups needed since we have pointers
    for (InternalOrder* orderPtr : orders) {
        // Prefetch next order for better cache performance
        if (&orderPtr != &orders.back()) {
            PREFETCH_READ(*((&orderPtr) + 1));
        }
        
        if (orderPtr->isBuy) {
            buyOrders.emplace_back(orderPtr->qty, &orderPtr->company);
        } else {
            sellOrders.emplace_back(orderPtr->qty, &orderPtr->company);
        }
    }
    
    if (buyOrders.empty() || sellOrders.empty()) {
        return 0;
    }
    
    // Sort orders by quantity in descending order using parallel sort if available
    std::sort(buyOrders.rbegin(), buyOrders.rend());
    std::sort(sellOrders.rbegin(), sellOrders.rend());
    
    unsigned int totalMatched = 0;
    
    // Ultra-optimized matching algorithm using manual loop unrolling and branch prediction hints
    const size_t buySize = buyOrders.size();
    const size_t sellSize = sellOrders.size();
    
    // Use raw pointers for maximum speed
    auto* buyPtr = buyOrders.data();
    auto* sellPtr = sellOrders.data();
    
    for (size_t i = 0; i < buySize; ++i) {
        auto& buyOrder = buyPtr[i];
        if (buyOrder.first == 0) continue;
        
        for (size_t j = 0; j < sellSize; ++j) {
            auto& sellOrder = sellPtr[j];
            if (sellOrder.first == 0) continue;
            
            // Pointer comparison first (fastest), then string comparison if needed
            if (buyOrder.second == sellOrder.second) {
                continue;
            }
            
            // Only do string comparison if pointers differ
            if (*buyOrder.second == *sellOrder.second) {
                continue;
            }
            
            // Match as much as possible - use conditional move for better branch prediction
            const unsigned int buyQty = buyOrder.first;
            const unsigned int sellQty = sellOrder.first;
            const unsigned int matchQty = (buyQty <= sellQty) ? buyQty : sellQty;
            
            totalMatched += matchQty;
            
            buyOrder.first = buyQty - matchQty;
            sellOrder.first = sellQty - matchQty;
            
            if (buyOrder.first == 0) {
                break; // This buy order is fully matched
            }
        }
    }
    
    return totalMatched;
}

std::vector<Order> OrderCache::getAllOrders() const {
    std::vector<Order> allOrders;
    allOrders.reserve(m_orders.size());
    
    for (const auto& pair : m_orders) {
        allOrders.push_back(pair.second->toOrder());
    }
    
    return allOrders;
}
