#ifdef _WIN32
#define NOMINMAX
#include "gui.hpp"
#include "gui_log.hpp"
#include <Windows.h>
#include <ShObjIdl.h>
#include <shellapi.h>
#include <commctrl.h>
#include <richedit.h>
#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>
#include <cwchar>

namespace spl {
namespace {
enum Control : int {
    IdRom=110,IdRomFile,IdRomFolder,IdOutput,IdOutputBrowse,
    IdJava,IdJavaBrowse,IdJwud,IdJwudBrowse,IdAutoLaunch,
    IdInstall,IdValidate,IdLaunch,IdCancel,IdStatus,IdLog,IdProgress,IdOpenLog,IdControls
};
std::wstring wide(const std::string& s) {
    if(s.empty())return {};
    int n=MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),nullptr,0);
    std::wstring w(n,L'\0');
    MultiByteToWideChar(CP_UTF8,0,s.data(),int(s.size()),w.data(),n);
    return w;
}
std::wstring value(HWND control) {
    int n=GetWindowTextLengthW(control);
    std::wstring out(n+1,L'\0');
    GetWindowTextW(control,out.data(),n+1);
    out.resize(n);return out;
}
std::wstring choose(HWND owner,bool folder,const std::wstring& prior) {
    IFileOpenDialog* dlg=nullptr;
    auto hr=CoCreateInstance(CLSID_FileOpenDialog,nullptr,CLSCTX_INPROC_SERVER,IID_PPV_ARGS(&dlg));
    if(FAILED(hr))return {};
    DWORD flags=0;dlg->GetOptions(&flags);
    dlg->SetOptions(flags|FOS_FORCEFILESYSTEM|(folder?FOS_PICKFOLDERS:0));
    dlg->SetTitle(folder?L"Select directory":L"Select game dump or executable");
    if(!folder) {
        COMDLG_FILTERSPEC filters[]={{L"Game dumps and tools",L"*.zip;*.wud;*.wux;*.exe;*.jar"},
                                    {L"All files",L"*.*"}};
        dlg->SetFileTypes(2,filters);
    }
    if(!prior.empty()) {
        auto directory=std::filesystem::path(prior);
        if(!folder)directory=directory.parent_path();
        IShellItem* item=nullptr;
        if(!directory.empty()&&SUCCEEDED(SHCreateItemFromParsingName(directory.c_str(),nullptr,IID_PPV_ARGS(&item)))) {
            dlg->SetFolder(item);item->Release();
        }
    }
    std::wstring result;
    if(SUCCEEDED(dlg->Show(owner))) {
        IShellItem* item=nullptr;
        if(SUCCEEDED(dlg->GetResult(&item))) {
            PWSTR str=nullptr;
            if(SUCCEEDED(item->GetDisplayName(SIGDN_FILESYSPATH,&str))) {
                result=str;CoTaskMemFree(str);
            }
            item->Release();
        }
    }
    dlg->Release();return result;
}

