#include <iostream>
#include <type_traits>
#include <functional>
#include <memory>
#include <string>
#include <cctype>

#include "Parser.hpp"

#include "IConfs.hpp"

namespace Orpy
{
    enum State
    {
        READ_METHOD,
        READ_URI,
        READ_PROTOCOL,
        READ_HEADERS,
        DONE,
        ERROR
    };

    enum MULTIPARTState
    {
        HEADER,
        DISPOSITION,
        NAME,
        FILE,
        TYPE,
        DATA,
        END,
        FINISH
    };

    std::string getValue(std::vector<char> buffer, int& position, bool check = false)
    {
        std::string Value;
        bool port = false;
        while (buffer[position] != '\r' && buffer[position + 1] != '\n')
        {
            if (check && buffer[position] == ':')
            {
                port = true;
                position++;
            }
            else if (port)
                position++;
            else
            {
                Value.push_back(buffer[position]);

                position++;
            }
        }

        position += 2; //Remove \r\n

        return Value;
    }

    void urlDecode(std::string& value)
    {
        std::string encoded = value;
        value = "";

        for (size_t i = 0; i < encoded.size(); ++i)
        {
            if (encoded[i] == '%')
            {
                std::stringstream ss;
                ss << std::hex << encoded.substr(i, 3).substr(1);
                unsigned int asciiCode;
                ss >> asciiCode;
                char asciiChar = static_cast<char>(asciiCode);
                std::string normalChar(1, asciiChar);

                value += normalChar;

                i += 2;
            }
            else
                value += encoded[i];
        }
    }

