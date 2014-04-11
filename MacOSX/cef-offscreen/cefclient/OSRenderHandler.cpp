//
//  OSRenderHandler.cpp
//  cefclient
//
//  Created by Thomas Marrinan on 4/3/14.
//
//

#include "OSRenderHandler.h"

OSRenderHandler::OSRenderHandler(int width, int height, int idx, void (*paint_callback)(int)) {
    m_width = width;
    m_height = height;
    m_browserIdx = idx;
    m_paintCallback = paint_callback;
}

void OSRenderHandler::SetRenderSize(CefRefPtr<CefBrowser> browser, int width, int height) {
    m_width = width;
    m_height = height;
    
    browser->GetHost()->WasResized();
}

bool OSRenderHandler::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect &rect) {
    rect = CefRect(0, 0, m_width, m_height);
    return true;
}

void OSRenderHandler::OnPaint(CefRefPtr<CefBrowser> browser, PaintElementType type, const RectList &dirtyRects, const void *buffer, int width, int height) {
    // length has size of raw jpeg character array
    // jpeg is raw jpeg character array
    unsigned long length, base64_length;
    unsigned char* jpeg = BGRA_to_JPEG((unsigned char*)buffer, width, height, 80, &length);
    char* jpeg_base64 = base64_encode(jpeg, length, &base64_length);
    jpegFrame = std::string(jpeg_base64, base64_length);
    
    delete [] jpeg;
    delete [] jpeg_base64;
    
    m_paintCallback(m_browserIdx);
}

std::string OSRenderHandler::GetJPEGFrame() {
    return jpegFrame;
}

void OSRenderHandler::PointerMove(CefRefPtr<CefBrowser> browser, int mouseX, int mouseY) {
    CefMouseEvent mouse_event;
    mouse_event.x = mouseX;
    mouse_event.y = mouseY;
    mouse_event.modifiers = 0;
    
    browser->GetHost()->SendMouseMoveEvent(mouse_event, false);
}

void OSRenderHandler::PointerPress(CefRefPtr<CefBrowser> browser, int mouseX, int mouseY, std::string button) {
    CefMouseEvent mouse_event;
    mouse_event.x = mouseX;
    mouse_event.y = mouseY;
    mouse_event.modifiers = 0;
    
    CefBrowserHost::MouseButtonType btn;
    if(button == "left") btn = MBT_LEFT;
    if(button == "right") btn = MBT_RIGHT;
    
    browser->GetHost()->SendMouseClickEvent(mouse_event, btn, false, 1);
    browser->GetHost()->SendFocusEvent(true);
}

void OSRenderHandler::PointerRelease(CefRefPtr<CefBrowser> browser, int mouseX, int mouseY, std::string button) {
    CefMouseEvent mouse_event;
    mouse_event.x = mouseX;
    mouse_event.y = mouseY;
    mouse_event.modifiers = 0;
    
    CefBrowserHost::MouseButtonType btn;
    if(button == "left") btn = MBT_LEFT;
    if(button == "right") btn = MBT_RIGHT;
    
    browser->GetHost()->SendMouseClickEvent(mouse_event, btn, true, 1);
}

void OSRenderHandler::PointerScroll(CefRefPtr<CefBrowser> browser, int mouseX, int mouseY, int deltaX, int deltaY) {
    CefMouseEvent mouse_event;
    mouse_event.x = mouseX;
    mouse_event.y = mouseY;
    mouse_event.modifiers = 0;
    
    browser->GetHost()->SendMouseWheelEvent(mouse_event, deltaX, deltaY);
}

void OSRenderHandler::SpecialKey(CefRefPtr<CefBrowser>browser, int charCode, std::string state) {
    unsigned int nativeCode = JavaScriptCodeToNativeCode(charCode);
    unsigned short unmodCode = JavaScriptCodeToUnmodifiedCharCode(charCode);
    
    if(state == "down") {
        CefKeyEvent keyEvent;
        keyEvent.character = unmodCode;
        keyEvent.unmodified_character = unmodCode;
        keyEvent.native_key_code = nativeCode;
        keyEvent.modifiers = 0;
        keyEvent.type = KEYEVENT_KEYDOWN;
        
        printf("KeyDown: %d\n", nativeCode);
        
        browser->GetHost()->SendKeyEvent(keyEvent);
    }
    else if(state == "up") {
        CefKeyEvent keyEvent;
        keyEvent.character = unmodCode;
        keyEvent.unmodified_character = unmodCode;
        keyEvent.native_key_code = nativeCode;
        keyEvent.modifiers = 0;
        keyEvent.type = KEYEVENT_KEYUP;
        
        printf("KeyUp: %d\n", nativeCode);
        
        browser->GetHost()->SendKeyEvent(keyEvent);
    }
}

