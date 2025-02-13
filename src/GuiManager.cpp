#include "GUIManager.h"
#include "nlohmann/json.hpp"
#include <fstream>
#include <filesystem>
#define STB_IMAGE_IMPLEMENTATION
// #include "stb_image.h"
#include <stb_image.h>
#include <curl/curl.h>
#include <GLFW/glfw3.h>
#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>
#include <Windows.h>
// #include <misc/freetype/imgui_freetype.h>
#include <imgui_freetype.h>

static void glfw_error_callback(int error, const char *description)
{
    std::cerr << "GLFW Error " << error << ": " << description << std::endl;
}

GUIManager::GUIManager()
    : window(nullptr),
      backgroundTexture(0),
      championSplashTexture(0),
      windowOffset(10.0f),
      currentState(WindowState::Default),
      selectedChampionIndex(-1),
      isChampionSplashLoaded(false),
      iconTexture(0),
      isIconLoaded(false),
      skillTextures(5, 0),
      areSkillIconsLoaded(false),
      isRandomizing(false),
      hasRandomChampion(false),
      comboSelectedIndex(-1),
      historyFilePath(std::filesystem::current_path() / "item_history.txt") 
{
    // nothing
}

GUIManager::~GUIManager()
{
    isRandomizing.store(false);
    if (randomizationThread.joinable())
    {
        randomizationThread.join();
    }
    if (images[0].pixels)
    {
        stbi_image_free(images[0].pixels);
    }
    Cleanup();
}

bool GUIManager::Initialize( int width, int height, const char* title )
{
    glfwSetErrorCallback( glfw_error_callback );
    if ( !glfwInit() )
    {
        return false;
    }

    // Get the primary monitor
    GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
    if ( !primaryMonitor )
    {
        std::cerr << "Failed to get primary monitor" << std::endl;
        return false;
    }

    // Get the monitor's resolution
    const GLFWvidmode* mode = glfwGetVideoMode( primaryMonitor );
    if ( !mode )
    {
        std::cerr << "Failed to get video mode" << std::endl;
        return false;
    }

    // Calculate the position for the center of the screen
    int xpos = ( mode->width  - width  ) / 2 ;
    int ypos = ( mode->height - height ) / 2 ;

    glfwWindowHint( GLFW_SCALE_TO_MONITOR, GLFW_TRUE );
    glfwWindowHint( GLFW_DECORATED, GLFW_FALSE );
    window = glfwCreateWindow( width, height, title, NULL, NULL );
    if ( window == NULL )
    {
        return false;
    }

    // Set the window position to center it
    glfwSetWindowPos( window, xpos, ypos );

    // HICON hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_ICON1));
    // if (hIcon) {
    // 	HWND hwnd = glfwGetWin32Window(window);
    // 	SendMessage(hwnd, WM_SETICON, ICON_SMALL, (LPARAM)hIcon);
    // 	SendMessage(hwnd, WM_SETICON, ICON_BIG, (LPARAM)hIcon);
    // }

    glfwMakeContextCurrent( window );
    glfwSwapInterval( 1 );

    windowSize = ImVec2( width, height );
    glfwSetWindowSizeCallback( window, WindowResizeCallback );

    float xScale = 1.0f, yScale = 1.0f;
    // glfwGetWindowContentScale(window, &xScale, &yScale);

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ApplyCustomStyles();
    ImGuiIO &io = ImGui::GetIO();

    // defaultFont = io.Fonts->AddFontFromFileTTF(".\\assets\\recharge bd.ttf", 14.0f * xScale );
    //defaultFont = io.Fonts->AddFontFromFileTTF(".\\data\\recharge bd.ttf", 14.0f * xScale);
    defaultFont = io.Fonts->AddFontFromFileTTF(".\\data\\D2Coding-Ver1.3.2-20180524.ttf", 14.0f * xScale, NULL, io.Fonts->GetGlyphRangesKorean() );
    if (defaultFont == nullptr)
    {
        std::cerr << "Failed to load default font" << std::endl;
        return false;
    }

    // smallFont = io.Fonts->AddFontFromFileTTF(".\\assets\\recharge bd.ttf", 10.0f * xScale );
    smallFont = io.Fonts->AddFontFromFileTTF(".\\data\\recharge bd.ttf", 10.0f * xScale);
    if (smallFont == nullptr)
    {
        std::cerr << "Failed to load small font" << std::endl;
        return false;
    }

    ImGui_ImplGlfw_InitForOpenGL( window, true );
    ImGui_ImplOpenGL3_Init( "#version 130" );

    // backgroundTexture = LoadTexture(".\\assets\\image.png");
    backgroundTexture = this->LoadTexture( ".\\data\\image.png" );

    if ( !dataManager.FetchLanguageData())
    {
        std::cerr << "Failed to fetch language data" << std::endl;
        return false;
    }

    if ( !dataManager.FetchChampionData() )
    {
        std::cerr << "Failed to fetch champion data" << std::endl;
        return false;
    }

    if ( !dataManager.FetchItemData() )
    {
        std::cerr << "Failed to fetch item data" << std::endl;
        return false;
    }

    InitializeHistory();

    // if (!LoadIconTexture(".\\assets\\icon.png")) {
    if ( !LoadIconTexture( ".\\data\\icon.png" ) )
    {
        std::cerr << "Failed to load icon texture" << std::endl;
    }

    return true;
}

void GUIManager::HandleDragging()
{
    if (ImGui::IsMouseDragging(0) && ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByPopup | ImGuiHoveredFlags_AllowWhenBlockedByActiveItem))
    {
        ImVec2 delta = ImGui::GetMouseDragDelta(0);
        if (delta.x != 0 || delta.y != 0)
        {
            int x, y;
            glfwGetWindowPos(window, &x, &y);
            glfwSetWindowPos(window, x + delta.x, y + delta.y);
            ImGui::ResetMouseDragDelta(0);
        }
    }
}

void GUIManager::Render()
{
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

    HandleDragging();

    RenderBackground();
    RenderGUI();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    glfwSwapBuffers(window);
}

bool GUIManager::ShouldClose()
{
    return glfwWindowShouldClose(window);
}

void GUIManager::Cleanup()
{
    glDeleteTextures(1, &backgroundTexture);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    for (auto &texture : skillTextures)
    {
        if (texture != 0)
        {
            glDeleteTextures(1, &texture);
        }
    }
    glfwDestroyWindow(window);
    glfwTerminate();
}

void GUIManager::ApplyCustomStyles()
{
    ImGuiStyle &style = ImGui::GetStyle();
    style.FrameRounding = 5.0f;
    style.GrabRounding = 5.0f;
    style.FramePadding = ImVec2(10, 10);
    style.ItemSpacing = ImVec2(10, 10);

    ImVec4 *colors = style.Colors;
    colors[ImGuiCol_Button] = ImVec4(0.3608f, 0.3608f, 0.3608f, 0.7f);
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.1412f, 0.1412f, 0.1412f, 0.9f);
    colors[ImGuiCol_ButtonActive] = ImVec4(0.2f, 0.2f, 0.2f, 1.0f);
}

/**
 * 텍스쳐 이미지 로드하여 OpenGL 텍스쳐 생성 후 ID 반환
 */
GLuint GUIManager::LoadTexture( const char* filename )
{
    int width, height, nrChannels;

    // 이미지 데이터 로드
    unsigned char* data = stbi_load( filename, &width, &height, &nrChannels, 0 );

    GLuint texture;
    glGenTextures( 1, &texture );
    glBindTexture( GL_TEXTURE_2D, texture );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );

    // 이미지 데이터 해제
    stbi_image_free( data );
    return texture;
}

