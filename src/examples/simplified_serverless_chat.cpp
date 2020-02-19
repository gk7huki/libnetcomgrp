/**
 * File: simplified_serverless_chat.cpp
 * 
 * A simplified example of how to use netcomgrp library
 * to implement an serverless chat client.
 * Uses DHT's KadC implementation to find the
 * participants to the chat.
 * 
 * This is not a practical example, as no chat output
 * will be shown unless enter pressed since the main loop
 * is waiting for input in std::cin.getline(). A more
 * comprehensive example would use threads or ACE's reactor
 * to wait for input. But this example is good for
 * showing the basic usage in as simple terms as possible.
 * 
 * Of the options the only one needing some explanation
 * is perhaps the announcement address:
 * The announcement address should be the IP you can be
 * contacted from outside. If you do not specify it, 
 * it will be tried to be figured by using the DHT implementation.
 * It's good to know, however, that due to some broken clients
 * in Overnet network KadC can not always return the correct
 * one. In that case you should specify it yourself.
 * 
 * You will need to replace the initialization file for
 * KadC, "kadc.ini", with a more recent version to get the
 * initial contact nodes.
 * 
 * Also if you need more than one version of the client running
 * on the same machine, you will need to copy kadc.ini and
 * change the ports it uses so that the two instances don't
 * use the same ports.
 */
#include <map>
#include <list>
#include <string>
#include <utility>
#include <iostream>
#include <sstream>

#include <ace/Get_Opt.h>

#include <netcomgrp/serverless/group.h>
#include <dht/kadc/client.h>

std::string chat_room_id;
std::string kadc_init_file = "kadc.ini";

dht::client              *dht_client;
netcomgrp::group         *grp;
netcomgrp::addr_inet_type addr_ann;

const char *usage = 
"\nUsage: serverless_chat [opts] [chat_room_identifier]\n" \
" opts:\n" \
"  -d <dht>            dht implementation to use. " 
                      "Not used at the moment, only KadC available. \n" \
"  -a <port>|<ip:port> announcement address.\n" \
"  -k <file>           kadc init file to use, default is kadc.ini.\n" \
"\n";

void do_chat() {
	class chat_observer : public netcomgrp::event_observer {
	public:
		virtual int data_received(const void *data, 
		                          size_t len, 
		                          const netcomgrp::node *from) 
		{		
			std::string line;
			
			line.append(static_cast<const char *>(data), len);
			
			std::cout << std::endl;
			std::cout << "Received data " << len << " bytes: " << std::endl;
			std::cout << '[' << from->addr().get_host_addr() << ':'
			          << from->addr().get_port_number()
			          << ']' << ' ' 
			          << line << std::endl;
			     
	 		return 0;
		}
		virtual int state_changed(int state) {
			std::cout << "netcomgrp state changed to "
			          << grp->state_str(state)
			          << std::endl;
			return 0;
		}
	};
	chat_observer o;	
	grp->observer_attach(&o, 
	                     netcomgrp::event_observer::mask_data_received |
	                     netcomgrp::event_observer::mask_state_changed);
	
	grp->join(chat_room_id.c_str());

	std::cout << "Trying to join to chat group " << chat_room_id
	          << std::endl;
	          	
	while (grp->in_state() != netcomgrp::group::not_joined) {
		if (grp->in_state() == netcomgrp::group::joined) {
			char buffer[1024];
			// Obtain an input string to send to recipients
			std::cout << "Waiting for input > ";
			std::cin.getline(buffer, sizeof(buffer));
			std::string str = buffer;
			if (str.size() > 0) {
				if (str == "quit") {
					grp->leave();
				} else {
					grp->send(str.data(), str.size());
				}
			}			
		}
		grp->process();
	}	
}

dht::client *
dht_create() {
	dht::client *c = new dht::kadc::client;
	dht::name_value_map conf;
	
	std::cout << "Using kadc init file: " << kadc_init_file << std::endl;
	
	conf.set("init_file", kadc_init_file);
	c->init(conf);
	return c;
}

netcomgrp::group *
group_create(dht::client *n) {
	netcomgrp::serverless::group *g = new netcomgrp::serverless::group;
	// Set the node that is used to find potential peers that are participating
	// in the chat.
	g->dht_client(n);
	// Set announcement address to use
	g->addr_announce(addr_ann);
	return g;
}

void parse_cmd_line_opts(ACE_Get_Opt &cmd_opts, std::map<char, std::string> &conf_values) {
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

void parse_cmd_line_chat_id(ACE_Get_Opt &cmd_opts) {
	chat_room_id = "chat://serverless_chat/";
	if (cmd_opts.argc() - cmd_opts.opt_ind() != 1) 
		chat_room_id += "default";
	else 	
		chat_room_id += cmd_opts.argv()[cmd_opts.opt_ind()];
	
	ACE_DEBUG((LM_DEBUG, "Using chat room id: %s\n", chat_room_id.c_str()));
}

void do_main(int argc, ACE_TCHAR *argv[]) {
	static const ACE_TCHAR options[] = ":a:d:k:";
	ACE_Get_Opt cmd_opts(argc, argv, options, 1, 1);

	std::map<char, std::string> conf_opts;
	
	conf_opts['d'] = "kadc";
	conf_opts['a'] = "";

	parse_cmd_line_opts(cmd_opts,  conf_opts);
	parse_cmd_line_chat_id(cmd_opts);

	if (!conf_opts['a'].empty()) {
		std::cout << "Parsing announcement address " << conf_opts['a'] << std::endl;
		addr_ann.string_to_addr(conf_opts['a'].c_str());
	}
	std::cout << "Announcement address is: " 
	     << addr_ann.get_host_addr() << ":"
	     << addr_ann.get_port_number() << std::endl;
	     
	dht_client = dht_create();
	grp        = group_create(dht_client);

	do_chat();	

	std::cout << "Exiting" << std::endl;
}

void cleanup() {
	if (dht_client) {
		std::cout << "Deleting dht" << std::endl;
		delete dht_client;
	}
	if (grp) {
		std::cout << "Deleting netcomgrp" << std::endl;
		delete grp;
	}
}

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{	
	try {
		do_main(argc, argv);
	} catch (std::exception &e) {
		ACE_ERROR((LM_ERROR, "Exception caught:\n"));
		ACE_ERROR((LM_ERROR, "%s\n", e.what()));
		ACE_ERROR((LM_ERROR, usage));
		return -1;
	} catch (const char *err) {
		std::cerr << "Error: " << err << std::endl;
		std::cerr << usage << std::endl;
		return -1;
	}

	cleanup();

	return 0;
}