void OSRenderHandler::Keyboard(CefRefPtr<CefBrowser>browser, int charCode) {
    CefKeyEvent keyEvent;
    keyEvent.character = charCode;
    keyEvent.type = KEYEVENT_CHAR;
    
    browser->GetHost()->SendKeyEvent(keyEvent);
}


unsigned char* OSRenderHandler::BGRA_to_RGB(unsigned char* bgra, int width, int height) {
    unsigned char* rgb = new unsigned char[width*height*3];
    
    for(int x=0; x<width*height; x++) {
        rgb[3*x + 0] = bgra[4*x + 2];
        rgb[3*x + 1] = bgra[4*x + 1];
        rgb[3*x + 2] = bgra[4*x + 0];
    }
    
    return rgb;
}

unsigned char* OSRenderHandler::BGRA_to_JPEG(unsigned char* bgra, int width, int height, int quality, unsigned long* arr_length) {
    struct jpeg_compress_struct cinfo;
    struct jpeg_error_mgr jerr;
    
    JSAMPROW row_pointer[1];
    int row_stride;
    
    cinfo.err = jpeg_std_error(&jerr);
    jpeg_create_compress(&cinfo);
    
    unsigned char *jpeg_buffer_raw = NULL;
    unsigned long outbuffer_size = 0;
    jpeg_mem_dest(&cinfo, &jpeg_buffer_raw, &outbuffer_size);
    
    cinfo.image_width = width;
    cinfo.image_height = height;
    cinfo.input_components = 3;
    cinfo.in_color_space = JCS_RGB;
    
    jpeg_set_defaults(&cinfo);
    jpeg_set_quality(&cinfo, quality, TRUE);
    jpeg_start_compress(&cinfo, TRUE);
    
    row_stride = width * 4;
    
    while(cinfo.next_scanline < cinfo.image_height) {
        unsigned char* rgb = BGRA_to_RGB(&bgra[cinfo.next_scanline * row_stride], width, 1);
        row_pointer[0] = (JSAMPROW) rgb;
        jpeg_write_scanlines(&cinfo, row_pointer, 1);
        delete [] rgb;
    }
    
    jpeg_finish_compress(&cinfo);
    jpeg_destroy_compress(&cinfo);
    
    *arr_length = outbuffer_size;
    
    return jpeg_buffer_raw;
}

char* OSRenderHandler::base64_encode(unsigned char* bin, unsigned long input_length, unsigned long *output_length) {
    *output_length = 4 * ((input_length + 2) / 3);
    
    char*encoded_data = new char[*output_length];
    if (encoded_data == NULL) return NULL;
    
    const char encoding_table[] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
                                   'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',
                                   'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',
                                   'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',
                                   'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',
                                   'o', 'p', 'q', 'r', 's', 't', 'u', 'v',
                                   'w', 'x', 'y', 'z', '0', '1', '2', '3',
                                   '4', '5', '6', '7', '8', '9', '+', '/'};
    const int mod_table[] = {0, 2, 1};
    
    unsigned long i=0, j=0;
    while (i<input_length) {
        uint32_t octet_a = i < input_length ? bin[i++] : 0;
        uint32_t octet_b = i < input_length ? bin[i++] : 0;
        uint32_t octet_c = i < input_length ? bin[i++] : 0;
        
        uint32_t triple = (octet_a << 0x10) + (octet_b << 0x08) + octet_c;
        
        encoded_data[j++] = encoding_table[(triple >> 3 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 2 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 1 * 6) & 0x3F];
        encoded_data[j++] = encoding_table[(triple >> 0 * 6) & 0x3F];
    }
    
    for (int i=0; i<mod_table[input_length % 3]; i++)
        encoded_data[*output_length - 1 - i] = '=';
    
    return encoded_data;
}

