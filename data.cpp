#include <iostream>
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <map>
#include <rapidjson/document.h>
#include <vector>
using namespace std;
using namespace rapidjson;

struct MatchSummary{
    string matchID;
    int placement;
    int level;
    int goldLeft; // gold left after player eliminated
    int playersEliminated; // num players that participant eliminated
    int lastRound; //  If the player was eliminated in stage 2-1 their last_round would be 5
    float timeEliminated; // number of seconds before participant eliminated
    int totalDmg; // damage to other players
    float gameLength;
    bool win;
    vector<string> traits;
    vector<string> units;
};
class HandleAPI {
private:
    string api_key = ""; // insert you own riot api key here
    // info needed to get your puuid by riot id
    string region = "";
    string riot_name ="";
    string tag = "";
    string puuid = "";

private:
    /**
     * Uses the region, name and tag to fetch and store puuid
     */
    void initAccount(){
        string url = getPuuidUrl();
        Document doc = getDataJSON(url);
        puuid = getPUUID(doc);
    }
/**
 * These getURL functions will build the correct url containing the JSON data needed
 */
private:
    string getPuuidUrl(){
        stringstream ss;
        ss << "https://" << region << ".api.riotgames.com/riot/account/v1/accounts/by-riot-id/"
           << riot_name << "/" << tag << "?api_key=" << api_key;
        return ss.str();
    }
private:
    string getMatchIDsUrl(){
        stringstream ss;
        ss <<"https://" << region << ".api.riotgames.com/tft/match/v1/matches/by-puuid/" << puuid << "/ids?start=0&count=20&api_key=" << api_key;
        return ss.str();
    }
private:
    string getMatchDataUrl(string& matchID){
        stringstream ss;
        ss << "https://" << region << ".api.riotgames.com/tft/match/v1/matches/" << matchID << "?api_key=" << api_key;
        return ss.str();
    }
private:
    Document getDataJSON(string& url) {
        string response;
        CURL *curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // transfer this data instead of outputting it
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_perform(curl);// connects to the site and performs the transfer, calling the callback function.
            curl_easy_cleanup(curl);
        }
        Document doc;
        doc.Parse(response.c_str());
        return doc;
    }

private:
    std::string getPUUID(Document& doc) {
        puuid = string(doc["puuid"].GetString());
        return puuid;
    }

private:
    vector<string> getMatchIDs() {
        string url = getMatchIDsUrl();
        Document doc = getDataJSON(url);
        vector<string> matchIDs;
        for (auto& v : doc.GetArray()) {
            if (v.IsString())
                matchIDs.push_back(v.GetString());
        }
        return matchIDs;
    }

private:
    vector<MatchSummary> parseMatches(vector<string>& matchIDs){
        vector<MatchSummary> summaries;
        for (string id: matchIDs){
              string url = getMatchDataUrl(id);
              Document doc = getDataJSON(url);
              MatchSummary summary = parseMatch(doc);
              summaries.push_back(summary);
        }
        return summaries;
    }

private:
     MatchSummary parseMatch(Document& doc) {
        MatchSummary summary{};
        summary.matchID = doc["metadata"]["match_id"].GetString();
        summary.gameLength = doc["info"]["game_length"].GetFloat();
        auto& players = doc["info"]["participants"];
        for (auto& p: players.GetArray()){
            if (puuid == p["puuid"].GetString()){
                summary.placement = p["placement"].GetInt();
                summary.level = p["level"].GetInt();
                summary.playersEliminated = p["players_eliminated"].GetInt();
                summary.lastRound = p["last_round"].GetInt();
                summary.goldLeft = p["gold_left"].GetInt();
                summary.timeEliminated = p["time_eliminated"].GetFloat();
                summary.totalDmg = p["total_damage_to_players"].GetInt();
                summary.win = p["win"].GetBool();
                break;
            }
        }
        return summary;
    }

/**
 * This function should be the only one called from outside of class.
 *
 * @returns A resizeable array of MatchSummaries.
 */
public:
    vector<MatchSummary> getData(){
        // first get the puuid of the player from the riod name and tag
        initAccount();
        // next get the last 20 match ids
        vector<string> matchIDs = getMatchIDs();
        // parse matches will iterate through all matches, calling parseMatch to get a summary of each game
        return parseMatches(matchIDs);


    }

/**
 * Handles incomping data by repeadetly calling as chunks of data from server
 * @param ptr points to the delivered data
 * @param size always size 1
 * @param nmemb is the size of that data
 * @param userdata
 * @return callback returns the number of bytes processed
 */
public:
    static size_t write_data(void *ptr, size_t size, size_t nmemb, void *userdata) {
        std::string* str = static_cast<std::string*>(userdata);
        str->append(static_cast<char*>(ptr), size * nmemb); // append raw data to string
        return size * nmemb;
    }
};

struct averages{
    int numMatches;
    float placement;
    float level;
    float goldLeft;
    float playersEliminated;
    float lastRound;
    float timeEliminated;
    float totalDmg;
    float gameLength;
};
averages calcAverages(vector<MatchSummary>& matches, int numMatches){
    averages avg{};
    avg.numMatches = numMatches;

    float sumPlacement = 0;
    float sumLevel = 0;
    float sumGoldLeft = 0;
    float sumPlayersEliminated = 0;
    float sumLastRound = 0;
    float sumTimeEliminated = 0;
    float sumTotalDmg = 0;
    float sumGameLength = 0;
    if (numMatches > 0){
        for (size_t i=0; i < numMatches; ++i){
            sumPlacement += matches[i].placement;
            sumLevel += matches[i].level;
            sumGoldLeft += matches[i].goldLeft;
            sumPlayersEliminated += matches[i].playersEliminated;
            sumLastRound += matches[i].lastRound;
            sumTimeEliminated += matches[i].timeEliminated;
            sumTotalDmg += matches[i].totalDmg;
            sumGameLength += matches[i].gameLength;
        }
    }
    avg.placement = sumPlacement / numMatches;
    avg.level = sumLevel / numMatches;
    avg.goldLeft = sumGoldLeft / numMatches;
    avg.playersEliminated = sumPlayersEliminated / numMatches;
    avg.lastRound = sumLastRound / numMatches;
    avg.timeEliminated = sumTimeEliminated / numMatches;
    avg.totalDmg = sumTotalDmg / numMatches;
    avg.gameLength = sumGameLength / numMatches;

    return avg;

}
int main() {
    HandleAPI api;
    vector<MatchSummary> matches = api.getData();
    averages avg = calcAverages(matches,2);
    cout << "numMatches: " << avg.numMatches<< '\n'
          << "placement: " << avg.placement<< '\n'
          << "level: " << avg.level << '\n'
          << "goldLeft: " << avg.goldLeft << '\n'
          << "playersEliminated: " << avg.playersEliminated << '\n'
          << "lastRound: " << avg.lastRound << '\n'
          << "timeEliminated: " << avg.timeEliminated << '\n'
          << "totalDmg: "  << avg.totalDmg  << '\n'
          << "gameLength: " << avg.gameLength << '\n';

    return 0;
}