
function(buildsys_xcode_codesign target)
  if (BUILDSYS_IOS)
    set(DEVELOPMENT_TEAM_ID "" CACHE STRING "Your TeamID Eg. 2347GVV3KC")
    if (DEVELOPMENT_TEAM_ID)
      set_target_properties(${target} PROPERTIES
        XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "iPhone Developer"
        XCODE_ATTRIBUTE_DEVELOPMENT_TEAM ${DEVELOPMENT_TEAM_ID}
      )
    endif()
  endif()
endfunction(buildsys_xcode_codesign)

