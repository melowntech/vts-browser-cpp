
function(buildsys_xcode_codesign target)
  message(STATUS "attempting to codesign ${target}")
  if (BUILDSYS_IOS AND NOT BUILDSYS_IOS_NO_CODESIGN_ID)
    set(DEVELOPMENT_TEAM_ID "$ENV{DEVELOPMENT_TEAM_ID}" CACHE STRING "Your TeamID Eg. 2347GVV3KC")
    if (DEVELOPMENT_TEAM_ID)
      set_target_properties(${target} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${DEVELOPMENT_TEAM_ID}
      )
      message(STATUS "codesign enabled for ${target}")
    endif()
  endif()
endfunction(buildsys_xcode_codesign)

