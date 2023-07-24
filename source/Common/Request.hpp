#pragma once

namespace Orpy
{
    struct MP_DATA
    {
        std::string Disposition = "";
        std::string name = "";

        bool isFile = false;
        std::string filename = "";
        std::string type = "";
        std::vector<char> data;
    };

    struct HttpRequest
    {
        std::string method = "";

        // IP
        std::string clientIP = "";

        // User-Agent / Bot
        std::string useragent = "";
        bool isBot = false;

        // URL
        std::string URI = "";
        std::string URL = "";        
        std::vector<std::string> commands;
        int nCommands = 0;

        // Host
        std::string Host = "";

        // Referer
        std::string Referer = "";

        // Origin
        std::string Origin = "";

        // Supported Encoding
        // not implemented yet
        bool br = false;

        // Content-Length
        int contentLength = 0;
        std::string contentType = ""; // basic form

        // Cookies
        std::unordered_map<std::string, std::string> cookies;

        // GET VARs
        bool hasGET = false;
        std::unordered_map<std::string, std::string> GET;

        // POST
        bool isPOST = false;
        std::unordered_map<std::string, std::string> POST;
        bool done = false;
                
        bool isMultipart = false;
        std::string boundary = "";
        std::unordered_map<std::string, MP_DATA> MULTIPART;
                
        // is FILE?
        bool isFile = false;
        std::string filePath = "";
        std::string fileModifiedSince = "";
    };
}