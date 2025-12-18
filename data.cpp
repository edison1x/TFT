#include <iostream>
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <rapidjson/document.h>
#include <vector>
using namespace std;
using namespace rapidjson;


class HandleAPI {
private:
    string api_key = ""; // insert you own riot api key here
    // info needed to get your puuid by riot id
    string region = "";
    string riot_name = "";
    string tag = "";
    string id = "";
public:
    void initAccount(){
        string url = getPuuidUrl();
        Document doc = getDataJSON(url);
        id = getPUUID(doc);
    }
public:
    string getPuuidUrl(){
        stringstream ss;
        ss << "https://" << region << ".api.riotgames.com/riot/account/v1/accounts/by-riot-id/"
           << riot_name << "/" << tag << "?api_key=" << api_key;
        return ss.str();
    }
public:
    string getMatchidsUrl(){
        stringstream ss;
        ss <<"https://" << region << ".api.riotgames.com/tft/match/v1/matches/by-puuid/" << id << "/ids?start=0&count=20&api_key=" << api_key;
        return ss.str();
    }
public:
    string getMatchDataUrl(string matchId){
        stringstream ss;
        ss << "https://" << region << ".api.riotgames.com/tft/match/v1/matches/" << matchId << "?api_key=" << api_key;
        return ss.str();
    }
public:
    Document getDataJSON(string url) {
        string response;
        CURL *curl = curl_easy_init();
        if (curl) {
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data); // transfer this data instead of outputting it
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            CURLcode res = curl_easy_perform(curl);// connects to the site and performs the transfer, calling the callback function.
            curl_easy_cleanup(curl);
        }
        Document doc;
        doc.Parse(response.c_str());
        return doc;
    }

public:
    std::string getPUUID(Document& doc) {
        id = std::string(doc["puuid"].GetString());
        return id;
    }

public:
    vector<string> getMatchIDs() {
        string url = getMatchidsUrl();
        Document doc = getDataJSON(url);
        vector<string> matchids;
        for (auto& v : doc.GetArray()) {
            if (v.IsString())
                matchids.push_back(v.GetString());
        }
        return matchids;
    }

public:

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


int main() {
    HandleAPI api;
    api.initAccount();
    api.getMatchIDs();
    return 0;
}