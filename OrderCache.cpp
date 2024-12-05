// Todo: your implementation of the OrderCache...
#include "OrderCache.h"
#include <cstring>
///
/// \brief The mersene_hash class
/// a marsene hash for example or FNV or custom focused for performance or space
///
struct mersene_hash
{
    size_t operator()(const std::string& s)
    {
        size_t result = 2166136261u;
        for(auto it = s.cbegin(); it != s.cend(); it++) {
            result = 127 * result + static_cast<unsigned char>(*it);
        }
        return result;
    }
};

std::unordered_map<std::string, OrderCache::eSide> OrderCache::gEnum
{
    { "Buy", OrderCache::eSide::BUY },
        { "Sell", OrderCache::eSide::SELL },
        { "BUY", OrderCache::eSide::BUY },
        { "SELL", OrderCache::eSide::SELL },
};


void OrderCache::addOrder(Order order) {
  // Todo...
    m_orders.push_back(order);
    byu_sell_ctx ctx;
    paired ctx2;
    ctx.company = mersene_hash{}(order.company()) & 0xFFFFFFFF;
    ctx.qty = order.qty();
    ctx2.p[0].v = ctx.company;
    ctx2.p[1].v = ctx.qty;

    switch (gEnum[order.side()]) {
    case OrderCache::eSide::BUY: {
        m_buyers[order.securityId()].push_back(ctx);
        m_buyers2[order.securityId()].push_back(ctx2);
        break;
    }
    case OrderCache::eSide::SELL: {
        m_sellers[order.securityId()].push_back(ctx);
        m_sellers2[order.securityId()].push_back(ctx2);
        break;
    }
    case OrderCache::eSide::SIZE:
    case OrderCache::eSide::UNKNOWN:
    defaut:
        break;
    }
}

void OrderCache::cancelOrder(const std::string& orderId) {
  // Todo...
    if (m_orders.empty())
      return;
    for(auto it = m_orders.cbegin(); it < m_orders.cend() ; ) {
        if (orderId == it->orderId()) {
            clear_buy_sell(it->securityId()); //also clear buy/sell entities
            m_orders.erase(it);
        }
        else
            it++;
    }
}

void OrderCache::cancelOrdersForUser(const std::string& user) {
  // Todo...
    if (m_orders.empty())
        return;

    for(auto it = m_orders.cbegin(); it < m_orders.cend() ; ) {
        if (user == it->user()) {
            clear_buy_sell(it->securityId());
            m_orders.erase(it);
        }
        else
            it++;
    }
}

void OrderCache::cancelOrdersForSecIdWithMinimumQty(const std::string& securityId, unsigned int minQty) {
  // Todo...
    if (m_orders.empty())
        return;
    for(auto it = m_orders.cbegin(); it < m_orders.cend() ; ) {
        if (securityId == it->securityId() && minQty <= it->qty()) {
            clear_buy_sell(it->securityId());
            m_orders.erase(it);
        }
        else
            it++;
    }
}

unsigned int OrderCache::getMatchingSizeForSecurity(const std::string& securityId) {
  // Todo...
//NOTE
//can be optimized with prod/cons threads model
//1 thread represent buyers
//2 thread represent sellers
// the logic may remain the same both will update local ret val
// then sync the reslut of the both threads
    if (m_orders.empty())
      return 0;
    unsigned int ret = 0;
    if (m_buyers.find(securityId) != m_buyers.end() && m_sellers.find(securityId) != m_sellers.end()) {
        for(auto& s : m_sellers[securityId]) {
            if (s.qty == 0) continue;
            for(auto& b : m_buyers[securityId]) {
                if (b.qty == 0) continue;
                if (s.company != b.company) {
                    if (b.qty <= s.qty ) {
                        ret += b.qty;
                        s.qty -= b.qty;
                        b.qty = 0; //pop
                    }
                    if (s.qty <= b.qty) {
                        ret += s.qty;
                        b.qty -= s.qty;
                        s.qty = 0; //pop
                    }
                }
             }
        }
    }
    clear_buy_sell(securityId);
    return ret;
}

std::vector<Order> OrderCache::getAllOrders() const {
  // Todo...
    return m_orders;
}

//helper to clear buy/sell
void OrderCache::clear_buy_sell(const std::string& id)
{
    auto b = m_buyers.find(id);
    auto s = m_sellers.find(id);
    if (b != m_buyers.end()) m_buyers.erase(b);
    if (s != m_sellers.end()) m_sellers.erase(s);
}



