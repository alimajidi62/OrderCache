// Implementation of the OrderCache class
#include "OrderCache.h"
#include <algorithm>
#include <stdexcept>

void OrderCache::addOrder(Order order) {
    // Cache string references to avoid repeated method calls
    const std::string& orderId = order.orderId();
    const std::string& securityId = order.securityId();
    const std::string& side = order.side();
    const std::string& user = order.user();
    const std::string& company = order.company();
    
    // Optimized validation - check most likely failures first
    if (orderId.empty() || m_orders.find(orderId) != m_orders.end()) {
        return; // Skip invalid or duplicate order IDs (most common failure)
    }
    
    // Quick validation without repeated string comparisons
    if (securityId.empty() || side.empty() || user.empty() || company.empty() || order.qty() == 0) {
        return; // Skip invalid orders
    }
    
    // Optimized side validation using first character check
    if (side[0] != 'B' && side[0] != 'S') {
        return; // Quick rejection for invalid side
    }
    if (side.size() < 3 || (side != "Buy" && side != "Sell")) {
        return; // Full validation only if needed
    }
    
    // Use move semantics where possible and optimize insertions
    auto orderResult = m_orders.emplace(orderId, std::move(order));
    if (!orderResult.second) {
        return; // Insertion failed (shouldn't happen due to earlier check)
    }
    
    // Index by user - use emplace to avoid unnecessary construction
    m_ordersByUser[user].emplace(orderId);
    
    // Index by security ID - reserve space if this is a new security
    auto& secOrders = m_ordersBySecId[securityId];
    secOrders.push_back(orderId);
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