struct WinWindow {
    Options opts;
    SetupSession session;
    HWND window=nullptr;
    HFONT font=nullptr;
    HMODULE rich_edit_module=nullptr;
    HWND rom=nullptr,out=nullptr,java=nullptr,jwud=nullptr,autolaunch=nullptr;
    HWND heading=nullptr,subtitle=nullptr,rom_label=nullptr,output_label=nullptr,java_label=nullptr,jwud_label=nullptr;
    HWND install=nullptr,validate=nullptr,launch=nullptr,cancel=nullptr,status=nullptr,log=nullptr,progress=nullptr,open_log=nullptr,controls=nullptr;
    std::wstring transcript;
    bool last_validation=false;
    int displayed_percent=-1;
    explicit WinWindow(Options o,const fs::path& exe):opts(std::move(o)),session(exe){}
    ~WinWindow(){if(font)DeleteObject(font);if(rich_edit_module)FreeLibrary(rich_edit_module);}
    HWND ctrl(DWORD ex,const wchar_t* kind,const wchar_t* title,DWORD style,int id=0) {
        HWND h=CreateWindowExW(ex,kind,title,WS_CHILD|WS_VISIBLE|style,0,0,20,20,
                               window,reinterpret_cast<HMENU>(static_cast<INT_PTR>(id)),GetModuleHandleW(nullptr),nullptr);
        SendMessageW(h,WM_SETFONT,reinterpret_cast<WPARAM>(font),TRUE);return h;
    }
    HWND label(const wchar_t* str) {return ctrl(0,L"STATIC",str,SS_LEFT);}
    HWND entry(const fs::path& path,int id) {
        HWND h=ctrl(WS_EX_CLIENTEDGE,L"EDIT",path.c_str(),WS_TABSTOP|ES_AUTOHSCROLL,id);
        return h;
    }
    HWND button(const wchar_t* str,int id) {return ctrl(0,L"BUTTON",str,WS_TABSTOP|BS_PUSHBUTTON,id);}
    HWND make_window(HINSTANCE instance) {
        WNDCLASSEXW c{sizeof(c)};
        c.lpfnWndProc=window_proc;c.hInstance=instance;c.hCursor=LoadCursorW(nullptr,MAKEINTRESOURCEW(32512));
        c.hbrBackground=reinterpret_cast<HBRUSH>(COLOR_WINDOW+1);
        c.lpszClassName=L"SplatoonPortSetupWindow";
        RegisterClassExW(&c);
        window=CreateWindowExW(0,c.lpszClassName,L"Splatoon Port - Setup",WS_OVERLAPPEDWINDOW,
                               CW_USEDEFAULT,CW_USEDEFAULT,880,740,nullptr,nullptr,instance,this);
        if(!window)throw std::runtime_error("Unable to open Windows setup window");
        ShowWindow(window,SW_SHOWDEFAULT);UpdateWindow(window);return window;
    }
    void initialize() {
        rich_edit_module=LoadLibraryW(L"Msftedit.dll");
        INITCOMMONCONTROLSEX common{sizeof(common),ICC_PROGRESS_CLASS};
        InitCommonControlsEx(&common);
        font=CreateFontW(-17,0,0,0,FW_NORMAL,0,0,0,DEFAULT_CHARSET,0,0,CLEARTYPE_QUALITY,
                         DEFAULT_PITCH|FF_DONTCARE,L"Segoe UI");
        heading=label(L"SPLATOON 1 / PC PORT");
        subtitle=label(L"Select your game dump below. Game-derived code stays on your computer.");
        rom_label=label(L"Game dump (source: raw folder, ZIP, WUD, or WUX)");
        rom=entry(opts.rom,IdRom);
        button(L"Choose file",IdRomFile);button(L"Choose folder",IdRomFolder);
        output_label=label(L"Installation directory (destination for compiled game)");
        out=entry(opts.output,IdOutput);button(L"Browse",IdOutputBrowse);
        java_label=label(L"Java executable (for WUD/WUX only)");
        java=entry(opts.java,IdJava);button(L"Browse",IdJavaBrowse);
        jwud_label=label(L"JWUDTool.jar (for WUD/WUX only)");
        jwud=entry(opts.jwud,IdJwud);button(L"Browse",IdJwudBrowse);
        autolaunch=ctrl(0,L"BUTTON",L"Launch game after installation",BS_AUTOCHECKBOX|WS_TABSTOP,IdAutoLaunch);
        SendMessageW(autolaunch,BM_SETCHECK,opts.no_launch?BST_UNCHECKED:BST_CHECKED,0);
        install=button(L"Install / Rebuild",IdInstall);
        validate=button(L"Validate dump",IdValidate);
        launch=button(L"Launch installed game",IdLaunch);
        cancel=button(L"Cancel",IdCancel);
        open_log=button(L"Open full log",IdOpenLog);
        controls=button(L"Controls...",IdControls);
        progress=ctrl(0,PROGRESS_CLASSW,L"",PBS_MARQUEE,IdProgress);
        SendMessageW(progress,PBM_SETRANGE32,0,100);
        status=label(L"Ready to set up.");
        log=ctrl(WS_EX_CLIENTEDGE,rich_edit_module?MSFTEDIT_CLASS:L"EDIT",
                 L"No game files or generated code are included in the release.\r\n",
                 ES_MULTILINE|ES_READONLY|WS_VSCROLL|ES_AUTOVSCROLL|ES_NOHIDESEL,IdLog);
        if(rich_edit_module) {
            SendMessageW(log,EM_EXLIMITTEXT,0,static_cast<LPARAM>(max_gui_log_chars));
            SendMessageW(log,EM_SETOPTIONS,ECOOP_OR,ECO_AUTOVSCROLL);
        } else {
            SendMessageW(log,EM_SETLIMITTEXT,static_cast<WPARAM>(max_gui_log_chars),0);
        }
        resize();update_controls();SetTimer(window,1,120,nullptr);
    }
    void resize() {
        if(!window||!rom||!heading||!subtitle||!rom_label||!output_label||!java_label||!jwud_label)return;
        RECT rc{};GetClientRect(window,&rc);
        int width=std::max(500,int(rc.right)-48);
        int x=24,y=19;
        auto move=[&](HWND h,int left,int top,int w,int ht){MoveWindow(h,left,top,w,ht,TRUE);};
        move(heading,x,y,width,32);y+=35;
        move(subtitle,x,y,width,22);y+=41;
        auto field=[&](HWND field_label,HWND e,HWND a,HWND b) {
            move(field_label,x,y,width,21);y+=24;
            int bwidth=b?108:0;
            int awidth=108;
            move(e,x,y,width-awidth-bwidth-16,30);
            move(a,x+width-awidth-bwidth-8,y,awidth,30);
            if(b)move(b,x+width-bwidth,y,bwidth,30);
            y+=49;
        };
        field(rom_label,rom,GetDlgItem(window,IdRomFile),GetDlgItem(window,IdRomFolder));
        field(output_label,out,GetDlgItem(window,IdOutputBrowse),nullptr);
        field(java_label,java,GetDlgItem(window,IdJavaBrowse),nullptr);
        field(jwud_label,jwud,GetDlgItem(window,IdJwudBrowse),nullptr);
        move(autolaunch,x,y,width,23);y+=38;
        int bw=128;
        move(install,x,y,bw,33);move(validate,x+bw+8,y,bw,33);
        move(launch,x+2*(bw+8),y,165,33);
        move(cancel,x+2*(bw+8)+173,y,90,33);
        move(open_log,x+2*(bw+8)+271,y,145,33);
        move(controls,x+2*(bw+8)+424,y,102,33);
        y+=47;move(progress,x,y,width,12);y+=24;
        move(status,x,y,width,22);y+=30;
        move(log,x,y,width,std::max(75,int(rc.bottom)-y-20));
    }
    void update_controls() {
        bool busy=session.running();
        EnableWindow(install,!busy);EnableWindow(validate,!busy);
        EnableWindow(launch,!busy && session.can_launch(fs::path(value(out))));
        EnableWindow(cancel,busy);
        EnableWindow(controls,!busy && !value(out).empty());
        EnableWindow(open_log,!session.full_log_path().empty()&&fs::is_regular_file(session.full_log_path()));
        for(auto h:{rom,out,java,jwud,autolaunch,GetDlgItem(window,IdRomFile),GetDlgItem(window,IdRomFolder),
                    GetDlgItem(window,IdOutputBrowse),GetDlgItem(window,IdJavaBrowse),GetDlgItem(window,IdJwudBrowse)})
            EnableWindow(h,!busy);
    }
    void set_status(const std::wstring& msg){SetWindowTextW(status,msg.c_str());}
    void begin(bool verify) {
        opts.rom=fs::path(value(rom));opts.output=fs::path(value(out));
        opts.java=fs::path(value(java));opts.jwud=fs::path(value(jwud));
        if(opts.rom.empty()||opts.output.empty()) {set_status(L"Please choose a dump and destination.");return;}
        if(!fs::exists(opts.rom)) {set_status(L"The selected dump does not exist.");return;}
        last_validation=verify;
        transcript.clear();SetWindowTextW(log,L"");
        try {session.start(opts,verify);}
        catch(const std::exception& e) {set_status(wide(e.what()));return;}
        displayed_percent=-1;
        SendMessageW(progress,PBM_SETPOS,0,0);
        SendMessageW(progress,PBM_SETMARQUEE,TRUE,30);
        set_status(verify?L"Validating game dump...":L"Setting up... see live output below.");
        update_controls();
    }
    void open_full_log() {
        const auto& path=session.full_log_path();
        if(path.empty()||!fs::is_regular_file(path)) {
            set_status(L"No setup log is available yet.");
            return;
        }
        HINSTANCE result=ShellExecuteW(window,L"open",path.c_str(),nullptr,nullptr,SW_SHOWNORMAL);
        if(reinterpret_cast<INT_PTR>(result)<=32)set_status(L"Could not open the full setup log.");
    }
    void scroll_log_to_end() {
        if(rich_edit_module) {
            CHARRANGE range{-1,-1};
            SendMessageW(log,EM_EXSETSEL,0,reinterpret_cast<LPARAM>(&range));
        } else SendMessageW(log,EM_SETSEL,static_cast<WPARAM>(-1),static_cast<LPARAM>(-1));
        SendMessageW(log,EM_SCROLLCARET,0,0);
        SendMessageW(log,WM_VSCROLL,MAKEWPARAM(SB_BOTTOM,0),0);
    }
    void configure_controls() {
        const fs::path destination=value(out);
        if(destination.empty()) {set_status(L"Select the installation directory first.");return;}
        if(open_controls_editor(destination))set_status(L"Controls saved. Restart the game to apply bindings.");
        else set_status(L"Controls editor closed without saving.");
    }
    void launch_game() {
        fs::path directory=value(out);
        auto binary=directory/L"Splatoon.exe";
        if(!fs::is_regular_file(binary)) {set_status(L"No installed game found.");return;}
        HINSTANCE result=ShellExecuteW(window,L"open",binary.c_str(),nullptr,directory.c_str(),SW_SHOWNORMAL);
        if(reinterpret_cast<INT_PTR>(result)<=32)set_status(L"Failed to start installed game.");
        else set_status(L"Launched installed game.");
    }
    void tick() {
        std::string part;int result=0;
        SetupProgress update;
        bool ended=session.consume(part,result,&update);
        if(!part.empty()) {
            if(part.find("Downloading")!=std::string::npos)set_status(L"Downloading required tools...");
            else if(part.find("Validated supported")!=std::string::npos)set_status(L"Game dump verified.");
            else if(part.find("recompile")!=std::string::npos)set_status(L"Generating native code...");
            auto text=windows_log_newlines(wide(part));
            bool trimmed=append_gui_log(transcript,text);
            if(trimmed) {
                SetWindowTextW(log,transcript.c_str());
            } else {
                scroll_log_to_end();
                SendMessageW(log,EM_REPLACESEL,FALSE,reinterpret_cast<LPARAM>(text.c_str()));
            }
            scroll_log_to_end();
        }
        if(!update.stage.empty()) {
            displayed_percent=update.percent;
            if(displayed_percent>=0) {
                SendMessageW(progress,PBM_SETMARQUEE,FALSE,0);
                SendMessageW(progress,PBM_SETPOS,displayed_percent,0);
                set_status(wide(update.stage+" ("+std::to_string(displayed_percent)+"%)"));
            } else {
                SendMessageW(progress,PBM_SETMARQUEE,TRUE,30);
                set_status(wide(update.stage+"..."));
            }
        }
        if(ended) {
            SendMessageW(progress,PBM_SETMARQUEE,FALSE,0);
            SendMessageW(progress,PBM_SETPOS,result==0?100:0,0);
            set_status(result==0?L"Operation completed successfully.":L"Setup failed or was cancelled. Review the log.");
            update_controls();
            if(result==0 && !last_validation && session.can_launch(fs::path(value(out))) &&
               SendMessageW(autolaunch,BM_GETCHECK,0,0)==BST_CHECKED)launch_game();
        }
        if(GetEnvironmentVariableW(L"SPLATOON_GUI_SMOKE_TEST",nullptr,0)>0)DestroyWindow(window);
    }
    void choose_for(HWND field,bool folder) {
        auto selected=choose(window,folder,value(field));
        if(!selected.empty())SetWindowTextW(field,selected.c_str());
    }
    LRESULT message(UINT id,WPARAM wparam,LPARAM lparam) {
        switch(id) {
        case WM_CREATE:initialize();return 0;
        case WM_SIZE:resize();return 0;
        case WM_TIMER:tick();return 0;
        case WM_COMMAND:
            if(HIWORD(wparam)==EN_CHANGE && LOWORD(wparam)==IdOutput) {
                if(out && launch)update_controls();
                return 0;
            }
            if(HIWORD(wparam)!=BN_CLICKED)return 0;
            switch(LOWORD(wparam)) {
            case IdRomFile:choose_for(rom,false);break;
            case IdRomFolder:choose_for(rom,true);break;
            case IdOutputBrowse:choose_for(out,true);update_controls();break;
            case IdJavaBrowse:choose_for(java,false);break;
            case IdJwudBrowse:choose_for(jwud,false);break;
            case IdInstall:begin(false);break;
            case IdValidate:begin(true);break;
            case IdLaunch:launch_game();break;
            case IdCancel:session.cancel();set_status(L"Cancelling setup...");break;
            case IdOpenLog:open_full_log();break;
            case IdControls:configure_controls();break;
            }
            return 0;
        case WM_GETMINMAXINFO: {
            auto* info=reinterpret_cast<MINMAXINFO*>(lparam);
            info->ptMinTrackSize={760,640};return 0;
        }
        case WM_CLOSE:session.cancel();DestroyWindow(window);return 0;
        case WM_DESTROY:KillTimer(window,1);PostQuitMessage(0);return 0;
        }
        return DefWindowProcW(window,id,wparam,lparam);
    }
    static LRESULT CALLBACK window_proc(HWND h,UINT msg,WPARAM wparam,LPARAM lparam) {
        auto* window=reinterpret_cast<WinWindow*>(GetWindowLongPtrW(h,GWLP_USERDATA));
        if(msg==WM_NCCREATE) {
            auto* data=reinterpret_cast<CREATESTRUCTW*>(lparam);
            window=static_cast<WinWindow*>(data->lpCreateParams);
            SetWindowLongPtrW(h,GWLP_USERDATA,reinterpret_cast<LONG_PTR>(window));
            window->window=h;
        }
        return window?window->message(msg,wparam,lparam):DefWindowProcW(h,msg,wparam,lparam);
    }
};
}
int run_gui(const Options& options,const fs::path& executable) {
    HRESULT initialized=CoInitializeEx(nullptr,COINIT_APARTMENTTHREADED);
    try {
        WinWindow win(options,executable);
        win.make_window(GetModuleHandleW(nullptr));
        MSG msg{};
        while(GetMessageW(&msg,nullptr,0,0)>0) {
            TranslateMessage(&msg);DispatchMessageW(&msg);
        }
        if(SUCCEEDED(initialized))CoUninitialize();
        return int(msg.wParam);
    } catch(...) {
        if(SUCCEEDED(initialized))CoUninitialize();
        throw;
    }
}
}
#endif
