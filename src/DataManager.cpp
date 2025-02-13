#include "DataManager.h"
#include <iostream>

DataManager::DataManager() 
    : client("ddragon.leagueoflegends.com"), itemClient("cdn.merakianalytics.com"),
    defaultLanguage("ko_KR")  // Set default language to Korean
{
    // nothing
}

bool DataManager::FetchLanguageData()
{
    auto resLanguages = client.Get("/cdn/languages.json");
    if (resLanguages && resLanguages->status == 200)
    {
        auto languages = nlohmann::json::parse(resLanguages->body);
        //auto languageCount = languages.size();
        //auto firstLanguage = languages[0].get<std::string>();
        auto languageIds = languages.get<std::vector<std::string>>();
        // for (auto language : languages)
        // {
        //     std::cout << language << std::endl;
        // }

        return true;
    }

    return false;
}

bool DataManager::FetchChampionData() 
{
    auto res = client.Get( "/cdn/14.14.1/data/en_US/champion.json" );
    if ( res && res->status == 200 ) 
    {
        championData = nlohmann::json::parse( res->body );
        ProcessChampionData();
        return true;
    }

    std::cerr << "Failed to fetch champion data" << std::endl;
    return false;
}


/**
 * ~/cdn/14.14.1/data/{regionId}/champion/{championId}.json 에서 특정 챔피언의 데이터를 가져오는 메서드.
 */
bool DataManager::FetchSpecificChampionData( const std::string& championId ) const 
{
    if ( specificChampionData.find( championId ) != specificChampionData.end() )
    {
        return true;  // Data already fetched
    }

    auto res = client.Get( ("/cdn/14.14.1/data/en_US/champion/" + championId + ".json").c_str() );
    if ( res && res->status == 200 )
    {
        specificChampionData[championId] = nlohmann::json::parse( res->body );
        return true;
    }

    std::cerr << "Failed to fetch specific champion data for " << championId << std::endl;
    return false;
}


void DataManager::ProcessChampionData() 
{
    championNames.clear();
    championNameToIdMap.clear();
    for ( auto& [key, value] : championData["data"].items() )
    {
        std::string name = value["name"];
        championNames.push_back( name );
        championNameToIdMap[name] = key;
    }
}

const std::vector<std::string>& DataManager::GetChampionNames() const 
{
    return championNames;
}

std::string DataManager::GetChampionId( const std::string& championName ) const
{
    auto it = championNameToIdMap.find( championName );
    if ( it != championNameToIdMap.end() ) 
    {
        return it->second;
    }

    return championName; // Fallback to the name if ID is not found
}

std::string DataManager::GetChampionImageUrl( const std::string& championId ) const 
{
    return "https://ddragon.leagueoflegends.com/cdn/img/champion/splash/" + championId + "_0.jpg";
}

std::string DataManager::GetChampionIconUrl( const std::string& championId ) const 
{
    return "http://ddragon.leagueoflegends.com/cdn/14.14.1/img/champion/" + championId + ".png";
}

nlohmann::json DataManager::GetChampionStats( const std::string& championName ) const 
{
    std::string championId = GetChampionId( championName );
    FetchSpecificChampionData( championId );
    return specificChampionData.at( championId )["data"][championId]["stats"];
}

std::string DataManager::GetChampionTitle( const std::string& championName ) const 
{
    std::string championId = GetChampionId( championName );
    return championData["data"][championId]["title"];
}

std::string DataManager::GetChampionLore( const std::string& championName ) const {
    std::string championId = GetChampionId( championName);
    FetchSpecificChampionData( championId );
    return specificChampionData.at(championId)["data"][championId]["lore"];
}

std::vector<std::string> DataManager::GetChampionTags( const std::string& championName ) const {
    std::string championId = GetChampionId( championName );
    return championData["data"][championId]["tags"].get<std::vector<std::string>>();
}

nlohmann::json DataManager::GetChampionSpells(const std::string &championName) const
{
    std::string championId = GetChampionId(championName);
    FetchSpecificChampionData(championId);
    return specificChampionData.at(championId)["data"][championId]["spells"];
}