void GUIManager::RenderBackground()
{
    int display_w, display_h;
    glfwGetFramebufferSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.45f, 0.55f, 0.60f, 1.00f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, backgroundTexture);
    glBegin(GL_QUADS);
    glTexCoord2f(0, 1);
    glVertex2f(-1, -1);
    glTexCoord2f(1, 1);
    glVertex2f(1, -1);
    glTexCoord2f(1, 0);
    glVertex2f(1, 1);
    glTexCoord2f(0, 0);
    glVertex2f(-1, 1);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

void GUIManager::RenderGUI()
{
    ImGuiViewport *viewport = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(viewport->Pos);
    ImGui::SetNextWindowSize(viewport->Size);

    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

    ImColor separatorColor(ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::Begin("Full Window", nullptr,
                 ImGuiWindowFlags_NoDecoration |
                     ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoSavedSettings |
                     ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoNavFocus |
                     ImGuiWindowFlags_NoScrollWithMouse);

    float windowWidth = ImGui::GetWindowWidth();
    float buttonHeight = 30.0f;
    float logoWidth = 32.0f;         // Width of your logo
    float closeButtonsWidth = 60.0f; // Width of close and minimize buttons
    float mainButtonsWidth = windowWidth - logoWidth - closeButtonsWidth;
    float sectionWidth = mainButtonsWidth / 3.0f;
    float separatorThickness = 0.5f;

    // Logo
    ImGui::SetCursorPos(ImVec2(0, 0));
    if (isIconLoaded)
    {
        // ImGui::Image((void*)(intptr_t)iconTexture, ImVec2(logoWidth, buttonHeight));
        ImGui::Image((ImTextureID)(intptr_t)iconTexture, ImVec2(logoWidth, buttonHeight));
    }

    // Define colors for normal and hovered states
    ImVec4 normalTextColor(1.0f, 1.0f, 1.0f, 1.0f);           // White
    ImVec4 hoveredTextColor(0.8431f, 0.7255f, 0.4745f, 1.0f); // Gold/Yellow

    // Push custom styles for main buttons
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.0706f, 0.0706f, 0.0706f, 0.2f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.1412f, 0.1412f, 0.1412f, 0.4f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleVar(ImGuiStyleVar_FrameRounding, 0.0f);

    // Champions button
    ImGui::SetCursorPos(ImVec2(logoWidth + separatorThickness, 0));
    bool championsHovered = ImGui::IsMouseHoveringRect(
        ImGui::GetCursorScreenPos(),
        ImVec2(ImGui::GetCursorScreenPos().x + sectionWidth - separatorThickness, ImGui::GetCursorScreenPos().y + buttonHeight));
    ImGui::PushStyleColor(ImGuiCol_Text, championsHovered ? hoveredTextColor : normalTextColor);
    if (ImGui::Button("CHAMPIONS", ImVec2(sectionWidth - separatorThickness, buttonHeight)))
    {
        currentState = WindowState::Champions;
    }
    ImGui::PopStyleColor();

    // Items button
    ImGui::SetCursorPos(ImVec2(logoWidth + sectionWidth + separatorThickness, 0));
    bool itemsHovered = ImGui::IsMouseHoveringRect(
        ImGui::GetCursorScreenPos(),
        ImVec2(ImGui::GetCursorScreenPos().x + sectionWidth - separatorThickness, ImGui::GetCursorScreenPos().y + buttonHeight));
    ImGui::PushStyleColor(ImGuiCol_Text, itemsHovered ? hoveredTextColor : normalTextColor);
    if (ImGui::Button("ITEMS", ImVec2(sectionWidth - separatorThickness, buttonHeight)))
    {
        currentState = WindowState::Items;
    }
    ImGui::PopStyleColor();

    // Summoner's Spells button
    ImGui::SetCursorPos(ImVec2(logoWidth + sectionWidth * 2 + separatorThickness, 0));
    bool spellsHovered = ImGui::IsMouseHoveringRect(
        ImGui::GetCursorScreenPos(),
        ImVec2(ImGui::GetCursorScreenPos().x + sectionWidth - separatorThickness, ImGui::GetCursorScreenPos().y + buttonHeight));
    ImGui::PushStyleColor(ImGuiCol_Text, spellsHovered ? hoveredTextColor : normalTextColor);
    if (ImGui::Button("SUMMONER'S SPELLS", ImVec2(sectionWidth - separatorThickness, buttonHeight)))
    {
        currentState = WindowState::SummonerSpells;
    }
    ImGui::PopStyleColor();

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    // Draw separator before close buttons
    ImGui::GetWindowDrawList()->AddRectFilled(
        ImVec2(windowWidth - closeButtonsWidth - separatorThickness, 0),
        ImVec2(windowWidth - closeButtonsWidth, buttonHeight),
        separatorColor);

    // Close and minimize buttons
    ImGui::SetCursorPos(ImVec2(windowWidth - closeButtonsWidth, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 0.5f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.5f, 0.5f, 0.5f, 0.5f));

    // Horizontal separator below all buttons
    ImGui::GetWindowDrawList()->AddLine(
        ImVec2(0, buttonHeight),
        ImVec2(windowWidth, buttonHeight),
        separatorColor);

    if (ImGui::Button("--", ImVec2(30, buttonHeight)))
    {
        glfwIconifyWindow(window);
    }
    ImGui::SameLine(0, 0);
    if (ImGui::Button("X", ImVec2(30, buttonHeight)))
    {
        glfwSetWindowShouldClose(window, GLFW_TRUE);
    }

    ImGui::PopStyleColor(3);
    ImGui::PopStyleVar();

    // Handle dragging (allow dragging from the top bar)
    if (ImGui::IsMouseClicked(0) && ImGui::IsMouseHoveringRect(ImVec2(0, 0), ImVec2(windowWidth, buttonHeight)))
    {
        isDragging = true;
        dragStartPos = ImGui::GetMousePos();
    }
    else if (ImGui::IsMouseReleased(0))
    {
        isDragging = false;
    }

    if (isDragging)
    {
        ImVec2 delta = ImVec2(ImGui::GetMousePos().x - dragStartPos.x, ImGui::GetMousePos().y - dragStartPos.y);
        int x, y;
        glfwGetWindowPos(window, &x, &y);
        glfwSetWindowPos(window, x + delta.x, y + delta.y);
        dragStartPos = ImGui::GetMousePos();
    }

    // Add a small resize handle in the bottom-right corner
    ImVec2 windowPos = ImGui::GetWindowPos();
    ImVec2 windowSize = ImGui::GetWindowSize();
    ImVec2 resizeHandlePos = ImVec2(windowPos.x + windowSize.x - 20, windowPos.y + windowSize.y - 20);
    ImGui::GetWindowDrawList()->AddTriangleFilled(
        ImVec2(resizeHandlePos.x + 20, resizeHandlePos.y + 20),
        ImVec2(resizeHandlePos.x + 20, resizeHandlePos.y),
        ImVec2(resizeHandlePos.x, resizeHandlePos.y + 20),
        IM_COL32(200, 200, 200, 255));

    // Handle resizing
    if (ImGui::IsMouseHoveringRect(resizeHandlePos, ImVec2(resizeHandlePos.x + 20, resizeHandlePos.y + 20)))
    {
        ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeNWSE);
        if (ImGui::IsMouseClicked(0))
        {
            isResizing = true;
            resizeStartPos = ImGui::GetMousePos();
        }
    }

    if (ImGui::IsMouseReleased(0))
    {
        isResizing = false;
    }

    if (isResizing)
    {
        ImVec2 delta = ImVec2(ImGui::GetMousePos().x - resizeStartPos.x, ImGui::GetMousePos().y - resizeStartPos.y);
        windowSize.x += delta.x;
        windowSize.y += delta.y;
        glfwSetWindowSize(window, windowSize.x, windowSize.y);
        resizeStartPos = ImGui::GetMousePos();
    }

    // Render the appropriate window based on the current state
    ImGui::SetCursorPos(ImVec2(0, buttonHeight));
    ImGui::BeginChild("Content Window", ImVec2(viewport->Size.x, viewport->Size.y - buttonHeight), false,
                      ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings);

    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));

    switch (currentState)
    {
    case WindowState::Champions:
        RenderChampionsWindow();
        break;
    case WindowState::Items:
        RenderItemsWindow();
        break;
    case WindowState::SummonerSpells:
        RenderSummonerSpellsWindow();
        break;
    default:
        RenderDefaultWindow();
        break;
    }

    ImGui::PopStyleColor();
    ImGui::EndChild();

    ImGui::End();
    ImGui::PopStyleVar(3);
}

