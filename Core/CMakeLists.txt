INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    "${CMAKE_SOURCE_DIR}/CustomVtkFilters"
)

QT4_WRAP_CPP(SketchBioCoreMoc
sketchmodel.h
structurereplicator.h
)
SOURCE_GROUP("Generated" FILES ${SketchBioCoreMoc})

SET(SketchBioCoreSrcs
transformmanager.cpp
transformmanager.h
structurereplicator.cpp
structurereplicator.h
measuringtape.cpp
measuringtape.h
modelmanager.cpp
modelmanager.h
sketchobject.cpp
sketchobject.h
modelinstance.cpp
modelinstance.h
objectgroup.cpp
objectgroup.h
objectchangeobserver.h
OperationState.h
sketchmodel.cpp
sketchmodel.h
modelutilities.cpp
modelutilities.h
worldmanager.cpp
worldmanager.h
connector.cpp
connector.h
springconnection.cpp
springconnection.h
hand.cpp
hand.h
sketchproject.cpp
sketchproject.h
physicsutilities.cpp
physicsutilities.h
physicsstrategy.cpp
physicsstrategy.h
physicsstrategyfactory.cpp
physicsstrategyfactory.h
transformequals.cpp
transformequals.h
undostate.cpp
undostate.h
keyframe.cpp
keyframe.h
colormaptype.cpp
colormaptype.h
groupidgenerator.h
sketchioconstants.h
sketchtests.h
SettingsHelpers.cpp
SettingsHelpers.h
${SketchBioCoreMoc}
)

ADD_LIBRARY(SketchBioCore ${SketchBioCoreSrcs})

ADD_SUBDIRECTORY( test )
