#include "constexpr_test.h"
#include "outputs.h"
#include "type_mappings.h"

template <typename _IntT>
static void assert_constexpr(_IntT result, _IntT dividend, _IntT divisor) {
    _IntT expected = dividend / divisor;
    PRINT_INFO(F("Testing constexpr generation: "));
    PRINT_INFO(type_name<_IntT>::get_name());
    if (result!=expected) {
        PRINT_INFO(": ");
        PRINT_INFO(dividend);
        PRINT_INFO(" / ");
        PRINT_INFO(divisor);
        PRINT_INFO(" = ");
        PRINT_INFO(expected);
        PRINT_ERROR(F(" FAILED  ("));
        PRINT_ERROR(result);
        PRINT_ERROR(F(")"));
        PRINT_ERROR(F("\n"));
        exit(1);
    } else {
        PRINT_INFO(F("\n"));
    }
}
void test_constexpr(void) {
    // {
    //     constexpr int16_t dividend = -17359;
    //     int16_t divisor = libdivide::libdivide_s16_recover(constexprS16);
    //     assert_constexpr(libdivide::libdivide_s16_do(dividend, constexprS16), dividend, divisor);
    //     std::cout << "Branch free "; assert_constexpr(libdivide::libdivide_s16_branchfree_do(dividend, constexprS16BF), dividend, divisor);
    // }
    // {
    //     constexpr uint16_t dividend = 17359;
    //     constexpr uint16_t divisor = 99;
    //     constexpr auto constexprU16 = libdivide::libdivide_u16_gen(divisor);
    //     assert_constexpr(libdivide::libdivide_u16_do(dividend, constexprU16), dividend, divisor);
    //     constexpr auto constexprU16BF = libdivide::libdivide_u16_branchfree_gen(divisor);
    //     std::cout << "Branch free "; assert_constexpr(libdivide::libdivide_u16_branchfree_do(dividend, constexprU16BF), dividend, divisor);
    // }
    {
        constexpr int32_t dividend = INT32_MAX/7U;
        int32_t divisor = libdivide::libdivide_s32_recover(constexprS32);
        assert_constexpr(libdivide::libdivide_s32_do(dividend, constexprS32), dividend, divisor);
        std::cout << "Branch free "; assert_constexpr(libdivide::libdivide_s32_branchfree_do(dividend, constexprS32BF), dividend, divisor);
    }    
    {
        constexpr uint32_t dividend = UINT32_MAX/7U;
        uint32_t divisor = libdivide::libdivide_u32_recover(constexprU32);
        assert_constexpr(libdivide::libdivide_u32_do(dividend, constexprU32), dividend, divisor);
        std::cout << "Branch free "; assert_constexpr(libdivide::libdivide_u32_branchfree_do(dividend, constexprU32BF), dividend, divisor);
    }   
    {
        constexpr int64_t dividend = INT64_MAX/7U;
        int64_t divisor = libdivide::libdivide_s64_recover(constexprS64);
        assert_constexpr(libdivide::libdivide_s64_do(dividend, constexprS64), dividend, divisor);
        std::cout << "Branch free "; assert_constexpr(libdivide::libdivide_s64_branchfree_do(dividend, constexprS64BF), dividend, divisor);
    } 
    {
        constexpr uint64_t dividend = UINT64_MAX/7U;
        uint64_t divisor = libdivide::libdivide_u64_recover(constexprU64);
        assert_constexpr(libdivide::libdivide_u64_do(dividend, constexprU64), dividend, divisor);
        std::cout << "Branch free "; assert_constexpr(libdivide::libdivide_u64_branchfree_do(dividend, constexprU64BF), dividend, divisor);
    }          
}