########### CMakeMaker autogenerated file ###########
cmake_minimum_required(VERSION 3.6)



set(cpp_files
  ./source/input_factory.cc
  ./source/opengl/system/gl_input_system.cc
  ./source/system/input_system.cc
  ./include/input_commands.h
  ./include/input_factory.h
  ./include/key_definitions.h
  ./include/component/contiguous_input.h
  ./include/component/cursor_input.h
  ./include/component/toggle_input.h
  ./include/system/input_system.h
  ./source/opengl/system/gl_input_system.h
)

source_group(include FILES
  ./include/input_commands.h
  ./include/input_factory.h
  ./include/key_definitions.h
)

source_group(include/component FILES
  ./include/component/contiguous_input.h
  ./include/component/cursor_input.h
  ./include/component/toggle_input.h
)

source_group(include/system FILES
  ./include/system/input_system.h
)

source_group(source FILES
  ./source/input_factory.cc
)

source_group(source/opengl FILES
)

source_group(source/opengl/system FILES
  ./source/opengl/system/gl_input_system.cc
  ./source/opengl/system/gl_input_system.h
)

source_group(source/system FILES
  ./source/system/input_system.cc
)

add_library(input STATIC ${cpp_files})

include_directories(input
  ../slib_core/./include/
  ../slib_core/./include/math/
  ../slib_core/./include/templates/types/
  ../slib_core/include
  ../slib_core/include/math
  ../slib_core/include/templates/types
  ../slib_graphics/./include/
  ../slib_graphics/include
  ../source_shared
  ./include
  ./include/component
  ./include/system
  ./source
  ./source/opengl
  ./source/opengl/system
  ./source/system
  D:/API/PhysX/PhysX_3.4/Include
  D:/API/PhysX/PxShared/include
  D:/API/include
  D:/API/tbb/include
  F:/Dependencies/1.2.141.2/Include
)

target_link_libraries(input
  core
  graphics
  D:/API/lib/release/static/glfw3.lib
  optimized D:/API/PhysX/PhysX_3.4/Lib/vc15win64/PhysX3_x64.lib
  optimized D:/API/PhysX/PhysX_3.4/Lib/vc15win64/PhysX3Common_x64.lib
  optimized D:/API/PhysX/PhysX_3.4/Lib/vc15win64/PhysX3Cooking_x64.lib
  optimized D:/API/PhysX/PhysX_3.4/Lib/vc15win64/PhysX3Extensions.lib
  optimized D:/API/PhysX/PhysX_3.4/Lib/vc15win64/PhysX3CharacterKinematic_x64.lib
  optimized D:/API/PhysX/PxShared/lib/vc15win64/PxFoundation_x64.lib
  optimized D:/API/PhysX/PxShared/lib/vc15win64/PxPvdSDK_x64.lib
  debug D:/API/PhysX/PhysX_3.4/Lib/vc15win64/PhysX3DEBUG_x64.lib
  debug D:/API/PhysX/PhysX_3.4/Lib/vc15win64/PhysX3CommonDEBUG_x64.lib
  debug D:/API/PhysX/PhysX_3.4/Lib/vc15win64/PhysX3CookingDEBUG_x64.lib
  debug D:/API/PhysX/PhysX_3.4/Lib/vc15win64/PhysX3ExtensionsDEBUG.lib
  debug D:/API/PhysX/PhysX_3.4/Lib/vc15win64/PhysX3CharacterKinematicDEBUG_x64.lib
  debug D:/API/PhysX/PxShared/lib/vc15win64/PxFoundationDEBUG_x64.lib
  debug D:/API/PhysX/PxShared/lib/vc15win64/PxPvdSDKDEBUG_x64.lib
  optimized D:/API/tbb/lib/intel64/vc14/tbb.lib
  debug D:/API/tbb/lib/intel64/vc14/tbb_debug.lib
)

add_dependencies(input ALL_PRE_BUILD)