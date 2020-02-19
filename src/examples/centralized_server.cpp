#include <ace/Get_Opt.h>
#include <ace/Logging_Strategy.h>

#include <list>
#include <string>
#include <utility>
#include <iostream>
#include <fstream>

#include <netcomgrp/centralized/server/group.h>

#include "chat_base.h"

#define array_sizeof(a) (sizeof(a) / sizeof(a[0]))

namespace {
	ACE_Logging_Strategy *logging_strategy = NULL;
}

using namespace std;

std::string chat_room_id;
std::string statistics_file;
netcomgrp::group *grp;

const char *usage = 
"\nUsage: centralized_server [opts] port_number\n" \
" opts:\n" \
" -s logfile\n" \
" -m logsize (default 2000k)\n" \
" -N logamount (default 2)\n" \
" -i log check interval in secs (default 10)\n" \
" -f log parameters (default VERBOSE_LITE)\n" \
" -c statistics file\n" \
"\n";

class server : public chat_base {
    std::ofstream _out;
public:
    server(netcomgrp::group *g,
           const std::string &chat_room) 
      : chat_base(g, chat_room)
    {
        if (!statistics_file.empty()) {
            _out.open(statistics_file.c_str(), 
                      std::ios::app | 
                      std::ios::binary);
        
            if (!_out.is_open())
                throw "Could not open statistics file";
                 
            _out << time(NULL) << ';' << "started" << std::endl;   
        }
    }

    virtual ~server() {
        if (_out.is_open()) {
            _out << time(NULL) << ';' << "stopped" << std::endl;   
            _out.close();
        }
    }
    // Netcomgrp event observer 
    virtual int node_added(const netcomgrp::node *n) {
        chat_base::node_added(n);
        if (_out.is_open()) {
            _out << time(NULL) << ';' 
                 << n->addr().get_host_addr() << ':'
                 << n->addr().get_port_number() << ';'
                 << 'a' << std::endl;            
        }
        return 0;
    }
    virtual int node_removed(const netcomgrp::node *n) {
        chat_base::node_removed(n);
        if (_out.is_open()) {
            _out << time(NULL) << ';' 
                 << n->addr().get_host_addr() << ':'
                 << n->addr().get_port_number() << ';'
                 << 'r' << std::endl;
        }
        return 0;
    }
    
};

void operation_simple() {
	server *cb = new server(grp, chat_room_id);
	cb->stdin_handler(false);
	cb->run();
    delete cb;
}

void parse_cmd_line_opts(ACE_Get_Opt &cmd_opts, map<char, string> &conf_values) {
	int option;
	while ((option = cmd_opts ()) != EOF) {
		switch (option) {
		case 's':
		case 'm':
		case 'N':
		case 'i':
		case 'f':
        case 'c':
			conf_values[option] = cmd_opts.opt_arg();
			break;
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

void conf_logging(map<char, string> &conf_opts) {
	if (conf_opts['s'].empty()) {
		return;
	}

	const char *log_s_pars[] = {
		"-s", conf_opts['s'].c_str(),
		"-m", conf_opts['m'].c_str(),
		"-N", conf_opts['N'].c_str(),
		"-i", conf_opts['i'].c_str(),
		"-f", conf_opts['f'].c_str()
	};

	logging_strategy = new ACE_Logging_Strategy;
	
	// TODO logging strategy's init unfortunately
	// writes temporarily to the passed strings due
	// to using strtok... using std::strings 
	// as values is technically wrong but will do for
	// now. Better to make a copy of the log_s_pars,
	// maybe with an encapsulating class	
	if (logging_strategy->init(array_sizeof(log_s_pars), 
	                           const_cast<char **>(log_s_pars))) 
	{
		cerr << "Could not open log system, params used: " << endl;
	                           
		for (unsigned int i = 0; i < array_sizeof(log_s_pars); i++)
			cerr << log_s_pars[i] << " ";
		cerr << endl;
	}		
}

netcomgrp::group *
group_create() {
	netcomgrp::centralized::server::group *g = 
		new netcomgrp::centralized::server::group;
	return g;
}
	
int
do_main(int argc, ACE_TCHAR *argv[]) {
	static const ACE_TCHAR options[] = ":s:m:N:i:f:c:";
	ACE_Get_Opt cmd_opts(argc, argv, options, 1, 1);
	
	map<char, string> conf_opts;
	conf_opts['m'] = "2000";
	conf_opts['N'] = "2";
	conf_opts['i'] = "10";
	conf_opts['f'] = "VERBOSE_LITE";
	
	parse_cmd_line_opts(cmd_opts,  conf_opts);
	conf_logging(conf_opts);
	parse_cmd_line_chat_id(cmd_opts);

    statistics_file = conf_opts['c'];
    if (!statistics_file.empty()) {
        ACE_DEBUG((LM_INFO, 
                  "Using statistics file: %s\n", statistics_file.c_str()));
    }
	ACE_DEBUG((LM_INFO, "Starting centralized_server\n"));
	
	// size_t tout_msec = 1000; // 1 sec timeout;
	// size_t send_amnt = 8;    // send count 8 before giving up
    size_t tout_msec = 0; // Use defaults
    size_t send_amnt = 0;    // Use defaults
	
	if (tout_msec != 0) {
		reudp::time_value_type tout;
		tout.msec(tout_msec);
		reudp::config::timeout(tout);
	}
	if (send_amnt != 0) {
		reudp::config::send_try_count(send_amnt);
	}
		
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
		cerr << excstr << endl << usage;
		return -1;
	}
	
	delete logging_strategy;

	ACE_DEBUG((LM_INFO, "Stopping centralized_server\n"));
	
	return 0;
}

