#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_GetNetworkParams(void);
extern void func_icmp(void);
extern void func_SendARP(void);

const struct test winetest_testlist[] =
{
    { "GetNetworkParams",     func_GetNetworkParams },
    { "icmp",                 func_icmp },
    { "SendARP",              func_SendARP },

    { 0, 0 }
};

