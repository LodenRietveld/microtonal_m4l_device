set(MAX_SDK_SRC /Users/macbook/data/Coding_ext/dependencies/max-sdk/source)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

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