nlohmann::json DataManager::GetChampionPassive(const std::string &championName) const
{
    std::string championId = GetChampionId(championName);
    FetchSpecificChampionData(championId);
    return specificChampionData.at(championId)["data"][championId]["passive"];
}

nlohmann::json DataManager::GetChampionData(const std::string &championId) const
{
    return championData["data"][championId];
}

nlohmann::json DataManager::GetChampionSkins(const std::string &championName) const
{
    std::string championId = GetChampionId(championName);
    FetchSpecificChampionData(championId);
    return specificChampionData.at(championId)["data"][championId]["skins"];
}

std::string DataManager::GetChampionSkinImageUrl(const std::string &championId, const std::string &skinNum) const
{
    return "https://ddragon.leagueoflegends.com/cdn/img/champion/splash/" + championId + "_" + skinNum + ".jpg";
}

std::vector<std::string> DataManager::GetChampionAllyTips(const std::string &championName) const
{
    std::string championId = GetChampionId(championName);
    FetchSpecificChampionData(championId);
    return specificChampionData.at(championId)["data"][championId]["allytips"].get<std::vector<std::string>>();
}

std::vector<std::string> DataManager::GetChampionEnemyTips(const std::string &championName) const
{
    std::string championId = GetChampionId(championName);
    FetchSpecificChampionData(championId);
    return specificChampionData.at(championId)["data"][championId]["enemytips"].get<std::vector<std::string>>();
}

// item window functions
bool DataManager::FetchItemData()
{
    auto res = itemClient.Get("/riot/lol/resources/latest/en-US/items.json");
    if (res && res->status == 200)
    {
        try
        {
            itemData = nlohmann::json::parse(res->body);
            ProcessItemData();
            // std::cout << "Loaded " << itemData.size() << " items" << std::endl;
            return true;
        }
        catch (const std::exception &e)
        {
            std::cerr << "Exception parsing item data: " << e.what() << std::endl;
        }
    }
    std::cerr << "Failed to fetch item data" << std::endl;
    return false;
}

bool DataManager::FetchSpecificItemData(const std::string &itemId) const
{
    // Check if we already have this item's data
    if (specificItemData.find(itemId) != specificItemData.end())
    {
        return true; // Data already fetched
    }
    // Check if we have already loaded the entire item data
    if (itemData.empty())
    {
        // If not, fetch the entire item data
        auto res = itemClient.Get("/riot/lol/resources/latest/en-US/items.json");
        if (!res || res->status != 200)
        {
            std::cerr << "Failed to fetch item data" << std::endl;
            return false;
        }
        itemData = nlohmann::json::parse(res->body);
    }
    // Search for the specific item in the itemData
    auto it = itemData.find(itemId);
    if (it != itemData.end())
    {
        // Store the specific item data
        specificItemData[itemId] = *it;
        // Update itemNames and itemNameToIdMap if not already present
        std::string itemName = (*it)["name"];
        if (std::find(itemNames.begin(), itemNames.end(), itemName) == itemNames.end())
        {
            itemNames.push_back(itemName);
            itemNameToIdMap[itemName] = itemId;
        }
        return true;
    }
    else
    {
        std::cerr << "Item ID " << itemId << " not found in item data" << std::endl;
        return false;
    }
}

void DataManager::ProcessItemData()
{
    itemNames.clear();
    itemNameToIdMap.clear();
    for (auto &[key, value] : itemData.items())
    {
        std::string name = value["name"];
        itemNames.push_back(name);
        itemNameToIdMap[name] = key;
    }
}

const std::vector<std::string> &DataManager::GetItemNames() const
{
    return itemNames;
}

std::vector<std::string> DataManager::GetItemsByTag(const std::string &tag) const
{
    std::vector<std::string> itemsWithTag;
    try
    {
        for (const auto &[itemId, itemData] : itemData.items())
        {
            if (itemData.contains("shop") && itemData["shop"].contains("tags"))
            {
                const auto &tags = itemData["shop"]["tags"];
                if (std::find(tags.begin(), tags.end(), tag) != tags.end())
                {
                    itemsWithTag.push_back(itemId);
                }
            }
        }
        std::cout << "Found " << itemsWithTag.size() << " items with tag: " << tag << std::endl;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in GetItemsByTag: " << e.what() << std::endl;
    }
    return itemsWithTag;
}

