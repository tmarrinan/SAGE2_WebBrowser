#include <iostream>
#include <string>
#include <vector>

#include "include/cef_app.h"

#include "websocketIO.h"
#include "BrowserClient.h"
#include "OSRenderHandler.h"

// function prototypes
std::vector<std::string> split(std::string s, char delim);
void ws_open(websocketIO* ws);
void ws_initialize(websocketIO* ws, std::string data);
void ws_openWebBrowser(websocketIO* ws, std::string data);
void ws_requestNextFrame(websocketIO* ws, std::string data);
void ws_stopMediaCapture(websocketIO* ws, std::string data);
void ws_setItemPositionAndSize(websocketIO* ws, std::string data);
void ws_finishedResize(websocketIO* ws, std::string data);
void ws_eventInItem(websocketIO* ws, std::string data);
void onPaint(int browserIdx);

// globals
bool continuous_resize;
std::string uniqueID;
websocketIO* wsio;
std::vector<BrowserClient*> browserWindows;
std::vector<bool> windowOpen;
std::vector<bool> server_ready;
std::vector<bool> frame_updated;
char szWorkingDir[500];


int main(int argc, char* argv[]) {
    CefMainArgs main_args(argc, argv);
    
    int exit_code = CefExecuteProcess(main_args, NULL);
	if (exit_code >= 0)
		return exit_code;
    
    getcwd(szWorkingDir, sizeof(szWorkingDir));
    
	
	CefSettings settings;
	CefInitialize(main_args, settings, NULL);
    
    std::string ws_uri = "wss://localhost:443";
    if(argc == 2) ws_uri = argv[1];
    
    continuous_resize = false;
    
    wsio = new websocketIO();
    wsio->openCallback(ws_open);
    
    wsio->on("initialize", ws_initialize);
    wsio->on("openWebBrowser", ws_openWebBrowser);
    wsio->on("requestNextFrame", ws_requestNextFrame);
    wsio->on("stopMediaCapture", ws_stopMediaCapture);
    wsio->on("setItemPositionAndSize", ws_setItemPositionAndSize);
    wsio->on("finishedResize", ws_finishedResize);
    wsio->on("eventInItem", ws_eventInItem);
    
    wsio->run(ws_uri);
     
    CefRunMessageLoop();
    CefShutdown();
    
    
    return 0;
}

/******** Helper Functions ********/
std::vector<std::string> split(std::string s, char delim) {
    std::stringstream ss(s);
    std::string item;
    std::vector<std::string> elems;
    while (std::getline(ss, item, delim)) {
        elems.push_back(item);
    }
    return elems;
}

/******** WebSocket Callback Functions ********/
void ws_open(websocketIO* ws) {
    printf("WEBSOCKET OPEN\n");
    
    // send addClient message
    boost::property_tree::ptree data;
    data.put<std::string>("clientType", "webBrowser");
    data.put<bool>("sendsMediaStreamFrames", true);
    data.put<bool>("receivesWindowModification", true);
    data.put<bool>("receivesInputEvents", true);
    
    ws->emit("addClient", data);
}

void ws_initialize(websocketIO* ws, std::string data) {
    boost::property_tree::ptree json_data;
    std::istringstream iss(data);
    boost::property_tree::read_json(iss, json_data);
    
    uniqueID = json_data.get<std::string> ("data.UID");
    printf("ID: %s\n", uniqueID.c_str());
}

void ws_openWebBrowser(websocketIO* ws, std::string data) {
    boost::property_tree::ptree json_data;
    std::istringstream iss(data);
    boost::property_tree::read_json(iss, json_data);
    
    std::string wb_url = json_data.get<std::string> ("data.url");
    
    printf("\nOPEN WEB BROWSER: %s\n\n", wb_url.c_str());
    
    // CREATE NEW BROWSER
    CefBrowserSettings browserSettings;
    CefWindowInfo window_info;
    window_info.SetAsOffScreen(NULL);
    
    int browserIdx = browserWindows.size();
    int width = 1366; //int width = 1920;
    int height = 768; //int height = 1080;
    OSRenderHandler* osrHandler = new OSRenderHandler(width, height, browserIdx, onPaint);
    CefRefPtr<BrowserClient> browserClient = new BrowserClient(osrHandler);
    CefBrowserHost::CreateBrowser(window_info, browserClient.get(), wb_url, browserSettings, NULL);
    
    browserWindows.push_back(browserClient);
    windowOpen.push_back(true);
    server_ready.push_back(false);
    frame_updated.push_back(false);
    
    // send startNewMediaStream message
    boost::property_tree::ptree emit_data;
    emit_data.put<std::string>("id", uniqueID+"|"+boost::to_string(browserIdx));
    emit_data.put<std::string>("title", "SAGE2 Web Browser");
    emit_data.put<std::string>("src", "");
    emit_data.put<int>("width", width);
    emit_data.put<int>("height", height);
    ws->emit("startNewMediaStream", emit_data);
}

