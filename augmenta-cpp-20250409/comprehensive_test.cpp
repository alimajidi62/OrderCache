#include "OrderCache.h"
#include <iostream>
#include <cassert>
#include <chrono>
#include <random>
#include <algorithm>

class TestRunner {
private:
    int passed_tests = 0;
    int total_tests = 0;
    
public:
    void test(const std::string& test_name, bool condition) {
        total_tests++;
        if (condition) {
            passed_tests++;
            std::cout << "[PASS] " << test_name << std::endl;
        } else {
            std::cout << "[FAIL] " << test_name << std::endl;
        }
    }
    
    void summary() {
        std::cout << "\n=== Test Summary ===" << std::endl;
        std::cout << "Passed: " << passed_tests << "/" << total_tests << std::endl;
        if (passed_tests == total_tests) {
            std::cout << "All tests PASSED!" << std::endl;
        } else {
            std::cout << "Some tests FAILED!" << std::endl;
        }
    }
    
    bool all_passed() const {
        return passed_tests == total_tests;
    }
};

void testExample1(TestRunner& tr) {
    std::cout << "\n=== Testing README Example 1 ===" << std::endl;
    OrderCache cache;
    
    cache.addOrder(Order("OrdId1", "SecId1", "Buy", 1000, "User1", "CompanyA"));
    cache.addOrder(Order("OrdId2", "SecId2", "Sell", 3000, "User2", "CompanyB"));
    cache.addOrder(Order("OrdId3", "SecId1", "Sell", 500, "User3", "CompanyA"));
    cache.addOrder(Order("OrdId4", "SecId2", "Buy", 600, "User4", "CompanyC"));
    cache.addOrder(Order("OrdId5", "SecId2", "Buy", 100, "User5", "CompanyB"));
    cache.addOrder(Order("OrdId6", "SecId3", "Buy", 1000, "User6", "CompanyD"));
    cache.addOrder(Order("OrdId7", "SecId2", "Buy", 2000, "User7", "CompanyE"));
    cache.addOrder(Order("OrdId8", "SecId2", "Sell", 5000, "User8", "CompanyE"));
    
    tr.test("Example 1: SecId1 matching = 0", cache.getMatchingSizeForSecurity("SecId1") == 0);
    tr.test("Example 1: SecId2 matching = 2700", cache.getMatchingSizeForSecurity("SecId2") == 2700);
    tr.test("Example 1: SecId3 matching = 0", cache.getMatchingSizeForSecurity("SecId3") == 0);
}

void testExample2(TestRunner& tr) {
    std::cout << "\n=== Testing README Example 2 ===" << std::endl;
    OrderCache cache;
    
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
    
    tr.test("Example 2: SecId1 matching = 300", cache.getMatchingSizeForSecurity("SecId1") == 300);
    tr.test("Example 2: SecId2 matching = 1000", cache.getMatchingSizeForSecurity("SecId2") == 1000);
    tr.test("Example 2: SecId3 matching = 600", cache.getMatchingSizeForSecurity("SecId3") == 600);
}

void testExample3(TestRunner& tr) {
    std::cout << "\n=== Testing README Example 3 ===" << std::endl;
    OrderCache cache;
    
    cache.addOrder(Order("OrdId1", "SecId3", "Sell", 100, "User1", "Company1"));
    cache.addOrder(Order("OrdId2", "SecId3", "Sell", 200, "User3", "Company2"));
    cache.addOrder(Order("OrdId3", "SecId1", "Buy", 300, "User2", "Company1"));
    cache.addOrder(Order("OrdId4", "SecId3", "Sell", 400, "User5", "Company2"));
    cache.addOrder(Order("OrdId5", "SecId2", "Sell", 500, "User2", "Company1"));
    cache.addOrder(Order("OrdId6", "SecId2", "Buy", 600, "User3", "Company2"));
    cache.addOrder(Order("OrdId7", "SecId2", "Sell", 700, "User1", "Company1"));
    cache.addOrder(Order("OrdId8", "SecId1", "Sell", 800, "User2", "Company1"));
    cache.addOrder(Order("OrdId9", "SecId1", "Buy", 900, "User5", "Company2"));
    cache.addOrder(Order("OrdId10", "SecId1", "Sell", 1000, "User1", "Company1"));
    cache.addOrder(Order("OrdId11", "SecId2", "Sell", 1100, "User6", "Company2"));
    
    tr.test("Example 3: SecId1 matching = 900", cache.getMatchingSizeForSecurity("SecId1") == 900);
    tr.test("Example 3: SecId2 matching = 600", cache.getMatchingSizeForSecurity("SecId2") == 600);
    tr.test("Example 3: SecId3 matching = 0", cache.getMatchingSizeForSecurity("SecId3") == 0);
}

