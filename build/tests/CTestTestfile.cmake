# CMake generated Testfile for 
# Source directory: C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/tests
# Build directory: C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/build/tests
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(CoreTests "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/build/Debug/test_core.exe")
  set_tests_properties(CoreTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/tests/CMakeLists.txt;9;add_test;C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(CoreTests "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/build/Release/test_core.exe")
  set_tests_properties(CoreTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/tests/CMakeLists.txt;9;add_test;C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(CoreTests "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/build/MinSizeRel/test_core.exe")
  set_tests_properties(CoreTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/tests/CMakeLists.txt;9;add_test;C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/tests/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(CoreTests "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/build/RelWithDebInfo/test_core.exe")
  set_tests_properties(CoreTests PROPERTIES  _BACKTRACE_TRIPLES "C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/tests/CMakeLists.txt;9;add_test;C:/Users/nikla/OneDrive/lathe_ecosystem/cam/IntuiCAM/tests/CMakeLists.txt;0;")
else()
  add_test(CoreTests NOT_AVAILABLE)
endif()