unsigned int OSRenderHandler::JavaScriptCodeToNativeCode(unsigned short JSCode) {
    switch (JSCode) {
        case 8:
            return kVK_Delete;
        case 9:
            return kVK_Tab;
        case 13:
            return kVK_Return;
        case 16:
            return kVK_Shift;
        case 17:
            return kVK_Control;
        case 18:
            return kVK_Option;
        case 20:
            return kVK_CapsLock;
        case 27:
            return kVK_Escape;
        case 32:
            return kVK_Space;
        case 33:
            return kVK_PageUp;
        case 34:
            return kVK_PageDown;
        case 35:
            return kVK_End;
        case 36:
            return kVK_Home;
        case 37:
            return kVK_LeftArrow;
        case 38:
            return kVK_UpArrow;
        case 39:
            return kVK_RightArrow;
        case 40:
            return kVK_DownArrow;
        case 46:
            return kVK_ForwardDelete;
        case 48:
            return kVK_ANSI_0;
        case 49:
            return kVK_ANSI_1;
        case 50:
            return kVK_ANSI_2;
        case 51:
            return kVK_ANSI_3;
        case 52:
            return kVK_ANSI_4;
        case 53:
            return kVK_ANSI_5;
        case 54:
            return kVK_ANSI_6;
        case 55:
            return kVK_ANSI_7;
        case 56:
            return kVK_ANSI_8;
        case 57:
            return kVK_ANSI_9;
        case 65:
            return kVK_ANSI_A;
        case 66:
            return kVK_ANSI_B;
        case 67:
            return kVK_ANSI_C;
        case 68:
            return kVK_ANSI_D;
        case 69:
            return kVK_ANSI_E;
        case 70:
            return kVK_ANSI_F;
        case 71:
            return kVK_ANSI_G;
        case 72:
            return kVK_ANSI_H;
        case 73:
            return kVK_ANSI_I;
        case 74:
            return kVK_ANSI_J;
        case 75:
            return kVK_ANSI_K;
        case 76:
            return kVK_ANSI_L;
        case 77:
            return kVK_ANSI_M;
        case 78:
            return kVK_ANSI_N;
        case 79:
            return kVK_ANSI_O;
        case 80:
            return kVK_ANSI_P;
        case 81:
            return kVK_ANSI_Q;
        case 82:
            return kVK_ANSI_R;
        case 83:
            return kVK_ANSI_S;
        case 84:
            return kVK_ANSI_T;
        case 85:
            return kVK_ANSI_U;
        case 86:
            return kVK_ANSI_V;
        case 87:
            return kVK_ANSI_W;
        case 88:
            return kVK_ANSI_X;
        case 89:
            return kVK_ANSI_Y;
        case 90:
            return kVK_ANSI_Z;
        case 91:
            return kVK_Command;
        case 93:
            return kVK_Command;
        case 96:
            return kVK_ANSI_Keypad0;
        case 97:
            return kVK_ANSI_Keypad1;
        case 98:
            return kVK_ANSI_Keypad2;
        case 99:
            return kVK_ANSI_Keypad3;
        case 100:
            return kVK_ANSI_Keypad4;
        case 101:
            return kVK_ANSI_Keypad5;
        case 102:
            return kVK_ANSI_Keypad6;
        case 103:
            return kVK_ANSI_Keypad7;
        case 104:
            return kVK_ANSI_Keypad8;
        case 105:
            return kVK_ANSI_Keypad9;
        case 106:
            return kVK_ANSI_KeypadMultiply;
        case 107:
            return kVK_ANSI_KeypadPlus;
        case 109:
            return kVK_ANSI_KeypadMinus;
        case 110:
            return kVK_ANSI_KeypadDecimal;
        case 111:
            return kVK_ANSI_KeypadDivide;
        case 112:
            return kVK_F1;
        case 113:
            return kVK_F2;
        case 114:
            return kVK_F3;
        case 115:
            return kVK_F4;
        case 116:
            return kVK_F5;
        case 117:
            return kVK_F6;
        case 118:
            return kVK_F7;
        case 119:
            return kVK_F8;
        case 120:
            return kVK_F9;
        case 121:
            return kVK_F10;
        case 122:
            return kVK_F11;
        case 123:
            return kVK_F12;
        case 186:
            return kVK_ANSI_Semicolon;
        case 187:
            return kVK_ANSI_Equal;
        case 188:
            return kVK_ANSI_Comma;
        case 189:
            return kVK_ANSI_Minus;
        case 190:
            return kVK_ANSI_Period;
        case 191:
            return kVK_ANSI_Slash;
        case 192:
            return kVK_ANSI_Grave;
        case 219:
            return kVK_ANSI_LeftBracket;
        case 220:
            return kVK_ANSI_Backslash;
        case 221:
            return kVK_ANSI_RightBracket;
        case 222:
            return kVK_ANSI_Quote;
        default:
            return 0;
    }
    return 0;
}

