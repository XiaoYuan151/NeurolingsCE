# cmake/MacApp.cmake  (include mode)
#
# Defines the 'macapp' custom target that assembles a macOS .app bundle
# from src/packaging/NeurolingsCE.app, copies the built binary into
# Contents/MacOS, and runs macdeployqt to embed Qt frameworks.
#
# Required variables (set before include()-ing this file):
#   MACAPP_TARGET           - CMake target to package (e.g. NeurolingsCE)
#   MACAPP_BUNDLE_NAME      - .app folder name (e.g. NeurolingsCE)
#   MACAPP_BUNDLE_EXEC      - CFBundleExecutable / Contents/MacOS/<name>
#   MACAPP_SKELETON_DIR     - Path to src/packaging/NeurolingsCE.app
#   MACAPP_OUTPUT_DIR       - Where to place the generated .app
#
# Optional:
#   MACAPP_EXTRA_INCLUDES   - Extra files to copy into the .app
#   MACAPP_CLI_TARGET       - CLI target whose binary should ride along
#                             inside Contents/MacOS (alongside the GUI)

foreach(_var MACAPP_TARGET MACAPP_BUNDLE_NAME MACAPP_BUNDLE_EXEC
             MACAPP_SKELETON_DIR MACAPP_OUTPUT_DIR)
  if(NOT DEFINED ${_var})
    message(FATAL_ERROR "cmake/MacApp.cmake: ${_var} must be set before include()-ing this file.")
  endif()
endforeach()

# Locate macdeployqt. Prefer the one shipped with the Qt6 install we just
# resolved via find_package; fall back to PATH.
get_target_property(_qmake_exec Qt6::qmake IMPORTED_LOCATION)
if(_qmake_exec)
  get_filename_component(_qt_bin_dir "${_qmake_exec}" DIRECTORY)
endif()

find_program(MACDEPLOYQT_EXECUTABLE
  NAMES macdeployqt6 macdeployqt
  HINTS ${_qt_bin_dir}
  DOC "Path to macdeployqt(6)")

if(NOT MACDEPLOYQT_EXECUTABLE)
  message(WARNING "cmake/MacApp.cmake: macdeployqt not found; the 'macapp' "
                  "target will skip Qt framework embedding. Set "
                  "MACDEPLOYQT_EXECUTABLE on the cmake command line if needed.")
endif()

set(_app_bundle "${MACAPP_OUTPUT_DIR}/${MACAPP_BUNDLE_NAME}.app")

# Use a stamp file as the custom command's OUTPUT so add_custom_target can
# wire dependencies cleanly.
set(_macapp_stamp "${CMAKE_CURRENT_BINARY_DIR}/macapp.stamp")

# Extra `-libpath=` hints. Homebrew installs each Qt keg into its own prefix
# (qtsvg, qtimageformats, ...), so macdeployqt's auto-discovery — which only
# follows the binary's direct rpaths — can't find sibling frameworks like
# QtSvg that are pulled in transitively by plugins. Pass each known keg dir
# explicitly. Plus anything the caller wants to add via
# MACDEPLOYQT_EXTRA_ARGS.
set(_deploy_extra_args "")
foreach(_keg qtsvg qtimageformats qtmultimedia qttools qtbase)
  if(EXISTS "/opt/homebrew/opt/${_keg}/lib")
    list(APPEND _deploy_extra_args "-libpath=/opt/homebrew/opt/${_keg}/lib")
  endif()
endforeach()
if(DEFINED MACDEPLOYQT_EXTRA_ARGS)
  list(APPEND _deploy_extra_args ${MACDEPLOYQT_EXTRA_ARGS})
endif()

set(_deploy_cmd "${CMAKE_COMMAND}" -E echo "macdeployqt not found; skipping framework embed")
if(MACDEPLOYQT_EXECUTABLE)
  set(_deploy_cmd "${MACDEPLOYQT_EXECUTABLE}" "${_app_bundle}" ${_deploy_extra_args})
endif()

set(_extra_copy_cmds "")
if(DEFINED MACAPP_CLI_TARGET AND TARGET ${MACAPP_CLI_TARGET})
  list(APPEND _extra_copy_cmds
    COMMAND ${CMAKE_COMMAND} -E copy
      "$<TARGET_FILE:${MACAPP_CLI_TARGET}>"
      "${_app_bundle}/Contents/MacOS/")
endif()

add_custom_command(
  OUTPUT ${_macapp_stamp}
  COMMAND ${CMAKE_COMMAND} -E rm -rf "${_app_bundle}"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${MACAPP_OUTPUT_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    "${MACAPP_SKELETON_DIR}" "${_app_bundle}"
  # The skeleton ships Info.plist.in alongside Info.plist so configure_file
  # has something to expand. macdeployqt would otherwise try to codesign
  # the unrelated .in file as a bundle component, breaking the signature.
  COMMAND ${CMAKE_COMMAND} -E rm -f "${_app_bundle}/Contents/Info.plist.in"
  COMMAND ${CMAKE_COMMAND} -E make_directory "${_app_bundle}/Contents/MacOS"
  COMMAND ${CMAKE_COMMAND} -E copy
    "$<TARGET_FILE:${MACAPP_TARGET}>"
    "${_app_bundle}/Contents/MacOS/${MACAPP_BUNDLE_EXEC}"
  ${_extra_copy_cmds}
  COMMAND ${_deploy_cmd}
  # macdeployqt signs each dylib individually but sometimes touches a dylib
  # again after signing it (in particular libbrotlicommon.1.dylib), which
  # invalidates the per-dylib signatures. A final adhoc deep re-sign of the
  # whole bundle covers that. `-` is the adhoc identity.
  COMMAND codesign --force --deep --sign - "${_app_bundle}"
  COMMAND ${CMAKE_COMMAND} -E touch ${_macapp_stamp}
  DEPENDS ${MACAPP_TARGET}
  COMMENT "Assembling ${MACAPP_BUNDLE_NAME}.app and running macdeployqt"
  VERBATIM
)

add_custom_target(macapp DEPENDS ${_macapp_stamp})