void GUIManager::RenderDefaultWindow()
{
    auto title = u8"Select one of the options above(선택하세요)";

    ImVec2 textSize = ImGui::CalcTextSize(title);
    ImVec2 windowSize = ImGui::GetContentRegionAvail();
    ImVec2 cursorPos(
        (windowSize.x - textSize.x) * 0.5f,
        (windowSize.y - textSize.y) * 0.5f);

    ImGui::SetCursorPos(cursorPos);
    ImGui::Text(title);
}

// Champions window functions implementation -------------------------------------------------------------------------------------------------------------------
void GUIManager::RenderChampionsWindow()
{
    const auto &championNames = dataManager.GetChampionNames();

    // Display champion splash art as background if a champion is selected
    if (isChampionSplashLoaded && selectedChampionIndex >= 0)
    {
        ImGui::GetWindowDrawList()->AddImage(
            //(void*)(intptr_t)championSplashTexture,
            (ImTextureID)(intptr_t)championSplashTexture,
            ImGui::GetWindowPos(),
            ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 128) // 50% opacity
        );
    }

    // Create a semi-transparent overlay for the controls
    ImGui::SetCursorPos(ImVec2(0, 0));
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.1f, 0.1f, 0.1f, 0.0f));
    ImGui::BeginChild("ControlsOverlay", ImVec2(ImGui::GetWindowWidth(), 60), false, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoScrollbar);

    // Set custom colors
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));        // Dark background for input fields
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // Slightly lighter when hovered
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));  // Even lighter when active
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));        // Dark background for combo popup
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));           // White text
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));         // Slightly lighter background for selected item
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));  // Even lighter for hovered item
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));   // Lightest for active/clicked item
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));         // Dark button color
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));  // Lighter button color when hovered
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));   // Even lighter when clicked

    // Champion selection dropdown with search
    static char searchBuffer[256] = "";
    ImGui::SetNextItemWidth(300); // Set the width of the combo box to 300 pixels
    ImGui::SetCursorPos(ImVec2(10, 5));
    if (ImGui::BeginCombo("##ChampionSelect", selectedChampionIndex >= 0 ? championNames[selectedChampionIndex].c_str() : "Select Champion"))
    {
        // Add a search input field at the top of the combo box
        ImGui::PushItemWidth(-1);
        ImGui::SetCursorPos(ImVec2(10, 10));
        if (ImGui::InputText("##Search", searchBuffer, IM_ARRAYSIZE(searchBuffer)))
        {
            // Convert search to lowercase for case-insensitive comparison
            std::string search = searchBuffer;
            std::transform(search.begin(), search.end(), search.begin(), ::tolower);
        }
        ImGui::PopItemWidth();
        ImGui::Separator();

        for (int i = 0; i < championNames.size(); i++)
        {
            // Convert champion name to lowercase for case-insensitive comparison
            std::string lowerChampName = championNames[i];
            std::transform(lowerChampName.begin(), lowerChampName.end(), lowerChampName.begin(), ::tolower);

            // Only display champions that match the search
            if (lowerChampName.find(searchBuffer) != std::string::npos)
            {
                bool is_selected = (selectedChampionIndex == i);
                if (ImGui::Selectable(championNames[i].c_str(), is_selected))
                {
                    if (selectedChampionIndex != i)
                    { // Check if a different champion is selected
                        selectedChampionIndex = i;
                        std::string championId = dataManager.GetChampionId(championNames[i]);
                        LoadChampionSplash(championId);
                        LoadChampionIcon(championId);
                        areSkillIconsLoaded = false;
                        selectedSkill = "";    // Reset selected skill when changing champion
                        skillDescription = ""; // Clear skill description
                        // Reset tip-related states
                        showAllyTip = false;
                        showEnemyTip = false;
                        allyTips.clear();
                        enemyTips.clear();
                        allyTipIndices.clear();
                        enemyTipIndices.clear();
                        currentAllyTipIndex = 0;
                        currentEnemyTipIndex = 0;
                    }
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Pop custom colors
    ImGui::PopStyleColor(11);

    // Add Random Champion button
    ImGui::SameLine();
    if (ImGui::Button("Random Champion"))
    {
        if (!isRandomizing.load())
        {
            RandomizeChampion();
        }
    }
    ImGui::EndChild();
    ImGui::PopStyleColor(); // Pop the ChildBg color

    // Load the randomly selected champion
    if (!isRandomizing.load() && hasRandomChampion.load())
    {
        hasRandomChampion.store(false); // Reset the flag

        std::string championName = championNames[selectedChampionIndex];
        std::string championId = dataManager.GetChampionId(championName);
        LoadChampionSplash(championId);
        LoadChampionIcon(championId);
        areSkillIconsLoaded = false;
        selectedSkill = "";
        skillDescription = "";
        showAllyTip = false;
        showEnemyTip = false;
        allyTips.clear();
        enemyTips.clear();
        allyTipIndices.clear();
        enemyTipIndices.clear();
        currentAllyTipIndex = 0;
        currentEnemyTipIndex = 0;
    }

    // Display champion splash art as background
    if (isChampionSplashLoaded && selectedChampionIndex >= 0)
    {
        ImGui::GetWindowDrawList()->AddImage(
            //(void*)(intptr_t)championSplashTexture,
            (ImTextureID)(intptr_t)championSplashTexture,
            ImGui::GetWindowPos(),
            ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y),
            ImVec2(0, 0), ImVec2(1, 1),
            IM_COL32(255, 255, 255, 128) // 50% opacity
        );
    }

    if (selectedChampionIndex >= 0)
    {
        std::string championName = championNames[selectedChampionIndex];
        std::string championId = dataManager.GetChampionId(championName);

        // Display champion icon
        if (isChampionIconLoaded)
        {
            ImGui::SetCursorPos(ImVec2(10, 50));
            // ImGui::Image((void*)(intptr_t)championIconTexture, ImVec2(64, 64));
            ImGui::Image((ImTextureID)(intptr_t)championIconTexture, ImVec2(64, 64));
        }

        // Display champion info
        ImGui::SetCursorPos(ImVec2(10, 120));
        ImGui::BeginChild("ChampionInfo", ImVec2(300, 500), true, ImGuiWindowFlags_NoScrollbar);

        nlohmann::json stats = dataManager.GetChampionStats(championName);
        std::string title = dataManager.GetChampionTitle(championName);
        std::string lore = dataManager.GetChampionLore(championName);
        auto tags = dataManager.GetChampionTags(championName);
        ImGui::Indent(5.0f);
        ImGui::Text("Champion: %s", championName.c_str());
        ImGui::Text("Title: %s", title.c_str());
        ImGui::Text("Tags: ");
        for (const auto &tag : tags)
        {
            ImGui::SameLine();
            ImGui::Text("%s", tag.c_str());
        }
        ImGui::Text("Base Stats:");
        ImGui::Text("HP: %.0f (+ %.0f per level)", stats["hp"].get<float>(), stats["hpperlevel"].get<float>());
        ImGui::Text("Armor: %.1f (+ %.2f per level)", stats["armor"].get<float>(), stats["armorperlevel"].get<float>());
        ImGui::Text("Magic Resist: %.1f (+ %.2f per level)", stats["spellblock"].get<float>(), stats["spellblockperlevel"].get<float>());
        ImGui::Text("Move Speed: %.0f", stats["movespeed"].get<float>());
        ImGui::Text("Attack Damage: %.0f (+ %.0f per level)", stats["attackdamage"].get<float>(), stats["attackdamageperlevel"].get<float>());
        ImGui::Text("Attack Speed: %.3f (+ %.1f%% per level)", stats["attackspeed"].get<float>(), stats["attackspeedperlevel"].get<float>());
        ImGui::Text("Attack Range: %.0f", stats["attackrange"].get<float>());
        ImGui::Text("HP Regen: %.1f (+ %.1f per level)", stats["hpregen"].get<float>(), stats["hpregenperlevel"].get<float>());
        ImGui::Unindent(5.0f);
        ImGui::EndChild();

        // Display champion lore
        ImGui::SetCursorPos(ImVec2(320, 120)); // Adjust position as needed
        ImGui::BeginChild("ChampionLore", ImVec2(ImGui::GetWindowWidth() - 330, 100), true, ImGuiWindowFlags_HorizontalScrollbar);

        ImGui::Indent(5.0f);
        ImGui::TextWrapped("%s", lore.c_str());
        ImGui::Unindent(5.0f);
        ImGui::EndChild();

        // Load and display skill icons
        if (!areSkillIconsLoaded)
        {
            LoadSkillIcons(championId);
        }

        // Display skill icons and buttons
        ImGui::SetCursorPos(ImVec2(320, 230));
        std::string skillNames[] = {"Passive", "Q", "W", "E", "R"};
        float buttonWidth = 80.0f;
        float buttonHeight = 35.0f;
        float iconSize = 75.0f;
        float spacing = 15;

        for (int i = 0; i < 5; ++i)
        {
            if (i > 0)
                ImGui::SameLine(0, spacing);

            ImGui::BeginGroup();
            if (skillTextures[i] != 0)
            {
                // ImGui::Image((void*)(intptr_t)skillTextures[i], ImVec2(iconSize, iconSize));
                ImGui::Image((ImTextureID)(intptr_t)skillTextures[i], ImVec2(iconSize, iconSize));
            }

            std::string buttonLabel = (i == 0) ? "Passive" : skillNames[i] + " Ability";
            bool isSelected = (selectedSkill == (i == 0 ? "Passive" : championId + " " + skillNames[i]));

            if (isSelected)
            {
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8431f, 0.7255f, 0.4745f, 1.0f));
            }

            if (ImGui::Button(buttonLabel.c_str(), ImVec2(buttonWidth, buttonHeight)))
            {
                if (isSelected)
                {
                    selectedSkill = "";
                    skillDescription = "";
                }
                else
                {
                    selectedSkill = (i == 0) ? "Passive" : championId + " " + skillNames[i];
                    skillDescription = skillDescriptions[selectedSkill];
                }
            }

            if (isSelected)
            {
                ImGui::PopStyleColor();
            }

            ImGui::EndGroup();
        }

        // Display skill description
        if (!selectedSkill.empty())
        {
            ImGui::SetCursorPos(ImVec2(320, 360)); // Adjusted position
            ImGui::BeginChild("SkillDescription", ImVec2(ImGui::GetWindowWidth() - 330, 70), true, ImGuiWindowFlags_HorizontalScrollbar);
            ImGui::Indent(5.0f);
            ImGui::TextWrapped("%s", skillDescription.c_str());
            ImGui::Unindent(5.0f);
            ImGui::EndChild();
        }

        // Add Skins button
        ImGui::SetCursorPos(ImVec2(320, 440)); // Adjust this position as needed
        if (ImGui::Button("Skins"))
        {
            showSkins = !showSkins; // Toggle skins display
            if (showSkins)
            {
                currentSkinIndex = 0; // Reset to first skin when showing skins
            }
        }

        // Display skins if showSkins is true
        if (showSkins)
        {
            auto skins = dataManager.GetChampionSkins(championName);
            if (!skins.empty())
            {
                const auto &currentSkin = skins[currentSkinIndex];
                std::string skinName = currentSkin["name"];
                std::string skinNum = std::to_string(currentSkin["num"].get<int>());
                std::string skinKey = championId + "_" + skinNum;

                // Load skin texture if not already loaded
                if (skinTextures.find(skinKey) == skinTextures.end())
                {
                    std::string skinImageUrl = dataManager.GetChampionSkinImageUrl(championId, skinNum);
                    skinTextures[skinKey] = LoadSkinTexture(skinImageUrl);
                }

                // Display skin image
                ImGui::SetCursorPos(ImVec2(390, 440)); // Adjust position as needed
                // ImGui::Image((void*)(intptr_t)skinTextures[skinKey], ImVec2(240, 136)); // Adjust size as needed
                ImGui::Image((ImTextureID)(intptr_t)skinTextures[skinKey], ImVec2(240, 136)); // Adjust size as needed

                // Display skin name in a chat box style
                ImGui::SetCursorPos(ImVec2(390, 580)); // Adjusted position
                ImGui::BeginChild("SkinName", ImVec2(240, 40), true);
                ImGui::Indent(5.0f);
                ImGui::Text("%s", skinName.c_str());
                ImGui::Unindent(5.0f);
                ImGui::EndChild();

                // Navigation buttons outside the chat box
                ImGui::SetCursorPos(ImVec2(390, 625)); // Adjusted position
                if (currentSkinIndex > 0)
                {
                    if (ImGui::ArrowButton("##left", ImGuiDir_Left))
                    {
                        currentSkinIndex--;
                    }
                    ImGui::SameLine();
                }

                ImGui::SetCursorPos(ImVec2(595, 625)); // Adjusted position for right arrow
                if (currentSkinIndex < skins.size() - 1)
                {
                    if (ImGui::ArrowButton("##right", ImGuiDir_Right))
                    {
                        currentSkinIndex++;
                    }
                }
            }
        }

        // Add Ally Tip button
        ImGui::SetCursorPos(ImVec2(650, 440));
        if (ImGui::Button("Ally Tips"))
        {
            showAllyTip = !showAllyTip;
            if (showAllyTip && allyTips.empty())
            {
                allyTips = dataManager.GetChampionAllyTips(championName);
                if (!allyTips.empty())
                {
                    RandomizeTips(allyTips, allyTipIndices);
                    currentAllyTipIndex = 0;
                }
            }
        }

        if (showAllyTip)
        {
            ImGui::SetCursorPos(ImVec2(650, 480));
            if (!allyTips.empty())
            {
                if (ImGui::Button("Next Ally Tip"))
                {
                    currentAllyTipIndex = (currentAllyTipIndex + 1) % allyTipIndices.size();
                }
            }

            ImGui::SetCursorPos(ImVec2(795, 440));
            ImGui::BeginChild("AllyTip", ImVec2(240, 90), true);
            ImGui::Indent(5.0f);
            if (!allyTips.empty())
            {
                size_t index = allyTipIndices[currentAllyTipIndex];
                ImGui::TextWrapped("%s", allyTips[index].c_str());
            }
            else
            {
                ImGui::TextWrapped("No ally tips available for this champion.");
            }
            ImGui::Unindent(5.0f);
            ImGui::EndChild();
        }

        // For Enemy Tips
        ImGui::SetCursorPos(ImVec2(650, 535));
        if (ImGui::Button("Enemy Tips"))
        {
            showEnemyTip = !showEnemyTip;
            if (showEnemyTip && enemyTips.empty())
            {
                enemyTips = dataManager.GetChampionEnemyTips(championName);
                if (!enemyTips.empty())
                {
                    RandomizeTips(enemyTips, enemyTipIndices);
                    currentEnemyTipIndex = 0;
                }
            }
        }

        if (showEnemyTip)
        {
            ImGui::SetCursorPos(ImVec2(650, 575));
            if (!enemyTips.empty())
            {
                if (ImGui::Button("Next Enemy Tip"))
                {
                    currentEnemyTipIndex = (currentEnemyTipIndex + 1) % enemyTipIndices.size();
                }
            }

            ImGui::SetCursorPos(ImVec2(795, 575));
            ImGui::BeginChild("EnemyTip", ImVec2(240, 80), true);
            ImGui::Indent(5.0f);
            if (!enemyTips.empty())
            {
                size_t index = enemyTipIndices[currentEnemyTipIndex];
                ImGui::TextWrapped("%s", enemyTips[index].c_str());
            }
            else
            {
                ImGui::TextWrapped("No enemy tips available for this champion.");
            }
            ImGui::Indent(5.0f);
            ImGui::EndChild();
        }
    }
}

