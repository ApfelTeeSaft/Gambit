#include <windows.h>
#include <wincodec.h>

int main() {
    const GUID format{};
    IWICBitmapSource* converted = nullptr;
    const HRESULT result = WICConvertBitmapSource(format, nullptr, &converted);
    return SUCCEEDED(result) ? 1 : 0;
}
