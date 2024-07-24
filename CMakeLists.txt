# 要求的cmake最低版本
cmake_minimum_required (VERSION 3.8)

# 如果支持，请为 MSVC 编译器启用热重载。
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

# 工程名称
project (MyJSON)
# 生成共享库
add_library(lljson lljson.c)
# 生成可执行文件
add_executable (MyJSON test.c)
# 连接共享库
target_link_libraries(MyJSON lljson)

