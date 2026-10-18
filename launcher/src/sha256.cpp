#include "sha256.hpp"
#include <array>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <vector>

namespace spl::crypto {
namespace {
constexpr uint32_t constants[64] = {
    0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
    0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
    0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
    0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
    0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
    0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
    0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
    0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
};
uint32_t rotate(uint32_t x, unsigned n) { return (x >> n) | (x << (32 - n)); }
struct Digest {
    uint32_t h[8] = {0x6a09e667,0xbb67ae85,0x3c6ef372,0xa54ff53a,
                      0x510e527f,0x9b05688c,0x1f83d9ab,0x5be0cd19};
    std::array<uint8_t, 64> buffer{};
    uint64_t bytes = 0;
    size_t buffered = 0;
    void block(const uint8_t* p) {
        uint32_t w[64];
        for (int i=0; i<16; ++i) w[i] = uint32_t(p[i*4])<<24 | uint32_t(p[i*4+1])<<16 | uint32_t(p[i*4+2])<<8 | p[i*4+3];
        for (int i=16; i<64; ++i) {
            auto a=w[i-15], b=w[i-2];
            w[i] = (rotate(b,17)^rotate(b,19)^(b>>10)) + w[i-7]
                   + (rotate(a,7)^rotate(a,18)^(a>>3)) + w[i-16];
        }
        auto a=h[0],b=h[1],c=h[2],d=h[3],e=h[4],f=h[5],g=h[6],z=h[7];
        for (int i=0;i<64;++i) {
            uint32_t t1=z+(rotate(e,6)^rotate(e,11)^rotate(e,25))+((e&f)^(~e&g))+constants[i]+w[i];
            uint32_t t2=(rotate(a,2)^rotate(a,13)^rotate(a,22))+((a&b)^(a&c)^(b&c));
            z=g;g=f;f=e;e=d+t1;d=c;c=b;b=a;a=t1+t2;
        }
        const uint32_t values[]{a,b,c,d,e,f,g,z};
        for(int i=0;i<8;++i) h[i]+=values[i];
    }
    void update(const uint8_t* p, size_t n) {
        bytes += n;
        while(n) {
            size_t count=std::min(n,64-buffered);
            std::copy(p,p+count,buffer.begin()+buffered);
            p+=count;n-=count;buffered+=count;
            if(buffered==64){block(buffer.data());buffered=0;}
        }
    }
    std::string finish() {
        uint64_t bits=bytes*8;
        uint8_t pad[128]{};pad[0]=0x80;
        size_t padLength = buffered<56 ? 56-buffered : 120-buffered;
        update(pad,padLength);
        for(int i=7;i>=0;--i) pad[7-i]=uint8_t(bits>>(i*8));
        update(pad,8);
        std::ostringstream s;s<<std::hex<<std::setfill('0');
        for(auto x:h) s<<std::setw(8)<<x;
        return s.str();
    }
};
}
std::string sha256(const std::filesystem::path& file) {
    std::ifstream in(file,std::ios::binary);
    if(!in) throw std::runtime_error("Cannot read file for SHA-256: " + file.string());
    Digest digest;
    std::array<char, 1<<16> buffer{};
    while(in) {
        in.read(buffer.data(),buffer.size());
        auto count=in.gcount();
        if(count>0) digest.update(reinterpret_cast<const uint8_t*>(buffer.data()),size_t(count));
    }
    if(in.bad()) throw std::runtime_error("Error hashing file: " + file.string());
    return digest.finish();
}
}
