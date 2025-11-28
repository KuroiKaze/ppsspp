cmake_minimum_required(VERSION 3.11)

project(retro_game)

add_executable(${PROJECT_NAME} main.c player/player.c enemies/enemy.c ui/ui.c background/background.c map/map.c map/map.h)

include(FindPkgConfig)
pkg_search_module(SDL2 REQUIRED sdl2)
pkg_search_module(SDL2_IMAGE REQUIRED SDL2_image)

target_include_directories(${PROJECT_NAME} PRIVATE
        ${SDL2_INCLUDE_DIRS}
        ${SDL2_IMAGE_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
        SDL2
        SDL2_image

        # Bildformate
        png
        jpeg
        z

        # Grafik & Audio
        GL
        pspvram
        pspvfpu
        pspgu
        pspaudio
        psphprm

        # Input & Debug
        pspctrl
        pspdebug      # <--- WICHTIG: Hier hinzufügen! Fix für pspDebugScreen...

        # Kernel
        pspdisplay
        pspge
        pspsdk
        pspuser
        pspnet

        # Standard
        m
        c
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
            BUILD_PRX # PRX Flag
    )
endif()