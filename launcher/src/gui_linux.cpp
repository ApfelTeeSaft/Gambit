#ifndef _WIN32
#include "gui.hpp"
#include "gui_log.hpp"
#include <dlfcn.h>
#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>

namespace spl {
namespace {
using Widget=void;
using Boolean=int;
using SignalHandler=void(*)(Widget*,void*);
struct GtkApi {
    void* handle=nullptr;
#define G(F, RET, ...) RET (*F)(__VA_ARGS__)=nullptr
    G(gtk_init_check,Boolean,int*,char***);
    G(gtk_window_new,Widget*,int);
    G(gtk_window_set_title,void,Widget*,const char*);
    G(gtk_window_set_default_size,void,Widget*,int,int);
    G(gtk_window_set_resizable,void,Widget*,Boolean);
    G(gtk_container_add,void,Widget*,Widget*);
    G(gtk_container_set_border_width,void,Widget*,unsigned);
    G(gtk_box_new,Widget*,int,int);
    G(gtk_box_pack_start,void,Widget*,Widget*,Boolean,Boolean,unsigned);
    G(gtk_box_pack_end,void,Widget*,Widget*,Boolean,Boolean,unsigned);
    G(gtk_label_new,Widget*,const char*);
    G(gtk_label_set_xalign,void,Widget*,float);
    G(gtk_label_set_line_wrap,void,Widget*,Boolean);
    G(gtk_label_set_text,void,Widget*,const char*);
    G(gtk_entry_new,Widget*);
    G(gtk_entry_get_text,const char*,Widget*);
    G(gtk_entry_set_text,void,Widget*,const char*);
    G(gtk_button_new_with_label,Widget*,const char*);
    G(gtk_check_button_new_with_label,Widget*,const char*);
    G(gtk_toggle_button_get_active,Boolean,Widget*);
    G(gtk_toggle_button_set_active,void,Widget*,Boolean);
    G(gtk_widget_show_all,void,Widget*);
    G(gtk_widget_set_sensitive,void,Widget*,Boolean);
    G(gtk_widget_set_size_request,void,Widget*,int,int);
    G(gtk_scrolled_window_new,Widget*,Widget*,Widget*);
    G(gtk_scrolled_window_set_policy,void,Widget*,int,int);
    G(gtk_text_view_new,Widget*);
    G(gtk_text_view_get_buffer,Widget*,Widget*);
    G(gtk_text_view_set_editable,void,Widget*,Boolean);
    G(gtk_text_view_set_cursor_visible,void,Widget*,Boolean);
    G(gtk_text_view_set_wrap_mode,void,Widget*,int);
    G(gtk_text_buffer_set_text,void,Widget*,const char*,int);
    G(gtk_text_buffer_insert_at_cursor,void,Widget*,const char*,int);
    G(gtk_text_buffer_get_insert,Widget*,Widget*);
    G(gtk_text_view_scroll_mark_onscreen,void,Widget*,Widget*);
    G(gtk_progress_bar_new,Widget*);
    G(gtk_progress_bar_pulse,void,Widget*);
    G(gtk_progress_bar_set_fraction,void,Widget*,double);
    G(gtk_file_chooser_dialog_new,Widget*,const char*,Widget*,int,...);
    G(gtk_dialog_run,int,Widget*);
    G(gtk_file_chooser_get_filename,char*,Widget*);
    G(gtk_file_chooser_set_filename,Boolean,Widget*,const char*);
    G(gtk_widget_destroy,void,Widget*);
    G(gtk_main,void);
    G(gtk_main_quit,void);
    G(g_signal_connect_data,unsigned long,void*,const char*,void*,void*,void*,int);
    G(g_timeout_add,unsigned,unsigned,Boolean(*)(void*),void*);
    G(g_free,void,void*);
#undef G
    GtkApi() {
        handle=dlopen("libgtk-3.so.0",RTLD_NOW|RTLD_LOCAL);
        if(!handle) throw std::runtime_error("GTK 3 runtime is missing (install libgtk-3-0 or gtk3)");
#define LOAD(F) F=reinterpret_cast<decltype(F)>(dlsym(handle,#F)); if(!F) throw std::runtime_error("GTK 3 API unavailable: " #F)
        LOAD(gtk_init_check);LOAD(gtk_window_new);LOAD(gtk_window_set_title);
        LOAD(gtk_window_set_default_size);LOAD(gtk_window_set_resizable);
        LOAD(gtk_container_add);LOAD(gtk_container_set_border_width);
        LOAD(gtk_box_new);LOAD(gtk_box_pack_start);LOAD(gtk_box_pack_end);
        LOAD(gtk_label_new);LOAD(gtk_label_set_xalign);LOAD(gtk_label_set_line_wrap);LOAD(gtk_label_set_text);
        LOAD(gtk_entry_new);LOAD(gtk_entry_get_text);LOAD(gtk_entry_set_text);
        LOAD(gtk_button_new_with_label);LOAD(gtk_check_button_new_with_label);
        LOAD(gtk_toggle_button_get_active);LOAD(gtk_toggle_button_set_active);
        LOAD(gtk_widget_show_all);LOAD(gtk_widget_set_sensitive);LOAD(gtk_widget_set_size_request);
        LOAD(gtk_scrolled_window_new);LOAD(gtk_scrolled_window_set_policy);
        LOAD(gtk_text_view_new);LOAD(gtk_text_view_get_buffer);
        LOAD(gtk_text_view_set_editable);LOAD(gtk_text_view_set_cursor_visible);
        LOAD(gtk_text_view_set_wrap_mode);LOAD(gtk_text_buffer_set_text);
        LOAD(gtk_text_buffer_insert_at_cursor);
        LOAD(gtk_text_buffer_get_insert);LOAD(gtk_text_view_scroll_mark_onscreen);
        LOAD(gtk_progress_bar_new);
        LOAD(gtk_progress_bar_pulse);LOAD(gtk_progress_bar_set_fraction);
        LOAD(gtk_file_chooser_dialog_new);LOAD(gtk_dialog_run);
        LOAD(gtk_file_chooser_get_filename);LOAD(gtk_file_chooser_set_filename);
        LOAD(gtk_widget_destroy);LOAD(gtk_main);LOAD(gtk_main_quit);
        LOAD(g_signal_connect_data);LOAD(g_timeout_add);LOAD(g_free);
#undef LOAD
    }
    ~GtkApi(){if(handle) dlclose(handle);}
};

struct Window {
    GtkApi g;
    Options opts;
    SetupSession session;
    Widget *win=nullptr,*rom=nullptr,*out=nullptr,*java=nullptr,*jwud=nullptr;
    Widget *status=nullptr,*progress=nullptr,*log=nullptr,*start=nullptr,*verify=nullptr,*launch=nullptr,*cancel=nullptr;
    Widget *launch_when_done=nullptr;
    bool exit_requested=false;
    bool installed=false;
    bool last_validation=false;
    int displayed_percent=-1;
    std::string transcript;
    explicit Window(Options o,const fs::path& exe):opts(std::move(o)),session(exe) {
        if(!g.gtk_init_check(nullptr,nullptr)) throw std::runtime_error("No graphical Linux session found (use --cli for headless mode)");
        if(opts.output.empty()) opts.output=opts.repository/"Game";
        installed=session.can_launch(opts.output);
        build();
    }
    void signal(Widget* w,const char* name,SignalHandler cb) {
        g.g_signal_connect_data(w,name,reinterpret_cast<void*>(cb),this,nullptr,0);
    }
    Widget* text(const char* value) {
        auto* w=g.gtk_label_new(value);g.gtk_label_set_xalign(w,0.0F);return w;
    }
    void add(Widget* parent,Widget* child,bool expand=false) {g.gtk_box_pack_start(parent,child,expand,expand,0);}
    Widget* button(Widget* row,const char* label,SignalHandler cb) {
        auto* b=g.gtk_button_new_with_label(label);add(row,b);signal(b,"clicked",cb);return b;
    }
    void row_field(Widget* parent,const char* title,Widget*& field,const std::string& value,
                   const char* action=nullptr,SignalHandler browse=nullptr,const char* other=nullptr,SignalHandler browse2=nullptr) {
        add(parent,text(title));
        auto* row=g.gtk_box_new(0,8);add(parent,row);
        field=g.gtk_entry_new();g.gtk_entry_set_text(field,value.c_str());add(row,field,true);
        if(action) button(row,action,browse);
        if(other) button(row,other,browse2);
    }
    void build() {
        win=g.gtk_window_new(0);
        g.gtk_window_set_title(win,"Splatoon Port - Setup");
        g.gtk_window_set_default_size(win,790,665);
        g.gtk_window_set_resizable(win,1);
        signal(win,"delete-event",reinterpret_cast<SignalHandler>(+[](Widget*,void*,void* data)->Boolean {
            auto& s=*static_cast<Window*>(data);s.session.cancel();s.g.gtk_main_quit();return 0;
        }));
        auto* base=g.gtk_box_new(1,12);g.gtk_container_set_border_width(base,20);
        g.gtk_container_add(win,base);
        add(base,text("SPLATOON 1  /  PC PORT"));
        auto* subtitle=text("Select a dump of the supported EUR v16 game. Setup recompiles it only on this computer.");
        g.gtk_label_set_line_wrap(subtitle,1);add(base,subtitle);
        row_field(base,"Game dump (source: raw folder, ZIP, WUD, or WUX)",rom,opts.rom.string(),
                  "Choose file",file_cb,"Choose folder",folder_cb);
        row_field(base,"Installation directory (destination for compiled game)",out,opts.output.string(),"Browse",output_cb);
        signal(out,"changed",output_changed_cb);
        row_field(base,"Java executable for encrypted WUD/WUX (optional)",java,opts.java.string(),"Browse",java_cb);
        row_field(base,"JWUDTool.jar for encrypted WUD/WUX (optional)",jwud,opts.jwud.string(),"Browse",jwud_cb);
        launch_when_done=g.gtk_check_button_new_with_label("Launch game when setup completes");
        g.gtk_toggle_button_set_active(launch_when_done,!opts.no_launch);add(base,launch_when_done);
        auto* actions=g.gtk_box_new(0,8);add(base,actions);
        start=button(actions,"Install / Rebuild",start_cb);
        verify=button(actions,"Validate dump",verify_cb);
        launch=button(actions,"Launch installed game",launch_cb);
        cancel=button(actions,"Cancel setup",cancel_cb);
        progress=g.gtk_progress_bar_new();add(base,progress);
        status=text(installed?"Existing installation found. You can launch it or select a new dump.":"Ready to set up.");
        add(base,status);
        auto* scroll=g.gtk_scrolled_window_new(nullptr,nullptr);
        g.gtk_scrolled_window_set_policy(scroll,1,1);g.gtk_widget_set_size_request(scroll,-1,170);
        add(base,scroll,true);
        log=g.gtk_text_view_new();g.gtk_text_view_set_editable(log,0);
        g.gtk_text_view_set_cursor_visible(log,0);g.gtk_text_view_set_wrap_mode(log,2);
        g.gtk_container_add(scroll,log);
        g.gtk_text_buffer_set_text(g.gtk_text_view_get_buffer(log),
            "No game files or generated code are included in the release.\nDownloads require an internet connection.\n",-1);
        update_controls();
        g.gtk_widget_show_all(win);
        g.g_timeout_add(120,tick_cb,this);
    }
    void update_controls() {
        bool busy=session.running();
        g.gtk_widget_set_sensitive(start,!busy);
        g.gtk_widget_set_sensitive(verify,!busy);
        g.gtk_widget_set_sensitive(launch,!busy&&session.can_launch(g.gtk_entry_get_text(out)));
        g.gtk_widget_set_sensitive(cancel,busy);
        g.gtk_widget_set_sensitive(launch_when_done,!busy);
        for(auto* entry:{rom,out,java,jwud})g.gtk_widget_set_sensitive(entry,!busy);
    }
    void pick(Widget* target,bool folder) {
        auto* dialog=g.gtk_file_chooser_dialog_new(folder?"Select directory":"Select file",win,folder?2:0,
                                              "_Cancel",-6,folder?"_Select":"_Open",-3,nullptr);
        auto* current=g.gtk_entry_get_text(target);
        if(current&&*current)g.gtk_file_chooser_set_filename(dialog,current);
        int response=g.gtk_dialog_run(dialog);
        if(response==-3) {
            auto* path=g.gtk_file_chooser_get_filename(dialog);
            if(path) {g.gtk_entry_set_text(target,path);g.g_free(path);}
        }
        g.gtk_widget_destroy(dialog);
    }
    void append(std::string chunk) {
        bool trimmed=append_gui_log(transcript,chunk);
        auto* buffer=g.gtk_text_view_get_buffer(log);
        if(trimmed)g.gtk_text_buffer_set_text(buffer,transcript.c_str(),-1);
        else g.gtk_text_buffer_insert_at_cursor(buffer,chunk.c_str(),int(chunk.size()));
        g.gtk_text_view_scroll_mark_onscreen(log,g.gtk_text_buffer_get_insert(buffer));
        if(chunk.find("Downloading")!=std::string::npos)g.gtk_label_set_text(status,"Downloading required tools...");
        else if(chunk.find("Validated supported")!=std::string::npos)g.gtk_label_set_text(status,"Game dump verified.");
        else if(chunk.find("recompile")!=std::string::npos)g.gtk_label_set_text(status,"Generating native code...");
        else if(chunk.find("[100%]")!=std::string::npos)g.gtk_label_set_text(status,"Completing build...");
    }
    void begin(bool validation) {
        opts.rom=g.gtk_entry_get_text(rom);
        opts.output=g.gtk_entry_get_text(out);
        opts.java=g.gtk_entry_get_text(java);
        opts.jwud=g.gtk_entry_get_text(jwud);
        if(opts.rom.empty()||opts.output.empty()) {
            g.gtk_label_set_text(status,"Please select a dump and installation directory.");return;
        }
        if(!fs::exists(opts.rom)) {
            g.gtk_label_set_text(status,"The selected dump does not exist.");return;
        }
        last_validation=validation;
        displayed_percent=-1;
        g.gtk_progress_bar_set_fraction(progress,0.0);
        transcript.clear();
        g.gtk_text_buffer_set_text(g.gtk_text_view_get_buffer(log),"",-1);
        try {session.start(opts,validation);}
        catch(const std::exception& e) {g.gtk_label_set_text(status,e.what());return;}
        g.gtk_label_set_text(status,validation?"Validating dump...":"Setting up (this may take time)...");
        update_controls();
    }
    void launch_installed() {
        fs::path output=g.gtk_entry_get_text(out);
        if(!session.can_launch(output)) {g.gtk_label_set_text(status,"No installed game executable found.");return;}
        pid_t child=fork();
        if(child==0) {
            chdir(output.c_str());
            auto exe=(output/"Splatoon").string();
            execl(exe.c_str(),exe.c_str(),static_cast<char*>(nullptr));
            _exit(127);
        }
        if(child<0)g.gtk_label_set_text(status,"Unable to launch game.");
        else g.gtk_label_set_text(status,"Launched installed game.");
    }
    static Boolean tick_cb(void* data) {
        auto& s=*static_cast<Window*>(data);
        std::string log;int result=0;
        SetupProgress update;
        bool finished=s.session.consume(log,result,&update);
        if(!log.empty())s.append(std::move(log));
        if(!update.stage.empty()) {
            s.displayed_percent=update.percent;
            if(update.percent>=0) {
                s.g.gtk_progress_bar_set_fraction(s.progress,update.percent/100.0);
                s.g.gtk_label_set_text(s.status,
                    (update.stage+" ("+std::to_string(update.percent)+"%)").c_str());
            } else {
                s.g.gtk_label_set_text(s.status,(update.stage+"...").c_str());
            }
        }
        if(s.session.running()&&s.displayed_percent<0)s.g.gtk_progress_bar_pulse(s.progress);
        if(finished) {
            s.g.gtk_progress_bar_set_fraction(s.progress,result==0?1.0:0.0);
            s.g.gtk_label_set_text(s.status,result==0?"Operation completed successfully.":"Setup failed or was cancelled; review the log.");
            s.update_controls();
            if(result==0 && !s.last_validation && s.session.can_launch(s.g.gtk_entry_get_text(s.out)) &&
               s.g.gtk_toggle_button_get_active(s.launch_when_done)) s.launch_installed();
        }
        if(const char* smoke=std::getenv("SPLATOON_GUI_SMOKE_TEST");smoke&&*smoke=='1') {
            s.g.gtk_main_quit();return 0;
        }
        return 1;
    }
#define CB(name,body) static void name(Widget*,void* data){auto& s=*static_cast<Window*>(data);body;}
    CB(file_cb,s.pick(s.rom,false))
    CB(folder_cb,s.pick(s.rom,true))
    CB(output_cb,s.pick(s.out,true))
    CB(java_cb,s.pick(s.java,false))
    CB(jwud_cb,s.pick(s.jwud,false))
    CB(output_changed_cb,s.update_controls())
    CB(start_cb,s.begin(false))
    CB(verify_cb,s.begin(true))
    CB(launch_cb,s.launch_installed())
    CB(cancel_cb,s.session.cancel();s.g.gtk_label_set_text(s.status,"Cancelling setup..."))
#undef CB
};
}
int run_gui(const Options& options,const fs::path& executable) {
    try {
        Window window(options,executable);
        window.g.gtk_main();
        return 0;
    } catch(const std::exception& e) {
        std::cerr<<"Graphical setup unavailable: "<<e.what()<<"\nUse SplatoonSetup --cli to run the terminal setup.\n";
        return 2;
    }
}
}
#endif
