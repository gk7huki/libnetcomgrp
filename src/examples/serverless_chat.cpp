#include <ace/Get_Opt.h>

#include <list>
#include <string>
#include <utility>
#include <iostream>

// For now like this, maybe later should use a factory to abstract away
// the used DHT implementation
#include <dht/kadc/client.h>
#include <netcomgrp/serverless/group.h>

#include "chat_base.h"

using namespace std;

string chat_room_id;
string kadc_init_file = "kadc.ini";

bool quit;

dht::client      *dht_client;
netcomgrp::group *grp;
netcomgrp::addr_inet_type addr_ann;

const char *usage = 
"\nUsage: serverless_chat [opts] chat-room-id\n" \
" opts:\n" \
"  -d <dht>            dht implementation to use. TODO. \n" \
"  -a <port>|<ip:port> announcement address.\n" \
"  -k <file>           kadc init file to use, default is kadc.ini.\n" \
"\n";

void do_chat() {
	chat_base *cb = new chat_base(grp, chat_room_id);
	
	cb->run();	
}

void parse_cmd_line_opts(ACE_Get_Opt &cmd_opts, map<char, string> &conf_values) {
	int option;
	while ((option = cmd_opts ()) != EOF) {
		switch (option) {
		case ':':
			throw "option missing";
		case 'a':
		case 'd':
			conf_values[option] = cmd_opts.opt_arg();
			break;
		case 'k':
			kadc_init_file = cmd_opts.opt_arg();
			break;			
		default:
			throw "invalid option";
		}
	}
}

void parse_cmd_line_chat_id(ACE_Get_Opt &cmd_opts)
{
	if (cmd_opts.argc() - cmd_opts.opt_ind() != 1) 
		throw "one chat room id must be specified";
	
	chat_room_id  = "chat://serverless_chat/";
	chat_room_id += cmd_opts.argv()[cmd_opts.opt_ind()];
	
	ACE_DEBUG((LM_DEBUG, "Using chat room id: %s\n", chat_room_id.c_str()));
}

dht::client *
dht_create() {
	dht::client *n = new dht::kadc::client;
	dht::name_value_map conf;
	
	cout << "Using kadc init file: " << kadc_init_file << endl;
	
	conf.set("init_file", kadc_init_file);
	n->init(conf);
	return n;
}

netcomgrp::group *
group_create(dht::client *n) {
	netcomgrp::serverless::group *g = new netcomgrp::serverless::group;
	// Set the client that is used to find potential peers that are participating
	// in the chat.
	g->dht_client(n);
	// Set announcement address to use
	g->addr_announce(addr_ann);
	return g;
}
	
int
do_main(int argc, ACE_TCHAR *argv[]) {
	static const ACE_TCHAR options[] = ":a:d:k:";
	ACE_Get_Opt cmd_opts(argc, argv, options, 1, 1);

	map<char, string> conf_opts;
	
	conf_opts['d'] = "kadc";
	conf_opts['a'] = "";

	parse_cmd_line_opts(cmd_opts,  conf_opts);
	parse_cmd_line_chat_id(cmd_opts);

	if (!conf_opts['a'].empty()) {
		cout << "Parsing announcement address " << conf_opts['a'] << endl;
		addr_ann.string_to_addr(conf_opts['a'].c_str());
	}
	cout << "Announcement address is: " 
	     << addr_ann.get_host_addr() << ":"
	     << addr_ann.get_port_number() << endl;
	     
	ACE_DEBUG((LM_DEBUG, "dht_create\n"));
	dht_client = dht_create();
	ACE_DEBUG((LM_DEBUG, "group_create\n"));
	grp      = group_create(dht_client);
	
	do_chat();

	cout << "Exiting" << endl;
		
	return 0;
}

void cleanup() {
	if (dht_client) {
		cout << "Deinitialising dht" << endl;
		delete dht_client;
	}
}

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
	const char *excstr      = NULL;
	bool received_exception = false;

	ACE_LOG_MSG->set_flags(ACE_Log_Msg::VERBOSE_LITE);
	ACE_LOG_MSG->priority_mask(~LM_DEBUG, ACE_Log_Msg::PROCESS);
	
	try {
		do_main(argc, argv);
	} catch (exception &e) {
		received_exception = true;
		excstr = e.what();
	} catch (const char *str) {
		received_exception = true;
		excstr = str;
	}
	
	cleanup();
		
	if (received_exception) {
		ACE_ERROR((LM_ERROR, "Exception caught:\n"));
		ACE_ERROR((LM_ERROR, "%s\n", excstr));
		ACE_ERROR((LM_ERROR, usage));
		return -1;
	}
	return 0;
}

