#include "gui.hpp"
#include "../../port/runtime/input_bindings.h"
#include <SDL.h>
#include <algorithm>
#include <cstdlib>
#include <array>
#include <cmath>
#include <filesystem>
#include <string>
#include <vector>
#include <unordered_map>

namespace spl {
namespace {
using namespace spl_controls;
struct Glyph {char ch;std::array<const char*,7> pixels;};
static const Glyph glyphs[]={
{'A',{" ### ","#   #","#   #","#####","#   #","#   #","#   #"}},
{'B',{"#### ","#   #","#   #","#### ","#   #","#   #","#### "}},
{'C',{" ####","#    ","#    ","#    ","#    ","#    "," ####"}},
{'D',{"#### ","#   #","#   #","#   #","#   #","#   #","#### "}},
{'E',{"#####","#    ","#    ","#### ","#    ","#    ","#####"}},
{'F',{"#####","#    ","#    ","#### ","#    ","#    ","#    "}},
{'G',{" ####","#    ","#    ","#  ##","#   #","#   #"," ####"}},
{'H',{"#   #","#   #","#   #","#####","#   #","#   #","#   #"}},
{'I',{"#####","  #  ","  #  ","  #  ","  #  ","  #  ","#####"}},
{'J',{"  ###","   # ","   # ","   # ","#  # ","#  # "," ##  "}},
{'K',{"#   #","#  # ","# #  ","##   ","# #  ","#  # ","#   #"}},
{'L',{"#    ","#    ","#    ","#    ","#    ","#    ","#####"}},
{'M',{"#   #","## ##","# # #","#   #","#   #","#   #","#   #"}},
{'N',{"#   #","##  #","# # #","#  ##","#   #","#   #","#   #"}},
{'O',{" ### ","#   #","#   #","#   #","#   #","#   #"," ### "}},
{'P',{"#### ","#   #","#   #","#### ","#    ","#    ","#    "}},
{'Q',{" ### ","#   #","#   #","#   #","# # #","#  # "," ## #"}},
{'R',{"#### ","#   #","#   #","#### ","# #  ","#  # ","#   #"}},
{'S',{" ####","#    ","#    "," ### ","    #","    #","#### "}},
{'T',{"#####","  #  ","  #  ","  #  ","  #  ","  #  ","  #  "}},
{'U',{"#   #","#   #","#   #","#   #","#   #","#   #"," ### "}},
{'V',{"#   #","#   #","#   #","#   #","#   #"," # # ","  #  "}},
{'W',{"#   #","#   #","#   #","# # #","# # #","## ##","#   #"}},
{'X',{"#   #","#   #"," # # ","  #  "," # # ","#   #","#   #"}},
{'Y',{"#   #","#   #"," # # ","  #  ","  #  ","  #  ","  #  "}},
{'Z',{"#####","    #","   # ","  #  "," #   ","#    ","#####"}},
{'0',{" ### ","#   #","#  ##","# # #","##  #","#   #"," ### "}},
{'1',{"  #  "," ##  ","  #  ","  #  ","  #  ","  #  ","#####"}},
{'2',{" ### ","#   #","    #","   # ","  #  "," #   ","#####"}},
{'3',{"#### ","    #","    #"," ### ","    #","    #","#### "}},
{'4',{"   # ","  ## "," # # ","#  # ","#####","   # ","   # "}},
{'5',{"#####","#    ","#    ","#### ","    #","    #","#### "}},
{'6',{" ### ","#    ","#    ","#### ","#   #","#   #"," ### "}},
{'7',{"#####","    #","   # ","  #  "," #   "," #   "," #   "}},
{'8',{" ### ","#   #","#   #"," ### ","#   #","#   #"," ### "}},
{'9',{" ### ","#   #","#   #"," ####","    #","    #"," ### "}},
{' ',{"     ","     ","     ","     ","     ","     ","     "}},
{'-',{"     ","     ","     ","#####","     ","     ","     "}},
{'+',{"     ","  #  ","  #  ","#####","  #  ","  #  ","     "}},
{'/',{"    #","    #","   # ","  #  "," #   ","#    ","#    "}},
{'_',{"     ","     ","     ","     ","     ","     ","#####"}},
{':',{"     ","  #  ","  #  ","     ","  #  ","  #  ","     "}},
{'.',{"     ","     ","     ","     ","     ","  #  ","     "}},
{'[',{" ### "," #   "," #   "," #   "," #   "," #   "," ### "}},
{']',{" ### ","   # ","   # ","   # ","   # ","   # "," ### "}},
{'(',{"   # ","  #  "," #   "," #   "," #   ","  #  ","   # "}},
{')',{" #   ","  #  ","   # ","   # ","   # ","  #  "," #   "}},
{'?',{" ### ","#   #","    #","   # ","  #  ","     ","  #  "}},
{'=',{"     ","#####","     ","#####","     ","     ","     "}},
{'#',{" # # ","#####"," # # ","#####"," # # ","     ","     "}},
{'<',{"   # ","  #  "," #   ","#    "," #   ","  #  ","   # "}},
{'>',{" #   ","  #  ","   # ","    #","   # ","  #  "," #   "}},
{'!',{"  #  ","  #  ","  #  ","  #  ","  #  ","     ","  #  "}},
{'*',{"     ","# # #"," ### ","#####"," ### ","# # #","     "}},
{',',{"     ","     ","     ","     ","  #  ","  #  "," #   "}}
};
void rect(SDL_Renderer* r,int x,int y,int w,int h, SDL_Color c) {
    SDL_SetRenderDrawColor(r,c.r,c.g,c.b,255);
    SDL_Rect area{x,y,w,h};SDL_RenderFillRect(r,&area);
}
void text(SDL_Renderer* r,int x,int y,const std::string& text,SDL_Color color,int scale=2,int limit=90) {
    SDL_SetRenderDrawColor(r,color.r,color.g,color.b,255);
    int count=0;
    for(unsigned char raw:text) {
        if(count++>=limit)break;
        char letter=raw>='a'&&raw<='z'?char(raw-'a'+'A'):char(raw);
        const Glyph* found=nullptr;
        for(const auto& g:glyphs)if(g.ch==letter){found=&g;break;}
        if(!found){x+=6*scale;continue;}
        for(int row=0;row<7;++row)for(int col=0;col<5;++col) {
            if(found->pixels[row][col]!='#')continue;
            SDL_Rect pixel{x+col*scale,y+row*scale,scale,scale};
            SDL_RenderFillRect(r,&pixel);
        }
        x+=6*scale;
    }
}
constexpr SDL_Color fg{235,241,250,255},muted{159,179,199,255},accent{75,160,225,255};
std::string keyboard_label(const Binding& b) {
    if(b.key>0&&b.key<SDL_NUM_SCANCODES) {
        const char* key=SDL_GetScancodeName(static_cast<SDL_Scancode>(b.key));
        return key&&*key?key:"Key";
    }
    if(b.mouse>0)return "Mouse "+std::to_string(b.mouse);
    return "Unbound";
}
std::string controller_label(const Binding& b,bool raw) {return pad_label(b,raw);}
constexpr int row_height=32,top_y=193,view_rows=13,width=1050,height=770;
struct Editor {
    SDL_Window* window=nullptr;
    SDL_Renderer* renderer=nullptr;
    Config config;
    std::filesystem::path destination;
    SDL_GameController* mapped=nullptr;
    SDL_Joystick* joystick=nullptr;
    int device_index=-1,scroll=0,capture_row=-1,capture_column=-1,dropdown_scroll=0;
    std::string selected_key="keyboard",state;
    std::unordered_map<std::string,Config> drafts;
    std::vector<Choice> available;
    bool done=false,saved=false,sdl_started=false,dropdown_open=false;
    struct Choice {int index;std::string key,label;};
    explicit Editor(std::filesystem::path dir):destination(std::move(dir)/"controls.ini") {
        const Config active=load_selected(destination.parent_path());
        selected_key=active.controller_mode ?
            (!active.controller_key.empty()?active.controller_key:active.controller_guid) : "keyboard";
        config=active;
        drafts[selected_key]=active;
    }
    ~Editor() {
        close_device();
        if(renderer)SDL_DestroyRenderer(renderer);
        if(window)SDL_DestroyWindow(window);
        if(sdl_started)SDL_QuitSubSystem(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER);
    }
    void close_device() {
        if(mapped){SDL_GameControllerClose(mapped);mapped=nullptr;}
        if(joystick){SDL_JoystickClose(joystick);joystick=nullptr;}
    }
    SDL_Joystick* active_joystick() const {return mapped?SDL_GameControllerGetJoystick(mapped):joystick;}
    bool raw_device() const {return joystick!=nullptr;}
    bool same_device(int instance) const {
        auto* j=active_joystick();return j&&SDL_JoystickInstanceID(j)==instance;
    }
    void refresh_choices() {
        std::vector<Choice> result{{-1,"keyboard","KEYBOARD / MOUSE"}};
        bool matched=selected_key=="keyboard";
        for(int i=0;i<SDL_NumJoysticks();++i) {
            auto* j=SDL_JoystickOpen(i);
            if(!j)continue;
            const auto key=device_key(j);
            const char* name=SDL_JoystickName(j);
            const std::string label=std::string(SDL_IsGameController(i)?"GAMEPAD ":"JOYSTICK ")+
                std::to_string(i+1)+" - "+(name?name:"UNKNOWN");
            SDL_JoystickClose(j);
            result.push_back({i,key,label});
            if(key==selected_key)matched=true;
        }
        if(!matched&&!selected_key.empty())result.push_back({-2,selected_key,"SAVED CONTROLLER (DISCONNECTED)"});
        available=std::move(result);
    }
    const std::vector<Choice>& choices() const {return available;}
    void cache() {drafts[selected_key]=config;}
    void choose_device(int index, const std::string& key) {
        cache();close_device();capture_row=capture_column=-1;device_index=index;
        if(index>=0&&index<SDL_NumJoysticks()) {
            if(SDL_IsGameController(index))mapped=SDL_GameControllerOpen(index);
            if(!mapped)joystick=SDL_JoystickOpen(index);
            if(!active_joystick()){device_index=-2;state="Could not open selected device";return;}
        }
        selected_key=index==-1?"keyboard":key;
        auto found=drafts.find(selected_key);
        if(found!=drafts.end())config=found->second;
        else config=read_profile(destination.parent_path(),selected_key);
        if(index==-1) {
            config.controller_mode=false;
            config.controller_guid.clear();config.controller_key.clear();
            state="KEYBOARD / MOUSE active. Its own bindings will be saved.";
        } else {
            config.controller_mode=true;
            config.controller_key=selected_key;
            config.controller_guid=selected_key.substr(0,std::min(size_t(32),selected_key.size()));
            state=index==-2?"Saved controller is disconnected; other devices will not be used":
                "CONTROLLER active. Keyboard/mouse gameplay input is disabled.";
            if(found==drafts.end() && !std::filesystem::exists(profile_path(destination.parent_path(),selected_key)))
                config.mouse_camera=false;
        }
        refresh_choices();
    }
    bool initialize() {
        SDL_SetHint(SDL_HINT_GAMECONTROLLER_USE_BUTTON_LABELS,"0");
        SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS,"1");
        if(SDL_InitSubSystem(SDL_INIT_VIDEO|SDL_INIT_JOYSTICK|SDL_INIT_GAMECONTROLLER)!=0)return false;
        sdl_started=true;
        window=SDL_CreateWindow("Splatoon Port - Input Bindings",SDL_WINDOWPOS_CENTERED,
            SDL_WINDOWPOS_CENTERED,width,height,SDL_WINDOW_SHOWN);
        if(!window)return false;
        renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
        if(!renderer)renderer=SDL_CreateRenderer(window,-1,SDL_RENDERER_SOFTWARE);
        if(!renderer)return false;
        refresh_choices();
        int selected=-1;
        for(const auto& choice:choices())if(choice.key==selected_key){selected=choice.index;break;}
        // Upgrade v1 controller GUID selections without discarding their layout.
        if(config.controller_mode && selected_key.size()==32 && selected<0) {
            for(const auto& choice:choices()) {
                if(choice.index<0 || choice.key.substr(0,32)!=selected_key)continue;
                selected=choice.index;
                const auto legacy=selected_key;
                selected_key=choice.key;
                config.controller_key=selected_key;
                drafts.erase(legacy);
                drafts[selected_key]=config;
                refresh_choices();
                break;
            }
        }
        if(config.controller_mode && selected<0)selected=-2;
        device_index=selected;
        if(selected>=0) {
            if(SDL_IsGameController(selected))mapped=SDL_GameControllerOpen(selected);
            if(!mapped)joystick=SDL_JoystickOpen(selected);
        }
        state=config.controller_mode ?
            (active_joystick()?"CONTROLLER selected. Keyboard and mouse do not override gameplay":
                               "Selected controller is offline. Reconnect it or choose another device"):
            "KEYBOARD / MOUSE selected. Choose another device from the dropdown.";
        return true;
    }
    int entries() const {return ActionCount+4;}
    void clear_binding() {
        if(capture_row<0)return;
        if(capture_row<ActionCount) {
            auto& b=config.binding[capture_row];
            if(capture_column==0){b.key=SDL_SCANCODE_UNKNOWN;b.mouse=0;}
            else if(raw_device()){b.joy_button=b.joy_axis=b.joy_hat=-1;b.joy_hat_value=0;}
            else {b.pad_button=b.pad_axis=-1;}
        }else if(capture_column==1) {
            auto& a=config.axes[capture_row-ActionCount];
            if(raw_device())a.joy_source=-1;else a.source=-1;
        }
        capture_row=capture_column=-1;
        state="Binding cleared";
    }
    void capture_event(const SDL_Event& e) {
        if(capture_row<0)return;
        if(e.type==SDL_KEYDOWN) {
            if(e.key.keysym.scancode==SDL_SCANCODE_ESCAPE){capture_row=-1;state="Capture cancelled";return;}
            if(e.key.keysym.scancode==SDL_SCANCODE_BACKSPACE||
               e.key.keysym.scancode==SDL_SCANCODE_DELETE){clear_binding();return;}
            if(capture_column==0&&capture_row<ActionCount) {
                auto& b=config.binding[capture_row];
                b.key=e.key.keysym.scancode;b.mouse=0;capture_row=-1;state="Keyboard binding updated";
            }
        } else if(e.type==SDL_MOUSEBUTTONDOWN&&capture_column==0&&capture_row<ActionCount) {
            auto& b=config.binding[capture_row];
            b.key=SDL_SCANCODE_UNKNOWN;b.mouse=e.button.button;
            capture_row=-1;state="Mouse binding updated";
        } else if(capture_column==1&&active_joystick()) {
            if(raw_device()) {
                if(e.type==SDL_JOYBUTTONDOWN&&same_device(e.jbutton.which)&&capture_row<ActionCount) {
                    auto& b=config.binding[capture_row];b.joy_button=e.jbutton.button;
                    b.joy_axis=b.joy_hat=-1;capture_row=-1;state="Joystick button updated";
                } else if(e.type==SDL_JOYAXISMOTION&&same_device(e.jaxis.which)&&
                          std::abs(int(e.jaxis.value))>23000) {
                    if(capture_row<ActionCount) {
                        auto& b=config.binding[capture_row];b.joy_axis=e.jaxis.axis;
                        b.joy_sign=e.jaxis.value>0?1:-1;b.joy_button=b.joy_hat=-1;
                    } else config.axes[capture_row-ActionCount].joy_source=e.jaxis.axis;
                    capture_row=-1;state="Joystick axis updated";
                }else if(e.type==SDL_JOYHATMOTION&&same_device(e.jhat.which)&&
                         e.jhat.value!=SDL_HAT_CENTERED&&capture_row<ActionCount) {
                    auto& b=config.binding[capture_row];b.joy_hat=e.jhat.hat;
                    b.joy_hat_value=e.jhat.value;b.joy_button=b.joy_axis=-1;
                    capture_row=-1;state="Joystick hat updated";
                }
            } else {
                if(e.type==SDL_CONTROLLERBUTTONDOWN&&same_device(e.cbutton.which)&&
                   capture_row<ActionCount){
                    auto& b=config.binding[capture_row];b.pad_button=e.cbutton.button;
                    b.pad_axis=-1;capture_row=-1;state="Gamepad button updated";
                }else if(e.type==SDL_CONTROLLERAXISMOTION&&same_device(e.caxis.which)&&
                         std::abs(int(e.caxis.value))>23000){
                    if(capture_row<ActionCount) {
                        auto& b=config.binding[capture_row];b.pad_axis=e.caxis.axis;
                        b.pad_sign=e.caxis.value>0?1:-1;b.pad_button=-1;
                    }else config.axes[capture_row-ActionCount].source=e.caxis.axis;
                    capture_row=-1;state="Gamepad axis updated";
                }
            }
        }
    }
    bool save_all() {
        cache();
        for(const auto& [key,layout]:drafts) {
            const auto path=profile_path(destination.parent_path(),key);
            if(path.empty()||!save(path,layout)) {
                state="Could not save device profile: "+key;
                return false;
            }
        }
        if(!save(destination,config)) {
            state="Could not save active input device";
            return false;
        }
        return true;
    }
    void click(int x,int y,int button) {
        if(capture_row>=0)return;
        if(dropdown_open) {
            if(y>=93 && y<93+std::min(9,int(choices().size()))*33 && x>=23&&x<=1028) {
                const auto& list=choices();
                const int n=dropdown_scroll+(y-93)/33;
                if(n>=0&&n<int(list.size()))choose_device(list[n].index,list[n].key);
            }
            dropdown_open=false;
            return;
        }
        if(y>=55&&y<94){refresh_choices();dropdown_open=true;dropdown_scroll=0;return;}
        if(y>=105&&y<144){
            if(x>30&&x<355){config.mouse_camera=!config.mouse_camera;return;}
            if(x>460&&x<518)config.mouse_sensitivity=std::max(0.0001f,config.mouse_sensitivity*0.8f);
            if(x>700&&x<755)config.mouse_sensitivity=std::min(0.1f,config.mouse_sensitivity*1.25f);
            return;
        }
        if(y>=top_y&&y<top_y+view_rows*row_height) {
            const int row=scroll+(y-top_y)/row_height;
            if(row>=entries())return;
            const int column=config.controller_mode?1:0;
            if((column==0&&(x<340||x>=655))||
               (column==1&&(x<660||x>=1026)))return;
            if(row>=ActionCount&&column==0)return;
            if(column==1&&!active_joystick()){
                state="Selected controller is disconnected. Connect it to capture inputs.";
                return;
            }
            if(button==SDL_BUTTON_RIGHT&&column==1&&row>=ActionCount) {
                config.axes[row-ActionCount].inverted=!config.axes[row-ActionCount].inverted;
                state="Stick axis direction inverted";return;
            }
            capture_row=row;capture_column=column;
            state=column==0?"Press a key or mouse button (Delete clears; Esc cancels)":
                "Press selected controller button / move an axis (Delete clears)";
            return;
        }
        if(y>=681&&y<745) {
            if(x>24&&x<315){
                saved=save_all();
                if(saved){state="Saved active device and profiles";done=true;}
            }else if(x>357&&x<643){
                const bool controller=config.controller_mode;
                const auto guid=config.controller_guid,key=config.controller_key;
                config=defaults();config.controller_mode=controller;
                if(controller){config.controller_guid=guid;config.controller_key=key;config.mouse_camera=false;}
                state="Restored current device defaults (save to apply)";
            }else if(x>702&&x<1020)done=true;
        }
    }
    void event(const SDL_Event& e) {
        if(e.type==SDL_QUIT){done=true;return;}
        if(e.type==SDL_CONTROLLERDEVICEADDED||e.type==SDL_JOYDEVICEADDED ||
           e.type==SDL_CONTROLLERDEVICEREMOVED||e.type==SDL_JOYDEVICEREMOVED) {
            refresh_choices();
            const auto& list=choices();
            bool attached=false;
            for(const auto& choice:list) {
                if(choice.index<0 || choice.key!=selected_key)continue;
                attached=true;
                if(!active_joystick()||!SDL_JoystickGetAttached(active_joystick())) {
                    close_device();device_index=choice.index;
                    if(SDL_IsGameController(choice.index))mapped=SDL_GameControllerOpen(choice.index);
                    if(!mapped)joystick=SDL_JoystickOpen(choice.index);
                }
                break;
            }
            if(!attached&&config.controller_mode) {
                close_device();device_index=-2;
                state="Configured controller disconnected. Layout stays selected.";
            }
        }
        if(capture_row>=0){capture_event(e);return;}
        if(e.type==SDL_MOUSEBUTTONDOWN)click(e.button.x,e.button.y,e.button.button);
        else if(e.type==SDL_MOUSEWHEEL) {
            if(dropdown_open)dropdown_scroll=std::clamp(dropdown_scroll-e.wheel.y,0,
                std::max(0,int(choices().size())-9));
            else scroll=std::clamp(scroll-e.wheel.y,0,std::max(0,entries()-view_rows));
        }else if(e.type==SDL_KEYDOWN) {
            if(e.key.keysym.sym==SDLK_ESCAPE){
                if(dropdown_open)dropdown_open=false;else done=true;
            }else if(dropdown_open) {
                if(e.key.keysym.sym==SDLK_DOWN)dropdown_scroll=std::min(
                    std::max(0,int(choices().size())-9),dropdown_scroll+1);
                if(e.key.keysym.sym==SDLK_UP)dropdown_scroll=std::max(0,dropdown_scroll-1);
            }else if(e.key.keysym.sym==SDLK_PAGEDOWN)scroll=std::min(entries()-view_rows,scroll+view_rows);
            else if(e.key.keysym.sym==SDLK_PAGEUP)scroll=std::max(0,scroll-view_rows);
        }
    }
    void paint() {
        rect(renderer,0,0,width,height,{17,25,39,255});
        text(renderer,24,22,"SPLATOON PORT / INPUT BINDINGS",fg,3,65);
        rect(renderer,22,55,1006,38,{33,48,66,255});
        std::string device=config.controller_mode?"SAVED CONTROLLER (DISCONNECTED)":"KEYBOARD / MOUSE";
        if(active_joystick()) {
            const char* name=SDL_JoystickName(active_joystick());
            device=std::string(raw_device()?"JOYSTICK: ":"GAMEPAD: ")+(name?name:"UNKNOWN");
        }
        text(renderer,33,66,"DEVICE:",accent);
        text(renderer,137,66,device,fg,2,65);
        rect(renderer,974,59,50,30,{50,83,115,255});
        text(renderer,995,67,"V",fg);
        rect(renderer,22,103,1006,44,{27,43,60,255});
        text(renderer,38,116,std::string("MOUSE CAMERA: ")+(config.mouse_camera?"ON":"OFF"),accent,2);
        text(renderer,425,116,"SENS -",fg,2);
        text(renderer,528,116,std::to_string(int(config.mouse_sensitivity*100000.0f))+"/100000",fg);
        text(renderer,707,116,"+",accent);
        text(renderer,772,116,"RIGHT CLICK STICK AXIS TO FLIP",muted,2,28);
        text(renderer,35,163,"WII U ACTION",muted);
        text(renderer,357,163,"KEYBOARD / MOUSE",config.controller_mode?muted:accent);
        text(renderer,671,163,"SELECTED CONTROLLER",config.controller_mode?accent:muted);
        for(int i=0;i<view_rows;++i){
            const int row=scroll+i;
            if(row>=entries())break;
            const int y=top_y+i*row_height;
            rect(renderer,23,y,1003,row_height-2,row%2?SDL_Color{27,39,55,255}:SDL_Color{35,48,64,255});
            text(renderer,36,y+9,row<ActionCount?names[row]:
                (std::string(axis_ids[row-ActionCount])+" axis"),fg,2,23);
            if(row<ActionCount)text(renderer,356,y+9,keyboard_label(config.binding[row]),
                config.controller_mode?SDL_Color{100,115,133,255}:fg,2,23);
            std::string pad="N/A";
            if(row<ActionCount)pad=controller_label(config.binding[row],raw_device());
            else {
                const auto& a=config.axes[row-ActionCount];
                int source=raw_device()?a.joy_source:a.source;
                pad=source<0?"Unbound":(raw_device()?"Joystick axis "+std::to_string(source):
                    std::string(SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(source))));
                if(a.inverted)pad+=" (flipped)";
            }
            text(renderer,674,y+9,pad,config.controller_mode?fg:SDL_Color{100,115,133,255},2,28);
            if(capture_row==row) {
                rect(renderer,capture_column==0?343:661,y,3,row_height-2,accent);
                rect(renderer,capture_column==0?652:1022,y,3,row_height-2,accent);
            }
        }
        text(renderer,27,620,config.controller_mode?"CONTROLLER-ONLY GAMEPLAY / MOUSE CAMERA OPTIONAL":
            "KEYBOARD AND MOUSE ONLY / CONTROLLERS DISABLED",muted,2,72);
        text(renderer,27,649,state,accent,2,84);
        rect(renderer,24,682,285,51,{43,116,168,255});
        rect(renderer,356,682,285,51,{56,74,99,255});
        rect(renderer,688,682,338,51,{56,74,99,255});
        text(renderer,54,699,"SAVE AND CLOSE",fg,2);
        text(renderer,393,699,"RESTORE DEFAULTS",fg,2);
        text(renderer,757,699,"CANCEL",fg,2);
        if(dropdown_open) {
            const auto& list=choices();
            const int shown=std::min(9,int(list.size())-dropdown_scroll);
            rect(renderer,21,93,1009,std::max(1,shown)*33+4,{69,107,140,255});
            for(int i=0;i<shown;++i) {
                const auto& choice=list[dropdown_scroll+i];
                const bool selected=choice.key==selected_key;
                const int y=95+i*33;
                rect(renderer,25,y,1000,31,selected?SDL_Color{52,106,154,255}:
                    (i%2?SDL_Color{31,47,65,255}:SDL_Color{37,56,76,255}));
                text(renderer,37,y+9,(selected?"* ":"  ")+choice.label,selected?fg:muted,2,76);
            }
            if(list.size()>9)
                text(renderer,35,95+shown*33+7,"SCROLL TO SEE OTHER CONNECTED DEVICES",muted,2,52);
        }
        SDL_RenderPresent(renderer);
    }
    bool run() {
        if(!initialize())return false;
        if(const char* smoke=std::getenv("SPLATOON_CONTROLS_SMOKE_TEST");smoke&&*smoke=='1') {
            paint();return true;
        }
        while(!done) {
            SDL_Event e;
            while(SDL_PollEvent(&e))event(e);
            paint();
            SDL_Delay(20);
        }
        return saved;
    }
};
}
bool open_controls_editor(const fs::path& directory) {
    if(directory.empty())return false;
    Editor editor(directory);
    return editor.run();
}
}
