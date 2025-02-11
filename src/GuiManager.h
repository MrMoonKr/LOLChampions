#pragma once

//#include "imgui.h"
//#include "imgui_impl_glfw.h"
//#include "imgui_impl_opengl3.h"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <GLFW/glfw3.h>
#include <iostream>
#include <vector>
#include <string>
#include "DataManager.h"
#include <atomic>
#include <thread>
#include <mutex>
#include <random>
#include <algorithm>
#include <future>
#include <thread>
#include <numeric>
//#include "resource1.h"

class GUIManager {
public:
    GUIManager();
    ~GUIManager();
    GLFWimage images[1];
    bool Initialize(int width, int height, const char* title);
    void HandleDragging();
    bool ShouldClose();
    void Render();
    void Cleanup();
    void SetWindowOffset(float offset) { windowOffset = offset; }
    void RandomizeChampion();

    void RenderItemsDetail();
    void DisplayItemsByTag(const std::string& tag);

private:
    GLFWwindow* window;
    GLuint backgroundTexture;
    GLuint LoadTexture(const char* filename);
    GLuint championSplashTexture;
    GLuint championIconTexture;
    ImFont* smallFont;
    ImFont* defaultFont;

    float buttonHeight;
    float windowOffset;

    enum class WindowState {
        Default,
        Champions,
        Items,
        SummonerSpells
    };
    WindowState currentState;

    DataManager dataManager;  
    int selectedChampionIndex;

    std::future<void> championDataFuture;
    std::future<GLuint> championSplashFuture;
    std::future<GLuint> championIconFuture;
    std::vector<std::future<GLuint>> skillIconFutures;

    void ApplyCustomStyles();
    
    void RenderBackground();
    void RenderGUI();
    void RenderDefaultWindow();
    void RenderChampionsWindow();   
    void RenderItemsWindow();

    void CleanupSkinTextures();

    bool isChampionSplashLoaded;
    bool LoadIconTexture(const char* filename);
    void LoadSkillIcons(const std::string& championId);
    void LoadSkillIcon(const std::string& iconFilename, int index);
    GLuint LoadSkinTexture(const std::string& url);
    void RandomizeTips(const std::vector<std::string>& tips, std::vector<size_t>& indices);
    bool isChampionIconLoaded;
    void LoadChampionSplash(const std::string& championName);
    void LoadChampionIcon(const std::string& championName);
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp);
    bool isDragging = false;
    ImVec2 dragStartPos;
    bool isResizing = false;
    ImVec2 resizeStartPos;
    ImVec2 windowSize;
    static void WindowResizeCallback(GLFWwindow* window, int width, int height);
    GLuint iconTexture;
    bool isIconLoaded;
    std::vector<GLuint> skillTextures;
    bool areSkillIconsLoaded;
    std::string selectedSkill;
    std::string skillDescription;
    std::map<std::string, std::string> skillDescriptions;
    bool showSkins = false;
    size_t currentSkinIndex = 0;
    std::map<std::string, GLuint> skinTextures;
    std::vector<std::string> allyTips;
    std::vector<std::string> enemyTips;
    std::vector<size_t> tipIndices;
    size_t currentTipIndex;
    std::mutex tipMutex;
    bool showAllyTip = false;
    std::mt19937 rng;  // Random number generator
    bool showEnemyTip = false;
    std::vector<size_t> allyTipIndices;
    std::vector<size_t> enemyTipIndices;
    size_t currentAllyTipIndex = 0;
    size_t currentEnemyTipIndex = 0;
    std::vector<size_t> championIndices;
    std::thread randomizationThread;
    std::atomic<bool> isRandomizing;
    std::atomic<bool> hasRandomChampion;

    // Item window related
    struct ItemHistoryEntry {
        std::string itemId;
        std::string tag;
        bool isTagView;
        int selectedIndex;  
    };
    struct HistoryEntry {
        std::string itemId;
        std::string tag;
        bool isTagView;
        int selectedIndex;
    };

    std::vector<std::string> currentItemTags;
    std::vector<std::string> currentItems;
    int comboSelectedIndex = -1;
    bool showFighterItems = false;
    bool showMarksmanItems = false;
    bool showAssassinItems = false;
    bool showMageItems = false;
    bool showTankItems = false;
    bool showSupportItems = false;
    int selectedItemIndex = -1;
    std::map<std::string, GLuint> itemTextures;
    std::vector<ItemHistoryEntry> backwardHistory;
    std::filesystem::path historyFilePath;
    std::vector<std::string> forwardHistory;
    std::vector<HistoryEntry> history;
    size_t currentHistoryIndex;
    std::string currentTag;

    GLuint LoadTextureFromURL(const std::string& url);
    void DisplayItem(const std::string& itemId);
    void UpdateItemState(const std::string& itemId, const std::string& tag, bool isTagView, int selectedIndex, bool addToHistory);
    void InitializeHistory();
    void SaveHistory();
    void GoBack();
    void GoForward();




    // Summoner Spells window related
    void RenderSummonerSpellsWindow();
    int selectedGameModeIndex = -1;
    char gameModeSearchBuffer[256] = "";
    std::map<std::string, GLuint> summonerSpellTextures;
    GLuint LoadSummonerSpellTexture(const std::string& spellId);


};