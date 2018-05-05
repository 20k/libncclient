This is the client lib for net_code, the official client uses everything here and so its guaranteed to work (as well as the official client

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

Currently, the queue deals with containing user and auth information, as these are partially internally managed. Auth data should currently always be stored in a file named key.key, with the raw binary data in

To set the currently running user, use sd_set_user(shared, {user_name, length_of_user_name}). To set auth data use sd_set_auth(shared, {auth_data, length_of_auth_data}). It is important to remember that auth data (and other strings) may contain nulls, you must not assume they are null terminated

You only need to set auth data once on startup, if it exists, as shown in setup. It is otherwise managed automatically

## API

One major feature of libncclient is the ability to hide the entire server api from you, which is accessible from c_server_api. Usage of this is optional, but the underlying communication format is liable to change

### Creating Requests

There are 4 kinds of server request:

1. Poll
2. Generic
3. Autocomplete
4. Chat api

1. Polling should be performed every x seconds (preferably 1s) with sa_do_poll_server(shared);

2. Generic server requests can be achieved as such:

```
const char* msg = "#scripts.core()";

sized_string some_request = sa_make_generic_server_command({msg, strlen(msg)});

sd_add_back_write(shared, some_request);

free_sized_string(some_request);
```

3. Autocomplete requests can be achieved as such:

```
const char* scriptname = "scripts.core";

sized_string some_request = sa_make_autocomplete_request({scriptname, strlen(scriptname)});

sd_add_back_write(shared, some_request);

free_sized_string(some_request);
```

Or additionally sa_do_autocomplete_request(shared, {scriptname, strlen(scriptname)}) can be used

4. The chat api request is the same as a generic command except it suppresses output. Chat api requests can be achieved as such:

```
const char* chat_message = "hello";
const char* chat_channel = "0000";

sized_string some_request = sa_make_chat_command({chat_channel, strlen(chat_channel)}, {chat_message, strlen(chat_message)});

sd_add_back_write(shared, some_request);

free_sized_string(some_request);
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

There are 5 kinds of server response:

server_command_command, -> response to a generic command
server_command_chat_api, -> response to a client_poll
server_command_server_scriptargs, -> response to a valid script argument request
server_command_server_scriptargs_invalid, -> response to an invalid script argument request
server_command_server_scriptargs_ratelimit, -> response if you are requesting too many script arguments per second

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
	}
	
	if(command.type == server_command_server_scriptargs_ratelimit)
	{
		sized_string name = sa_server_scriptargs_ratelimit_to_script_name(command_info);
		
		///name is the name of the script here that the server did not process as we were ratelimited
		///retry name after a delay
	}
	
	sa_destroy_server_command_info(command_info);
	
	free_sized_string(from_server);
}
```