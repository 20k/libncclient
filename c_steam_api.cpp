#include "c_steam_api.h"
#include "c_shared_data.h"
#include "nc_string_interop.hpp"

#include <iostream>
#include <vector>

#include <stdint.h>
#include <steamworks_sdk_142/sdk/public/steam/steam_api.h>
#include <steamworks_sdk_142/sdk/public/steam/isteamuser.h>

#include <mutex>

struct callback_environment
{
    std::mutex lock;

    STEAM_CALLBACK( callback_environment, OnGameOverlayActivated, GameOverlayActivated_t );
    //STEAM_CALLBACK( callback_environment, OnAuthResponse, GetAuthSessionTicketResponse_t );

    void OnRequestEncryptedAppTicket( EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse, bool bIOFailure );
	CCallResult< callback_environment, EncryptedAppTicketResponse_t > m_OnRequestEncryptedAppTicketCallResult;

    //STEAM_CALLRESULT( callback_environment, OnRequestEncryptedAppTicket, EncryptedAppTicketResponse_t );

    bool overlay_open = false;

    //bool has_ticket = false;
    //bool auth_finished = false;
    bool auth_in_progress = false;
    //HAuthTicket hticket;

    bool has_encrypted_ticket = false;
    std::vector<uint8_t> encrypted_app_ticket;
};

struct steamapi
{
    bool enabled = false;
    bool overlay_open = false;

    //std::vector<uint8_t> ticket;
    //uint32_t real_ticket_size = 0;

    //uint32_t hauthticket = 0;

    uint64_t steamapicall = 0;

    steamapi();
    ~steamapi();

    void handle_auth();
    bool auth_success();
    void pump_callbacks();
    bool is_overlay_open();
    std::vector<uint8_t> get_encrypted_token();
    bool should_wait_for_encrypted_token();

private:
    callback_environment secret_environment;
};


void callback_environment::OnGameOverlayActivated( GameOverlayActivated_t* pCallback )
{
    std::lock_guard guard(lock);

    overlay_open = pCallback->m_bActive;
}

void callback_environment::OnRequestEncryptedAppTicket( EncryptedAppTicketResponse_t *pEncryptedAppTicketResponse, bool bIOFailure )
{
    std::lock_guard guard(lock);

    if ( pEncryptedAppTicketResponse->m_eResult == k_EResultOK )
	{
		//uint8 rgubTicket[1024];
		uint32 cubTicket;
        encrypted_app_ticket.resize(1024);
		SteamUser()->GetEncryptedAppTicket( &encrypted_app_ticket[0], 1024, &cubTicket );

		encrypted_app_ticket.resize(cubTicket);

		std::cout << "successfully got encrypted auth ticket of length " << encrypted_app_ticket.size() << std::endl;

		has_encrypted_ticket = true;
		auth_in_progress = false;

		return;
	}
    else if ( pEncryptedAppTicketResponse->m_eResult == k_EResultLimitExceeded )
	{
		printf("Hit rate limit (1/min)\n");
	}
	else if ( pEncryptedAppTicketResponse->m_eResult == k_EResultDuplicateRequest )
	{
		printf("Already a pending auth request\n");
	}
	else if ( pEncryptedAppTicketResponse->m_eResult == k_EResultNoConnection )
	{
		printf("Steam not active\n");
	}

    std::cout << "Failed to get auth " << pEncryptedAppTicketResponse->m_eResult << std::endl;
    auth_in_progress = false;
}


/*void callback_environment::OnAuthResponse( GetAuthSessionTicketResponse_t* pResponse )
{
    if(pResponse->m_eResult == k_EResultOK)
    {
        hticket = pResponse->m_hAuthTicket;
        has_ticket = true;
    }

    auth_finished = true;
}*/

steamapi::steamapi()
{
    enabled = SteamAPI_Init();

    std::cout << "steamapi support is " << enabled << std::endl;
}

void steamapi::pump_callbacks()
{
    if(!enabled)
        return;

    SteamAPI_RunCallbacks();
}

void steamapi::handle_auth()
{
    if(!enabled)
        return;

    SteamAPICall_t scall = SteamUser()->RequestEncryptedAppTicket(nullptr, 0);
    //hauthticket = SteamUser()->GetAuthSessionTicket(&ticket[0], ticket.size(), &real_ticket_size);

    std::lock_guard guard(secret_environment.lock);
    secret_environment.auth_in_progress = true;
    secret_environment.m_OnRequestEncryptedAppTicketCallResult.Set( scall, &secret_environment, &callback_environment::OnRequestEncryptedAppTicket );
}

bool steamapi::auth_success()
{
    std::lock_guard guard(secret_environment.lock);
    printf("Inside function\n");

    if(secret_environment.auth_in_progress)
        return false;

    if(!secret_environment.has_encrypted_ticket)
        return false;

    return true;
}

steamapi::~steamapi()
{
    if(enabled)
        SteamAPI_Shutdown();
}

bool steamapi::is_overlay_open()
{
    std::lock_guard guard(secret_environment.lock);

    return secret_environment.overlay_open;
}

std::vector<uint8_t> steamapi::get_encrypted_token()
{
    std::lock_guard guard(secret_environment.lock);

    return secret_environment.encrypted_app_ticket;
}

bool steamapi::should_wait_for_encrypted_token()
{
    std::lock_guard guard(secret_environment.lock);

    return secret_environment.auth_in_progress;
}

__declspec(dllexport) c_steam_api steam_api_alloc()
{
    return new steamapi();
}

__declspec(dllexport) void steam_api_destroy(c_steam_api csapi)
{
    if(csapi == nullptr)
        return;

    delete csapi;
}

__declspec(dllexport) void steam_api_request_encrypted_token(c_steam_api csapi)
{
    csapi->handle_auth();
}

__declspec(dllexport) int steam_api_should_wait_for_encrypted_token(c_steam_api csapi)
{
    return csapi->should_wait_for_encrypted_token();
}

__declspec(dllexport) int steam_api_has_encrypted_token(c_steam_api csapi)
{
    return csapi->auth_success();
}

__declspec(dllexport) sized_string steam_api_get_encrypted_token(c_steam_api csapi)
{
    std::string str;

    auto data = csapi->get_encrypted_token();

    for(auto& i : data)
    {
        str.push_back(i);
    }

    return make_copy(str);
}

__declspec(dllexport) void steam_api_pump_events(c_steam_api csapi)
{
    csapi->pump_callbacks();
}

__declspec(dllexport) int steam_api_overlay_is_open(c_steam_api csapi)
{
    return csapi->is_overlay_open();
}
