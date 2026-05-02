#include "job.hpp"

#include <regex>
#include <stdexcept>
#include <string>

using namespace std;


namespace compute_jobs {
namespace {

//get values against key from json format 
string get_string(const string& json, const string& key) {
    string search = "\"" + key + "\"";
    // raise eror in case of wrong syntax
    // missng key
    size_t pos = json.find(search);
    if (pos == string::npos) throw runtime_error("Missing string field: " + key);
    // missing :
    pos = json.find(':', pos + search.length());
    if (pos == string::npos) throw runtime_error("Missing string field: " + key);
    // missing "
    pos = json.find('"', pos);
    if (pos == string::npos) throw runtime_error("Missing string field: " + key);
    pos++;
    size_t end = pos;
    // end is "
    while (end < json.length() && json[end] != '"') {
        if (json[end] == '\\') end += 2;
        else end++;
    }
    if (end >= json.length()) throw runtime_error("Missing string field: " + key);
    // return substring from : to "
    return json.substr(pos, end - pos);
}

// get integet walut
int get_int(const string& json, const string& key) {
        // raise eror in case of wrong syntax
    // missng key
    string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == string::npos) throw runtime_error("Missing integer field: " + key);
    // missing ":
    pos = json.find(':', pos + search.length());
    if (pos == string::npos) throw runtime_error("Missing integer field: " + key);
    pos++;
    while (pos < json.length() && isspace(json[pos])) pos++;
    size_t end = pos;
    // missing " detect end
    if (end < json.length() && json[end] == '-') end++;
    while (end < json.length() && isdigit(json[end])) end++;
    if (pos == end) throw runtime_error("Missing integer field: " + key);
    // convert into integer
    return stoi(json.substr(pos, end - pos));
}

// get optional string walut
string get_optional_string(const string& json, const string& key, const string& default_val) {
    // check if syntax is valid
    string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == string::npos) return default_val;
    pos = json.find(':', pos + search.length());
    if (pos == string::npos) return default_val;
    pos = json.find('"', pos);
    if (pos == string::npos) return default_val;
    pos++;
    size_t end = pos;
    while (end < json.length() && json[end] != '"') {
        if (json[end] == '\\') end += 2;
        else end++;
    }
    if (end >= json.length()) return default_val;
    // return stirng substring in case preseent
    return json.substr(pos, end - pos);
}

// get optional integer walut
int get_optional_int(const string& json, const string& key, int default_val) {
    string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == string::npos) return default_val;
    pos = json.find(':', pos + search.length());
    if (pos == string::npos) return default_val;
    pos++;
    while (pos < json.length() && isspace(json[pos])) pos++;
    size_t end = pos;
    if (end < json.length() && json[end] == '-') end++;
    while (end < json.length() && isdigit(json[end])) end++;
    if (pos == end) return default_val;
    // return integer in case preseent
    return stoi(json.substr(pos, end - pos));
}

}

// recieve jobs from payloads
Job receive_forwarded_job(const string& master_payload) {
    Job job(get_string(master_payload, "name"),
            get_string(master_payload, "executable"),
            get_int(master_payload, "priority"),
            get_int(master_payload, "time_required"),
            get_int(master_payload, "min_memory"),
            get_int(master_payload, "min_cores"),
            get_int(master_payload, "max_memory"),
            get_optional_int(master_payload, "gpu_required", 0));

    job.executable_name = get_optional_string(master_payload, "executable_name", "");
    job.executable_b64 = get_optional_string(master_payload, "executable_b64", "");

    job.set_submission_id(get_int(master_payload, "submission_id"));
    job.set_receipt_id(get_int(master_payload, "receipt_id"));
    job.set_sender(get_string(master_payload, "sender"));

    return job;
}

}