void GUIManager::CleanupSkinTextures()
{
    for (auto &pair : skinTextures)
    {
        glDeleteTextures(1, &pair.second);
    }
    skinTextures.clear();
}

void GUIManager::LoadChampionSplash(const std::string &championName)
{
    CURL *curl = curl_easy_init();
    if (curl)
    {
        std::string url = dataManager.GetChampionImageUrl(championName);
        std::string imageData;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imageData);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK)
        {
            int width, height, channels;
            unsigned char *image = stbi_load_from_memory(
                reinterpret_cast<const unsigned char *>(imageData.data()),
                imageData.size(), &width, &height, &channels, 4);

            if (image)
            {
                if (isChampionSplashLoaded)
                {
                    glDeleteTextures(1, &championSplashTexture);
                }

                glGenTextures(1, &championSplashTexture);
                glBindTexture(GL_TEXTURE_2D, championSplashTexture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

                stbi_image_free(image);
                isChampionSplashLoaded = true;
            }
        }
    }
}

void GUIManager::LoadChampionIcon(const std::string &championName)
{
    // Similar to LoadChampionSplash, but for the icon
    CURL *curl = curl_easy_init();
    if (curl)
    {
        std::string url = dataManager.GetChampionIconUrl(championName); // You'll need to add this function to DataManager
        std::string imageData;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imageData);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK)
        {
            int width, height, channels;
            unsigned char *image = stbi_load_from_memory(
                reinterpret_cast<const unsigned char *>(imageData.data()),
                imageData.size(), &width, &height, &channels, 4);

            if (image)
            {
                if (isChampionIconLoaded)
                {
                    glDeleteTextures(1, &championIconTexture);
                }

                glGenTextures(1, &championIconTexture);
                glBindTexture(GL_TEXTURE_2D, championIconTexture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

                stbi_image_free(image);
                isChampionIconLoaded = true;
            }
        }
    }
}