void ws_requestNextFrame(websocketIO* ws, std::string data) {
    boost::property_tree::ptree json_data;
    std::istringstream iss(data);
    boost::property_tree::read_json(iss, json_data);
    
    std::string streamId = json_data.get<std::string> ("data.streamId");
    int idx = atoi(streamId.c_str());
    
    if(windowOpen[idx]) { // if borwser window is still open
        server_ready[idx] = true;
        if(frame_updated[idx]) {
            frame_updated[idx] = false;
            server_ready[idx] = false;
            
            OSRenderHandler* osrHandler = browserWindows[idx]->GetOSRenderHandler();
            std::string jpegFrame = osrHandler->GetJPEGFrame();
            
            boost::property_tree::ptree emit_data;
            emit_data.put<std::string>("id", uniqueID+"|"+boost::to_string(idx));
            emit_data.put<std::string>("src", jpegFrame);
            emit_data.put<std::string>("encoding", "base64");
            ws->emit("updateMediaStreamFrame", emit_data);
        }
    }
}

void ws_stopMediaCapture(websocketIO* ws, std::string data) {
    boost::property_tree::ptree json_data;
    std::istringstream iss(data);
    boost::property_tree::read_json(iss, json_data);
    
    std::string streamId = json_data.get<std::string> ("data.streamId");
    int idx = atoi(streamId.c_str());
    
    printf("STOP MEDIA CAPTURE: %d\n", idx);
    
    CefRefPtr<CefBrowser> browser = browserWindows[idx]->GetBrower();
    browser->GetHost()->CloseBrowser(false);
    windowOpen[idx] = false;
}

void ws_setItemPositionAndSize(websocketIO* ws, std::string data) {
    // update browser window size during resize
    if(continuous_resize){
        boost::property_tree::ptree json_data;
        std::istringstream iss(data);
        boost::property_tree::read_json(iss, json_data);
        
        std::string id = json_data.get<std::string> ("data.elemId");
        std::vector<std::string> elemData = split(id, '|');
        std::string uid = "";
        int idx = -1;
        if(elemData.size() == 2){
            uid = elemData[0];
            idx = atoi(elemData[1].c_str());
        }
    
        if(uid == uniqueID) {
            if(windowOpen[idx]) { // if borwser window is still open
                std::string w = json_data.get<std::string> ("data.elemWidth");
                std::string h = json_data.get<std::string> ("data.elemHeight");
                
                float width = atof(w.c_str());
                float height = atof(h.c_str());
                
                OSRenderHandler* osrHandler = browserWindows[idx]->GetOSRenderHandler();
                CefRefPtr<CefBrowser> browser = browserWindows[idx]->GetBrower();
                osrHandler->SetRenderSize(browser, (int)width, (int)height);
            }
        }
    }
}

void ws_finishedResize(websocketIO* ws, std::string data) {
    boost::property_tree::ptree json_data;
    std::istringstream iss(data);
    boost::property_tree::read_json(iss, json_data);
    
    std::string id = json_data.get<std::string> ("data.id");
    std::vector<std::string> elemData = split(id, '|');
    std::string uid = "";
    int idx = -1;
    if(elemData.size() == 2){
        uid = elemData[0];
        idx = atoi(elemData[1].c_str());
    }
    
    if(uid == uniqueID) {
        if(windowOpen[idx]) { // if borwser window is still open
            std::string w = json_data.get<std::string> ("data.elemWidth");
            std::string h = json_data.get<std::string> ("data.elemHeight");
            
            float width = atof(w.c_str());
            float height = atof(h.c_str());
            
            OSRenderHandler* osrHandler = browserWindows[idx]->GetOSRenderHandler();
            CefRefPtr<CefBrowser> browser = browserWindows[idx]->GetBrower();
            osrHandler->SetRenderSize(browser, (int)width, (int)height);
        }
    }
}

