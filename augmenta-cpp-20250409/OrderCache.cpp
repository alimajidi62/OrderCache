// Implementation of the OrderCache class
#include "OrderCache.h"
#include <algorithm>

void OrderCache::addOrder(Order order) {
    // Input validation
    if (order.orderId().empty() || order.securityId().empty() || 
        order.side().empty() || order.user().empty() || order.company().empty() ||
        order.qty() == 0) {
        return; // Skip invalid orders
    }
    
    // Validate order side
    if (order.side() != "Buy" && order.side() != "Sell") {
        return; // Skip invalid side
    }
    
    const std::string& orderId = order.orderId();
    
    // Don't add duplicate order IDs
    if (m_orders.find(orderId) != m_orders.end()) {
        return;
    }
    
    // Store the order
    m_orders[orderId] = order;
    
    // Index by user
    m_ordersByUser[order.user()].insert(orderId);
    
    // Index by security ID
    m_ordersBySecId[order.securityId()].push_back(orderId);
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
    
    // Collect all buy and sell orders
    std::vector<std::pair<unsigned int, std::string>> buyOrders;  // qty, company
    std::vector<std::pair<unsigned int, std::string>> sellOrders; // qty, company
    
    for (const std::string& orderId : secIt->second) {
        auto orderIt = m_orders.find(orderId);
        if (orderIt == m_orders.end()) {
            continue; // Order not found (shouldn't happen)
        }
        
        const Order& order = orderIt->second;
        
        if (order.side() == "Buy") {
            buyOrders.push_back({order.qty(), order.company()});
        } else if (order.side() == "Sell") {
            sellOrders.push_back({order.qty(), order.company()});
        }
    }
    
    // Sort orders by quantity in descending order for greedy matching
    std::sort(buyOrders.rbegin(), buyOrders.rend());
    std::sort(sellOrders.rbegin(), sellOrders.rend());
    
    // Create working copies of the orders for matching
    std::vector<std::pair<unsigned int, std::string>> buyOrdersCopy = buyOrders;
    std::vector<std::pair<unsigned int, std::string>> sellOrdersCopy = sellOrders;
    
    unsigned int totalMatched = 0;
    
    // Greedy matching algorithm
    for (auto& buyOrder : buyOrdersCopy) {
        if (buyOrder.first == 0) continue;
        
        for (auto& sellOrder : sellOrdersCopy) {
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