size_t GUIManager::WriteCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    ((std::string *)userp)->append((char *)contents, size * nmemb);
    return size * nmemb;
}

/**
 * 윈도우 크기 조절 콜백 메서드.
 */
void GUIManager::WindowResizeCallback( GLFWwindow* window, int width, int height )
{
    glViewport(0, 0, width, height);
}

/**
 * 아이콘 이미지를 텍스처로 로드하는 메서드.
 */
bool GUIManager::LoadIconTexture( const char* filename )
{
    int width, height, channels;
    unsigned char* image = stbi_load( filename, &width, &height, &channels, 4 );
    if ( image == nullptr )
    {
        std::cerr << "Failed to load icon image: " << filename << std::endl;
        return false;
    }

    glGenTextures( 1, &iconTexture );
    glBindTexture( GL_TEXTURE_2D, iconTexture );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image );

    stbi_image_free( image );
    isIconLoaded = true;
    return true;
}

void GUIManager::LoadSkillIcons(const std::string &championId)
{
    auto spells = dataManager.GetChampionSpells(championId);
    auto passive = dataManager.GetChampionPassive(championId);

    // Load passive icon and description
    LoadSkillIcon(passive["image"]["full"], 0);
    skillDescriptions["Passive"] = passive["name"].get<std::string>() + ": " + passive["description"].get<std::string>();

    // Load skill icons and descriptions
    std::string skillNames[] = {"Q", "W", "E", "R"};
    for (int i = 0; i < spells.size() && i < 4; ++i)
    {
        LoadSkillIcon(spells[i]["image"]["full"], i + 1);
        skillDescriptions[championId + " " + skillNames[i]] =
            spells[i]["name"].get<std::string>() + ": " + spells[i]["description"].get<std::string>();
    }

    areSkillIconsLoaded = true;
}

void GUIManager::LoadSkillIcon(const std::string &iconFilename, int index)
{
    CURL *curl = curl_easy_init();
    if (curl)
    {
        std::string url = "http://ddragon.leagueoflegends.com/cdn/14.14.1/img/passive/" + iconFilename;
        if (index > 0)
        {
            url = "http://ddragon.leagueoflegends.com/cdn/14.14.1/img/spell/" + iconFilename;
        }
        std::string imageData;

        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imageData);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK)
        {
            int width, height, channels;
            unsigned char *image = stbi_load_from_memory(
                reinterpret_cast<const unsigned char *>(imageData.data()),
                imageData.size(), &width, &height, &channels, 4);

            if (image)
            {
                if (skillTextures[index] != 0)
                {
                    glDeleteTextures(1, &skillTextures[index]);
                }

                glGenTextures(1, &skillTextures[index]);
                glBindTexture(GL_TEXTURE_2D, skillTextures[index]);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);

                stbi_image_free(image);
            }
        }
    }
}

GLuint GUIManager::LoadSkinTexture(const std::string &url)
{
    CURL *curl = curl_easy_init();
    GLuint texture = 0;
    if (curl)
    {
        std::string imageData;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imageData);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK)
        {
            int width, height, channels;
            unsigned char *image = stbi_load_from_memory(
                reinterpret_cast<const unsigned char *>(imageData.data()),
                imageData.size(), &width, &height, &channels, 4);

            if (image)
            {
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
                stbi_image_free(image);
            }
            else
            {
                std::cerr << "Failed to load skin image: " << url << std::endl;
            }
        }
        else
        {
            std::cerr << "Failed to download skin image: " << url << std::endl;
        }
    }
    return texture;
}

void GUIManager::RandomizeTips(const std::vector<std::string> &tips, std::vector<size_t> &indices)
{
    std::lock_guard<std::mutex> lock(tipMutex);

    indices.resize(tips.size());
    std::iota(indices.begin(), indices.end(), 0);

    std::random_device rd;
    std::mt19937 g(rd());

    std::shuffle(indices.begin(), indices.end(), g);
}

void GUIManager::RandomizeChampion()
{
    if (isRandomizing.load())
        return; // Don't start a new randomization if one is in progress

    isRandomizing.store(true);
    hasRandomChampion.store(false);
    const auto &championNames = dataManager.GetChampionNames();

    randomizationThread = std::thread([this, championNames]()
                                      {
		std::vector<size_t> indices(championNames.size());
		std::iota(indices.begin(), indices.end(), 0);

		std::random_device rd;
		std::mt19937 g(rd());
		std::shuffle(indices.begin(), indices.end(), g);

		// Select a single random champion
		{
			std::lock_guard<std::mutex> lock(tipMutex);
			selectedChampionIndex = indices[0];
		}

		isRandomizing.store(false);
		hasRandomChampion.store(true); });

    randomizationThread.detach(); // Allow the thread to run independently
}