unsigned short OSRenderHandler::JavaScriptCodeToUnmodifiedCharCode(unsigned short JSCode) {
    switch (JSCode) {
        case 8:
            return kBackspaceCharCode;
        case 9:
            return kTabCharCode;
        case 13:
            return kReturnCharCode;
        case 16:
            return 0;
        case 17:
            return 0;
        case 18:
            return 0;
        case 20:
            return 0;
        case 27:
            return kEscapeCharCode;
        case 32:
            return kSpaceCharCode;
        case 33:
            return 65535; // NSPageUpFunctionKey
        case 34:
            return 65535; // NSPageDownFunctionKey
        case 35:
            return 65535; // NSEndFunctionKey
        case 36:
            return 65535; // NSHomeFunctionKey
        case 37:
            return 65535; // NSLeftArrowFunctionKey
        case 38:
            return 65535; // NSUpArrowFunctionKey
        case 39:
            return 65535; // NSRightArrowFunctionKey
        case 40:
            return 65535; // NSDownArrowFunctionKey
        case 46:
            return kDeleteCharCode;
        case 48:
            return '0';
        case 49:
            return '1';
        case 50:
            return '2';
        case 51:
            return '3';
        case 52:
            return '4';
        case 53:
            return '5';
        case 54:
            return '6';
        case 55:
            return '7';
        case 56:
            return '8';
        case 57:
            return '9';
        case 65:
            return 'a';
        case 66:
            return 'b';
        case 67:
            return 'c';
        case 68:
            return 'd';
        case 69:
            return 'e';
        case 70:
            return 'f';
        case 71:
            return 'g';
        case 72:
            return 'h';
        case 73:
            return 'i';
        case 74:
            return 'j';
        case 75:
            return 'k';
        case 76:
            return 'l';
        case 77:
            return 'm';
        case 78:
            return 'n';
        case 79:
            return 'o';
        case 80:
            return 'p';
        case 81:
            return 'q';
        case 82:
            return 'r';
        case 83:
            return 's';
        case 84:
            return 't';
        case 85:
            return 'u';
        case 86:
            return 'v';
        case 87:
            return 'w';
        case 88:
            return 'x';
        case 89:
            return 'y';
        case 90:
            return 'z';
        case 91:
            return 0;
        case 93:
            return 0;
        case 96:
            return '0';
        case 97:
            return '1';
        case 98:
            return '2';
        case 99:
            return '3';
        case 100:
            return '4';
        case 101:
            return '5';
        case 102:
            return '6';
        case 103:
            return '7';
        case 104:
            return '8';
        case 105:
            return '9';
        case 106:
            return '*';
        case 107:
            return '+';
        case 109:
            return '-';
        case 110:
            return '.';
        case 111:
            return '/';
        case 112:
            return 65535; // NSF1FunctionKey
        case 113:
            return 65535; // NSF2FunctionKey
        case 114:
            return 65535; // NSF3FunctionKey
        case 115:
            return 65535; // NSF4FunctionKey
        case 116:
            return 65535; // NSF5FunctionKey
        case 117:
            return 65535; // NSF6FunctionKey
        case 118:
            return 65535; // NSF7FunctionKey
        case 119:
            return 65535; // NSF8FunctionKey
        case 120:
            return 65535; // NSF9FunctionKey
        case 121:
            return 65535; // NSF10FunctionKey
        case 122:
            return 65535; // NSF11FunctionKey
        case 123:
            return 65535; // NSF12FunctionKey
        case 186:
            return ';';
        case 187:
            return '=';
        case 188:
            return ',';
        case 189:
            return '-';
        case 190:
            return '.';
        case 191:
            return '/';
        case 192:
            return '`';
        case 219:
            return '[';
        case 220:
            return '\\';
        case 221:
            return ']';
        case 222:
            return '\'';
        default:
            return 0;
    }
    return 0;
}
