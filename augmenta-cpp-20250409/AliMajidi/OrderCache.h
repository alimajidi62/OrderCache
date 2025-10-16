#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <memory>
#include <array>
#include <string_view>

class Order
{

 public:

  // do not alter signature of this constructor
  Order(
      const std::string& ordId,
      const std::string& secId,
      const std::string& side,
      const unsigned int qty,
      const std::string& user,
      const std::string& company)
      : m_orderId(ordId),
        m_securityId(secId),
        m_side(side),
        m_qty(qty),
        m_user(user),
        m_company(company) { }

  // do not alter these accessor methods
  std::string orderId() const    { return m_orderId; }
  std::string securityId() const { return m_securityId; }
  std::string side() const       { return m_side; }
  std::string user() const       { return m_user; }
  std::string company() const    { return m_company; }
  unsigned int qty() const       { return m_qty; }

 private:

  // use the below to hold the order data
  // do not remove the these member variables
  std::string m_orderId;     // unique order id
  std::string m_securityId;  // security identifier
  std::string m_side;        // side of the order, eg Buy or Sell
  unsigned int m_qty;        // qty for this order
  std::string m_user;        // user name who owns this order
  std::string m_company;     // company for user

};

// Provide an implementation for the OrderCacheInterface interface class.
// Your implementation class should hold all relevant data structures you think
// are needed.
class OrderCacheInterface
{

 public:

  // implement the 6 methods below, do not alter signatures

  // add order to the cache
  virtual void addOrder(Order order) = 0;

  // remove order with this unique order id from the cache
  virtual void cancelOrder(const std::string& orderId) = 0;

  // remove all orders in the cache for this user
  virtual void cancelOrdersForUser(const std::string& user) = 0;

  // remove all orders in the cache for this security with qty >= minQty
  virtual void cancelOrdersForSecIdWithMinimumQty(const std::string& securityId, unsigned int minQty) = 0;

  // return the total qty that can match for the security id
  virtual unsigned int getMatchingSizeForSecurity(const std::string& securityId) = 0;

  // return all orders in cache in a vector
  virtual std::vector<Order> getAllOrders() const = 0;

};

// Todo: Your implementation of the OrderCache...
class OrderCache : public OrderCacheInterface
{
 private:
   // Internal order representation optimized for performance
   struct InternalOrder {
       std::string orderId;
       std::string securityId;
       std::string side;
       unsigned int qty;
       std::string user;
       std::string company;
       
       // Cache frequently used comparisons
       bool isBuy;
       
       InternalOrder(const Order& order) 
           : orderId(order.orderId())
           , securityId(order.securityId())  
           , side(order.side())
           , qty(order.qty())
           , user(order.user())
           , company(order.company())
           , isBuy(side == "Buy")
       {}
       
       Order toOrder() const {
           return Order{orderId, securityId, side, qty, user, company};
       }
   };

   // Simplified memory management - no pool for now to avoid bugs
   struct OrderPool {
       std::vector<std::unique_ptr<InternalOrder>> orders;
       
       OrderPool() { 
           orders.reserve(1100000); // Pre-allocate for 1M+ orders
       }
       
       InternalOrder* acquire(const Order& order) {
           orders.emplace_back(std::make_unique<InternalOrder>(order));
           return orders.back().get();
       }
       
       void release(InternalOrder*) {
           // No actual release - memory stays allocated until destruction
       }
   };

 public:

  void addOrder(Order order) override;

  void cancelOrder(const std::string& orderId) override;

  void cancelOrdersForUser(const std::string& user) override;

  void cancelOrdersForSecIdWithMinimumQty(const std::string& securityId, unsigned int minQty) override;

  unsigned int getMatchingSizeForSecurity(const std::string& securityId) override;

  std::vector<Order> getAllOrders() const override;

 public:
   // Constructor to pre-allocate capacity
   OrderCache() {
       // Pre-allocate for expected load - assume ~1M orders, 1K users, 1K securities
       m_orders.reserve(1100000);
       m_ordersByUser.reserve(1200);
       m_ordersBySecId.reserve(1200);
       
       // Pre-allocate buckets to avoid rehashing
       m_orders.max_load_factor(0.7f);
       m_ordersByUser.max_load_factor(0.7f);
       m_ordersBySecId.max_load_factor(0.7f);
   }

 private:

   // Memory pool for efficient allocation
   OrderPool m_pool;
   
   // Store orders by order ID for quick lookup and cancellation
   std::unordered_map<std::string, InternalOrder*> m_orders;
   
   // Store order pointers grouped by user for efficient user-based cancellation
   std::unordered_map<std::string, std::vector<InternalOrder*>> m_ordersByUser;
   
   // Store order pointers grouped by security ID for efficient matching calculations
   std::unordered_map<std::string, std::vector<InternalOrder*>> m_ordersBySecId;
   
   // Cache for string validation to avoid repeated checks
   mutable std::unordered_set<std::string> m_validatedUsers;
   mutable std::unordered_set<std::string> m_validatedCompanies;
   mutable std::unordered_set<std::string> m_validatedSecurities;
   
   // Helper for fast validation
   inline bool isValidSide(const std::string& side) const noexcept {
       return (side.size() == 3 && side == "Buy") || (side.size() == 4 && side == "Sell");
   }
   
   inline bool isValidString(const std::string& str) const noexcept {
       return !str.empty();
   }

};
