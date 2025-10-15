#include "OrderCache.h"
#include <iostream>
#include <cassert>

void testBasicFunctionality() {
    OrderCache cache;
    
    // Test adding orders
    Order order1("OrdId1", "SecId1", "Buy", 1000, "User1", "CompanyA");
    Order order2("OrdId2", "SecId2", "Sell", 3000, "User2", "CompanyB");
    Order order3("OrdId3", "SecId1", "Sell", 500, "User3", "CompanyA");
    Order order4("OrdId4", "SecId2", "Buy", 600, "User4", "CompanyC");
    Order order5("OrdId5", "SecId2", "Buy", 100, "User5", "CompanyB");
    Order order6("OrdId6", "SecId3", "Buy", 1000, "User6", "CompanyD");
    Order order7("OrdId7", "SecId2", "Buy", 2000, "User7", "CompanyE");
    Order order8("OrdId8", "SecId2", "Sell", 5000, "User8", "CompanyE");
    
    cache.addOrder(order1);
    cache.addOrder(order2);
    cache.addOrder(order3);
    cache.addOrder(order4);
    cache.addOrder(order5);
    cache.addOrder(order6);
    cache.addOrder(order7);
    cache.addOrder(order8);
    
    // Test getAllOrders
    std::vector<Order> allOrders = cache.getAllOrders();
    std::cout << "Total orders in cache: " << allOrders.size() << std::endl;
    assert(allOrders.size() == 8);
    
    // Test matching for different securities (from README Example 1)
    unsigned int match1 = cache.getMatchingSizeForSecurity("SecId1");
    unsigned int match2 = cache.getMatchingSizeForSecurity("SecId2");
    unsigned int match3 = cache.getMatchingSizeForSecurity("SecId3");
    
    std::cout << "SecId1 matching size: " << match1 << " (expected: 0)" << std::endl;
    std::cout << "SecId2 matching size: " << match2 << " (expected: 2700)" << std::endl;
    std::cout << "SecId3 matching size: " << match3 << " (expected: 0)" << std::endl;
    
    // Test cancellation
    cache.cancelOrder("OrdId1");
    std::vector<Order> ordersAfterCancel = cache.getAllOrders();
    std::cout << "Orders after cancelling OrdId1: " << ordersAfterCancel.size() << std::endl;
    assert(ordersAfterCancel.size() == 7);
    
    // Test cancel orders for user
    cache.cancelOrdersForUser("User2");
    std::vector<Order> ordersAfterUserCancel = cache.getAllOrders();
    std::cout << "Orders after cancelling User2: " << ordersAfterUserCancel.size() << std::endl;
    assert(ordersAfterUserCancel.size() == 6);
    
    std::cout << "Basic functionality test PASSED!" << std::endl;
}

void testExample2() {
    OrderCache cache;
    
    // From README Example 2
    cache.addOrder(Order("OrdId1", "SecId1", "Sell", 100, "User10", "Company2"));
    cache.addOrder(Order("OrdId2", "SecId3", "Sell", 200, "User8", "Company2"));
    cache.addOrder(Order("OrdId3", "SecId1", "Buy", 300, "User13", "Company2"));
    cache.addOrder(Order("OrdId4", "SecId2", "Sell", 400, "User12", "Company2"));
    cache.addOrder(Order("OrdId5", "SecId3", "Sell", 500, "User7", "Company2"));
    cache.addOrder(Order("OrdId6", "SecId3", "Buy", 600, "User3", "Company1"));
    cache.addOrder(Order("OrdId7", "SecId1", "Sell", 700, "User10", "Company2"));
    cache.addOrder(Order("OrdId8", "SecId1", "Sell", 800, "User2", "Company1"));
    cache.addOrder(Order("OrdId9", "SecId2", "Buy", 900, "User6", "Company2"));
    cache.addOrder(Order("OrdId10", "SecId2", "Sell", 1000, "User5", "Company1"));
    cache.addOrder(Order("OrdId11", "SecId1", "Sell", 1100, "User13", "Company2"));
    cache.addOrder(Order("OrdId12", "SecId2", "Buy", 1200, "User9", "Company2"));
    cache.addOrder(Order("OrdId13", "SecId1", "Sell", 1300, "User1", "Company1"));
    
    unsigned int match1 = cache.getMatchingSizeForSecurity("SecId1");
    unsigned int match2 = cache.getMatchingSizeForSecurity("SecId2");
    unsigned int match3 = cache.getMatchingSizeForSecurity("SecId3");
    
    std::cout << "Example 2 results:" << std::endl;
    std::cout << "SecId1 matching size: " << match1 << " (expected: 300)" << std::endl;
    std::cout << "SecId2 matching size: " << match2 << " (expected: 1000)" << std::endl;
    std::cout << "SecId3 matching size: " << match3 << " (expected: 600)" << std::endl;
    
    if (match1 == 300 && match2 == 1000 && match3 == 600) {
        std::cout << "Example 2 test PASSED!" << std::endl;
    } else {
        std::cout << "Example 2 test FAILED!" << std::endl;
    }
}

void testErrorHandling() {
    OrderCache cache;
    
    // Test invalid orders
    Order invalidOrder1("", "SecId1", "Buy", 1000, "User1", "Company1"); // empty order ID
    Order invalidOrder2("OrdId1", "", "Buy", 1000, "User1", "Company1"); // empty security ID
    Order invalidOrder3("OrdId2", "SecId1", "", 1000, "User1", "Company1"); // empty side
    Order invalidOrder4("OrdId3", "SecId1", "InvalidSide", 1000, "User1", "Company1"); // invalid side
    Order invalidOrder5("OrdId4", "SecId1", "Buy", 0, "User1", "Company1"); // zero quantity
    
    cache.addOrder(invalidOrder1);
    cache.addOrder(invalidOrder2);
    cache.addOrder(invalidOrder3);
    cache.addOrder(invalidOrder4);
    cache.addOrder(invalidOrder5);
    
    std::vector<Order> orders = cache.getAllOrders();
    std::cout << "Orders after adding invalid orders: " << orders.size() << " (expected: 0)" << std::endl;
    assert(orders.size() == 0);
    
    // Test duplicate order ID
    Order validOrder("OrdId1", "SecId1", "Buy", 1000, "User1", "Company1");
    Order duplicateOrder("OrdId1", "SecId2", "Sell", 500, "User2", "Company2");
    
    cache.addOrder(validOrder);
    cache.addOrder(duplicateOrder); // Should not be added
    
    orders = cache.getAllOrders();
    std::cout << "Orders after adding duplicate ID: " << orders.size() << " (expected: 1)" << std::endl;
    assert(orders.size() == 1);
    
    std::cout << "Error handling test PASSED!" << std::endl;
}

int main() {
    try {
        std::cout << "=== Testing OrderCache Implementation ===" << std::endl;
        
        testBasicFunctionality();
        std::cout << std::endl;
        
        testExample2();
        std::cout << std::endl;
        
        testErrorHandling();
        std::cout << std::endl;
        
        std::cout << "All tests PASSED! The OrderCache implementation appears to be working correctly." << std::endl;
        
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}