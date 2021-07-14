// Silence MSVC sprintf unsafe warnings
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <sstream>
#include <iomanip>
#include <string>
#include <type_traits>
#include <algorithm>
#include <ctype.h>

#include "libdivide.h"
#include "type_mappings.h"

inline std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
               [](unsigned char c){ return (char)::tolower(c); });
    return str;
}

inline std::string to_upper(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
               [](unsigned char c){ return (char)::toupper(c); });
    return str;
}

template <typename _IntT>
void generator(void (*pGen)(_IntT)) {
    // We're dealing with integer division. For any denominator:
    //  if (numerator<denominator) numerator/denominator==0
    //  if (numerator==denominator) numerator/denominator==1
    // Compilers know this. So there's no point in generating constants > MAX/2. 
    for (_IntT denom = std::numeric_limits<_IntT>::min()/2;
            denom<std::numeric_limits<_IntT>::max()/2;
            ++denom ) {
        if (denom!=0) {
            pGen(denom);
        }
    }
}

template <typename _IntT>
static std::string const_macro_name(_IntT denom) {
    std::string name(to_upper(type_tag<_IntT>::get_tag()));
    name += "LD_DENOM_";
    if (denom<0) {
        name += "MINUS_";
        denom = -denom;
    }
    name += std::to_string(denom);
    return name;
}

template <typename _IntT>
static std::string magic_tostr(_IntT magic) {
    // Don't use hex representation here. Some hex literals are considered unsigned, so
    // will cause a warning when cast to unsigned
    std::stringstream stream;
    stream << "(" << type_name<_IntT>::get_name() << ")" << magic;
    return stream.str();
}

static std::string more_tostr(uint8_t more) {
    std::stringstream stream;
    stream << "(uint8_t)" << (uint16_t)more;
    return stream.str();
}

template <typename _IntT>
void generate_constant(_IntT denom) {
    auto magic = libdivide_gen(denom);
    std::cout << "#define " << const_macro_name(denom)+"_MAGIC" << " "  << magic_tostr(magic.magic) << "\n";
    std::cout << "#define " << const_macro_name(denom)+"_MORE" << " "  << more_tostr(magic.more) << "\n";
} 

int main(int argc, char *argv[]) {
    if (argc!=2) {
        std::cout
                << "Usage: fast_div_generator [DATATYPE]\n"
                   "\n"
                   "[DATATYPE] in [s16, u16]"
                << std::endl;
            exit(1);        
    }

    std::string data_type(to_lower(argv[1]));
    
    if (data_type == type_tag<int16_t>::get_tag()) {
        generator(&generate_constant<int16_t>);
    } else if (data_type == type_tag<uint16_t>::get_tag()) {
        generator(&generate_constant<uint16_t>);
    }
}