    State parse(State current_state, std::vector<char> buffer, HttpRequest* request)
    {
        switch (current_state)
        {
        case READ_METHOD:
            if (buffer[request->position] == 'G' && buffer[request->position + 1] == 'E' && buffer[request->position + 2] == 'T')
            {
                //GET
                request->method = "GET";

                request->position += 4;

                return READ_URI;
            }
            else if (buffer[request->position] == 'P' && buffer[request->position + 1] == 'O' && buffer[request->position + 2] == 'S' && buffer[request->position + 3] == 'T')
            {
                request->method = "POST";

                request->isPOST = true;

                request->position += 5;

                return READ_URI;
            }

            request->response.status = 501; //not implemented

            return ERROR;
            break;
        case READ_URI:
            if (buffer[request->position] == '?')
            {
                request->hasGET = true;

                ++request->position;

                // ?v=hKXVqnEIBh0&list=RDQM58p3c0RNCrI&index=9
                std::string key = "";
                while (buffer[request->position] != 32)
                {
                    if (buffer[request->position] == '=')
                    {
                        ++request->position; //jump =

                        while (buffer[request->position] != '&' && buffer[request->position] != 32)
                        {
                            request->GET[key].push_back(buffer[request->position]);

                            ++request->position;
                        }

                        urlDecode(request->GET[key]);

                        if (buffer[request->position] == 32)
                            break;

                        key = "";
                    }
                    else
                        key.push_back(buffer[request->position]);

                    ++request->position;
                }

                return READ_PROTOCOL;
            }
            else if (buffer[request->position] == '%')
            {
                if (request->position + 2 < buffer.size())
                {
                    if (std::isdigit(buffer[request->position + 1]) && std::isdigit(buffer[request->position + 2]))
                    {
                        std::string hex;
                        hex.push_back(buffer[request->position + 1]);
                        hex.push_back(buffer[request->position + 2]);

                        request->URI.push_back(buffer[request->position]);
                        request->URI.push_back(buffer[request->position + 1]);
                        request->URI.push_back(buffer[request->position + 2]);

                        int value = std::stoi(hex, nullptr, 16);
                        std::string character;
                        character.push_back(static_cast<char>(value));
                        request->URL += character;
                        request->commands[request->nCommands - 1] += character;

                        request->position += 3;
                    }
                    else
                    {
                        request->URI.push_back(buffer[request->position]);
                        request->URL.push_back(buffer[request->position]);
                        request->commands[request->nCommands - 1].push_back(buffer[request->position]);

                        ++request->position;
                    }
                }
            }
            else
            {
                if (buffer[request->position] == '/')
                {
                    request->nCommands++;
                    request->commands.push_back("");
                }
                else
                {
                    request->commands[request->nCommands - 1].push_back(buffer[request->position]);
                }

                request->URI.push_back(buffer[request->position]);
                request->URL.push_back(buffer[request->position]);

                ++request->position;
            }

            if (buffer[request->position] == ' ') //(isspace(buffer[request->position]))
                return READ_PROTOCOL;
            else
                return READ_URI;

            break;
        case READ_PROTOCOL:
            if (buffer[request->position] == '\r' && buffer[request->position + 1] == '\n')
            {
                request->position += 2;

                return READ_HEADERS;
            }
            else
            {
                ++request->position;

                return READ_PROTOCOL;
            }

            break;
        case READ_HEADERS:
            if ((buffer[request->position] == 'H' || buffer[request->position] == 'h') && buffer[request->position + 1] == 'o' && buffer[request->position + 3] == 't')
            {
                //Host
                request->position += 6;

                request->Host = getValue(buffer, request->position, true);

                if (!_confs->Get(request->Host, request->Conf))
                {
                    request->response.status = 444;

                    return ERROR;
                }

                if (request->Conf.redirect != "")
                {
                    request->response.status = 303;
                    request->response.location = request->Conf.redirect + request->URI;

                    return ERROR;
                }

                return READ_HEADERS;
            }
            else if ((buffer[request->position] == 'A' || buffer[request->position] == 'a') && buffer[request->position + 13] == 'n' && buffer[request->position + 14] == 'g')
            {
                //Accept-Encoding: gzip, deflate, br
                request->position += 16;

                while (buffer[request->position] != '\r' && buffer[request->position + 1] != '\n')
                {
                    if (buffer[request->position] == 'b' && buffer[request->position + 1] == 'r')
                        request->br = true;

                    ++request->position;
                }

                request->position += 2; //Remove \r\n

                return READ_HEADERS;
            }
            else if (request->isPOST && (buffer[request->position] == 'C' || buffer[request->position] == 'c') && buffer[request->position + 7] == '-' && buffer[request->position + 8] == 'T' && buffer[request->position + 9] == 'y' && buffer[request->position + 10] == 'p' && buffer[request->position + 11] == 'e')
            {
                // Content-Type: application/x-www-form-urlencoded
                request->position += 14;

                /*
                0 : application/x-www-form-urlencoded
                1 : multipart/form-data
                2 : application/json
                3 : application/xml
                4 : application/pdf
                5 : application/octet-stream
                */

                // Content-Type: multipart/form-data; boundary=[boundary string]

                if (buffer[request->position] == 'm' && buffer[request->position + 18] == 'a')
                {
                    request->position += 30;

                    request->isMultipart = true;
                    request->boundary = "--";
                    request->boundary += getValue(buffer, request->position);
                }
                else
                {
                    request->contentType = getValue(buffer, request->position);

                    if (request->contentType.find("application/x-www-form-urlencoded") != 0)
                    {
                        request->isPOST = false;
                        request->response.status = 501; //not implemented                        

                        return ERROR;
                    }
                }

                return READ_HEADERS;
            }
            else if (request->isPOST && (buffer[request->position] == 'C' || buffer[request->position] == 'c') && buffer[request->position + 12] == 't' && buffer[request->position + 13] == 'h')
            {
                //Content-Length: 37    
                request->position += 16;

                request->contentLength = std::stoi(getValue(buffer, request->position));

                return READ_HEADERS;
            }
            else if ((buffer[request->position] == 'C' || buffer[request->position] == 'c') && buffer[request->position + 4] == 'i' && buffer[request->position + 5] == 'e')
            {
                //Cookie: session_id=abc123; user_id=1;
                request->position += 8;

                std::string value;
                std::string key;
                while (1)
                {
                    if (buffer[request->position] == '=')
                    {
                        request->cookies[value] = "";
                        key = value;
                        value = "";
                    }
                    else if (buffer[request->position] == ';' || buffer[request->position] == '\r')
                    {
                        request->cookies[key] = value;
                        value = "";

                        if (buffer[request->position] == '\r')
                            break;

                        request->position++; //jump the space?
                    }
                    else
                    {
                        value.push_back(buffer[request->position]);
                    }

                    request->position++;
                }

                return READ_HEADERS;
            }
            else if ((buffer[request->position] == 'O' || buffer[request->position] == 'o') && buffer[request->position + 1] == 'r' && buffer[request->position + 2] == 'i' && buffer[request->position + 3] == 'g' && buffer[request->position + 4] == 'i' && buffer[request->position + 6] == ':')
            {
                //Origin: http://localhost:8888
                request->position += 8;

                std::string url = getValue(buffer, request->position);
                std::string http = "http://";
                std::string https = "https://";

                if (url.find(http) == 0)
                    url.erase(0, http.length());
                else if (url.find(https) == 0)
                    url.erase(0, https.length());

                if (url.find(":") != std::string::npos)
                    url.erase(url.find(":"), 1);

                request->Origin = url;

                return READ_HEADERS;
            }
            else if ((buffer[request->position] == 'R' || buffer[request->position] == 'r') && buffer[request->position + 6] == 'r')
            {
                //Referer: http://localhost:8888/
                request->position += 9;

                request->Referer = getValue(buffer, request->position);

                return READ_HEADERS;
            }
            else if ((buffer[request->position] == 'I' || buffer[request->position] == 'i') && buffer[request->position + 2] == '-' && buffer[request->position + 11] == '-')
            {
                //If-Modified-Since
                request->position += 19;

                request->fileModifiedSince = getValue(buffer, request->position);

                return READ_HEADERS;
            }
            else if (buffer[request->position] == 'I' && buffer[request->position + 1] == 'P')
            {
                //IP
                request->position += 4;

                request->clientIP = getValue(buffer, request->position);

                return READ_HEADERS;
            }
            else if ((buffer[request->position] == 'U' || buffer[request->position] == 'u') && buffer[request->position + 4] == '-')
            {
                //User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/113.0.0.0 Safari/537.36
                request->position += 12;

                request->useragent = getValue(buffer, request->position);

                std::string useragent = request->useragent;
                std::transform(useragent.begin(), useragent.end(), useragent.begin(), ::tolower); // find() is case sensitive, check lower string instead

                if (useragent.find("bot") != std::string::npos || useragent.find("crawler") != std::string::npos || useragent.find("spider") != std::string::npos)
                    request->isBot = true;

                return READ_HEADERS;
            }
            else if (buffer[request->position] == '\r' && buffer[request->position + 1] == '\n' && buffer[request->position + 2] == '\r' && buffer[request->position + 3] == '\n')
            {
                request->position += 4;

                return DONE;
            }
            else
            {
                ++request->position;

                return READ_HEADERS;
            }

            break;
        case DONE:
            return DONE;

            break;
        }

        return ERROR;
    }

