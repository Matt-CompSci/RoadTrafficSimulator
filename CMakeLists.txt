# Set the name of the project and target:
SET(TARGET "raaAssignment")



# Create cmake project
cmake_minimum_required( VERSION 2.6 )
project( ${TARGET} )

# Import OpenSceneGraph
find_package( OpenThreads )
find_package( osg )
find_package( osgDB )
find_package( osgUtil )
find_package( osgViewer )

macro( config_project PROJNAME LIBNAME )
include_directories( ${${LIBNAME}_INCLUDE_DIR} )
target_link_libraries( ${PROJNAME} ${${LIBNAME}_LIBRARY} )
endmacro()




file(GLOB source_files
    "raaAssignment2/raaAssignment2/*.h"
    "raaAssignment2/raaAssignment2/*.cpp"
)



add_executable( ${TARGET} ${source_files} )
config_project( ${TARGET} OPENTHREADS )
config_project( ${TARGET} OSG )
config_project( ${TARGET} OSGDB )
config_project( ${TARGET} OSGUTIL )
config_project( ${TARGET} OSGVIEWER )
config_project( ${TARGET} OSGSIM )
config_project( ${TARGET} OSGFX )
config_project( ${TARGET} OSGGA )
config_project( ${TARGET} OSGTERRAIN )
config_project( ${TARGET} OSGTEXT )
config_project( ${TARGET} OSGWIDGET )
config_project( ${TARGET} OSGMANIPULATOR )
config_project( ${TARGET} OSGSHADOW )