void ws_eventInItem(websocketIO* ws, std::string data) {
    boost::property_tree::ptree json_data;
    std::istringstream iss(data);
    boost::property_tree::read_json(iss, json_data);
    
    std::string id = json_data.get<std::string> ("data.elemId");
    std::vector<std::string> elemData = split(id, '|');
    std::string uid = "";
    int idx = -1;
    if(elemData.size() == 2){
        uid = elemData[0];
        idx = atoi(elemData[1].c_str());
    }
    
    if(uid == uniqueID) {
        if(windowOpen[idx]) { // if borwser window is still open
            OSRenderHandler* osrHandler = browserWindows[idx]->GetOSRenderHandler();
            CefRefPtr<CefBrowser> browser = browserWindows[idx]->GetBrower();
            
            std::string eventType = json_data.get<std::string> ("data.eventType");
            
            if(eventType == "pointerMove") {
                std::string x = json_data.get<std::string> ("data.itemRelativeX");
                std::string y = json_data.get<std::string> ("data.itemRelativeY");
                
                int mouseX = atoi(x.c_str());
                int mouseY = atoi(y.c_str());
                
                osrHandler->PointerMove(browser, mouseX, mouseY);
            }
            else if(eventType == "pointerPress") {
                std::string x = json_data.get<std::string> ("data.itemRelativeX");
                std::string y = json_data.get<std::string> ("data.itemRelativeY");
                std::string button = json_data.get<std::string> ("data.data.button");
                
                int mouseX = atoi(x.c_str());
                int mouseY = atoi(y.c_str());
                
                osrHandler->PointerPress(browser, mouseX, mouseY, button);
            }
            else if(eventType == "pointerRelease") {
                std::string x = json_data.get<std::string> ("data.itemRelativeX");
                std::string y = json_data.get<std::string> ("data.itemRelativeY");
                std::string button = json_data.get<std::string> ("data.data.button");
                
                int mouseX = atoi(x.c_str());
                int mouseY = atoi(y.c_str());
                
                osrHandler->PointerRelease(browser, mouseX, mouseY, button);
            }
            else if(eventType == "pointerScroll") {
                std::string x = json_data.get<std::string> ("data.itemRelativeX");
                std::string y = json_data.get<std::string> ("data.itemRelativeY");
                std::string wheelDelta = json_data.get<std::string> ("data.data.wheelDelta");
                
                int mouseX = atoi(x.c_str());
                int mouseY = atoi(y.c_str());
                int deltaY = atoi(wheelDelta.c_str());
                
                osrHandler->PointerScroll(browser, mouseX, mouseY, 0, deltaY);
            }
            else if(eventType == "specialKey") {
                std::string code = json_data.get<std::string> ("data.data.code");
                std::string state = json_data.get<std::string> ("data.data.state");
                
                int charCode = atoi(code.c_str());
                
                osrHandler->SpecialKey(browser, charCode, state);
            }
            else if(eventType == "keyboard") {
                std::string code = json_data.get<std::string> ("data.data.code");
                
                int charCode = atoi(code.c_str());
                
                osrHandler->Keyboard(browser, charCode);
            }
        }
    }
}

void onPaint(int browserIdx) {
    if(windowOpen[browserIdx]) { // if borwser window is still open
        OSRenderHandler* osrHandler = browserWindows[browserIdx]->GetOSRenderHandler();
        std::string jpegFrame = osrHandler->GetJPEGFrame();
        
        frame_updated[browserIdx] = true;
        if(server_ready[browserIdx]) {
            frame_updated[browserIdx] = false;
            server_ready[browserIdx] = false;
            
            boost::property_tree::ptree emit_data;
            emit_data.put<std::string>("id", uniqueID+"|"+boost::to_string(browserIdx));
            emit_data.put<std::string>("src", jpegFrame);
            emit_data.put<std::string>("encoding", "base64");
            wsio->emit("updateMediaStreamFrame", emit_data);
        }
    }
}


/******** Auxiliary App Functions ********/
std::string AppGetWorkingDirectory() {
  return szWorkingDir;
}

void AppQuitMessageLoop() {
  CefQuitMessageLoop();
}


