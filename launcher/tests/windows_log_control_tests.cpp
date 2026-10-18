#define NOMINMAX
#include <Windows.h>
#include <richedit.h>
#include <stdexcept>
#include <string>

int main() {
    auto library=LoadLibraryW(L"Msftedit.dll");
    if(!library)throw std::runtime_error("RichEdit library is missing");
    auto window=CreateWindowExW(0,MSFTEDIT_CLASS,L"",
        WS_POPUP|ES_MULTILINE|ES_READONLY|WS_VSCROLL|ES_AUTOVSCROLL,
        0,0,480,240,nullptr,nullptr,GetModuleHandleW(nullptr),nullptr);
    if(!window) {
        FreeLibrary(library);
        throw std::runtime_error("Cannot instantiate RichEdit control");
    }
    SendMessageW(window,EM_EXLIMITTEXT,0,8*1024*1024);
    std::wstring text(4096,L'a');
    text+=L"\r\n";
    for(int i=0;i<64;++i) {
        CHARRANGE end{-1,-1};
        SendMessageW(window,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&end));
        SendMessageW(window,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(text.c_str()));
    }
    CHARRANGE at_end{};
    SendMessageW(window,EM_EXGETSEL,0,reinterpret_cast<LPARAM>(&at_end));
    auto count=GetWindowTextLengthW(window);
    if(count<200000 || at_end.cpMin<200000 || at_end.cpMax!=at_end.cpMin) {
        DestroyWindow(window);FreeLibrary(library);
        throw std::runtime_error("RichEdit did not retain output beyond the Win32 edit limit");
    }
    SendMessageW(window,EM_SCROLLCARET,0,0);
    SendMessageW(window,WM_VSCROLL,MAKEWPARAM(SB_BOTTOM,0),0);
    DestroyWindow(window);FreeLibrary(library);
}