void testBasicOperations(TestRunner& tr) {
    std::cout << "\n=== Testing Basic Operations ===" << std::endl;
    OrderCache cache;
    
    // Test adding orders
    cache.addOrder(Order("Order1", "SEC1", "Buy", 100, "User1", "CompanyA"));
    cache.addOrder(Order("Order2", "SEC1", "Sell", 200, "User2", "CompanyB"));
    
    auto orders = cache.getAllOrders();
    tr.test("Add orders: 2 orders in cache", orders.size() == 2);
    
    // Test cancel order
    cache.cancelOrder("Order1");
    orders = cache.getAllOrders();
    tr.test("Cancel order: 1 order remaining", orders.size() == 1);
    
    // Test cancel non-existent order (should not crash)
    cache.cancelOrder("NonExistent");
    orders = cache.getAllOrders();
    tr.test("Cancel non-existent order: still 1 order", orders.size() == 1);
    
    // Test cancel orders for user
    cache.addOrder(Order("Order3", "SEC2", "Buy", 300, "User2", "CompanyC"));
    cache.addOrder(Order("Order4", "SEC2", "Sell", 400, "User3", "CompanyC"));
    
    cache.cancelOrdersForUser("User2");
    orders = cache.getAllOrders();
    tr.test("Cancel orders for User2: 1 order remaining", orders.size() == 1);
    
    // Test cancel orders for security with minimum qty
    cache.addOrder(Order("Order5", "SEC3", "Buy", 50, "User4", "CompanyD"));
    cache.addOrder(Order("Order6", "SEC3", "Buy", 150, "User5", "CompanyE"));
    
    cache.cancelOrdersForSecIdWithMinimumQty("SEC3", 100);
    orders = cache.getAllOrders();
    tr.test("Cancel orders with qty >= 100: Order5 should remain", orders.size() == 2);
    
    // Verify the remaining order is the one with qty < 100
    bool found_low_qty = false;
    for (const auto& order : orders) {
        if (order.orderId() == "Order5" && order.qty() == 50) {
            found_low_qty = true;
            break;
        }
    }
    tr.test("Low quantity order remains after minimum qty cancellation", found_low_qty);
}

void testErrorHandling(TestRunner& tr) {
    std::cout << "\n=== Testing Error Handling ===" << std::endl;
    OrderCache cache;
    
    // Test invalid orders (should be rejected)
    cache.addOrder(Order("", "SEC1", "Buy", 100, "User1", "Company1")); // empty ID
    cache.addOrder(Order("Order1", "", "Buy", 100, "User1", "Company1")); // empty security
    cache.addOrder(Order("Order2", "SEC1", "", 100, "User1", "Company1")); // empty side
    cache.addOrder(Order("Order3", "SEC1", "InvalidSide", 100, "User1", "Company1")); // invalid side
    cache.addOrder(Order("Order4", "SEC1", "Buy", 0, "User1", "Company1")); // zero qty
    cache.addOrder(Order("Order5", "SEC1", "Buy", 100, "", "Company1")); // empty user
    cache.addOrder(Order("Order6", "SEC1", "Buy", 100, "User1", "")); // empty company
    
    auto orders = cache.getAllOrders();
    tr.test("Invalid orders rejected", orders.size() == 0);
    
    // Test duplicate order ID
    cache.addOrder(Order("ValidOrder", "SEC1", "Buy", 100, "User1", "Company1"));
    cache.addOrder(Order("ValidOrder", "SEC2", "Sell", 200, "User2", "Company2")); // same ID
    
    orders = cache.getAllOrders();
    tr.test("Duplicate order ID rejected", orders.size() == 1);
    
    // Test empty security ID in matching
    tr.test("Empty security ID returns 0 match", cache.getMatchingSizeForSecurity("") == 0);
    
    // Test non-existent security ID
    tr.test("Non-existent security ID returns 0 match", cache.getMatchingSizeForSecurity("NonExistent") == 0);
}

