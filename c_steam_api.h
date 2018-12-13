#ifndef C_STEAM_API_H_INCLUDED
#define C_STEAM_API_H_INCLUDED

#include "c_shared_data.h"

#ifdef __cplusplus
extern "C"
{
#endif

    struct steamapi;

    typedef steamapi* c_steam_api;

    __declspec(dllexport) c_steam_api steam_api_alloc();
    __declspec(dllexport) void steam_api_destroy(c_steam_api csapi);

    __declspec(dllexport) void steam_api_request_encrypted_token(c_steam_api csapi, sized_view user_data);
    __declspec(dllexport) int steam_api_has_encrypted_token(c_steam_api csapi);
    __declspec(dllexport) int steam_api_should_wait_for_encrypted_token(c_steam_api csapi);
    __declspec(dllexport) sized_string steam_api_get_encrypted_token(c_steam_api csapi);

    __declspec(dllexport) void steam_api_pump_events(c_steam_api csapi);
    __declspec(dllexport) int steam_api_overlay_is_open(c_steam_api csapi);

    __declspec(dllexport) int steam_api_enabled(c_steam_api csapi);

#ifdef __cplusplus
}
#endif

#endif // C_STEAM_API_H_INCLUDED
