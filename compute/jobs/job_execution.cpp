#include "job.hpp"

#include <regex>
#include <stdexcept>
#include <string>

using namespace std;


namespace compute_jobs {
namespace {

string extract_string(const string& json, const string& key) {
    string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == string::npos) throw runtime_error("Missing string field: " + key);
    pos = json.find(':', pos + search.length());
    if (pos == string::npos) throw runtime_error("Missing string field: " + key);
    pos = json.find('"', pos);
    if (pos == string::npos) throw runtime_error("Missing string field: " + key);
    pos++;
    size_t end = pos;
    while (end < json.length() && json[end] != '"') {
        if (json[end] == '\\') end += 2;
        else end++;
    }
    if (end >= json.length()) throw runtime_error("Missing string field: " + key);
    return json.substr(pos, end - pos);
}

int extract_int(const string& json, const string& key) {
    string search = "\"" + key + "\"";
    size_t pos = json.find(search);
    if (pos == string::npos) throw runtime_error("Missing integer field: " + key);
    pos = json.find(':', pos + search.length());
    if (pos == string::npos) throw runtime_error("Missing integer field: " + key);
    pos++;
    while (pos < json.length() && isspace(json[pos])) pos++;
    size_t end = pos;
    if (end < json.length() && json[end] == '-') end++;
    while (end < json.length() && isdigit(json[end])) end++;
    if (pos == end) throw runtime_error("Missing integer field: " + key);
    return stoi(json.substr(pos, end - pos));
}

string extract_optional_string(const string& json, const string& key, const string& default_val) {
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
    return json.substr(pos, end - pos);
}

int extract_optional_int(const string& json, const string& key, int default_val) {
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
    return stoi(json.substr(pos, end - pos));
}

}

Job receive_forwarded_job(const string& master_payload) {
    Job job(extract_string(master_payload, "name"),
            extract_string(master_payload, "executable"),
            extract_int(master_payload, "priority"),
            extract_int(master_payload, "time_required"),
            extract_int(master_payload, "min_memory"),
            extract_int(master_payload, "min_cores"),
            extract_int(master_payload, "max_memory"),
            extract_optional_int(master_payload, "gpu_required", 0));

    job.executable_name = extract_optional_string(master_payload, "executable_name", "");
    job.executable_b64 = extract_optional_string(master_payload, "executable_b64", "");

    job.set_submission_id(extract_int(master_payload, "submission_id"));
    job.set_receipt_id(extract_int(master_payload, "receipt_id"));
    job.set_sender(extract_string(master_payload, "sender"));

    return job;
}

}
