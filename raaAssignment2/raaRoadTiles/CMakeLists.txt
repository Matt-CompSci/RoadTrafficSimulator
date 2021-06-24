file(GLOB source_files "*.h" "*.cpp")

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




add_library(raaRoadTiles ${source_files})
config_project( raaRoadTiles OPENTHREADS )
config_project( raaRoadTiles OSG )
config_project( raaRoadTiles OSGDB )
config_project( raaRoadTiles OSGUTIL )
config_project( raaRoadTiles OSGVIEWER )
config_project( raaRoadTiles OSGSIM )
config_project( raaRoadTiles OSGFX )
config_project( raaRoadTiles OSGGA )
config_project( raaRoadTiles OSGTERRAIN )
config_project( raaRoadTiles OSGTEXT )
config_project( raaRoadTiles OSGWIDGET )
config_project( raaRoadTiles OSGMANIPULATOR )
config_project( raaRoadTiles OSGSHADOW )



install(TARGETS raaRoadTiles
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