// Item window functions implementation -------------------------------------------------------------------------------------------------------------------------
void GUIManager::RenderItemsWindow()
{
    const auto &itemNames = dataManager.GetItemNames();
    if (itemNames.empty())
    {
        ImGui::Text("No items available.");
        return;
    }

    static char searchBuffer[256] = "";
    ImGui::SetNextItemWidth(300);
    ImGui::SetCursorPos(ImVec2(10, 5));

    // Set custom colors
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));

    // Item select combo box
    if (ImGui::BeginCombo("##ItemSelect", (comboSelectedIndex >= 0 && comboSelectedIndex < itemNames.size()) ? itemNames[comboSelectedIndex].c_str() : "Select Item"))
    {
        ImGui::PushItemWidth(-1);
        ImGui::SetCursorPos(ImVec2(10, 10));
        ImGui::InputText("##Search", searchBuffer, IM_ARRAYSIZE(searchBuffer));
        ImGui::PopItemWidth();
        ImGui::Separator();

        for (int i = 0; i < itemNames.size(); i++)
        {
            std::string lowerItemName = itemNames[i];
            std::transform(lowerItemName.begin(), lowerItemName.end(), lowerItemName.begin(), ::tolower);
            std::string search = searchBuffer;
            std::transform(search.begin(), search.end(), search.begin(), ::tolower);

            if (lowerItemName.find(search) != std::string::npos)
            {
                bool is_selected = (comboSelectedIndex == i);
                if (ImGui::Selectable(itemNames[i].c_str(), is_selected))
                {
                    if (comboSelectedIndex != i)
                    {
                        comboSelectedIndex = i;
                        std::string itemId = dataManager.GetItemId(itemNames[i]);
                        DisplayItem(itemId);
                        selectedItemIndex = 0;
                    }
                }
                if (is_selected)
                    ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    ImGui::PopStyleColor(11);

    // Navigation and tag buttons
    ImGui::NewLine();
    float windowWidth = ImGui::GetWindowWidth();
    float buttonHeight = 30.0f;
    float buttonWidth = 80.0f;

    const char *tags[] = {"FIGHTER", "MARKSMAN", "ASSASSIN", "MAGE", "TANK", "SUPPORT"};
    int numTags = IM_ARRAYSIZE(tags);

    // Back button
    ImGui::SetCursorPosX(25);
    if (ImGui::Button("Back", ImVec2(buttonWidth, buttonHeight)))
    {
        GoBack();
    }

    ImGui::SameLine();

    // Forward button (only if there's forward history)
    bool canGoForward = currentHistoryIndex < history.size() - 1;
    if (canGoForward)
    {
        if (ImGui::Button("Forward", ImVec2(buttonWidth, buttonHeight)))
        {
            GoForward();
        }
        ImGui::SameLine();
    }

    // Render tag buttons
    ImGui::SetCursorPosX(350);
    for (int i = 0; i < numTags; i++)
    {
        if (i > 0)
            ImGui::SameLine();
        bool isActiveTag = (currentTag == tags[i]);
        if (isActiveTag)
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8431f, 0.7255f, 0.4745f, 1.0f));
        if (ImGui::Button(tags[i], ImVec2(0, buttonHeight)))
        {
            DisplayItemsByTag(tags[i]);
        }
        if (isActiveTag)
            ImGui::PopStyleColor();
    }

    // Display items only if a tag is selected or items are being displayed
    if (!currentItems.empty() || !currentTag.empty())
    {
        ImGui::SetCursorPos(ImVec2(25, ImGui::GetCursorPosY() + 10));
        ImGui::BeginChild("ItemsList", ImVec2(ImGui::GetWindowWidth() - 50, 300), true);
        ImGui::Indent(2.5f);
        int itemsPerRow = 13;
        for (int i = 0; i < currentItems.size(); i++)
        {
            const auto &itemId = currentItems[i];
            std::string itemName = dataManager.GetSpecificItemName(itemId);
            std::string itemIconUrl = dataManager.GetItemImageUrl(itemId);
            GLuint itemTexture = LoadTextureFromURL(itemIconUrl);

            if (i % itemsPerRow != 0)
                ImGui::SameLine();
            // if (ImGui::ImageButton((void*)(intptr_t)itemTexture, ImVec2(64, 64))) {
            if (ImGui::ImageButton("", (ImTextureID)(intptr_t)itemTexture, ImVec2(64, 64)))
            {
                if (selectedItemIndex != i)
                {
                    UpdateItemState(itemId, "", false, i, true);
                }
            }
            if (ImGui::IsItemHovered())
            {
                ImGui::BeginTooltip();
                ImGui::Text("%s", itemName.c_str());
                ImGui::EndTooltip();
            }
        }
        ImGui::Unindent(2.5f);
        ImGui::EndChild();
    }
    else
    {
        // Display a message when no tag is selected and no items are displayed
        ImGui::SetCursorPos(ImVec2(25, ImGui::GetCursorPosY() + 10));
        ImGui::Text("Select a tag or item to view details.");
    }

    // Display item details only if an item is selected
    if (!currentItems.empty() && selectedItemIndex >= 0 && selectedItemIndex < currentItems.size())
    {
        std::string itemId = currentItems[selectedItemIndex];
        currentItemTags = dataManager.GetItemTags(itemId);

        float columnWidth = (ImGui::GetWindowWidth() - 50) / 2;
        ImVec2 statsWindowPos = ImGui::GetCursorPos();

        // Item stats column
        ImGui::SetCursorPos(ImVec2(25, ImGui::GetCursorPosY()));
        ImGui::BeginChild("ItemDetails", ImVec2(columnWidth - 10, 200), true);
        ImGui::Indent(5.0f);
        ImGui::Text("Name: %s", dataManager.GetSpecificItemName(itemId).c_str());
        ImGui::Text("Description: %s", dataManager.GetItemDescription(itemId).c_str());
        int cost = dataManager.GetItemCost(itemId);
        if (cost >= 0)
            ImGui::Text("Cost: %d", cost);

        auto stats = dataManager.GetItemStats(itemId);
        if (!stats.empty())
        {
            ImGui::Text("Stats:");
            for (auto &[statName, statValue] : stats.items())
            {
                if (statValue.contains("flat") && statValue["flat"].get<float>() != 0)
                {
                    ImGui::Text("  %s: %.2f", statName.c_str(), statValue["flat"].get<float>());
                }
                if (statValue.contains("percent") && statValue["percent"].get<float>() != 0)
                {
                    ImGui::Text("  %s: %.2f%%", statName.c_str(), statValue["percent"].get<float>());
                }
            }
        }

        auto itemData = dataManager.GetItemData(itemId);
        if (itemData.contains("active") && !itemData["active"].empty())
        {
            ImGui::Text("Active Ability:");
            for (const auto &active : itemData["active"])
            {
                ImGui::TextWrapped("  %s: %s", active["name"].get<std::string>().c_str(), active["effects"].get<std::string>().c_str());
                if (active.contains("cooldown") && !active["cooldown"].is_null())
                {
                    ImGui::Text("  Cooldown: %s", active["cooldown"].get<std::string>().c_str());
                }
            }
        }
        ImGui::Unindent(5.0f);
        ImGui::EndChild();

        // Builds Into column
        ImGui::SetCursorPos(ImVec2(25 + columnWidth, statsWindowPos.y));
        ImGui::BeginChild("BuildsInto", ImVec2(columnWidth - 10, 200), true);
        ImGui::Indent(5.0f);
        ImGui::Text("Builds Into:");
        std::vector<std::string> buildsInto = dataManager.GetItemBuildsInto(itemId);
        if (!buildsInto.empty())
        {
            for (const auto &buildItemId : buildsInto)
            {
                if (dataManager.ItemExists(buildItemId))
                {
                    std::string buildItemName = dataManager.GetSpecificItemName(buildItemId);
                    std::string buildItemIconUrl = dataManager.GetItemImageUrl(buildItemId);
                    GLuint buildItemTexture = LoadTextureFromURL(buildItemIconUrl);
                    // if (ImGui::ImageButton((void*)(intptr_t)buildItemTexture, ImVec2(32, 32))) {
                    if (ImGui::ImageButton("", (ImTextureID)(intptr_t)buildItemTexture, ImVec2(32, 32)))
                    {
                        DisplayItem(buildItemId);
                    }
                    ImGui::SameLine();
                    ImGui::Text("%s", buildItemName.c_str());
                    ImGui::Separator();
                }
            }
        }
        else
        {
            ImGui::Text("This item doesn't build into anything.");
        }
        ImGui::Unindent(5.0f);
        ImGui::EndChild();

        // Display item tags
        ImGui::Indent(25.0f);
        ImGui::Text("Tags:");
        for (const auto &tag : currentItemTags)
        {
            ImGui::SameLine();
            bool isHighlighted = (tag == currentTag) || (!currentTag.empty() && tag == currentTag);
            if (isHighlighted)
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8431f, 0.7255f, 0.4745f, 1.0f));
            if (ImGui::SmallButton(tag.c_str()))
            {
                DisplayItemsByTag(tag);
            }
            if (isHighlighted)
                ImGui::PopStyleColor();
        }
        ImGui::Unindent(25.0f);
    }
}

