cmake_minimum_required(VERSION 3.11)

project(retro_game)

add_executable(${PROJECT_NAME} main.c player/player.c enemies/enemy.c ui/ui.c background/background.c map/map.c map/map.h
        map/map.c)

include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2_IMAGE REQUIRED SDL2_image)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${SDL2_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
)

if(PSP)

    # Create an EBOOT.PBP file
    create_pbp_file(
        TARGET ${PROJECT_NAME}
        ICON_PATH NULL
        BACKGROUND_PATH NULL
        PREVIEW_PATH NULL
        TITLE ${PROJECT_NAME}
        VERSION 01.00
    )
    #create a prx file
    target_compile_definitions(${PROJECT_NAME} PRIVATE BUILD_PRX=1)

endif()
