#include <iostream>
#include <fmt/core.h>

#include "GuiManager.h"

int main( int argc, char* argv[] )
{
    std::cout << "Hello, World!" << std::endl;
    fmt::print( "Hello, fmt World!\n" );

    GUIManager guiManager;

    if ( !guiManager.Initialize( 1280, 720, "League of Legends Item Set Generator" ) )
    {
        std::cerr << "Failed to initialize GUI" << std::endl;
        return 1;
    }

    guiManager.SetWindowOffset( 24.0f );

    while ( !guiManager.ShouldClose() )
    {
        guiManager.Render();
    }

    return 0;
}
