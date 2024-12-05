// Todo: your implementation of the OrderCache...
#include "OrderCache.h"
#include <cstring>

#define CL_USE_DEPRECATED_OPENCL_1_2_APIS

#include <CL/opencl.h>

static const char* paralellbuysell= R"(
      struct byu_sell_ctx
      {
          unsigned int qty;
          unsigned int company;
      };

    __kernel void paralellbuysell(__global struct byu_sell_ctx *b , __global struct byu_sell_ctx *s, __global unsigned int* out)
    {
        int i = get_global_id(0);
        int j = get_global_id(1);
        unsigned int ret = 0;
        if (s[j].company != b[i].company) {
            if (b[j].qty <= s[i].qty ) {
                ret += b[j].qty;
                s[i].qty -= b[j].qty;
                b[j].qty = 0; //pop
            }
            if (s[i].qty <= b[j].qty) {
                ret += s[i].qty;
                b[j].qty -= s[i].qty;
                s[i].qty = 0; //pop
            }
        }
        *out += ret;
    }
)";

static const char* testcleanall= R"(
      struct byu_sell_ctx
      {
          unsigned int qty;
          unsigned int company;
      };

    __kernel void testcleanall(__global struct byu_sell_ctx *b , __global struct byu_sell_ctx *s, __global unsigned int* ret)
    {
        int i = get_global_id(0);
        int j = get_global_id(1);

        if (b[j].company != s[i].company) {
            if (b[j].qty <= s[i].qty ) {
                *ret += b[j].qty;
                s[i].qty -= b[j].qty;
                b[j].qty = 0; //pop
            }
            if (s[i].qty <= b[j].qty) {
                *ret += s[i].qty;
                b[j].qty -= s[i].qty;
                s[i].qty = 0; //pop
            }
        }
        SKIP: ;;
    }
)";




struct gpu_kernel
{
    gpu_kernel()
    {
        memset(&m_kernctx, 0 , sizeof (m_kernctx));
    }

    ~gpu_kernel()
    {
        clReleaseKernel(m_kernctx.kernel);
        clReleaseCommandQueue(m_kernctx.command_queue);
        clReleaseProgram(m_kernctx.program);
        clReleaseContext(m_kernctx.context);
    }