void GUIManager::RenderItemsDetail()
{
    static std::string selectedTag = "DefaultTag"; // Set a default tag
    static std::vector<std::string> tags = {"FIGHTER", "MARKSMAN", "ASSASSIN", "MAGE", "TANK", "SUPPORT"};

    // Combo box for selecting item categories/tags
    if (ImGui::BeginCombo("Select Tag", selectedTag.c_str()))
    {
        for (const auto &tag : tags)
        {
            bool isSelected = (selectedTag == tag);
            if (ImGui::Selectable(tag.c_str(), isSelected))
            {
                selectedTag = tag;
                DisplayItemsByTag(tag); // Use the updated function
            }
            if (isSelected)
            {
                ImGui::SetItemDefaultFocus();
            }
        }
        ImGui::EndCombo();
    }

    // Display item icons as buttons
    for (const auto &itemId : currentItems)
    {
        std::string itemIconUrl = dataManager.GetItemImageUrl(itemId);
        GLuint itemTexture = LoadTextureFromURL(itemIconUrl); // Load the texture
        // if (ImGui::ImageButton((void*)(intptr_t)itemTexture, ImVec2(64, 64))) {
        if (ImGui::ImageButton("", (ImTextureID)(intptr_t)itemTexture, ImVec2(64, 64)))
        {
            DisplayItem(itemId);
        }
    }
}

void GUIManager::DisplayItemsByTag(const std::string &tag)
{
    UpdateItemState("", tag, true, 0, true);
}

GLuint GUIManager::LoadTextureFromURL(const std::string &url)
{
    // Check if the texture is already loaded
    if (itemTextures.find(url) != itemTextures.end())
    {
        return itemTextures[url];
    }

    CURL *curl = curl_easy_init();
    GLuint texture = 0;
    if (curl)
    {
        std::string imageData;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &imageData);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK)
        {
            int width, height, channels;
            unsigned char *image = stbi_load_from_memory(
                reinterpret_cast<const unsigned char *>(imageData.data()),
                imageData.size(), &width, &height, &channels, 4);

            if (image)
            {
                glGenTextures(1, &texture);
                glBindTexture(GL_TEXTURE_2D, texture);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
                glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
                stbi_image_free(image);

                // Cache the loaded texture
                itemTextures[url] = texture;
            }
        }
    }
    return texture;
}

void GUIManager::DisplayItem(const std::string &itemId)
{
    UpdateItemState(itemId, "", false, 0, true);
}

void GUIManager::UpdateItemState(const std::string &itemId, const std::string &tag, bool isTagView, int selectedIndex, bool addToHistory = true)
{
    if (addToHistory)
    {
        // Remove any forward history
        if (currentHistoryIndex < history.size() - 1)
        {
            history.erase(history.begin() + currentHistoryIndex + 1, history.end());
        }

        // Add new entry
        history.push_back({itemId, tag, isTagView, selectedIndex});
        currentHistoryIndex = history.size() - 1;
    }

    if (isTagView)
    {
        currentItems = dataManager.GetItemsByTag(tag);
        currentTag = tag;
        selectedItemIndex = -1;
        comboSelectedIndex = -1;
        currentItemTags.clear(); // Clear item tags when viewing a tag
    }
    else
    {
        if (!dataManager.ItemExists(itemId))
        {
            std::cerr << "Invalid item ID: " << itemId << std::endl;
            return;
        }
        currentItems = {itemId};
        // Don't clear currentTag here
        selectedItemIndex = 0;
        comboSelectedIndex = -1;
        currentItemTags = dataManager.GetItemTags(itemId); // Store the item's tags

        // If currentTag is empty and the item has tags, set currentTag to the first tag
        if (currentTag.empty() && !currentItemTags.empty())
        {
            currentTag = currentItemTags[0];
        }

        const auto &allItems = dataManager.GetItemNames();
        auto it = std::find(allItems.begin(), allItems.end(), dataManager.GetSpecificItemName(itemId));
        if (it != allItems.end())
        {
            comboSelectedIndex = std::distance(allItems.begin(), it);
        }
    }
}

void GUIManager::InitializeHistory()
{
    history.clear();
    currentHistoryIndex = 0;
    history.push_back({"", "", true, -1}); // Represents the initial state
    currentHistoryIndex = 0;
}

void GUIManager::SaveHistory()
{
    std::ofstream file(historyFilePath, std::ios::trunc);
    for (const auto &entry : history)
    {
        file << entry.itemId << ',' << entry.tag << ',' << entry.isTagView << ',' << entry.selectedIndex << '\n';
    }
    file.close();
}

void GUIManager::GoBack()
{
    if (currentHistoryIndex > 0)
    {
        currentHistoryIndex--;
        const auto &entry = history[currentHistoryIndex];
        UpdateItemState(entry.itemId, entry.tag, entry.isTagView, entry.selectedIndex, false);
    }
}

void GUIManager::GoForward()
{
    if (currentHistoryIndex < history.size() - 1)
    {
        currentHistoryIndex++;
        const auto &entry = history[currentHistoryIndex];
        UpdateItemState(entry.itemId, entry.tag, entry.isTagView, entry.selectedIndex, false);
    }
}

