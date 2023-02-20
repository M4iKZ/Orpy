#pragma once

#include "Common.hpp"
#include "Response.hpp"

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
        int position = 0;

        std::string method;

        //URL
        std::string URI;
        std::string URL;

        std::vector<std::string> commands;
        int nCommands = 0;

        //Host
        std::string Host;

        //Referer
        std::string Referer = "";

        //Encoding
        //deflate and gzip not supported yet
        bool br = false;

        //Language
        std::string lang;

        //Content-Length
        int contentLength = 0;
        std::string contentType; // basic form

        //Cookies
        std::unordered_map<std::string, std::string> cookies;

        //GET VARs
        bool hasGET = false;
        std::unordered_map<std::string, std::string> GET;

        //POST
        bool isPOST = false;
        std::unordered_map<std::string, std::string> POST;
                
        bool isMultipart = false;
        std::string boundary = "";
        std::unordered_map<std::string, MP_DATA> MULTIPART;
        
        //is FILE?
        bool isFile = false;
        std::string filePath = "";

        //Error
        bool error = false;

        //Conf
        SITEData Conf = SITEData();

        //Repsponse
        HttpResponse response = HttpResponse();
    };
}