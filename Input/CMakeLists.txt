INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/core
)

QT4_WRAP_CPP(SketchBioInputMoc
vrpnserver.h
hydrainputmanager.h
hydrainputmode.h
objectgrabmode.h
coloreditingmode.h
transformeditingmode.h
springeditingmode.h
animationmode.h
)
SOURCE_GROUP("Generated" FILES ${SketchBioInputMoc})

SET(SketchBioInputSrcs
vrpnserver.cpp  # h file used by Moc, no need to include here
hydrainputmanager.cpp  # h file used by Moc, no need to include here
hydrainputmode.cpp  # h file used by Moc, no need to include here
objectgrabmode.cpp  # h file used by Moc, no need to include here
coloreditingmode.cpp  # h file used by Moc, no need to include here
transformeditingmode.cpp  # h file used by Moc, no need to include here
springeditingmode.cpp  # h file used by Moc, no need to include here
animationmode.cpp  # h file used by Moc, no need to include here
${SketchBioInputMoc}
)
ADD_LIBRARY(SketchBioInput ${SketchBioInputSrcs})
