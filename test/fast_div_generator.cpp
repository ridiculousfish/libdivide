// Silence MSVC sprintf unsafe warnings
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <sstream>
#include <iomanip>
#include <limits>
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
void generate_constant_macro(_IntT denom) {
    auto magic = libdivide_gen(denom);
    std::cout << "#define " << const_macro_name(denom)+"_MAGIC" << " "  << magic_tostr(magic.magic) << "\n";
    std::cout << "#define " << const_macro_name(denom)+"_MORE" << " "  << more_tostr(magic.more) << "\n";
} 


template <typename _IntT>
void generate_specialized_template(_IntT denom) {
    auto magic = libdivide_gen(denom);
    std::cout << "template<> struct libdivide_constants"
        << "<" << type_name<_IntT>::get_name() << "," << denom << "> "
        << "{ "
        << "static constexpr " << struct_selector<_IntT>::get_name() << " libdivide = { "
        <<              ".magic = " << magic_tostr(magic.magic) << ", "
        <<              ".more = " <<  more_tostr(magic.more) << "};"
        << "};\n";
}

template <typename _IntT>
void generate(bool generate_template) {
    void (*pGen)(_IntT) = &generate_constant_macro<_IntT>;
    if (generate_template)
    {
        pGen = &generate_specialized_template<_IntT>;
    }
    generator(pGen);
}


int main(int argc, char *argv[]) {
    if (argc!=3) {
        std::cout
                << "Usage: fast_div_generator [DATATYPE] [STYLE]\n"
                   "\n"
                   "[DATATYPE] in [s16, u16]\n"
                   "[STYLE] in [MACRO, TEMPLATE]"
                << std::endl;
            exit(1);        
    }

    std::string data_type(to_lower(argv[1]));
    bool generate_template = "template"==to_lower(argv[2]);

    if (data_type == type_tag<int16_t>::get_tag()) {
        generate<int16_t>(generate_template);
    } else if (data_type == type_tag<uint16_t>::get_tag()) {
        generate<uint16_t>(generate_template);
    }
}