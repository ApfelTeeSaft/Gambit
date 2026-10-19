#pragma once
#include <SDL.h>
#include <array>
#include <algorithm>
#include <charconv>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>

namespace spl_controls {
enum Action : int {
    A, B, X, Y, Up, Down, Left, Right, ZL, ZR, L, R, Plus, Minus, Home,
    LeftClick, RightClick, MoveUp, MoveDown, MoveLeft, MoveRight,
    AimUp, AimDown, AimLeft, AimRight, ActionCount
};
inline constexpr std::array<const char*,ActionCount> names = {
    "A","B","X","Y","DPad Up","DPad Down","DPad Left","DPad Right",
    "ZL","ZR","L","R","Plus","Minus","Home","Left Stick Click","Right Stick Click",
    "Move Up","Move Down","Move Left","Move Right",
    "Aim Up","Aim Down","Aim Left","Aim Right"
};
inline constexpr std::array<const char*,ActionCount> ids = {
    "a","b","x","y","up","down","left","right","zl","zr","l","r",
    "plus","minus","home","l3","r3","move_up","move_down","move_left","move_right",
    "aim_up","aim_down","aim_left","aim_right"
};
inline constexpr std::array<const char*,4> axis_ids = {"left_x","left_y","right_x","right_y"};
struct Binding {
    int key = SDL_SCANCODE_UNKNOWN;
    int mouse = 0;
    int pad_button = -1;
    int pad_axis = -1;
    int pad_sign = 1;
    int joy_button = -1;
    int joy_axis = -1;
    int joy_sign = 1;
    int joy_hat = -1;
    int joy_hat_value = 0;
};
struct Axis {
    int source = -1;
    bool inverted = false;
    int joy_source = -1;
};
struct Config {
    std::array<Binding,ActionCount> binding{};
    std::array<Axis,4> axes{};
    bool mouse_camera = true;
    float mouse_sensitivity = 0.005f;
    std::string controller_guid;
};
inline Config defaults() {
    Config c;
    for (auto& b : c.binding) b = {};
    const auto button=[&](Action a,int p){c.binding[a].pad_button=p;};
    button(A,SDL_CONTROLLER_BUTTON_B);button(B,SDL_CONTROLLER_BUTTON_A);
    button(X,SDL_CONTROLLER_BUTTON_Y);button(Y,SDL_CONTROLLER_BUTTON_X);
    button(Up,SDL_CONTROLLER_BUTTON_DPAD_UP);button(Down,SDL_CONTROLLER_BUTTON_DPAD_DOWN);
    button(Left,SDL_CONTROLLER_BUTTON_DPAD_LEFT);button(Right,SDL_CONTROLLER_BUTTON_DPAD_RIGHT);
    button(L,SDL_CONTROLLER_BUTTON_LEFTSHOULDER);button(R,SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);
    button(Plus,SDL_CONTROLLER_BUTTON_START);button(Minus,SDL_CONTROLLER_BUTTON_BACK);
    button(Home,SDL_CONTROLLER_BUTTON_GUIDE);
    button(LeftClick,SDL_CONTROLLER_BUTTON_LEFTSTICK);
    button(RightClick,SDL_CONTROLLER_BUTTON_RIGHTSTICK);
    c.binding[ZL].pad_axis=SDL_CONTROLLER_AXIS_TRIGGERLEFT;
    c.binding[ZR].pad_axis=SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    auto key=[&](Action a, SDL_Scancode s){c.binding[a].key=s;};
    key(B,SDL_SCANCODE_SPACE);key(X,SDL_SCANCODE_Q);key(Y,SDL_SCANCODE_E);
    key(R,SDL_SCANCODE_R);key(Plus,SDL_SCANCODE_F);
    key(Minus,SDL_SCANCODE_G);key(Home,SDL_SCANCODE_ESCAPE);
    key(MoveUp,SDL_SCANCODE_W);key(MoveDown,SDL_SCANCODE_S);
    key(MoveLeft,SDL_SCANCODE_A);key(MoveRight,SDL_SCANCODE_D);
    c.binding[ZR].mouse=SDL_BUTTON_LEFT;
    c.binding[ZL].mouse=SDL_BUTTON_RIGHT;
    c.axes={{{SDL_CONTROLLER_AXIS_LEFTX,false,-1},{SDL_CONTROLLER_AXIS_LEFTY,true,-1},
             {SDL_CONTROLLER_AXIS_RIGHTX,false,-1},{SDL_CONTROLLER_AXIS_RIGHTY,true,-1}}};
    return c;
}
inline bool valid_binding(const Binding& b) {
    return b.key>=0 && b.key<SDL_NUM_SCANCODES && b.mouse>=0 && b.mouse<=8 &&
        b.pad_button>=-1 && b.pad_button<SDL_CONTROLLER_BUTTON_MAX &&
        b.pad_axis>=-1 && b.pad_axis<SDL_CONTROLLER_AXIS_MAX &&
        (b.pad_sign==1 || b.pad_sign==-1) && b.joy_button>=-1 && b.joy_button<256 &&
        b.joy_axis>=-1 && b.joy_axis<256 && (b.joy_sign==1 || b.joy_sign==-1) &&
        b.joy_hat>=-1 && b.joy_hat<64 && b.joy_hat_value>=0 && b.joy_hat_value<=15;
}
inline bool active(const Binding& b, const Uint8* keys, Uint32 mouse, SDL_GameController* pad) {
    if(keys && b.key>0 && b.key<SDL_NUM_SCANCODES && keys[b.key])return true;
    if(b.mouse>0 && b.mouse<=8 && (mouse & SDL_BUTTON(b.mouse)))return true;
    if(pad) {
        if(b.pad_button>=0 && b.pad_button<SDL_CONTROLLER_BUTTON_MAX &&
           SDL_GameControllerGetButton(pad,static_cast<SDL_GameControllerButton>(b.pad_button)))return true;
        if(b.pad_axis>=0 && b.pad_axis<SDL_CONTROLLER_AXIS_MAX) {
            const int value=SDL_GameControllerGetAxis(pad,static_cast<SDL_GameControllerAxis>(b.pad_axis));
            if(value*b.pad_sign>16384)return true;
        }
    }
    return false;
}
inline bool keyboard_mouse(const Binding& b, const Uint8* keys, Uint32 mouse) {
    if(keys && b.key>0 && b.key<SDL_NUM_SCANCODES && keys[b.key])return true;
    return b.mouse>0 && b.mouse<=8 && (mouse & SDL_BUTTON(b.mouse));
}
inline bool gamepad(const Binding& b, SDL_GameController* pad, SDL_Joystick* raw=nullptr) {
    if(pad) {
        if(b.pad_button>=0 && b.pad_button<SDL_CONTROLLER_BUTTON_MAX &&
           SDL_GameControllerGetButton(pad,static_cast<SDL_GameControllerButton>(b.pad_button)))return true;
        if(b.pad_axis>=0 && b.pad_axis<SDL_CONTROLLER_AXIS_MAX) {
            const int value=SDL_GameControllerGetAxis(pad,static_cast<SDL_GameControllerAxis>(b.pad_axis));
            if(value*b.pad_sign>16384)return true;
        }
    }
    if(raw) {
        if(b.joy_button>=0 && b.joy_button<SDL_JoystickNumButtons(raw) &&
           SDL_JoystickGetButton(raw,b.joy_button))return true;
        if(b.joy_axis>=0 && b.joy_axis<SDL_JoystickNumAxes(raw) &&
           int(SDL_JoystickGetAxis(raw,b.joy_axis))*b.joy_sign>16384)return true;
        if(b.joy_hat>=0 && b.joy_hat<SDL_JoystickNumHats(raw) &&
           (SDL_JoystickGetHat(raw,b.joy_hat)&b.joy_hat_value))return true;
    }
    return false;
}
inline float axis_value(const Axis& binding, SDL_GameController* pad, SDL_Joystick* raw=nullptr) {
    float value=0;
    if(pad && binding.source>=0&&binding.source<SDL_CONTROLLER_AXIS_MAX)
        value=float(SDL_GameControllerGetAxis(pad,static_cast<SDL_GameControllerAxis>(binding.source)))/32767.f;
    else if(raw&&binding.joy_source>=0&&binding.joy_source<SDL_JoystickNumAxes(raw))
        value=float(SDL_JoystickGetAxis(raw,binding.joy_source))/32767.f;
    return std::clamp(binding.inverted ? -value:value,-1.f,1.f);
}
inline bool parse_integer(std::string_view value, int& n) {
    auto result=std::from_chars(value.data(),value.data()+value.size(),n);
    return result.ec==std::errc{} && result.ptr==value.data()+value.size();
}
inline std::string trim(std::string value) {
    const auto first=value.find_first_not_of(" \t\r\n");
    if(first==std::string::npos)return {};
    const auto last=value.find_last_not_of(" \t\r\n");
    return value.substr(first,last-first+1);
}
inline Config load(const std::filesystem::path& file) {
    Config c=defaults();
    std::ifstream input(file);
    std::string line;
    while(std::getline(input,line)) {
        if(auto comment=line.find('#');comment!=std::string::npos)line.resize(comment);
        const auto pos=line.find('=');
        if(pos==std::string::npos)continue;
        auto key=trim(line.substr(0,pos)), value=trim(line.substr(pos+1));
        if(key=="mouse_camera") {if(value=="0"||value=="1")c.mouse_camera=value=="1";continue;}
        if(key=="sensitivity") {
            try{const float s=std::stof(value);if(std::isfinite(s)&&s>=0.0001f&&s<=0.1f)c.mouse_sensitivity=s;}
            catch(...){} continue;
        }
        if(key=="controller_guid") {
            if(value.size()==32&&value.find_first_not_of("0123456789abcdefABCDEF")==std::string::npos)
                c.controller_guid=value;
            continue;
        }
        bool done=false;
        for(int i=0;i<ActionCount&&!done;++i) {
            const std::string prefix=std::string(ids[i])+".";
            if(key.rfind(prefix,0)!=0)continue;
            Binding candidate=c.binding[i];
            int n=0;
            if(!parse_integer(value,n)) {done=true;break;}
            auto kind=key.substr(prefix.size());
            if(kind=="key")candidate.key=n;
            else if(kind=="mouse")candidate.mouse=n;
            else if(kind=="pad_button")candidate.pad_button=n;
            else if(kind=="pad_axis")candidate.pad_axis=n;
            else if(kind=="pad_sign")candidate.pad_sign=n;
            else if(kind=="joy_button")candidate.joy_button=n;
            else if(kind=="joy_axis")candidate.joy_axis=n;
            else if(kind=="joy_sign")candidate.joy_sign=n;
            else if(kind=="joy_hat")candidate.joy_hat=n;
            else if(kind=="joy_hat_value")candidate.joy_hat_value=n;
            if(valid_binding(candidate))c.binding[i]=candidate;
            done=true;
        }
        if(done)continue;
        for(int i=0;i<4;++i) {
            std::string prefix=std::string(axis_ids[i])+".";
            if(key.rfind(prefix,0)!=0)continue;
            int n=0;
            if(!parse_integer(value,n))break;
            auto field=key.substr(prefix.size());
            if(field=="axis"&&n>=-1&&n<SDL_CONTROLLER_AXIS_MAX)c.axes[i].source=n;
            else if(field=="joy_axis"&&n>=-1&&n<256)c.axes[i].joy_source=n;
            else if(field=="invert"&&(n==0||n==1))c.axes[i].inverted=n!=0;
            break;
        }
    }
    return c;
}
inline bool save(const std::filesystem::path& file, const Config& c) {
    std::error_code ec;
    if(!file.parent_path().empty())std::filesystem::create_directories(file.parent_path(),ec);
    if(ec)return false;
    std::ofstream output(file,std::ios::binary|std::ios::trunc);
    if(!output)return false;
    output<<"# Splatoon Port input bindings v1\n";
    output<<"mouse_camera="<<int(c.mouse_camera)<<"\n";
    output<<"sensitivity="<<c.mouse_sensitivity<<"\n";
    output<<"controller_guid="<<c.controller_guid<<"\n";
    for(int i=0;i<ActionCount;++i) {
        const auto& b=c.binding[i];
        output<<ids[i]<<".key="<<b.key<<"\n";
        output<<ids[i]<<".mouse="<<b.mouse<<"\n";
        output<<ids[i]<<".pad_button="<<b.pad_button<<"\n";
        output<<ids[i]<<".pad_axis="<<b.pad_axis<<"\n";
        output<<ids[i]<<".pad_sign="<<b.pad_sign<<"\n";
        output<<ids[i]<<".joy_button="<<b.joy_button<<"\n";
        output<<ids[i]<<".joy_axis="<<b.joy_axis<<"\n";
        output<<ids[i]<<".joy_sign="<<b.joy_sign<<"\n";
        output<<ids[i]<<".joy_hat="<<b.joy_hat<<"\n";
        output<<ids[i]<<".joy_hat_value="<<b.joy_hat_value<<"\n";
    }
    for(int i=0;i<4;++i) {
        output<<axis_ids[i]<<".axis="<<c.axes[i].source<<"\n";
        output<<axis_ids[i]<<".joy_axis="<<c.axes[i].joy_source<<"\n";
        output<<axis_ids[i]<<".invert="<<int(c.axes[i].inverted)<<"\n";
    }
    output.flush();
    return output.good();
}
inline std::string pad_label(const Binding& b, bool raw=false) {
    if(raw) {
        if(b.joy_button>=0)return "Button "+std::to_string(b.joy_button);
        if(b.joy_axis>=0)return "Axis "+std::to_string(b.joy_axis)+(b.joy_sign>0?"+":"-");
        if(b.joy_hat>=0)return "Hat "+std::to_string(b.joy_hat)+"/"+std::to_string(b.joy_hat_value);
        return "Unbound";
    }
    if(b.pad_button>=0&&b.pad_button<SDL_CONTROLLER_BUTTON_MAX)
        return SDL_GameControllerGetStringForButton(static_cast<SDL_GameControllerButton>(b.pad_button));
    if(b.pad_axis>=0&&b.pad_axis<SDL_CONTROLLER_AXIS_MAX)
        return std::string(SDL_GameControllerGetStringForAxis(static_cast<SDL_GameControllerAxis>(b.pad_axis)))+
            (b.pad_sign>0?"+":"-");
    return "Unbound";
}
}