std::string DataManager::GetItemImageUrl(const std::string &itemId) const
{
    return itemData[itemId]["icon"];
}

std::string DataManager::GetItemId(const std::string &itemName) const
{
    auto it = itemNameToIdMap.find(itemName);
    if (it != itemNameToIdMap.end())
    {
        return it->second;
    }
    return itemName; // Fallback to the name if ID is not found
}

std::string DataManager::GetSpecificItemName(const std::string &itemId) const
{
    if (itemData.contains(itemId))
    {
        return itemData[itemId]["name"];
    }
    return "Unknown Item";
}

std::string DataManager::GetItemDescription(const std::string &itemId) const
{
    if (itemData.contains(itemId) && itemData[itemId].contains("simpleDescription"))
    {
        if (itemData[itemId].at("simpleDescription") != nullptr)
            return itemData[itemId]["simpleDescription"];
    }
    return "No description available";
}

std::vector<std::string> DataManager::GetItemBuildsFrom(const std::string &itemId) const
{
    FetchSpecificItemData(itemId);
    std::vector<std::string> buildsFrom;
    if (specificItemData.at(itemId).contains("buildsFrom"))
    {
        for (const auto &item : specificItemData.at(itemId)["buildsFrom"])
        {
            buildsFrom.push_back(item.get<std::string>());
        }
    }
    return buildsFrom;
}

