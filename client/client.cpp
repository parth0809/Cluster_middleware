#include "jobs/job.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <thread>

#include <cstdint>

#include "../common/base64.hpp"

using namespace std;

string read_socket_payload(int socket_fd);
bool send_socket_payload(int socket_fd, const string& payload);

namespace {

int choose_callback_port() {
    return 10000 + (getpid() % 1000);
}

string attach_callback_port(const string& payload, int callback_port) {
    size_t close_brace = payload.rfind('}');
    if (close_brace == string::npos) {
        return payload;
    }

    string updated = payload;
    updated.insert(close_brace, ",\"client_callback_port\":" + to_string(callback_port));
    return updated;
}

string attach_executable_upload(const string& payload, const string& executable_path) {
    if (executable_path.empty()) {
        return payload;
    }

    ifstream in(executable_path.c_str(), ios::binary);
    if (!in) {
        cerr << "[CLIENT WARN] Unable to open executable for upload at " << executable_path << endl;
        return payload;
    }

    vector<unsigned char> bytes((istreambuf_iterator<char>(in)), istreambuf_iterator<char>());
    string b64 = common::base64_encode(bytes);

    string name = executable_path;
    size_t sep = name.find_last_of('/');
    if (sep != string::npos) {
        name = name.substr(sep + 1);
    }

    size_t close_brace = payload.rfind('}');
    if (close_brace == string::npos) {
        return payload;
    }

    string updated = payload;
    updated.insert(close_brace,
                   ",\"executable_name\":\"" + name + "\",\"executable_b64\":\"" + b64 + "\"");
    return updated;
}

string attach_callback_address(const string& payload, const string& callback_address) {
    if (callback_address.empty()) {
        return payload;
    }

    size_t close_brace = payload.rfind('}');
    if (close_brace == string::npos) {
        return payload;
    }

    string updated = payload;
    updated.insert(close_brace, ",\"client_callback_address\":\"" + callback_address + "\"");
    return updated;
}

void recovery_listener(int callback_port) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        cerr << "[CLIENT WARN] Recovery listener socket creation failed." << endl;
        return;
    }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    sockaddr_in address{};
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(callback_port);

    if (bind(server_fd, reinterpret_cast<sockaddr*>(&address), sizeof(address)) < 0) {
        cerr << "[CLIENT WARN] Recovery listener bind failed on port " << callback_port << "." << endl;
        close(server_fd);
        return;
    }

    listen(server_fd, 10);

    while (true) {
        socklen_t addrlen = sizeof(address);
        int client_socket = accept(server_fd, reinterpret_cast<sockaddr*>(&address), &addrlen);
        if (client_socket < 0) {
            continue;
        }

        string message = read_socket_payload(client_socket);
        if (!message.empty()) {
            cout << "\n[CLIENT RECOVERY] Received result after master failover:" << endl;
            cout << message << endl;
            cout << "\nEnter config location (or 'exit' to quit): " << flush;
        }
        close(client_socket);
    }
}

}

string read_socket_payload(int socket_fd) {
    uint64_t net_len = 0;
    unsigned char* len_bytes = reinterpret_cast<unsigned char*>(&net_len);
    size_t got = 0;
    while (got < sizeof(net_len)) {
        int n = read(socket_fd, len_bytes + got, sizeof(net_len) - got);
        if (n <= 0) return "";
        got += static_cast<size_t>(n);
    }

    uint64_t len = 0;
    for (int i = 0; i < 8; ++i) {
        len = (len << 8) | static_cast<uint64_t>(len_bytes[i]);
    }

    string payload;
    payload.resize(static_cast<size_t>(len));
    size_t off = 0;
    while (off < payload.size()) {
        int n = read(socket_fd, &payload[off], payload.size() - off);
        if (n <= 0) return "";
        off += static_cast<size_t>(n);
    }
    return payload;
}

bool send_socket_payload(int socket_fd, const string& payload) {
    uint64_t len = static_cast<uint64_t>(payload.size());
    unsigned char header[8];
    for (int i = 7; i >= 0; --i) {
        header[i] = static_cast<unsigned char>(len & 0xFF);
        len >>= 8;
    }
    if (send(socket_fd, header, sizeof(header), 0) != static_cast<int>(sizeof(header))) {
        return false;
    }

    size_t off = 0;
    while (off < payload.size()) {
        int n = send(socket_fd, payload.data() + off, payload.size() - off, 0);
        if (n <= 0) return false;
        off += static_cast<size_t>(n);
    }
    return true;
}

