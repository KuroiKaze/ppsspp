cmake_minimum_required(VERSION 3.11)

project(retro_game)

add_executable(${PROJECT_NAME} main.c player/player.c 
    ui/ui.c background/background.c 
    map/map.c
    entity/entity.c
    enemies/enemy.c
    enemies/mummy/mummy.c
    enemies/slime/slime.c
    level/level.c
    interactables/chest.c
    bgm/bgmHandler.c
    )

include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2_IMAGE REQUIRED SDL2_image)
pkg_search_module(SDL2_TTF REQUIRED SDL2_ttf)
pkg_search_module(SDL2_MIXER REQUIRED SDL2_mixer)


target_include_directories(${PROJECT_NAME} PRIVATE
    ${SDL2_INCLUDE_DIRS}
    ${SDL2_IMAGE_INCLUDE_DIRS}
    ${SDL2_TTF_INCLUDE_DIRS}
    ${SDL2_MIXER_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        ${SDL2_LIBRARIES}
        ${SDL2_IMAGE_LIBRARIES}
        ${SDL2_TTF_LIBRARIES}
        ${SDL2_MIXER_LIBRARIES}
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
