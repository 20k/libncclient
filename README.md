This is the client lib for net_code, the official client uses everything here and so its guaranteed to work (as well as the official client does). You need to place all the dlls which are dependencies into the same directory as wherever ncclient.dll goes. The easiest way to get the 64bit dlls is from the game client, simply copy everything across, or use something like dependency walker to find them manually. If you try to FFI and get an error like "module not found", this is probably the issue

The websocket TLS/SSL port is 6770, and the unencrypted port is 6760. IP is available on request

# Concepts

There are two kinds of strings. Owning and non owning - owning strings are a sized_string, and non owning strings are a sized_view. You are expected to manage the lifetime of a sized string by using free_sized_string on sized strings returned by the api

The API is guaranteed not to take ownership or otherwise do anything unexpected with a sized_view

## Socket

The client totally hides away the implementation of the underlying socket. You do not need to deal with websockets whatsoever here

## Queue

The main way of interacting with the server is through the c_shared_data class. This represents a read and write queue with the server. It also contains auth and user data, which much be set manually by the application

The queue (and application generally) should be set up as following

```
///allocate queue
c_shared_data shared = sd_alloc();

if(file_exists("key.key"))
{
	std::string fauth = read_file_bin("key.key");

	sd_set_auth(shared, make_view(fauth));
}

///connect queue to server
nc_start(shared, HOST_IP, HOST_PORT);

///queue is now ready to use, and the client is connected to the server
...rest of application...

///tries to gracefully shut down the queue
sd_set_termination(shared);
///due to race conditions and bugs, it is currently not recommended to sd_destroy the queue. This will change in the future
///exit
```

In typical queue usage, reads arrive from the server, and writes go to the server, and it operates on a first-in first-out principle

You *must* check that the queue has data in it before attempting to read from the queue, like following

```
///in the main application loop

if(sd_has_front_read(shared))
{
	sized_string from_server = sd_get_front_read(shared);
	
	///do something with the string from_server.str of length from_server.num
	
	free_sized_string(from_server);
}
```

To write data to the queue to be sent to the server, you do

```
sized_view my_view = {some_string, length_of_some_string};

sd_add_back_write(shared, my_view);
```

Or more simply

```
sd_add_back_write(shared, {some_string, length_of_some_string});
```

### User and auth

Currently, the queue deals with containing user and auth information, as these are partially internally managed. Auth data should currently always be stored in a file named key.key, which contains raw binary

To set the currently running user, use sd_set_user(shared, {user_name, length_of_user_name}). To set auth data use sd_set_auth(shared, {auth_data, length_of_auth_data}). It is important to remember that auth data (and other strings) may contain nulls, you must not assume they are null terminated

You only need to set auth data once on startup, if it exists, as shown in setup. It is otherwise managed automatically

## API

One major feature of libncclient is the ability to hide the entire server api from you, which is accessible from c_server_api. Usage of this is optional, but the underlying communication format is liable to change

### Creating Requests

There are 5 kinds of server request:

1. Poll
2. Generic
3. Autocomplete
4. Chat api
5. Realtime Script Updates

```js
```

1. Polling should be performed every x seconds (preferably 1s) with sa_do_poll_server(shared);

2. Generic server requests can be achieved as such:

```
const char* msg = "#scripts.core()";

sized_string some_request = sa_make_generic_server_command({msg, strlen(msg)});

sd_add_back_write(shared, {some_request.str, some_request.num});

free_sized_string(some_request);
```

3. Autocomplete requests can be achieved as such:

```
const char* scriptname = "scripts.core";

sized_string some_request = sa_make_autocomplete_request({scriptname, strlen(scriptname)});

sd_add_back_write(shared, {some_request.str, some_request.num});

free_sized_string(some_request);
```

Or additionally sa_do_autocomplete_request(shared, {scriptname, strlen(scriptname)}) can be used

4. The chat api request is the same as a generic command except it suppresses output. Chat api requests can be achieved as such:

```
const char* chat_message = "hello";
const char* chat_channel = "0000";

sized_string some_request = sa_make_chat_command({chat_channel, strlen(chat_channel)}, {chat_message, strlen(chat_message)});

sd_add_back_write(shared, {some_request.str, some_request.num});

free_sized_string(some_request);
```

5. There are 3 kinds of realtime script updates you can send. `sa_do_send_keystrokes_to_script`, `sa_do_update_mouse_to_script`, `sa_do_send_script_info`

The mouse update is intentionally undocumented as the API is temporary and will change

To send keystrokes to a script, the following example would work

