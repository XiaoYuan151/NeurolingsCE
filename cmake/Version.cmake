function(load_version FILE)
  file(STRINGS "${FILE}" VERSION_LINES)

  set(_keys
    VERSION VERSION_MAJOR VERSION_MINOR VERSION_PATCH VERSION_NAME RELEASE_DATE
    APP_NAME APP_DISPLAY_NAME APP_EXECUTABLE APP_DESCRIPTION
    APP_COMPANY APP_COPYRIGHT
    APP_BUNDLE_ID APP_BUNDLE_NAME
    APP_ICON_NAME APP_DESKTOP_CATEGORIES
    EXTENSION_UUID EXTENSION_NAME EXTENSION_DESCRIPTION
  )

  foreach(_key IN LISTS _keys)
    set(${_key} "")
  endforeach()

  foreach(LINE ${VERSION_LINES})
    foreach(_key IN LISTS _keys)
      if(LINE MATCHES "^${_key}=(.*)$")
        set(${_key} "${CMAKE_MATCH_1}")
        break()
      endif()
    endforeach()
  endforeach()

  foreach(_key IN LISTS _keys)
    set(${_key} "${${_key}}" PARENT_SCOPE)
  endforeach()
  set(PROJECT_VERSION "${VERSION}" PARENT_SCOPE)
endfunction()
