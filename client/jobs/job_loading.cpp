#include "job.hpp"

#include <fstream>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>

using namespace std;

namespace client_jobs {


string read_file(const string& config_path) {
    ifstream input(config_path.c_str());
    if (!input) {
        throw runtime_error("Unable to open job config: " + config_path);
    }

    ostringstream buffer;
    buffer << input.rdbuf();
    return buffer.str();
}

string extract_string(const string& json, const string& key) {
    const regex pattern("\"" + key + "\"\\s*:\\s*\"([^\"]*)\"");
    smatch match;
    if (!regex_search(json, match, pattern)) {
        throw runtime_error("Missing string field: " + key);
    }

    return match[1].str();
}

int extract_int(const string& json, const string& key) {
    const regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    smatch match;
    if (!regex_search(json, match, pattern)) {
        throw runtime_error("Missing integer field: " + key);
    }

    return stoi(match[1].str());
}

int extract_optional_int(const string& json, const string& key, int default_val) {
    const regex pattern("\"" + key + "\"\\s*:\\s*(-?\\d+)");
    smatch match;
    if (!regex_search(json, match, pattern)) {
        return default_val;
    }
    return stoi(match[1].str());
}



Job load_job_from_config(const string& config_path) {
    const string json = read_file(config_path);

    return Job(extract_string(json, "name"),
               extract_string(json, "executable"),
               extract_int(json, "priority"),
               extract_int(json, "time_required"),
               extract_int(json, "min_memory"),
               extract_int(json, "min_cores"),
               extract_int(json, "max_memory"),
               extract_optional_int(json, "gpu_required", 0));
}

}
