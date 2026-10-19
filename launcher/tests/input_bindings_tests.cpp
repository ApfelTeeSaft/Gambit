#include "input_bindings.h"
#include <array>
#include <filesystem>
#include <iostream>

#define CHECK(cond) do {if(!(cond)) {std::cerr<<"Check failed at line "<<__LINE__<<": "<<#cond<<"\n";return 1;}} while(0)
int main() {
    using namespace spl_controls;
    const auto original=defaults();
    CHECK(original.binding[B].key==SDL_SCANCODE_SPACE);
    CHECK(original.binding[ZR].mouse==SDL_BUTTON_LEFT);
    CHECK(original.binding[ZL].pad_axis==SDL_CONTROLLER_AXIS_TRIGGERLEFT);
    CHECK(original.axes[3].inverted);
    CHECK(original.mouse_camera);
    CHECK(original.mouse_sensitivity>0);
    const auto missing=std::filesystem::temp_directory_path()/"splport-missing-controls.ini";
    const auto fallback=load(missing);
    CHECK(fallback.binding[B].key==SDL_SCANCODE_SPACE);
    auto modified=defaults();
    modified.binding[B].key=SDL_SCANCODE_J;
    modified.binding[ZR].mouse=4;
    modified.binding[A].pad_button=SDL_CONTROLLER_BUTTON_X;
    modified.binding[MoveUp].joy_button=11;
    modified.binding[Right].joy_hat=0;
    modified.binding[Right].joy_hat_value=SDL_HAT_RIGHT;
    modified.binding[ZL].joy_axis=5;
    modified.binding[ZL].joy_sign=-1;
    modified.axes[2].joy_source=7;
    modified.axes[2].inverted=true;
    modified.mouse_camera=false;
    modified.mouse_sensitivity=0.009f;
    modified.controller_guid="030000005e0400008e02000014010000";
    const auto dir=std::filesystem::temp_directory_path()/"splport-controls-roundtrip";
    const auto path=dir/"controls.ini";
    CHECK(save(path,modified));
    const auto loaded=load(path);
    CHECK(loaded.binding[B].key==SDL_SCANCODE_J);
    CHECK(loaded.binding[ZR].mouse==4);
    CHECK(loaded.binding[A].pad_button==SDL_CONTROLLER_BUTTON_X);
    CHECK(loaded.binding[MoveUp].joy_button==11);
    CHECK(loaded.binding[Right].joy_hat_value==SDL_HAT_RIGHT);
    CHECK(loaded.binding[ZL].joy_sign==-1);
    CHECK(loaded.axes[2].joy_source==7 && loaded.axes[2].inverted);
    CHECK(!loaded.mouse_camera);
    CHECK(std::abs(loaded.mouse_sensitivity-0.009f)<0.00001f);
    CHECK(loaded.controller_guid==modified.controller_guid);
    std::array<Uint8,SDL_NUM_SCANCODES> keys{};
    keys[SDL_SCANCODE_J]=1;
    CHECK(keyboard_mouse(loaded.binding[B],keys.data(),0));
    keys[SDL_SCANCODE_J]=0;
    CHECK(!keyboard_mouse(loaded.binding[B],keys.data(),0));
    CHECK(keyboard_mouse(loaded.binding[ZR],keys.data(),SDL_BUTTON(4)));
    CHECK(!gamepad(loaded.binding[ZL],nullptr));
    {
        std::ofstream corrupt(path,std::ios::trunc);
        corrupt<<"b.key=999999\nmouse_camera=x\nsensitivity=nan\nzl.joy_axis=999\n"
                  "left_x.axis=9999\ncontroller_guid=../../evil\n";
    }
    const auto invalid=load(path);
    CHECK(invalid.binding[B].key==SDL_SCANCODE_SPACE);
    CHECK(invalid.binding[ZL].joy_axis==-1);
    CHECK(invalid.axes[0].source==SDL_CONTROLLER_AXIS_LEFTX);
    CHECK(invalid.controller_guid.empty());
    CHECK(invalid.mouse_camera);
    CHECK(std::abs(invalid.mouse_sensitivity-0.005f)<0.00001f);
    std::error_code ec;
    std::filesystem::remove_all(dir,ec);
    std::cout<<"Controller/key/mouse binding defaults and persistence passed\n";
    return 0;
}