```
const char* keys_input_1 = "up";
const char* keys_input_2 = "space";
const char* keys_pressed_1 = "up";
const char* keys_released_1 = "g";

sized_string all_inputs[2] = {(sized_view){keys_input_1, strlen(keys_input_1}, (sized_view){keys_input_2, strlen(keys_input_2)}};
sized_string all_pressed[] = {(sized_view){keys_pressed_1, strlen(keys_pressed_1)}}
sized_string all_released[] = {(sized_view){keys_released_1, strlen(keys_released_1)}}

sa_do_send_keystrokes_to_script(shared, script_id, all_inputs, 2, all_pressed, 1, all_released, 1);
```

To send script info updates, these can be done as follows


```
int current_width = 40; ///characters
int current_height = 30;

sa_do_send_script_info(shared, script_id, current_width, current_height);
```

#### Caveats

When the user types a command, sa_is_local_command can be used to check if the official client would handle that command without sending anything to the server, eg the # command, or #edit. However the ability to execute these commands is currently not in the lib

When the user types a command, #up requires special handling, as the request internally is #up [scriptname] [scriptdata]. This can be achieved by the following code fragment

```
///user has typed 'hello' into their terminal
sized_string some_command = {"hello", strlen("hello")};
sized_view script_directory = {"./scripts/", strlen("./scripts/")};

sized_string current_user = sd_get_user(shared);

sized_string up_handled = sa_default_up_handling({current_user.str, current_user.num}, {some_command.str, some_command.num}, script_directory);

sized_string server_command = sa_make_generic_server_command({up_handled.str, up_handled.num});

sd_add_back_write(shared, {server_command.str, server_command.num});

free_sized_string(server_command);
free_sized_string(up_handled);
free_sized_string(current_user);
```

### Parsing Responses

There are 6 kinds of server response:

1. server_command_command, -> response to a generic command

2. server_command_chat_api, -> response to a client_poll

3. server_command_server_scriptargs, -> response to a valid script argument request

4. server_command_server_scriptargs_invalid, -> response to an invalid script argument request

5. server_command_server_scriptargs_ratelimit, -> response if you are requesting too many script arguments per second

6. server_command_command_realtime -> realtime script info

To use the server response parsing abilities, this should work as follows:

```
if(sd_has_front_read(shared))
{
	sized_string from_server = sd_get_front_read(shared);
	
	server_command_info command_info = sa_server_response_to_info({from_server.str, from_server.num});
	
	///received a generic server message
	if(command.type == server_command_command)
	{
		sized_string str = sa_command_to_human_readable(command_info);
		
		///push str to terminal
		
		free_sized_string(str);
	}
	
	if(command.type == server_command_chat_api)
	{
		chat_api_info chat_info = sa_chat_api_to_info(command_info);
		
		///for every message that we've been sent by the server
		for(int i=0; i < chat_info.num_msgs; i++)
		{
			///the channel that the message received is in
			///we dont own this string
			sized_string channel = chat_info.msgs[i].channel;
			sized_string msg = chat_info.msgs[i].msg;
			
			///do something with the message and the channel here
		}
		
		for(int i=0; i < chat_info.num_in_channels; i++)
		{
			///user is joined into this channel
			sized_string channel = chat_info.in_channels[i].channel;
			
			///do something with the channel that the current user is in
		}
		
		for(int i=0; i < chat_info.num_tells; i++)
		{
			///received a tell
			sized_string msg = chat_info.tells[i].msg;
			sized_string from = chat_info.tells[i].user;
			
			///push msg to terminal, its formatted so from is purely for informational use
		}
		
		sa_destroy_chat_api_info(chat_info);
	}	
	
	if(command.type == server_command_server_scriptargs)
	{
		script_argument_list args = sa_server_scriptargs_to_list(command_info);
		
		///this script has args.num autocomplete arguments
		for(int i=0; i < args.num; i++)
		{
			///get one autocomplete key and value
			sized_string key = args.args[i].key;
			sized_string val = args.args[i].val;
			
			///do something with key and val
		}
		
		sized_string scriptname = args.scriptname;
		
		///do something with scriptname
		
		sa_destroy_script_argument_list(args);
	}
	
	if(command.type == server_command_server_scriptargs_invalid)
	{
		///name may be 0 length
		sized_string name = sa_server_scriptargs_invalid_to_script_name(command_info);
		
		///should register name here as not being a valid name so we dont rerequest it
		
		free_sized_string(name);
	}
	
	if(command.type == server_command_server_scriptargs_ratelimit)
	{
		sized_string name = sa_server_scriptargs_ratelimit_to_script_name(command_info);
		
		///name is the name of the script here that the server did not process as we were ratelimited
		///retry name after a delay
		
		free_sized_string(name);
	}
    
    if(command.type == server_command_command_realtime)
    {
        realtime_info info = sa_command_realtime_to_info(command_info);
    
        ///do something with info
    
        sa_destroy_realtime_info(info);
    }
	
	sa_destroy_server_command_info(command_info);
	
	free_sized_string(from_server);
}
```

