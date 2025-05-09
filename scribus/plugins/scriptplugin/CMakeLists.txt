include_directories(
	${CMAKE_SOURCE_DIR}
	${CMAKE_SOURCE_DIR}/scribus
	${Python3_INCLUDE_DIRS}
	${SCRIBUS_AUTOGEN_INCLUDE_PATH}
)

# Gentoo has an option for no examples. Let's make the ebuild simpler and open up the option to others, but leave as a default for us
if (NOT WANT_NOEXAMPLES)
	add_subdirectory(samples)
endif ()
add_subdirectory(scripts)

set(SCRIPTER_PLUGIN_SOURCES
	cmdannotations.cpp
	cmdcell.cpp
	cmdcolor.cpp
	cmddialog.cpp
	cmddoc.cpp
	cmdgetprop.cpp
	cmdgetsetprop.cpp
	cmdmani.cpp
	cmdmisc.cpp
	cmdobj.cpp
	cmdpage.cpp
	cmdsetprop.cpp
	cmdstyle.cpp
	cmdtable.cpp
	cmdtext.cpp
	cmdutil.cpp
	guiapp.cpp
	objimageexport.cpp
	objpdffile.cpp
	objprinter.cpp
	pconsole.cpp
	prefs_scripter.cpp
	pyesstring.cpp
	runscriptdialog.cpp
	scriptercore.cpp
	scriptpaths.cpp
	scriptplugin.cpp
	svgimport.cpp
)

set(SCRIBUS_SCRIPTER_PLUGIN "scriptplugin")

add_library(${SCRIBUS_SCRIPTER_PLUGIN} MODULE 
	${SCRIPTER_PLUGIN_SOURCES}
)

target_link_libraries(${SCRIBUS_SCRIPTER_PLUGIN}
	${Python3_LIBRARIES}
	${SCRIBUS_API_LIB}
)

target_link_libraries(${SCRIBUS_SCRIPTER_PLUGIN} ${EXE_NAME})

if(WANT_PCH)
	target_precompile_headers(${SCRIBUS_SCRIPTER_PLUGIN} PRIVATE "../plugins_pch.h")
endif()

install(TARGETS ${SCRIBUS_SCRIPTER_PLUGIN}
	LIBRARY
	DESTINATION ${PLUGINDIR}
	PERMISSIONS ${PLUGIN_PERMISSIONS}
)

add_dependencies(Plugins ${SCRIBUS_SCRIPTER_PLUGIN})
