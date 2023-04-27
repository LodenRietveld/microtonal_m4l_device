# Requires cmake >= 3.26 for mac silicon
# Set this to where your max-sdk is installed (Download from https://github.com/Cycling74/max-sdk/)
set(MAX_SDK_SRC /Users/macbook/data/Coding_ext/dependencies/max-sdk/source)

# Used with vscode for intellisense/clang-tidy linting
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Set this to either the ableton max externals location or the regular max externals location (depending on where you want to use it)
set(C74_LIBRARY_OUTPUT_DIRECTORY "/Applications/Ableton Live 11 Suite.app/Contents/App-Resources/Max/Max.app/Contents/Resources/C74/externals/max/")

include(${MAX_SDK_SRC}/max-sdk-base/script/max-pretarget.cmake)

#############################################################
# MAX EXTERNAL
#############################################################

include_directories( 
	"${MAX_SDK_INCLUDES}"
	"${MAX_SDK_MSP_INCLUDES}"
	"${MAX_SDK_JIT_INCLUDES}"
)

file(GLOB PROJECT_SRC
     "*.h"
	 "*.c"
)
add_library( 
	${PROJECT_NAME} 
	MODULE
	${PROJECT_SRC}
)

include(${MAX_SDK_SRC}/max-sdk-base/script/max-posttarget.cmake)