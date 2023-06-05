include(GNUInstallDirs)

include(AddLLVM)
include(AddClang)

set(LLVM_LINK_COMPONENTS
  Support
)
add_clang_executable(concrete-class concrete-class.cpp)
target_link_libraries(concrete-class
    PRIVATE
    clangAST
    clangBasic
    clangFrontend
    clangSerialization
    clangTooling
)
target_include_directories(concrete-class
    PRIVATE
    ${CLANG_INCLUDE_DIRS}
    ${LLVM_INCLUDE_DIRS}
)

install(TARGETS concrete-class
    EXPORT ${PROJECT_NAME}Targets)
install(EXPORT ${PROJECT_NAME}Targets DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake)

