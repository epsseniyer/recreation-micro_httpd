#include <iostream>
#include <fstream>
#include <string>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unordered_map>
#include <jsoncpp/json/json.h>

std::unordered_map<std::string, std::string> routes;
std::unordered_map<std::string, std::string> returnToStatus;

void handleRequest(int clientSocket)
{
    char buffer[4096];
    memset(buffer, 0, sizeof(buffer));
    recv(clientSocket, buffer, sizeof(buffer) - 1, 0);

    std::string request(buffer);

    std::string method = request.substr(0, request.find(' '));
    std::string path = request.substr(request.find(' ') + 1);
    path = path.substr(0, path.find(' '));

    std::string response;
    if (routes.find(path) != routes.end())
    {
        response = "HTTP/1.1 200 OK\r\n"
                   "Content-Type: text/html\r\n"
                   "Content-Length: " +
                   std::to_string(routes[path].size()) + "\r\n"
                                                         "\r\n" +
                   routes[path];
    }
    else
    {
        std::string notFoundPage = returnToStatus["404"];
        if (notFoundPage.substr(0, 1) == "/")
        {
            std::ifstream file(notFoundPage.substr(1));
            if (file)
            {
                std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
                response = "HTTP/1.1 404 Not Found\r\n"
                           "Content-Type: text/html\r\n"
                           "Content-Length: " +
                           std::to_string(content.size()) + "\r\n"
                                                            "\r\n" +
                           content;
            }
            else
            {
                response = "HTTP/1.1 404 Not Found\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: 9\r\n"
                           "\r\n"
                           "Not Found";
            }
        }
        else
        {
            response = "HTTP/1.1 404 Not Found\r\n"
                       "Content-Type: text/plain\r\n"
                       "Content-Length: " +
                       std::to_string(notFoundPage.size()) + "\r\n"
                                                             "\r\n" +
                       notFoundPage;
        }
    }

    send(clientSocket, response.c_str(), response.size(), 0);
    close(clientSocket);
}

void loadConfig(const std::string &configFile)
{
    std::ifstream file(configFile);
    if (!file)
    {
        std::cerr << "Error loading config file!" << std::endl;
        return;
    }

    Json::Value root;
    file >> root;

    int port = root["port"].asInt();
    const Json::Value &routesJson = root["routes"];
    for (const auto &route : routesJson.getMemberNames())
    {
        routes[route] = routesJson[route].asString();
    }

    const Json::Value &returnToStatusJson = root["returnToStatus"];
    for (const auto &status : returnToStatusJson.getMemberNames())
    {
        returnToStatus[status] = returnToStatusJson[status].asString();
    }
    std::cout << "Config file loaded successfully." << std::endl;
    std::cout << "Listening on port " << port << std::endl;

    int serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0)
    {
        std::cerr << "Error creating server socket!" << std::endl;
        return;
    }

    sockaddr_in serverAddress{};
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0)
    {
        std::cerr << "Error binding server socket!" << std::endl;
        return;
    }

    if (listen(serverSocket, 10) < 0)
    {
        std::cerr << "Error listening on server socket!" << std::endl;
        return;
    }

    while (true)
    {
        sockaddr_in clientAddress{};
        socklen_t clientAddressLength = sizeof(clientAddress);
        int clientSocket = accept(serverSocket, (struct sockaddr *)&clientAddress, &clientAddressLength);

        if (clientSocket < 0)
        {
            std::cerr << "Error accepting client connection!" << std::endl;
            continue;
        }

        handleRequest(clientSocket);
    }
}

int main()
{
    loadConfig("config.json");
    return 0;
}
