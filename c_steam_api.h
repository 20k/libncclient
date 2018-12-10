#ifndef C_STEAM_API_H_INCLUDED
#define C_STEAM_API_H_INCLUDED

#ifdef __cplusplus
extern "C"
{
#endif

    struct steamapi;

    typedef steamapi* c_steam_api;

    __declspec(dllexport) c_steam_api steam_api_alloc();
    __declspec(dllexport) void steam_api_destroy(c_steam_api csapi);

    __declspec(dllexport) void steam_api_request_encrypted_token(c_steam_api csapi);
    __declspec(dllexport) int steam_api_has_encrypted_token(c_steam_api csapi);

#ifdef __cplusplus
}
#endif

#endif // C_STEAM_API_H_INCLUDED