    void parser(HttpRequest* request, std::vector<char>& buffer)
    {
        State current_state = READ_METHOD;
        while (request->position < buffer.size())
        {
            current_state = parse(current_state, buffer, request);

            if (current_state == DONE)
                break;
            else if (current_state == ERROR)
            {
                request->error = true;
                break;
            }
        }
    }

    bool getType(HttpRequest* request, std::string& format)
    {
        //CUSTOM SITE MIME TYPES
        auto conf = request->Conf.mimetypes.find(format);
        if (conf != request->Conf.mimetypes.end())
        {
            format = conf->second;
            return true;
        }

        //ORPY DEFAULT MIME TYPES
        auto it = ContentTypeMapping.find(format);
        if (it == ContentTypeMapping.end())
            return false;

        format = it->second;

        return true;
    }

    void getPOST(std::vector<char>& buffer, HttpRequest* request)
    {
        //formType=Form-Url-Encoded&name=&email=        
        if (request->position < buffer.size())
        {
            std::string key = "";

            while (buffer[request->position] != '\r' && buffer[request->position + 1] != '\n' && request->position + 1 < buffer.size())
            {
                if (buffer[request->position] == '=')
                {
                    ++request->position; //jump =

                    while (buffer[request->position] != '&' && buffer[request->position] != '\r' && request->position < buffer.size())
                    {
                        request->POST[key].push_back(buffer[request->position]);

                        ++request->position;
                    }

                    urlDecode(request->POST[key]);

                    key = "";
                }
                else
                    key.push_back(buffer[request->position]);

                ++request->position;
            }
        }
    }

