#ifndef _CHAT_BASE_H_
#define _CHAT_BASE_H_

#include <ace/Event_Handler.h>
#include <ace/Thread_Manager.h>

#include <list>
#include <string>
#include <utility>
#include <iostream>

#include <netcomgrp/group.h>

namespace {
	using namespace std;
}

class chat_base : public ACE_Event_Handler,
                  public netcomgrp::event_observer {
	netcomgrp::group *grp;
	bool quit;
	string chat_room_id;
	const char *help;

	// stdin stuff
	string _read;  // Data buffer
	string _line;  // When a full line has been read, store it here
	static const size_t _buf_size = 1024;
	bool _stdin_handler;
public:
	chat_base(netcomgrp::group *g,
	          const std::string &chat_room) 
	  : grp(g),
	    quit(false),
	    chat_room_id(chat_room),
	    _stdin_handler(true)
	{
		help = 	
	"\nCommands:\n" \
	" /debug [on|off] - turn on or off full debugging output\n" \
	" /list  - list all the participating nodes\n" \
	" /state - show state of group\n" \
	" /quit  - quit the application\n" \
	"\n";

	}
	virtual ~chat_base() {
		if (_stdin_handler) {
			if (-1 == ACE_Event_Handler::remove_stdin_handler(
		                 ACE_Reactor::instance(),
		                 ACE_Thread_Manager::instance())) 
		   {
				// throw "failed removing stdin handler";
		   }	
		}
	}
	inline bool stdin_handler(bool t) { return _stdin_handler = t; }
	inline bool stdin_handler() const { return _stdin_handler; }
	
	virtual int handle_input (ACE_HANDLE handle) {
		char buf[_buf_size];
		ssize_t n = ACE_OS::read(handle, buf, _buf_size);
	
		// cout << "Read " << n << " bytes from stdin" << endl;
		if (n > 0) {
			_read.append(buf, n);
			string::size_type pos;
			if ((pos = _read.find('\n')) != string::npos) {
				_line.assign(_read, 0, pos);
				_read.erase(0, pos + 1);
		
				// ACE has a bug, handle_events does not wake by itself?
				reactor()->notify();
						
				// cout << "Line size: " << _line.size() << endl;
				// cout << "Read size: " << _read.size() << endl;
				process_line(_line);
				_line.erase();
			}		
		}
		return 0;		
	}
	
	inline void cmd_prompt() {
		cout << "> ";
		cout.flush();
	}
	
	bool do_command(const string &line) {
		if (line[0] != '/') return false;
		
		if (line == "/help") {
			cout << help << endl;
		}
		else if (line == "/debug on") {
			cout << "Turning on debug output..." << endl;
			ACE_LOG_MSG->
			  priority_mask(ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) |
			                LM_DEBUG, ACE_Log_Msg::PROCESS);
		}
		else if (line == "/debug off") {
			cout << "Turning off debug output..." << endl;
			ACE_LOG_MSG->
			  priority_mask(ACE_LOG_MSG->priority_mask(ACE_Log_Msg::PROCESS) &
			                ~LM_DEBUG, ACE_Log_Msg::PROCESS);
		}
		else if (line == "/state") {
			cout << "State: " << grp->in_state_str() << endl;
		}
		else if (line == "/quit") {
			cout << "Quitting...\n";
			quit = true;
		}
		else if (line == "/list") {
			list<const netcomgrp::node *> l;
			list<const netcomgrp::node *>::iterator i;
			
			grp->nodes(&l);
			cout << l.size() << " nodes found participating " << endl;
			for (i = l.begin(); i != l.end(); i++) {
				cout << '[' << (*i)->addr().get_host_addr() << ':'
				            << (*i)->addr().get_port_number()
				     << ']';
				if (grp->self() && **i == *(grp->self())) cout << " - self";
				cout << endl;
			}
		} else {
			cout << "Unrecognized command '" << line << "', use /help" << endl;
		}
		
		return true;
	}
	
	void process_line(const string &line) {
		if (!do_command(line)) {
			// If it is not a command then send to the group
	    	cout << "Sending to group: " << line << endl;
	    	grp->send(line.c_str(), line.size());
		}
		cmd_prompt();
	}
	
	// Netcomgrp event observer	
	virtual int node_added(const netcomgrp::node *n) {
		cout << "New node found: " 
		     << n->addr().get_host_addr()   << ":"
		     << n->addr().get_port_number() << endl;
		return 0;
	}
	virtual int node_removed(const netcomgrp::node *n) {
		cout << "Node removed: " 
		     << n->addr().get_host_addr()   << ":"
		     << n->addr().get_port_number() << endl;
		return 0;
	}
	virtual int state_changed(int state) {
		switch (state) {
		case netcomgrp::group::joining:
			cout << "Joining chat room " << chat_room_id << endl; break;
		case netcomgrp::group::joined:
			cout << "Joined to the chat room" << endl; 
			cout << "To send to the group write the text and hit enter" << endl;
			break;
		case netcomgrp::group::not_joined:
			cout << "Not joined to the chat room" << endl; break;
		case netcomgrp::group::leaving:
			cout << "Leaving chat room " << chat_room_id << endl; break;
		default:
			cout << "Unknown state change received: "
			     << grp->state_str(state) << " (" 
			     << state << ")" << endl;
		}

		cmd_prompt();		
		return 0;
	}
	virtual int data_received(const void *data, 
	                          size_t len, 
	                          const netcomgrp::node *from) 
	{		
		string line;
		
		line.append(static_cast<const char *>(data), len);
		
		cout << endl;
		cout << "Received data " << len << " bytes: " << endl;
		cout << '[' << from->addr().get_host_addr() << ':'
		            << from->addr().get_port_number()
		     << ']' << ' ' 
		     << line << endl;
		     
       	cmd_prompt();
 		return 0;
	}
	
	void run() {
		// group_observer grp_handler;
		// stdin_handler stdin_h;
		if (_stdin_handler) {		
			if (-1 == ACE_Event_Handler::register_stdin_handler(
			             this,
		                 ACE_Reactor::instance(),
		                 ACE_Thread_Manager::instance())) 
		   {
				throw "failed registering stdin handler";
		   }
		}	
		
		grp->observer_attach(this, 
		                     netcomgrp::event_observer::mask_data_received |
		                     netcomgrp::event_observer::mask_node_added    |
		                     netcomgrp::event_observer::mask_node_removed  |
		                     netcomgrp::event_observer::mask_state_changed);
		
		ACE_DEBUG((LM_DEBUG, "group_join\n"));
		grp->join(chat_room_id.c_str());
		
		netcomgrp::time_value_type timeout(60);
		while (!quit) {
			ACE_DEBUG((LM_DEBUG, "calling process\n"));
			netcomgrp::time_value_type max_wait = timeout;
			grp->process(max_wait);
		}
		grp->leave();
	
		while (grp->in_state() != netcomgrp::group::not_joined) {
			cout << "Waiting until leaving done..." << endl;
			grp->process();
		}
	}
};

#endif //_CHAT_BASE_H_
