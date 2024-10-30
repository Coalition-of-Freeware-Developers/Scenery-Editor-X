# CMake generated Testfile for 
# Source directory: G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug
# Build directory: G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
if(CTEST_CONFIGURATION_TYPE MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
  add_test(test-bug_ms_vec_static "G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/Debug/test-bug_ms_vec_static.exe")
  set_tests_properties(test-bug_ms_vec_static PROPERTIES  _BACKTRACE_TRIPLES "G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/CMakeLists.txt;65;add_test;G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/CMakeLists.txt;1;glmCreateTestGTC;G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
  add_test(test-bug_ms_vec_static "G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/Release/test-bug_ms_vec_static.exe")
  set_tests_properties(test-bug_ms_vec_static PROPERTIES  _BACKTRACE_TRIPLES "G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/CMakeLists.txt;65;add_test;G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/CMakeLists.txt;1;glmCreateTestGTC;G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
  add_test(test-bug_ms_vec_static "G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/MinSizeRel/test-bug_ms_vec_static.exe")
  set_tests_properties(test-bug_ms_vec_static PROPERTIES  _BACKTRACE_TRIPLES "G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/CMakeLists.txt;65;add_test;G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/CMakeLists.txt;1;glmCreateTestGTC;G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/CMakeLists.txt;0;")
elseif(CTEST_CONFIGURATION_TYPE MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
  add_test(test-bug_ms_vec_static "G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/RelWithDebInfo/test-bug_ms_vec_static.exe")
  set_tests_properties(test-bug_ms_vec_static PROPERTIES  _BACKTRACE_TRIPLES "G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/CMakeLists.txt;65;add_test;G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/CMakeLists.txt;1;glmCreateTestGTC;G:/ART_REPO-TRAY/X-Plane 12/Scenery-Editor-X/external/glm/test/bug/CMakeLists.txt;0;")
else()
  add_test(test-bug_ms_vec_static NOT_AVAILABLE)
endif()