    void elaborateMultipart(std::vector<char>& buffer, HttpRequest* request)
    {
        bool work = true;
        MULTIPARTState state = HEADER;
        MP_DATA data;
        std::string boundary = "";
        std::string line = "";
        int position = request->position;
        while (work)
        {
            switch (state)
            {
            case HEADER:
                boundary = getValue(buffer, request->position);

                if (boundary == request->boundary)
                {
                    state = DISPOSITION;
                }
                else
                    state = FINISH;
                break;
            case DISPOSITION:
                if (buffer[request->position] == 'C' && buffer[request->position + 18] == 'n')
                {
                    request->position += 21;

                    while (buffer[request->position] != ';')
                    {
                        data.Disposition.push_back(buffer[request->position]);

                        ++request->position;
                    }

                    request->position += 2; //jump the space

                    state = NAME;
                }
                else
                    state = FINISH;
                break;
            case NAME:
                if (buffer[request->position] == 'n' && buffer[request->position + 3] == 'e')
                {
                    request->position += 6;

                    while (buffer[request->position] != '"')
                    {
                        data.name.push_back(buffer[request->position]);

                        ++request->position;
                    }

                    request->position += 3; //jump the space

                    if (buffer[request->position] == '\r')
                    {
                        state = DATA;

                        request->position += 2; //jump \r\n
                    }
                    else
                    {
                        state = FILE;
                    }
                }
                else
                    state = FINISH;
                break;
            case FILE:
                if (buffer[request->position] == 'f' && buffer[request->position + 7] == 'e')
                {
                    request->position += 10;

                    while (buffer[request->position] != '"')
                    {
                        data.filename.push_back(buffer[request->position]);

                        ++request->position;
                    }

                    if (data.filename != "")
                        data.isFile = true;

                    request->position += 3;

                    state = TYPE;
                }
                else
                    state = FINISH;
                break;
            case TYPE:
                if (buffer[request->position] == 'C' && buffer[request->position + 11] == 'e')
                {
                    request->position += 14;

                    data.type = getValue(buffer, request->position);

                    request->position += 2; //jump \r\n

                    state = DATA;
                }
                else
                    state = FINISH;
                break;
            case DATA:
                while (true)
                {
                    line.push_back(buffer[request->position]);

                    if (buffer[request->position] == '\r')
                    {
                        line.push_back(buffer[request->position + 1]);

                        data.data.insert(data.data.end(), line.begin(), line.end());

                        line = "";

                        request->position += 2;

                        continue;
                    }

                    ++request->position;

                    if (line == boundary)
                    {
                        //REMOVE \r\n
                        data.data.pop_back();
                        data.data.pop_back();

                        break;
                    }
                }

                line = "";

                state = END;
                break;
            case END:
                line = getValue(buffer, request->position);

                request->MULTIPART[data.name] = data;
                data = {};

                if (line == "")
                {
                    state = DISPOSITION;
                }
                else
                    state = FINISH;
                break;
            case FINISH:
                work = false;
                break;
            }
        }
    }
}