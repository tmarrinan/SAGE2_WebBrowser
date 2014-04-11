//
//  BrowserClient.cpp
//  cefclient
//
//  Created by Thomas Marrinan on 4/7/14.
//
//

#include "BrowserClient.h"

BrowserClient::BrowserClient(OSRenderHandler* renderHandler) {
    m_osrenderHandler = renderHandler;
    m_renderHandler = renderHandler;
}

OSRenderHandler* BrowserClient::GetOSRenderHandler() {
    return m_osrenderHandler;
}

CefRefPtr<CefRenderHandler> BrowserClient::GetRenderHandler() {
    return m_renderHandler;
}

CefRefPtr<CefLifeSpanHandler> BrowserClient::GetLifeSpanHandler() {
    return this;
}

void BrowserClient::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    // Must be executed on the UI thread.
    REQUIRE_UI_THREAD();
    // Protect data members from access on multiple threads.
    AutoLock lock_scope(this);
    
    if (!m_Browser.get())   {
        // Keep a reference to the main browser.
        m_Browser = browser;
        m_BrowserId = browser->GetIdentifier();
    }
    
    // Keep track of how many browsers currently exist.
    m_BrowserCount++;
}

bool BrowserClient::DoClose(CefRefPtr<CefBrowser> browser) {
    // Must be executed on the UI thread.
    REQUIRE_UI_THREAD();
    // Protect data members from access on multiple threads.
    AutoLock lock_scope(this);
    
    // Closing the main window requires special handling. See the DoClose()
    // documentation in the CEF header for a detailed description of this
    // process.
    if (m_BrowserId == browser->GetIdentifier()) {
        // Notify the browser that the parent window is about to close.
        browser->GetHost()->ParentWindowWillClose();
        
        // Set a flag to indicate that the window close should be allowed.
        m_bIsClosing = true;
    }
    
    // Allow the close. For windowed browsers this will result in the OS close
    // event being sent.
    return false;
}

void BrowserClient::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    // Must be executed on the UI thread.
    REQUIRE_UI_THREAD();
    // Protect data members from access on multiple threads.
    AutoLock lock_scope(this);
    
    if (m_BrowserId == browser->GetIdentifier()) {
        // Free the browser pointer so that the browser can be destroyed.
        m_Browser = NULL;
    }
}

CefRefPtr<CefBrowser> BrowserClient::GetBrower() {
    return m_Browser;
}

bool BrowserClient::IsClosing() {
    return m_bIsClosing;
}
