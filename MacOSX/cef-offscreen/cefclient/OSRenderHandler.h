//
//  OSRenderHandler.h
//  cefclient
//
//  Created by Thomas Marrinan on 4/3/14.
//
//

#ifndef cefclient_OSRenderHandler_h
#define cefclient_OSRenderHandler_h

#include <iostream>
#include <sstream>
#include <stdlib.h>
#include <jpeglib.h>

#include "include/cef_render_handler.h"

#include <Carbon/Carbon.h>


class OSRenderHandler : public CefRenderHandler {
private:
    int m_width;
    int m_height;
    int m_browserIdx;
    void (*m_paintCallback)(int);
    std::string jpegFrame;
    
    unsigned char* BGRA_to_RGB(unsigned char* bgra, int width, int height);
    unsigned char* BGRA_to_JPEG(unsigned char* bgra, int width, int height, int quality, unsigned long* arr_length);
	char* base64_encode(unsigned char* bin, unsigned long input_length, unsigned long *output_length);
    
    unsigned int JavaScriptCodeToNativeCode(unsigned short JSCode);
    unsigned short JavaScriptCodeToUnmodifiedCharCode(unsigned short JSCode);
    
public:
    OSRenderHandler(int width, int height, int idx, void (*paint_callback)(int));
    
    void SetRenderSize(CefRefPtr<CefBrowser> browser, int width, int height);
    bool GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect);
    void OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height);
    
    std::string GetJPEGFrame();
    void PointerMove(CefRefPtr<CefBrowser> browser, int mouseX, int mouseY);
    void PointerPress(CefRefPtr<CefBrowser> browser, int mouseX, int mouseY, std::string button);
    void PointerRelease(CefRefPtr<CefBrowser> browser, int mouseX, int mouseY, std::string button);
    void PointerScroll(CefRefPtr<CefBrowser> browser, int mouseX, int mouseY, int deltaX, int deltaY);
    void SpecialKey(CefRefPtr<CefBrowser>browser, int charCode, std::string state);
    void Keyboard(CefRefPtr<CefBrowser>browser, int charCode);
    
    IMPLEMENT_REFCOUNTING(OSRenderHandler);
};

#endif