# Things this API provides

1. A generic websock connection to the server with compression, implemented as a read/write queue

2. The ability to parse any server response into a datastructure which an application can consume

3. The ability to send any currently supported message to the server

4. Some degree of key.key file management (see code example on key.key under Queue on how to use)

5. Guaranteed compatibility with the server and current protocol

# Things this API does not provide

1. Parsing or tokenising of text to extract scriptnames for the purposes of requesting autocompletes

2. Syntax highlighting

3. Management of scripts

4. Local commands such as #, #dir, #edit etc

5. A GUI or windowing facility

# Raw Docs

## c_shared_data.h

`sized_string` -> this data structure owns a string, str points to the first element, and it has num characters. If an API returns a sized string, you must free it with free_sized_string

`sized_view` -> this data structure is a reference to a string, str points to the first element, and it has num characters. You are not required to free a sized view returned by the API, and functions taking a sized_view do not require the memory to be usable after the call. You do not need to free this type

`c_shared_data sd_alloc()` -> returns a new c_shared_data structure. This should be destroyed with sd_destroy before program termination to prevent memory leaks

`void sd_destroy(c_shared_data data)` -> frees a c_shared_data structure

`void sd_set_auth(c_shared_data data, sized_view auth)` -> attaches auth to the c_shared_data argument. This should be done prior to starting up the websocket connection with `nc_start` if auth is available in a key.key file

`int sd_has_front_read(c_shared_data data)` -> returns 1 if there is new data available for reading from the server

`int sd_has_front_write(c_shared_data data)` -> returns 1 if there is new data to be sent to the server. You probably don't want to use this

`sized_string sd_get_front_read(c_shared_data data)` -> returns the raw string received from the server with no processing. You must check if there is a front read available

`sized_string sd_get_front_write(c_shared_data data)` -> returns the raw string at the front of the write queue, which would ordinarily be processed by the server handler started up by `nc_start`. You must check if there is a front write before calling this. You probably dont want to call this function

`sized_string sd_add_back_write(c_shared_data data, sized_view write)` -> adds a raw string to be sent to the server, at the back of the queue

`sized_string sd_add_back_read(c_shared_data data, sized_view read)` -> adds a string to the read queue, as if read in from the server. Useful for locally producing text and handling it as if it were created by the server

`void sd_set_user(c_shared_data data, sized_view user)` -> sets the current user associated with the c_shared_data structure. Currently, unfortunately, you must parse outgoing messages to the server and look for "user <username>" commands, and then set the user manually based on that parsing. This will change in a future update

`sized_string sd_get_user(c_shared_data data)` -> returns the current user associated with the c_shared_data structure

`void sd_set_termination(c_shared_data data)` -> indicates that the internal websock client should terminate. Currently not fully reliable

`int sd_should_terminate(c_shared_data data)` -> returns 1 if the websock client should terminate, otherwise 0

`void sd_increment_termination_count(c_shared_data data)` -> you probably dont want to use this

`int sd_get_termination_count(c_shared_data data)` -> you probably dont want to use this

`void free_string(char*)` -> deprecated and to be removed
`void free_sized_string(sized_string str)` -> All sized_string's returned by the API must be freed with this function

## c_net_client.h

`nc_start(c_shared_data data, const char* host_ip, const char* host_port)` takes an allocated c_shared_data structure and initialises a websocket connection to the specified host ip and port

## c_server_api.h

`sized_string sa_make_chat_command(sized_view chat_channel, sized_view chat_msg)` -> returns a string that can be sent to the server to do a chat message, to the specified chat channel

`sized_string sa_make_generic_server_command(sized_view server_msg)` -> returns a string that can be sent to the server to do a generic server command, such as 1+1

`sized_string sa_make_autocomplete_request(sized_view scriptname)` -> returns a string that can be sent to the server to request an autocomplete for the specified scriptname

`int sa_is_local_command(sized_view server_msg)` -> returns 1 if the specified message (unprocessed, aka "#edit") should be handled purely locally, else 0

`sized_string sa_default_up_handling(sized_view for_user, sized_view server_msg, sized_view scripts_dir)` -> performs default #up handling, server_msg is a cli command such as "#up scriptname"

