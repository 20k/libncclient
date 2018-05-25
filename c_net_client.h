#ifndef C_NET_CLIENT_H_INCLUDED
#define C_NET_CLIENT_H_INCLUDED

#include "c_config.h"
#include "c_shared_data.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct net_client;
    typedef net_client* c_net_client;

    //c_net_client nc_alloc();
    //void nc_destroy(c_net_client data);

    //void nc_start(c_net_client data);

    DLL_EXPORT void SHIM(nc_start, c_shared_data data, const char* host_ip, const char* host_port);

#ifdef __cplusplus
}
#endif

#endif // C_NET_CLIENT_H_INCLUDED
