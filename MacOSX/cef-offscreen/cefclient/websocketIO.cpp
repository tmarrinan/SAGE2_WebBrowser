//
//  websocketIO.cpp
//  cefclient
//
//  Created by Thomas Marrinan on 4/3/14.
//
//

#include "websocketIO.h"

websocketIO::websocketIO() : m_open(false),m_done(false) {
    m_client.clear_access_channels(websocketpp::log::alevel::all);
    m_client.set_access_channels(websocketpp::log::alevel::app);
    m_client.init_asio();
    
    // Bind the handlers we are using
    using websocketpp::lib::placeholders::_1;
    using websocketpp::lib::placeholders::_2;
    using websocketpp::lib::bind;
    
    // Register our handlers
    m_client.set_socket_init_handler(bind(&websocketIO::on_socket_init, this, ::_1));
    m_client.set_tls_init_handler(bind(&websocketIO::on_tls_init, this, ::_1));
    m_client.set_message_handler(bind(&websocketIO::on_message, this, ::_1, ::_2));
    m_client.set_open_handler(bind(&websocketIO::on_open, this, ::_1));
    m_client.set_close_handler(bind(&websocketIO::on_close, this, ::_1));
}

void websocketIO::on_socket_init(websocketpp::connection_hdl hdl) {
    // init socket
}

websocketIO::context_ptr websocketIO::on_tls_init(websocketpp::connection_hdl hdl) {
    context_ptr ctx(new boost::asio::ssl::context(boost::asio::ssl::context::tlsv1));
    
    try {
        ctx->set_options(boost::asio::ssl::context::default_workarounds |
                         boost::asio::ssl::context::no_sslv2 |
                         boost::asio::ssl::context::single_dh_use);
    } catch (std::exception& e) {
        std::cout << e.what() << std::endl;
    }
    return ctx;
}

void websocketIO::on_open(websocketpp::connection_hdl hdl) {
    m_client.get_alog().write(websocketpp::log::alevel::app, "Connection opened");
    
    scoped_lock guard(m_lock);
    m_open = true;
    
    m_openCallback(this);
}

void websocketIO::on_close(websocketpp::connection_hdl hdl) {
    m_client.get_alog().write(websocketpp::log::alevel::app, "Connection closed");
    
    scoped_lock guard(m_lock);
    m_done = true;
}

void websocketIO::on_fail(websocketpp::connection_hdl hdl) {
    m_client.get_alog().write(websocketpp::log::alevel::app, "Connection failed");
    
    scoped_lock guard(m_lock);
    m_done = true;
}

void websocketIO::on_message(websocketpp::connection_hdl hdl, message_ptr msg) {
    boost::property_tree::ptree json;
    std::istringstream iss(msg->get_payload());
    boost::property_tree::read_json(iss, json);
    
    std::string func = json.get<std::string> ("func");
    if(messages.find(func) != messages.end()){
        messages[func](this, msg->get_payload());
    }
}

void websocketIO::send_message(std::string msg) {
    if (!m_open) {
        return;
    }
    
    websocketpp::lib::error_code ec;
    m_client.send(m_hdl, msg, websocketpp::frame::opcode::text, ec);
    
    if (ec) {
        m_client.get_alog().write(websocketpp::log::alevel::app, "Send Error: "+ec.message());
    }
}

void websocketIO::openCallback(void (*callback)(websocketIO*)) {
    m_openCallback = callback;
}

void websocketIO::on(std::string name, void (*callback)(websocketIO*, std::string)) {
    messages[name] = callback;
}

void websocketIO::emit(std::string name, boost::property_tree::ptree data) {
    boost::property_tree::ptree root;
    root.put<std::string>("func", name);
    root.put_child("data", data);
    
    std::ostringstream oss;
    boost::property_tree::write_json(oss, root, false);
    
    send_message(oss.str());
}

void websocketIO::run(const std::string & uri) {
    websocketpp::lib::error_code ec;
    client::connection_ptr con = m_client.get_connection(uri, ec);
    
    if (ec) {
        m_client.get_alog().write(websocketpp::log::alevel::app, ec.message());
    }
    
    m_hdl = con->get_handle();
    m_client.connect(con);
    
    websocketpp::lib::thread asio_thread(&client::run, &m_client);
}

