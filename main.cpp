#include "OrderCache.h"
#include <cstdio>
//local test file copied for debugging from gtest cpp just a dummy
int main(void)
{


    OrderCache cache;
    cache.addOrder(Order{"OrdId1", "SecId1", "Buy", 1000, "User1", "CompanyA"});
    cache.addOrder(Order{"OrdId2", "SecId2", "Sell", 3000, "User2", "CompanyB"});
    cache.addOrder(Order{"OrdId3", "SecId1", "Sell", 500, "User3", "CompanyA"});
    cache.addOrder(Order{"OrdId4", "SecId2", "Buy", 600, "User4", "CompanyC"});
    cache.addOrder(Order{"OrdId5", "SecId2", "Buy", 100, "User5", "CompanyB"});
    cache.addOrder(Order{"OrdId6", "SecId3", "Buy", 1000, "User6", "CompanyD"});
    cache.addOrder(Order{"OrdId7", "SecId2", "Buy", 2000, "User7", "CompanyE"});
    cache.addOrder(Order{"OrdId8", "SecId2", "Sell", 5000, "User8", "CompanyE"});

    unsigned int matchingSize = cache.getMatchingSizeForSecurity("SecId1");
    printf("[%d]\r\n", matchingSize);
    matchingSize = cache.getMatchingSizeForSecurity("SecId2");
    printf("[%d]\r\n", matchingSize);

    matchingSize = cache.getMatchingSizeForSecurity("SecId3");
    printf("[%d]\r\n", matchingSize);

    return 0;
}