    template<typename T>
    void dispatch(T* byuers, size_t bs, T* sellers, size_t ss, unsigned int* out)
    {

        size_t dim = 2;
        size_t global_offset[] = {0,0};
        size_t global_size[] = {bs, ss};
        size_t wsize[] = {};
        size_t globwsize = bs * ss *sizeof(T) * sizeof(unsigned int);
        cl_mem bufferbuy, buffersell, bufferret;
        size_t log_size;
        char* program_log = 0;
        clGetPlatformIDs(1, &m_kernctx.platform, &m_kernctx.platforms);
        clGetDeviceIDs(m_kernctx.platform, CL_DEVICE_TYPE_GPU, 1, &m_kernctx.device, &m_kernctx.numdevs);
        m_kernctx.context = clCreateContext(NULL, 1, &m_kernctx.device, NULL, NULL, NULL);
        m_kernctx.command_queue =     clCreateCommandQueue(m_kernctx.context, m_kernctx.device, 0, NULL);
        bufferbuy =   clCreateBuffer(m_kernctx.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR ,
                                   bs * sizeof(size_t) , byuers, NULL);

        buffersell = clCreateBuffer(m_kernctx.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR ,
                                    sizeof(size_t) * ss , sellers , NULL);

        bufferret = clCreateBuffer(m_kernctx.context, CL_MEM_READ_WRITE | CL_MEM_COPY_HOST_PTR,
                                    sizeof(unsigned int)  , out , NULL);



        m_kernctx.program = clCreateProgramWithSource(m_kernctx.context, 1, &testcleanall, NULL, &m_kernctx.compilerr);
        if (m_kernctx.compilerr) {
            return;
        }

        cl_int res = clBuildProgram(m_kernctx.program, 1, &m_kernctx.device, "", NULL, NULL);
        if (res < 0) {
            clGetProgramBuildInfo(m_kernctx.program, m_kernctx.device, CL_PROGRAM_BUILD_LOG,
                                  0, NULL, &log_size);
            program_log = (char*) calloc(log_size+1, sizeof(char));
            clGetProgramBuildInfo(m_kernctx.program, m_kernctx.device, CL_PROGRAM_BUILD_LOG,
                                  log_size+1, program_log, NULL);
            printf("%s\n", program_log);
            free(program_log);
            return;
        }
        cl_int callres = 0;
        m_kernctx.kernel = clCreateKernel(m_kernctx.program, "testcleanall", &callres);
        if (callres)
            return;
        callres |= clSetKernelArg(m_kernctx.kernel, 0, sizeof(cl_mem), &bufferbuy);
        callres |= clSetKernelArg(m_kernctx.kernel, 1, sizeof(cl_mem), &buffersell);
        callres |= clSetKernelArg(m_kernctx.kernel, 2, sizeof(cl_mem), &bufferret);
        if (callres) {
            return;
        }
        //    callres |= clGetKernelWorkGroupInfo(m_kernctx.kernel, m_kernctx.device,  CL_KERNEL_WORK_GROUP_SIZE, sizeof(size_t), &wsize, NULL);
        callres |= clEnqueueNDRangeKernel(m_kernctx.command_queue,
                                          m_kernctx.kernel,
                                          dim,
                                          global_offset,
                                          global_size,
                                          0,
                                          0,
                                          NULL,
                                          NULL);

        clFlush(m_kernctx.command_queue);
        clFinish(m_kernctx.command_queue);

       // callres |= clEnqueueWriteBuffer(m_kernctx.command_queue, bufferbuy, CL_TRUE, 0,  bs * sizeof(paired), byuers, 0, NULL, NULL);
       // callres |= clEnqueueWriteBuffer(m_kernctx.command_queue, buffersell, CL_TRUE, 0,  ss * sizeof(paired), sellers, 0, NULL, NULL);
        callres |= clEnqueueReadBuffer(m_kernctx.command_queue, bufferret, CL_TRUE, 0,  sizeof(unsigned int), out, 0, NULL, NULL);
        //    callres |= clEnqueueWriteBuffer(m_kernctx.command_queue, bufferkern, CL_TRUE, 0,  sizeof(float)*9, kern, 0, NULL, NULL);
        if (callres) {
            printf("failed to enqueue buffers\r\n");
            clReleaseMemObject(buffersell);
            clReleaseMemObject(bufferbuy);
            clReleaseMemObject(bufferret);
            return;
        }

        clReleaseMemObject(buffersell);
        clReleaseMemObject(bufferbuy);
        clReleaseMemObject(bufferret);

    }


private:
    struct {
        cl_command_queue command_queue ;
        cl_context context ;
        cl_device_id device ;
        size_t global_work_size ;
        cl_kernel kernel ;
        cl_platform_id platform ;
        cl_program program ;
        cl_int compilerr ;
        cl_uint platforms;
        cl_uint numdevs ;
    } m_kernctx;

};


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

 union hlp
{
    size_t v;
    struct {
        unsigned int v;
    } p[2];
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
#if 0
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
#else
   static  gpu_kernel k;
#if 1
    if (m_buyers.find(securityId) != m_buyers.end() && m_sellers.find(securityId) != m_sellers.end()) {
     k.dispatch(m_buyers[securityId].data(), m_buyers[securityId].size() ,
                   m_sellers[securityId].data() , m_sellers[securityId].size(),
                   &ret);
    }
#else
#define BH m_buyers2[securityId]
#define SH m_sellers2[securityId]

   if (m_buyers2.find(securityId) != m_buyers2.end() && m_sellers2.find(securityId) != m_sellers2.end())
       for(int i=0; i < SH.size(); i++) {
           if (SH[i].p[1].v == 0) continue;
           for(int j=0; j < BH.size(); j++) {
               if (BH[j].p[1].v == 0) continue;
               if (BH[j].p[0].v != SH[i].p[0].v)
               {
                   if (BH[j].p[1].v <= SH[i].p[1].v ) {
                       ret += BH[j].p[1].v;
                       SH[i].p[1].v -= BH[j].p[1].v;
                       BH[j].p[1].v = 0;
                   }
                   if (SH[i].p[1].v <= BH[j].p[1].v ) {
                       ret += SH[i].p[1].v;
                       BH[j].p[1].v -= SH[i].p[1].v;
                       SH[i].p[1].v = 0;
                   }
               }
           }
       }

#endif
#endif
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



