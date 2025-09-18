#pragma once 
#ifdef __clang__
#define RSL_CONSUMABLE(_typestate) [[clang::consumable(_typestate)]]
#define RSL_RETURN_TYPESTATE(_typestate) [[clang::return_typestate(_typestate)]]
#define RSL_SET_TYPESTATE(_typestate) [[clang::set_typestate(_typestate)]]
#define RSL_TEST_TYPESTATE(_typestate) [[clang::test_typestate(_typestate)]]
#define RSL_CALLABLE_WHEN(_typestate) [[clang::callable_when(_typestate)]]
#else
#define RSL_CONSUMABLE(_typestate)
#define RSL_RETURN_TYPESTATE(_typestate)
#define RSL_SET_TYPESTATE(_typestate)
#define RSL_TEST_TYPESTATE(_typestate)
#define RSL_CALLABLE_WHEN(_typestate)
#endif