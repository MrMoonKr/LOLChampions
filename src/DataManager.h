#pragma once

#include <string>
#include <vector>
#include <map>
#include <nlohmann/json.hpp>
#include <httplib.h>
#include <curl/curl.h>

class DataManager {
public:
    DataManager();
    // Champion window related functions
    bool FetchChampionData();
    bool FetchSpecificChampionData(const std::string& championId) const;  // Now const
    const std::vector<std::string>& GetChampionNames() const;
    std::string GetChampionId(const std::string& championName) const;
    std::string GetChampionImageUrl(const std::string& championId) const;
    std::string GetChampionIconUrl(const std::string& championId) const;
    nlohmann::json GetChampionStats(const std::string& championName) const;
    std::string GetChampionTitle(const std::string& championName) const;
    std::string GetChampionLore(const std::string& championName) const;
    std::vector<std::string> GetChampionTags(const std::string& championName) const;
    nlohmann::json GetChampionSpells(const std::string& championName) const;
    nlohmann::json GetChampionPassive(const std::string& championName) const;
    nlohmann::json GetChampionData(const std::string& championId) const;
    nlohmann::json GetChampionSkins(const std::string& championName) const;
    std::string GetChampionSkinImageUrl(const std::string& championId, const std::string& skinNum) const;
    std::vector<std::string> GetChampionAllyTips(const std::string& championName) const;
    std::vector<std::string> GetChampionEnemyTips(const std::string& championName) const;

    // Item window related functions
    bool FetchItemData();
    bool FetchSpecificItemData(const std::string& itemId) const;
    const std::vector<std::string>& GetItemNames() const;
    std::vector<std::string> GetItemsByTag(const std::string& tag) const;
    std::string GetItemId(const std::string& itemName) const;
    std::string GetSpecificItemName(const std::string& itemId) const;
    std::string GetItemDescription(const std::string& itemId) const;
    std::vector<std::string> GetItemBuildsFrom(const std::string& itemId) const;
    std::vector<std::string> GetItemBuildsInto(const std::string& itemId) const;
    std::string GetItemImageUrl(const std::string& itemId) const;
    int GetItemCost(const std::string& itemId) const;
    int GetItemSellPrice(const std::string& itemId) const;
    bool IsItemPurchasable(const std::string& itemId) const;
    std::vector<std::string> GetItemTags(const std::string& itemId) const;
    nlohmann::json GetItemStats(const std::string& itemId) const;
    nlohmann::json GetItemData(const std::string& itemId) const;
    nlohmann::json GetItemShopInfo(const std::string& itemId) const;
    std::vector<std::string> GetAllItemIds() const;
    std::string GetItemIdFromIconUrl(const std::string& url) const;
    bool ItemExists(const std::string& itemId) const;

    // Summoner spell window related functions
    struct GameMode {
        std::string mode;
        std::string description;
    };
    bool FetchGameModes();
    const std::vector<GameMode>& GetGameModes() const;
    struct SummonerSpell {
        std::string id;
        std::string name;
        std::string description;
        std::vector<std::string> modes;
        std::string cooldownBurn;
        int summonerLevel;
    };
    bool FetchSummonerSpells();
    const std::vector<SummonerSpell>& GetSummonerSpells() const;
    std::vector<SummonerSpell> GetSummonerSpellsForMode(const std::string& mode) const;



private:
    /**
     * @brief HTTP client for fetching champion data
     */
    mutable httplib::Client client;
    /**
     * champion.json
     */
    nlohmann::json championData;
    /**
     * Specific champion data for each champion
     */
    mutable std::map<std::string, nlohmann::json> specificChampionData;  // Now mutable
    std::vector<std::string> championNames;
    std::map<std::string, std::string> championNameToIdMap;

    void ProcessChampionData();

    /**
     * @brief HTTP client for fetching item data
     */
    mutable httplib::Client itemClient;

    mutable nlohmann::json itemData;
    mutable std::map<std::string, nlohmann::json> specificItemData;
    mutable std::vector<std::string> itemNames;
    mutable std::map<std::string, std::string> itemNameToIdMap;
    const std::set<std::string> validTags = { "FIGHTER", "ASSASSIN", "MARKSMAN", "MAGE", "TANK", "SUPPORT" };

    void ProcessItemData();

    // Summoner spell window related
    std::vector<GameMode> gameModes;
    std::vector<SummonerSpell> summonerSpells;
};