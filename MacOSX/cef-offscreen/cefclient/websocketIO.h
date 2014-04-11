//
//  websocketIO.h
//  cefclient
//
//  Created by Thomas Marrinan on 4/3/14.
//
//

#ifndef cefclient_websocketIO_h
#define cefclient_websocketIO_h

#include <iostream>
#include <sstream>
#include <string>
#include <map>
#include <boost/property_tree/ptree.hpp>
#include <boost/property_tree/json_parser.hpp>
#include <boost/bind.hpp>

#include "websocketpp/config/asio_client.hpp"
#include "websocketpp/client.hpp"
#include "websocketpp/common/thread.hpp"

class websocketIO {
public:
    typedef websocketpp::client<websocketpp::config::asio_tls_client> client;
    typedef websocketpp::config::asio_tls_client::message_type::ptr message_ptr;
    typedef websocketpp::lib::shared_ptr<boost::asio::ssl::context> context_ptr;
    typedef websocketpp::lib::lock_guard<websocketpp::lib::mutex> scoped_lock;
    typedef client::connection_ptr connection_ptr;
    
private:
    client m_client;
    websocketpp::connection_hdl m_hdl;
    websocketpp::lib::mutex m_lock;
    void (*m_openCallback)(websocketIO*);
    std::map<std::string, void ( *)(websocketIO*, std::string)> messages;
    bool m_open;
    bool m_done;
    
public:
    websocketIO();
    
    void on_socket_init(websocketpp::connection_hdl hdl);
    context_ptr on_tls_init(websocketpp::connection_hdl hdl);
    void on_open(websocketpp::connection_hdl hdl);
    void on_close(websocketpp::connection_hdl hdl);
    void on_fail(websocketpp::connection_hdl hdl);
    void on_message(websocketpp::connection_hdl hdl, message_ptr msg);
    void send_message(std::string msg);
    void run(const std::string & uri);
    
    void openCallback(void (*callback)(websocketIO*));
    void on(std::string name, void (*callback)(websocketIO*, std::string));
    void emit(std::string name, boost::property_tree::ptree data);
};


#endif