void testPerformance(TestRunner& tr) {
    std::cout << "\n=== Testing Performance ===" << std::endl;
    
    OrderCache cache;
    const int numOrders = 10000;
    
    // Generate test data
    std::vector<std::string> securities;
    std::vector<std::string> companies;
    std::vector<std::string> users;
    
    for (int i = 0; i < 100; i++) {
        securities.push_back("SEC" + std::to_string(i));
        companies.push_back("COMP" + std::to_string(i));
        users.push_back("USER" + std::to_string(i));
    }
    
    std::mt19937 gen(42); // Fixed seed for reproducibility
    std::uniform_int_distribution<> secDist(0, securities.size() - 1);
    std::uniform_int_distribution<> compDist(0, companies.size() - 1);
    std::uniform_int_distribution<> userDist(0, users.size() - 1);
    std::uniform_int_distribution<> sideDist(0, 1);
    std::uniform_int_distribution<> qtyDist(100, 10000);
    
    // Test adding orders
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < numOrders; i++) {
        std::string side = (sideDist(gen) == 0) ? "Buy" : "Sell";
        cache.addOrder(Order(
            "ORDER" + std::to_string(i),
            securities[secDist(gen)],
            side,
            qtyDist(gen),
            users[userDist(gen)],
            companies[compDist(gen)]
        ));
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Added " << numOrders << " orders in " << duration.count() << "ms" << std::endl;
    tr.test("Performance: Add 10K orders in reasonable time", duration.count() < 1000); // Less than 1 second
    
    // Test matching performance
    start = std::chrono::high_resolution_clock::now();
    
    unsigned int totalMatched = 0;
    for (const auto& sec : securities) {
        totalMatched += cache.getMatchingSizeForSecurity(sec);
    }
    
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Calculated matching for 100 securities in " << duration.count() << "ms" << std::endl;
    std::cout << "Total matched quantity: " << totalMatched << std::endl;
    tr.test("Performance: Matching calculation in reasonable time", duration.count() < 1000); // Less than 1 second
}

void testEdgeCases(TestRunner& tr) {
    std::cout << "\n=== Testing Edge Cases ===" << std::endl;
    
    OrderCache cache;
    
    // Test matching with same company (should not match)
    cache.addOrder(Order("Buy1", "SEC1", "Buy", 1000, "User1", "CompanyA"));
    cache.addOrder(Order("Sell1", "SEC1", "Sell", 500, "User2", "CompanyA"));
    
    tr.test("Same company orders don't match", cache.getMatchingSizeForSecurity("SEC1") == 0);
    
    // Test partial matching
    cache.addOrder(Order("Sell2", "SEC1", "Sell", 300, "User3", "CompanyB"));
    
    tr.test("Partial matching works", cache.getMatchingSizeForSecurity("SEC1") == 300);
    
    // Test multiple small sells vs one big buy
    OrderCache cache2;
    cache2.addOrder(Order("Buy1", "SEC1", "Buy", 1000, "User1", "CompanyA"));
    cache2.addOrder(Order("Sell1", "SEC1", "Sell", 100, "User2", "CompanyB"));
    cache2.addOrder(Order("Sell2", "SEC1", "Sell", 200, "User3", "CompanyC"));
    cache2.addOrder(Order("Sell3", "SEC1", "Sell", 300, "User4", "CompanyD"));
    
    tr.test("Multiple small orders match big order", cache2.getMatchingSizeForSecurity("SEC1") == 600);
    
    // Test case sensitivity
    OrderCache cache3;
    cache3.addOrder(Order("Order1", "SEC1", "buy", 100, "User1", "CompanyA")); // lowercase
    cache3.addOrder(Order("Order2", "SEC1", "Sell", 100, "User2", "CompanyB"));
    
    // Should not match because "buy" != "Buy"
    tr.test("Case sensitivity in side field", cache3.getMatchingSizeForSecurity("SEC1") == 0);
}

int main() {
    TestRunner tr;
    
    std::cout << "=== OrderCache Comprehensive Test Suite ===" << std::endl;
    std::cout << "Testing OrderCache implementation against README specifications" << std::endl;
    
    testBasicOperations(tr);
    testExample1(tr);
    testExample2(tr);
    testExample3(tr);
    testErrorHandling(tr);
    testEdgeCases(tr);
    testPerformance(tr);
    
    tr.summary();
    
    // Return 0 if all tests passed, 1 if any failed
    return tr.all_passed() ? 0 : 1;
}