#include "../common/config_parser.hpp"

int main() {
    cout << "Starting Client" << endl;
    
    string sys_config_json = common::read_file("client_config.json");
    string master_addr = common::extract_string(sys_config_json, "master_address", "");
    int master_port = common::extract_int(sys_config_json, "master_tcp_port", 8080);
    string callback_addr = common::extract_string(sys_config_json, "client_callback_address", "");

    if (master_addr.empty()) {
        cerr << "[CLIENT FATAL] Missing required config: master_address in client_config.json" << endl;
        return 1;
    }
    
    int callback_port = choose_callback_port();
    thread listener_thread(recovery_listener, callback_port);
    listener_thread.detach();
    cout << "Recovery listener active on port " << callback_port << endl;
    cout << "Targeting Master at " << master_addr << ":" << master_port << endl;
    
    string backup_addr = "";
    int backup_port = 0;

    int disc_sock = socket(AF_INET, SOCK_STREAM, 0);
    struct sockaddr_in disc_addr;
    disc_addr.sin_family = AF_INET;
    disc_addr.sin_port = htons(master_port);
    inet_pton(AF_INET, master_addr.c_str(), &disc_addr.sin_addr);

    if (connect(disc_sock, (struct sockaddr *)&disc_addr, sizeof(disc_addr)) >= 0) {
        string handshake = "{\"request\":\"discover_backup\"}";
        send_socket_payload(disc_sock, handshake);
        string response = read_socket_payload(disc_sock);
        backup_addr = common::extract_string(response, "backup_address");
        backup_port = common::extract_int(response, "backup_tcp_port");
        cout << "[DISCOVERY] Learned Backup Master located at " << backup_addr << ":" << backup_port << endl;
    } else {
        cout << "[DISCOVERY WARN] Could not reach Primary Master for handshake. Fallbacks unavailable." << endl;
    }
    close(disc_sock);
    
    while (true) {
        cout << "\nEnter config location (or 'exit' to quit): ";
        string config_path;
        if (!getline(cin, config_path) || config_path == "exit" || config_path == "quit") {
            break;
        }
        
        if (config_path.empty()) {
            continue;
        }

        string client_payload;
        try {
            client_payload = client_jobs::read_file(config_path);
        } catch (const std::exception& e) {
            cerr << " " << e.what() << endl;
            continue;
        }
        if (client_payload.empty()) {
            continue;
        }

        client_jobs::Job client_job = client_jobs::load_job_from_config(config_path);
        client_payload = attach_callback_port(client_payload, callback_port);
        client_payload = attach_callback_address(client_payload, callback_addr);
        client_payload = attach_executable_upload(client_payload, client_job.executable);
        cout << "Loaded job: " << client_job.name << " from " << config_path << endl;
        
        int sock = 0;
        struct sockaddr_in serv_addr;
        
        if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            cerr << "Socket creation error" << endl;
            continue;
        }
        
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(master_port);
        
        if (inet_pton(AF_INET, master_addr.c_str(), &serv_addr.sin_addr) <= 0) {
            cerr << " Invalid address or Address not supported" << endl;
            close(sock);
            continue;
        }
        
        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            cerr << "[CLIENT] Connection Failed to Primary Master (" << master_addr << ":" << master_port << ")" << endl;
            close(sock);
            if (!backup_addr.empty() && backup_port > 0) {
                cout << "[CLIENT] Attempting Backup Node Fallback Route: " << backup_addr << ":" << backup_port << "..." << endl;
                sock = socket(AF_INET, SOCK_STREAM, 0);
                serv_addr.sin_port = htons(backup_port);
                inet_pton(AF_INET, backup_addr.c_str(), &serv_addr.sin_addr);
                if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                    cerr << "[CLIENT FATAL] Connection to Backup Node also failed." << endl;
                    close(sock);
                    continue;
                }
            } else {
                continue;
            }
        }
        
        cout << " Connected to Active Master. Sending job definition..." << endl;
        send_socket_payload(sock, client_payload);
        
        cout << " Waiting for response..." << endl;
        string response = read_socket_payload(sock);

        if (!response.empty()) {
            cout << " Received response from Master:" << endl;
            cout << response << endl;
        } else {
            cout << " Connection closed without response. Waiting for recovery listener in case backup master completes the job." << endl;
        }
        
        close(sock);
    }
    
    return 0;
}
