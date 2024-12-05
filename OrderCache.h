#pragma once
//ilian @ linux ubuntu 22.04
#include <string>
#include <vector>
#include <queue>
#include <unordered_map>

union paired
{
    size_t v;
    struct {
        unsigned int v;
        unsigned char d[sizeof(unsigned int)];
    } p[2];
};


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
         : m_orderId{ordId},
         m_securityId{secId},
         m_side{side},
         m_qty{qty},
         m_user{user},
         m_company{company} { }

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

 public:

  void addOrder(Order order) override;

  void cancelOrder(const std::string& orderId) override;

  void cancelOrdersForUser(const std::string& user) override;

  void cancelOrdersForSecIdWithMinimumQty(const std::string& securityId, unsigned int minQty) override;

  unsigned int getMatchingSizeForSecurity(const std::string& securityId) override;

  std::vector<Order> getAllOrders() const override;

 private:
  void clear_buy_sell(const std::string& id);

  void clear_all_0_sallers();
  void clear_all_0_buyers();


  enum class eSide {
         BUY=1,
         SELL=2,
         UNKNOWN,
         SIZE
     };

//helper struct to aggrecate qty and company to match if same
// instead of taking the company as string from Order we can hash it
// and aggregate in a cache friendly structure and compare instead of
// strcmp that can grow if company names are long , just  a hash code
// assume we can optimize or improve the hash function for our needs
// the memory will grow as 16 bytes * num of of Orders witch will
// trade a bit more space for a drastic speed improvement due to the
// constant comparison int time against string
  struct byu_sell_ctx
  {
      unsigned int qty;
      unsigned int company; //hash is size_t but will grow for small strings 32 bit hash is enough
  };




  static   std::unordered_map<std::string, eSide> gEnum;
  std::unordered_map<std::string, std::vector<byu_sell_ctx> > m_buyers;
  std::unordered_map<std::string, std::vector<byu_sell_ctx> > m_sellers;

  std::unordered_map<std::string, std::vector<paired> > m_buyers2;
  std::unordered_map<std::string, std::vector<paired> > m_sellers2;

  std::vector<Order> m_orders;
   // Todo...

};