`void sa_do_poll_server(c_shared_data data)` -> performs a server poll. Must be rate limited manually, sending about 2 per second is recommended currently
`void sa_do_autocomplete_request(c_shared_data data, sized_view scriptname)` -> performs an autocomplete request for scriptname
`void sa_do_terminate_all_scripts(c_shared_data data)` -> requests that all realtime scripts be terminated
`void sa_do_terminate_script(c_shared_data data, int script_id)` -> requests that the realtime script with id script_id be terminated
`void sa_do_send_keystrokes_to_script(c_shared_data data, int script_id,
                                     sized_view* keystrokes, int num_keystrokes,
                                     sized_view* on_pressed, int num_pressed,
                                     sized_view* on_released, int num_released)` -> sends keystrokes to a realtime script with script_id, here, pointers are to an array of sized_view objects, of the corresponding num_ argument long. Keystrokes are traditional input keystrokes as if from a text editor, on_pressed is which keys were depressed since the last call, and on_released are which keys were released since the last call

`void sa_do_update_mouse_to_script(c_shared_data data, int script_id,
                                  float mousewheel_x, float mousewheel_y,
                                  float mouse_x,      float mouse_y)` -> sends a mouse update to the realtime script with script_id. You should accumulate mousewheels between calls. Mouse_x and mouse_y are not pixels, but in terms of characters. EG, mouse_y = 8.5 means that the mouse is offset by 8.5 characters from the top of the window 

`void sa_do_send_script_info(c_shared_data data, int script_id,
                            int width, int height)` -> sends general info to a realtime script with script_id. Width and height are not in pixels, but characters

`enum server_command_type` -> represents the type of server command received. Some api is in flux so check the header for all available commands

For the following structs, do not destroy the sized_strings, simply destroy the whole struct with the corresponding destroy function

`struct server_command_info` -> represents a server command, as well as the data after the command has been stripped of prefix information. This must be destroyed after being returned with `sa_destroy_server_command_info`

`struct realtime_info` -> id is the script id, msg colour coded text to be renderered repeatedly. On every update the new msg should overwrite the whole window contents. Width and height are not in pixels but characters, and should_close indicates that the script has signalled that it would like the window to be closed. This must be freed after being returned with `sa_destroy_realtime_info`

`struct chat_info` -> channel is the channel the message was sent to, msg is the message received. This does not need to be freed by the user explicitly

`struct chat_channel` -> represents a chat channel. This does not need to be freed by the user explicitly

`struct tell_info` -> represents a tell from a user with a message. This does not need to be freed explicitly

`struct chat_api_info` -> contains an array of msgs of length num_messages, as well as an array of channels that the current user is situated in of num_in_channels length, and a number of tells of num_tells length. This must be freed by the user with `sa_destroy_chat_api_info`

`struct script_argument` -> represents a single script argument. Does not need to be freed by the user
{
    sized_string key;
    sized_string val;
};

`struct script_argument_list` -> represents a list of script arguments for a scriptname. Args is an array which is num long

The following destroy functions should be called whenever you've finished manipulating the corresponding object to prevent memory leaks

`void sa_destroy_server_command_info(server_command_info info)` -> destroyed a server_command_info object

`void sa_destroy_realtime_info(realtime_info info)` -> destroyes a realtime_info object

`void sa_destroy_chat_api_info(chat_api_info info)` -> destroys a chat_api_info object

`void sa_destroy_script_argument_list(script_argument_list argl)` -> destroys a script_argument_list object

The following functions all must have their return types freed or destroyed in some form

`server_command_info sa_server_response_to_info(sized_view server_response)` -> converts a raw server response into a server command info type, which must be destroyed

`sized_string sa_command_to_human_readable(server_command_info info)` -> extracts the human readable string (it includes colour codes, and must be freed) from a server command. The type property must be server_command_command

`realtime_info sa_command_realtime_to_info(server_command_info info)` -> creates a realtime_info struct (which must be destroyed) from a server command, the type property of the server command must be server_command_command_realtime

`chat_api_info sa_chat_api_to_info(server_command_info info)` -> creates a chat_api_info struct (which must be destroyed) from a server command, the type property of the server command must be server_command_chat_api

`script_argument_list sa_server_scriptargs_to_list(server_command_info info)` -> creates a script_argument_list struct (which must be destroyed) from a server command, the type property of the server command must be server_command_server_scriptargs

`sized_string sa_server_scriptargs_invalid_to_script_name(server_command_info info)` -> returns the script name for an invalid autocomplete request (which must be freed, and may be null), the type property of the server command must be server_command_server_scriptargs_invalid

`sized_string sa_server_scriptargs_ratelimit_to_script_name(server_command_info info);` -> returns the script name  (which must be freed) from a failed autocomplete request due to rate limiting from a server command object, the type property of the server command must be server_command_server_scriptargs_ratelimit
