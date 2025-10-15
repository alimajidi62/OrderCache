// Implementation of the OrderCache class
#include "OrderCache.h"
#include <algorithm>
#include <stdexcept>

// C++17/20 compatibility for likely/unlikely attributes
#if __cplusplus >= 202002L
    // C++20 has [[likely]] and [[unlikely]]
#else
    // Fallback for older standards
    #define likely
    #define unlikely
#endif

void OrderCache::addOrder(Order order) {
    // Cache string references to avoid repeated method calls
    const std::string& orderId = order.orderId();
    
    // Fast path: check empty orderId first (most common invalid case)
    if (orderId.empty()) {
        return;
    }
    
    // Single hash map lookup and insertion - avoid double lookup
    auto result = m_orders.try_emplace(orderId, std::move(order));
    if (!result.second) {
        return; // Duplicate order ID
    }
    
    // Get reference to the inserted order (now moved)
    const Order& insertedOrder = result.first->second;
    
    // Cache remaining string references from the inserted order
    const std::string& securityId = insertedOrder.securityId();
    const std::string& side = insertedOrder.side();
    const std::string& user = insertedOrder.user();
    const std::string& company = insertedOrder.company();
    
    // Validate remaining fields - likely success path
    if (securityId.empty() || side.empty() || user.empty() || company.empty() || insertedOrder.qty() == 0) {
        // Remove the already inserted order on validation failure
        m_orders.erase(result.first);
        return;
    }
    
    // Optimized side validation - check length first, then first char, then full string
    const size_t sideLen = side.length();
    if (sideLen < 3 || sideLen > 4) {
        m_orders.erase(result.first);
        return;
    }
    
    const char firstChar = side[0];
    if (firstChar == 'B') {
        if (side != "Buy") {
            m_orders.erase(result.first);
            return;
        }
    } else if (firstChar == 'S') {
        if (side != "Sell") {
            m_orders.erase(result.first);
            return;
        }
    } else {
        m_orders.erase(result.first);
        return;
    }
    
    // Efficient indexing operations - use try_emplace and reserve
    auto userResult = m_ordersByUser.try_emplace(user);
    userResult.first->second.emplace(orderId);
    
    // For security indexing, reserve space if new security
    auto secResult = m_ordersBySecId.try_emplace(securityId);
    if (secResult.second) {
        secResult.first->second.reserve(64); // Reasonable default for new securities
    }
    secResult.first->second.emplace_back(orderId);
}

void OrderCache::cancelOrder(const std::string& orderId) {
    auto it = m_orders.find(orderId);
    if (it == m_orders.end()) {
        return; // Order not found
    }
    
    const Order& order = it->second;
    
    // Remove from user index
    auto userIt = m_ordersByUser.find(order.user());
    if (userIt != m_ordersByUser.end()) {
        userIt->second.erase(orderId);
        if (userIt->second.empty()) {
            m_ordersByUser.erase(userIt);
        }
    }
    
    // Remove from security ID index
    auto secIt = m_ordersBySecId.find(order.securityId());
    if (secIt != m_ordersBySecId.end()) {
        auto& orderIds = secIt->second;
        orderIds.erase(std::remove(orderIds.begin(), orderIds.end(), orderId), orderIds.end());
        if (orderIds.empty()) {
            m_ordersBySecId.erase(secIt);
        }
    }
    
    // Remove the order itself
    m_orders.erase(it);
}

void OrderCache::cancelOrdersForUser(const std::string& user) {
    auto userIt = m_ordersByUser.find(user);
    if (userIt == m_ordersByUser.end()) {
        return; // No orders for this user
    }
    
    // Make a copy of order IDs to avoid iterator invalidation
    std::vector<std::string> orderIds(userIt->second.begin(), userIt->second.end());
    
    // Cancel each order
    for (const std::string& orderId : orderIds) {
        cancelOrder(orderId);
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
    
    // Collect order IDs to cancel (to avoid iterator invalidation)
    std::vector<std::string> orderIdsToCancel;
    
    for (const std::string& orderId : secIt->second) {
        auto orderIt = m_orders.find(orderId);
        if (orderIt != m_orders.end() && orderIt->second.qty() >= minQty) {
            orderIdsToCancel.push_back(orderId);
        }
    }
    
    // Cancel the orders
    for (const std::string& orderId : orderIdsToCancel) {
        cancelOrder(orderId);
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
    
    // Use static thread-local vectors to avoid repeated allocations
    static thread_local std::vector<std::pair<unsigned int, std::string>> buyOrders;
    static thread_local std::vector<std::pair<unsigned int, std::string>> sellOrders;
    
    buyOrders.clear();
    sellOrders.clear();
    
    // Reserve space to avoid reallocations
    const size_t orderCount = secIt->second.size();
    buyOrders.reserve(orderCount / 2);
    sellOrders.reserve(orderCount / 2);
    
    // Collect orders directly without intermediate storage - use references for faster access
    const auto& orderIds = secIt->second;
    for (const std::string& orderId : orderIds) {
        auto orderIt = m_orders.find(orderId);
        if (orderIt == m_orders.end()) {
            continue; // Order not found (shouldn't happen)
        }
        
        const Order& order = orderIt->second;
        
        if (order.side() == "Buy") {
            buyOrders.emplace_back(order.qty(), order.company());
        } else if (order.side() == "Sell") {
            sellOrders.emplace_back(order.qty(), order.company());
        }
    }
    
    if (buyOrders.empty() || sellOrders.empty()) {
        return 0;
    }
    
    // Sort orders by quantity in descending order - but only once
    std::sort(buyOrders.rbegin(), buyOrders.rend());
    std::sort(sellOrders.rbegin(), sellOrders.rend());
    
    unsigned int totalMatched = 0;
    
    // Optimized matching algorithm - modify in place to avoid copies
    for (auto& buyOrder : buyOrders) {
        if (buyOrder.first == 0) continue;
        
        for (auto& sellOrder : sellOrders) {
            if (sellOrder.first == 0) continue;
            
            // Orders from the same company cannot match
            if (buyOrder.second == sellOrder.second) {
                continue;
            }
            
            // Match as much as possible
            unsigned int matchQty = std::min(buyOrder.first, sellOrder.first);
            totalMatched += matchQty;
            
            buyOrder.first -= matchQty;
            sellOrder.first -= matchQty;
            
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
        allOrders.push_back(pair.second);
    }
    
    return allOrders;
}
