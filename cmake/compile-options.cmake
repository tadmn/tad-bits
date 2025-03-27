
if (APPLE)
    enable_language(OBJC)
    enable_language(OBJCXX)
    set(CMAKE_OBJC_VISIBILITY_PRESET hidden)
    set(CMAKE_OBJCXX_VISIBILITY_PRESET hidden)
endif()

set(BUILD_SHARED_LIBS OFF CACHE BOOL "Never want shared if not specified")
if (${BUILD_SHARED_LIBS})
    message(WARNING "You have set BUILD_SHARED_LIBS to ON")
endif()

set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Compiler specific choices
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    add_compile_options(
            $<$<BOOL:${USE_SANITIZER}>:-fsanitize=address>
            $<$<BOOL:${USE_SANITIZER}>:-fsanitize=undefined>

            $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJC>,$<COMPILE_LANGUAGE:OBJCXX>>:-fno-char8_t>
    )

    add_link_options(
            $<$<BOOL:${USE_SANITIZER}>:-fsanitize=address>
            $<$<BOOL:${USE_SANITIZER}>:-fsanitize=undefined>
    )
    if (NOT APPLE)
        add_compile_options(-march=nehalem)
    endif()
endif()


if (WIN32)
    add_compile_definitions(_CRT_SECURE_NO_WARNINGS=1)
endif()

if (MSVC)
    add_compile_options(
            # Set source and executable charsets to UTF-8
            $<$<CXX_COMPILER_ID:MSVC>:/utf-8>
            # Do *not* use the new, breaking char8_t UTF-8 bits in C++20.
            $<$<COMPILE_LANGUAGE:CXX>:/Zc:char8_t->
            # make msvc define __cplulsplus properly
            $<$<COMPILE_LANGUAGE:CXX>:/Zc:__cplusplus>
    )
endif()

function(add_compiler_warnings TARGET)
    if((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang"))
        target_compile_options(${TARGET} PRIVATE
          -Werror
          -Wall
          -Wshadow-all
          -Wstrict-aliasing
          -Wuninitialized
          -Wunused-parameter
          -Wint-conversion
          -Wconditional-uninitialized
          -Wconstant-conversion
          -Wbool-conversion
          -Wextra-semi
          -Wunreachable-code
          -Wcast-align
          -Wshift-sign-overflow
          -Wmissing-prototypes
          -Wnullable-to-nonnull-conversion
          -Wno-ignored-qualifiers
          -Wswitch-enum
          -Wpedantic
          -Wdeprecated
          -Wfloat-equal
          -Wmissing-field-initializers
          $<$<OR:$<COMPILE_LANGUAGE:CXX>,$<COMPILE_LANGUAGE:OBJCXX>>:
          -Wzero-as-null-pointer-constant
          -Wunused-private-field
          -Woverloaded-virtual
          -Wreorder
          -Winconsistent-missing-destructor-override>
          $<$<OR:$<COMPILE_LANGUAGE:OBJC>,$<COMPILE_LANGUAGE:OBJCXX>>:
          -Wunguarded-availability
          -Wunguarded-availability-new>)
    endif()
endfunction()