// Summoner's Spells page ---------------------------------------------------------------------------------------------------------------------------------------
void GUIManager::RenderSummonerSpellsWindow()
{
    // Fetch game modes if not already done
    static bool gameModesLoaded = false;
    static bool fetchFailed = false;
    if (!gameModesLoaded && !fetchFailed)
    {
        ImGui::Text("Fetching game modes...");
        if (dataManager.FetchGameModes() && dataManager.FetchSummonerSpells())
        {
            gameModesLoaded = true;
        }
        else
        {
            fetchFailed = true;
            ImGui::Text("Failed to load data. Please try again later.");
            if (ImGui::Button("Retry"))
            {
                fetchFailed = false;
            }
            return;
        }
    }

    if (fetchFailed)
    {
        ImGui::Text("Failed to load data. Please try again later.");
        if (ImGui::Button("Retry"))
        {
            fetchFailed = false;
        }
        return;
    }

    const auto &gameModes = dataManager.GetGameModes();

    if (gameModes.empty())
    {
        ImGui::Text("No game modes available.");
        return;
    }

    // Game mode selection combo box
    ImGui::SetNextItemWidth(300);
    ImGui::SetCursorPos(ImVec2(10, 5));

    // Set custom colors
    ImGui::PushStyleColor(ImGuiCol_FrameBg, ImVec4(0.1f, 0.1f, 0.1f, 1.0f));        // Dark background for input fields
    ImGui::PushStyleColor(ImGuiCol_FrameBgHovered, ImVec4(0.2f, 0.2f, 0.2f, 1.0f)); // Slightly lighter when hovered
    ImGui::PushStyleColor(ImGuiCol_FrameBgActive, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));  // Even lighter when active
    ImGui::PushStyleColor(ImGuiCol_PopupBg, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));        // Dark background for combo popup
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 1.0f, 1.0f));           // White text
    ImGui::PushStyleColor(ImGuiCol_Header, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));         // Slightly lighter background for selected item
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));  // Even lighter for hovered item
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, ImVec4(0.5f, 0.5f, 0.5f, 1.0f));   // Lightest for active/clicked item
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.2f, 0.2f, 1.0f));         // Dark button color
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.3f, 0.3f, 0.3f, 1.0f));  // Lighter button color when hovered
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.4f, 0.4f, 1.0f));   // Even lighter when clicked

    if (ImGui::BeginCombo("##GameModeSelect",
                          selectedGameModeIndex >= 0 && selectedGameModeIndex < gameModes.size()
                              ? gameModes[selectedGameModeIndex].mode.c_str()
                              : "Select Game Mode"))
    { // Search input field
        ImGui::PushItemWidth(-1);
        ImGui::SetCursorPos(ImVec2(10, 10));
        ImGui::InputText("##GameModeSearch", gameModeSearchBuffer, IM_ARRAYSIZE(gameModeSearchBuffer));
        ImGui::PopItemWidth();
        ImGui::Separator();

        for (int i = 0; i < gameModes.size(); i++)
        {
            const auto &gameMode = gameModes[i];

            // Convert search and game mode to lowercase for case-insensitive comparison
            std::string search = gameModeSearchBuffer;
            std::string mode = gameMode.mode;
            std::transform(search.begin(), search.end(), search.begin(), ::tolower);
            std::transform(mode.begin(), mode.end(), mode.begin(), ::tolower);

            if (mode.find(search) != std::string::npos)
            {
                bool is_selected = (selectedGameModeIndex == i);
                if (ImGui::Selectable(gameMode.mode.c_str(), is_selected))
                {
                    selectedGameModeIndex = i;
                }

                if (is_selected)
                {
                    ImGui::SetItemDefaultFocus();
                }
            }
        }
        ImGui::EndCombo();
    }

    // Pop custom colors
    ImGui::PopStyleColor(11);

    // Display selected game mode description on the same line
    if (selectedGameModeIndex >= 0 && selectedGameModeIndex < gameModes.size())
    {
        ImGui::SameLine();
        ImGui::Text("MODE: %s", gameModes[selectedGameModeIndex].description.c_str());

        const std::string &selectedMode = gameModes[selectedGameModeIndex].mode;
        auto spells = dataManager.GetSummonerSpellsForMode(selectedMode);

        ImGui::NewLine();

        if (spells.empty())
        {
            ImGui::SetCursorPosX(25);
            ImGui::TextColored(ImVec4(1.0f, 0.0f, 0.0f, 1.0f), "This game mode does not allow the use of Summoner Spells.");
        }
        else
        {
            ImGui::SetCursorPosX(25);
            ImGui::Text("Summoner Spells Available in %s mode:", selectedMode.c_str());
            ImGui::NewLine();

            float windowWidth = ImGui::GetWindowWidth();
            float iconSize = 64.0f;
            float padding = 10.0f;
            int iconsPerRow = 13;     // Limit to 13 icons per row
            float leftOffset = 25.0f; // 25 pixels offset from the left

            static int selectedSpellIndex = -1;

            for (size_t i = 0; i < spells.size(); ++i)
            {
                if (i % iconsPerRow == 0)
                {
                    ImGui::NewLine();
                    ImGui::SetCursorPosX(leftOffset); // Set left offset
                }
                else if (i > 0)
                {
                    ImGui::SameLine(0, padding);
                }

                const auto &spell = spells[i];
                GLuint texture = LoadSummonerSpellTexture(spell.id);

                ImGui::PushID(static_cast<int>(i));
                // if (ImGui::ImageButton((void*)(intptr_t)texture, ImVec2(iconSize, iconSize))) {
                if (ImGui::ImageButton("", (ImTextureID)(intptr_t)texture, ImVec2(iconSize, iconSize)))
                {
                    selectedSpellIndex = static_cast<int>(i);
                }
                ImGui::PopID();

                if (ImGui::IsItemHovered())
                {
                    ImGui::BeginTooltip();
                    ImGui::Text("%s", spell.name.c_str());
                    ImGui::EndTooltip();
                }
            }

            // Display chat box for selected spell
            if (selectedSpellIndex >= 0 && selectedSpellIndex < spells.size())
            {
                const auto &selectedSpell = spells[selectedSpellIndex];
                ImGui::NewLine();
                ImGui::SetCursorPosX(25);
                ImGui::BeginChild("SpellDetails", ImVec2(windowWidth - 50, 100), true);
                ImGui::Indent(5.0f);
                ImGui::Text("Name: %s", selectedSpell.name.c_str());
                ImGui::Text("Summoner Level required to unlock: %d", selectedSpell.summonerLevel);
                ImGui::TextWrapped("Description: %s", selectedSpell.description.c_str());
                ImGui::Text("Cooldown: %s", selectedSpell.cooldownBurn.c_str());
                ImGui::Unindent(5.0f);
                ImGui::EndChild();

                // Display game mode tags as small buttons
                ImGui::NewLine();
                ImGui::SetCursorPosX(25);
                ImGui::Text("Available in:");
                ImGui::SameLine();

                float startX = ImGui::GetCursorPosX();
                float currentLineWidth = startX;
                float maxLineWidth = windowWidth - 50;
                float newLineIndent = 0.0f;

                ImGui::PushFont(smallFont);

                for (const auto &mode : selectedSpell.modes)
                {
                    ImVec2 textSize = ImGui::CalcTextSize(mode.c_str());
                    float buttonWidth = textSize.x + 20.0f;
                    float buttonHeight = textSize.y + 16.0f;

                    float buttonAndSpacingWidth = buttonWidth + ImGui::GetStyle().ItemSpacing.x;
                    if (currentLineWidth + buttonAndSpacingWidth > maxLineWidth)
                    {
                        ImGui::NewLine();
                        ImGui::SetCursorPosX(startX + newLineIndent);
                        currentLineWidth = startX + newLineIndent;
                    }

                    ImGui::AlignTextToFramePadding();

                    // Check if this mode is the currently selected mode
                    bool isActiveMode = (mode == selectedMode);
                    if (isActiveMode)
                    {
                        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.8431f, 0.7255f, 0.4745f, 1.0f));
                    }

                    if (ImGui::Button(mode.c_str(), ImVec2(buttonWidth, buttonHeight)))
                    {
                        auto it = std::find_if(gameModes.begin(), gameModes.end(),
                                               [&mode](const DataManager::GameMode &gm)
                                               { return gm.mode == mode; });
                        if (it != gameModes.end())
                        {
                            selectedGameModeIndex = std::distance(gameModes.begin(), it);
                        }
                    }

                    if (isActiveMode)
                    {
                        ImGui::PopStyleColor();
                    }

                    currentLineWidth += buttonAndSpacingWidth;
                    ImGui::SameLine();
                }

                ImGui::NewLine();
                ImGui::PopFont();
            }
        }
    }
}

GLuint GUIManager::LoadSummonerSpellTexture(const std::string &spellId)
{
    if (summonerSpellTextures.find(spellId) != summonerSpellTextures.end())
    {
        return summonerSpellTextures[spellId];
    }

    std::string url = "https://ddragon.leagueoflegends.com/cdn/14.14.1/img/spell/" + spellId + ".png";
    GLuint texture = LoadTextureFromURL(url);
    summonerSpellTextures[spellId] = texture;
    return texture;
}