std::vector<std::string> DataManager::GetItemBuildsInto(const std::string &itemId) const
{
    std::vector<std::string> buildsInto;
    try
    {
        FetchSpecificItemData(itemId);
        if (specificItemData.at(itemId).contains("buildsInto") &&
            specificItemData.at(itemId)["buildsInto"].is_array())
        {
            for (const auto &buildItem : specificItemData.at(itemId)["buildsInto"])
            {
                if (buildItem.is_string())
                {
                    buildsInto.push_back(buildItem.get<std::string>());
                }
                else if (buildItem.is_number())
                {
                    // Convert number to string
                    buildsInto.push_back(std::to_string(buildItem.get<int>()));
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error in GetItemBuildsInto for item " << itemId << ": " << e.what() << std::endl;
    }
    return buildsInto;
}

int DataManager::GetItemCost(const std::string &itemId) const
{
    if (itemData.contains(itemId) && itemData[itemId].contains("shop") &&
        itemData[itemId]["shop"].contains("prices") &&
        itemData[itemId]["shop"]["prices"].contains("total"))
    {
        return itemData[itemId]["shop"]["prices"]["total"];
    }
    return -1;
}

int DataManager::GetItemSellPrice(const std::string &itemId) const
{
    FetchSpecificItemData(itemId);
    if (specificItemData.at(itemId).contains("shop") && specificItemData.at(itemId)["shop"].contains("prices"))
    {
        return specificItemData.at(itemId)["shop"]["prices"]["sell"].get<int>();
    }
    return -1; // Return a default value or handle the case where the sell price is not available
}

bool DataManager::IsItemPurchasable(const std::string &itemId) const
{
    FetchSpecificItemData(itemId);
    if (specificItemData.at(itemId).contains("shop"))
    {
        return specificItemData.at(itemId)["shop"]["purchasable"].get<bool>();
    }
    return false; // Default value if the key is not found
}

std::vector<std::string> DataManager::GetItemTags(const std::string &itemId) const
{
    std::vector<std::string> tags;
    if (itemData.contains(itemId) && itemData[itemId].contains("shop") &&
        itemData[itemId]["shop"].contains("tags"))
    {
        for (const auto &tag : itemData[itemId]["shop"]["tags"])
        {
            std::string tagStr = tag.get<std::string>();
            if (validTags.find(tagStr) != validTags.end())
            {
                tags.push_back(tagStr);
            }
        }
    }
    return tags;
}

nlohmann::json DataManager::GetItemStats(const std::string &itemId) const
{
    if (itemData.contains(itemId) && itemData[itemId].contains("stats"))
    {
        return itemData[itemId]["stats"];
    }
    return nlohmann::json::object();
}

nlohmann::json DataManager::GetItemData(const std::string &itemId) const
{
    if (itemData.contains(itemId))
    {
        return itemData[itemId];
    }
    return nlohmann::json::object();
}

nlohmann::json DataManager::GetItemShopInfo(const std::string &itemId) const
{
    FetchSpecificItemData(itemId);
    if (specificItemData.at(itemId).contains("shop"))
    {
        return specificItemData.at(itemId)["shop"];
    }
    return {}; // Return an empty JSON object if the shop info is not found
}

std::vector<std::string> DataManager::GetAllItemIds() const
{
    std::vector<std::string> ids;
    for (const auto &[id, item] : itemData.items())
    {
        ids.push_back(id);
    }
    return ids;
}

std::string DataManager::GetItemIdFromIconUrl(const std::string &url) const
{
    for (const auto &[id, item] : itemData.items())
    {
        if (item["icon"] == url)
        {
            return id;
        }
    }
    return "";
}

bool DataManager::ItemExists(const std::string &itemId) const
{
    return itemData.contains(itemId);
}

// Summoner spell window related functions
static size_t WriteCallback(void *contents, size_t size, size_t nmemb, std::string *output)
{
    size_t totalSize = size * nmemb;
    output->append((char *)contents, totalSize);
    return totalSize;
}

bool DataManager::FetchGameModes()
{
    CURL *curl = curl_easy_init();
    if (!curl)
    {
        std::cerr << "Failed to initialize curl" << std::endl;
        return false;
    }

    std::string response;
    curl_easy_setopt(curl, CURLOPT_URL, "https://static.developer.riotgames.com/docs/lol/gameModes.json");
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK)
    {
        std::cerr << "curl_easy_perform() failed: " << curl_easy_strerror(res) << std::endl;
        return false;
    }

    try
    {
        auto json = nlohmann::json::parse(response);
        gameModes.clear();
        gameModes.push_back({"All Game Modes", "Showing all Summoner's Spells from all game modes."});
        for (const auto &mode : json)
        {
            gameModes.push_back({mode["gameMode"].get<std::string>(),
                                 mode["description"].get<std::string>()});
        }
        std::cout << "Successfully fetched " << gameModes.size() << " game modes" << std::endl;
        return true;
    }
    catch (const std::exception &e)
    {
        std::cerr << "Exception in parsing JSON: " << e.what() << std::endl;
        return false;
    }
}

const std::vector<DataManager::GameMode> &DataManager::GetGameModes() const
{
    return gameModes;
}

bool DataManager::FetchSummonerSpells()
{
    CURL *curl = curl_easy_init();
    if (curl)
    {
        std::string url = "https://ddragon.leagueoflegends.com/cdn/14.14.1/data/en_US/summoner.json";
        std::string response;
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);

        CURLcode res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);

        if (res == CURLE_OK)
        {
            auto json = nlohmann::json::parse(response);
            summonerSpells.clear();
            for (const auto &[key, value] : json["data"].items())
            {
                SummonerSpell spell;
                spell.id = value["id"];
                spell.name = value["name"];
                spell.description = value["description"];
                spell.modes = value["modes"].get<std::vector<std::string>>();
                spell.cooldownBurn = value["cooldownBurn"];
                spell.summonerLevel = value["summonerLevel"];
                summonerSpells.push_back(spell);
            }
            return true;
        }
    }
    std::cerr << "Failed to fetch summoner spells" << std::endl;
    return false;
}

const std::vector<DataManager::SummonerSpell> &DataManager::GetSummonerSpells() const
{
    return summonerSpells;
}

std::vector<DataManager::SummonerSpell> DataManager::GetSummonerSpellsForMode(const std::string &mode) const
{
    if (mode == "All Game Modes")
    {
        return summonerSpells;
    }
    std::vector<SummonerSpell> filteredSpells;
    for (const auto &spell : summonerSpells)
    {
        if (std::find(spell.modes.begin(), spell.modes.end(), mode) != spell.modes.end())
        {
            filteredSpells.push_back(spell);
        }
    }
    return filteredSpells;
}
