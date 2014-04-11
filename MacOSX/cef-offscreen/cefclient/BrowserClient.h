//
//  BrowserClient.h
//  cefclient
//
//  Created by Thomas Marrinan on 4/7/14.
//
//

#ifndef cefclient_BrowserClient_h
#define cefclient_BrowserClient_h

#include "util.h"
#include "include/cef_client.h"
#include "include/cef_life_span_handler.h"

#include "OSRenderHandler.h"

class BrowserClient : public CefClient, public CefLifeSpanHandler {
private:
    OSRenderHandler* m_osrenderHandler;
    CefRefPtr<CefRenderHandler> m_renderHandler;
    CefRefPtr<CefBrowser> m_Browser;
    int m_BrowserId;
    int m_BrowserCount;
    bool m_bIsClosing;
	
public:
    BrowserClient(OSRenderHandler* renderHandler);
    
    OSRenderHandler* GetOSRenderHandler();
    virtual CefRefPtr<CefRenderHandler> GetRenderHandler();
    virtual CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() OVERRIDE;
    
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) OVERRIDE;
    bool DoClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) OVERRIDE;
    
    CefRefPtr<CefBrowser> GetBrower();
    bool IsClosing();
    
    IMPLEMENT_REFCOUNTING(BrowserClient);
    IMPLEMENT_LOCKING(BrowserClient);
};

#endif
