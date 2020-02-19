#include <ace/Get_Opt.h>

#include <list>
#include <string>
#include <utility>
#include <iostream>

#include <netcomgrp/centralized/client/group.h>

#include "chat_base.h"

using namespace std;

string chat_room_id;
netcomgrp::group *grp = NULL;

const char *usage = 
"\nUsage: centralized_client [opts] server_addr:port_number\n" \
" opts:\n" \
"\n";

void operation_simple() {
	chat_base *cb = new chat_base(grp, chat_room_id);
	
	cb->run();	
}

void parse_cmd_line_opts(ACE_Get_Opt &cmd_opts, map<char, string> &conf_values) {
	int option;
	while ((option = cmd_opts ()) != EOF) {
		switch (option) {
		case ':':
			throw "option missing";
			break;			
		default:
			throw "invalid option";
		}
	}
}

void parse_cmd_line_chat_id(ACE_Get_Opt &cmd_opts)
{
	if (cmd_opts.argc() - cmd_opts.opt_ind() != 1) 
		throw "port number must be specified";
	
	chat_room_id = cmd_opts.argv()[cmd_opts.opt_ind()];
	
	ACE_DEBUG((LM_DEBUG, "Using room id: %s\n", chat_room_id.c_str()));
}

netcomgrp::group *
group_create() {
	netcomgrp::centralized::client::group *g = 
		new netcomgrp::centralized::client::group;
	return g;
}
	
int
do_main(int argc, ACE_TCHAR *argv[]) {
	static const ACE_TCHAR options[] = ":";
	ACE_Get_Opt cmd_opts(argc, argv, options, 1, 1);

	map<char, string> conf_opts;
	
	parse_cmd_line_opts(cmd_opts,  conf_opts);
	parse_cmd_line_chat_id(cmd_opts);
	     
	grp      = group_create();
	
	operation_simple();

	cout << "Exiting" << endl;
		
	return 0;
}

int
ACE_TMAIN (int argc, ACE_TCHAR *argv[])
{
	const char *excstr      = NULL;
	bool received_exception = false;

	ACE_LOG_MSG->set_flags(ACE_Log_Msg::VERBOSE_LITE);
	//ACE_LOG_MSG->priority_mask(~LM_DEBUG, ACE_Log_Msg::PROCESS);
	
	try {
		do_main(argc, argv);
	} catch (exception &e) {
		received_exception = true;
		excstr = e.what();
	} catch (const char *str) {
		received_exception = true;
		excstr = str;
	}
		
	if (received_exception) {
		ACE_ERROR((LM_ERROR, "Exception caught:\n"));
		ACE_ERROR((LM_ERROR, "%s\n", excstr));
		ACE_ERROR((LM_ERROR, usage));
		return -1;
	}
	return 0;
}

