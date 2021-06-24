# Get files from raaAssignment2 Project using GLOB
file(GLOB source_files
    "*.h"
    "*.cpp"
)

# Import OpenSceneGraph
find_package( OpenThreads )
find_package( osg )
find_package( osgDB )
find_package( osgUtil )
find_package( osgViewer )
find_package( osgSim )
find_package( osgFX )
find_package( osgGA )
find_package( osgTerrain )
find_package( osgText )
find_package( osgWidget )
find_package( osgManipulator )
find_package( osgShadow )

macro( config_project PROJNAME LIBNAME )
include_directories( ${${LIBNAME}_INCLUDE_DIR} )
target_link_libraries( ${PROJNAME} ${${LIBNAME}_LIBRARY} )
endmacro()

add_executable(raaAssignment2 ${source_files})
config_project( raaAssignment2 OPENTHREADS )
config_project( raaAssignment2 OSG )
config_project( raaAssignment2 OSGDB )
config_project( raaAssignment2 OSGUTIL )
config_project( raaAssignment2 OSGVIEWER )
config_project( raaAssignment2 OSGSIM )
config_project( raaAssignment2 OSGFX )
config_project( raaAssignment2 OSGGA )
config_project( raaAssignment2 OSGTERRAIN )
config_project( raaAssignment2 OSGTEXT )
config_project( raaAssignment2 OSGWIDGET )
config_project( raaAssignment2 OSGMANIPULATOR )
config_project( raaAssignment2 OSGSHADOW )

# Include raaRoadTiles Project
include_directories(../${CMAKE_SOURCE_DIR}/raaRoadTiles)
target_link_libraries(raaAssignment2 raaRoadTiles)

# Install
install(TARGETS raaAssignment2 RUNTIME DESTINATION bin)