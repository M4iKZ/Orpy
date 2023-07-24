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

    State parse(State current_state, std::unique_ptr<HTTPData>& data)
    {
        switch (current_state)
        {
        case READ_METHOD:
            if (data->buffer[data->position] == 'G' && data->buffer[data->position + 1] == 'E' && data->buffer[data->position + 2] == 'T')
            {
                //GET
                data->request.method = "GET";

                data->position += 4;

                return READ_URI;
            }
            else if (data->buffer[data->position] == 'P' && data->buffer[data->position + 1] == 'O' && data->buffer[data->position + 2] == 'S' && data->buffer[data->position + 3] == 'T')
            {
                data->request.method = "POST";

                data->request.isPOST = true;

                data->position += 5;

                return READ_URI;
            }

            data->response.status = 501; //not implemented

            return ERROR;
            break;
        case READ_URI:
            if (data->buffer[data->position] == '?')
            {
                data->request.hasGET = true;

                ++data->position;

                // ?v=hKXVqnEIBh0&list=RDQM58p3c0RNCrI&index=9
                std::string key = "";
                while (data->buffer[data->position] != 32)
                {
                    if (data->buffer[data->position] == '=')
                    {
                        ++data->position; //jump =

                        while (data->buffer[data->position] != '&' && data->buffer[data->position] != 32)
                        {
                            data->request.GET[key].push_back(data->buffer[data->position]);

                            ++data->position;
                        }

                        urlDecode(data->request.GET[key]);

                        if (data->buffer[data->position] == 32)
                            break;

                        key = "";
                    }
                    else
                        key.push_back(data->buffer[data->position]);

                    ++data->position;
                }

                return READ_PROTOCOL;
            }
            else if (data->buffer[data->position] == '%')
            {
                if (data->position + 2 < data->buffer.size())
                {
                    if (std::isdigit(data->buffer[data->position + 1]) && std::isdigit(data->buffer[data->position + 2]))
                    {
                        std::string hex;
                        hex.push_back(data->buffer[data->position + 1]);
                        hex.push_back(data->buffer[data->position + 2]);

                        data->request.URI.push_back(data->buffer[data->position]);
                        data->request.URI.push_back(data->buffer[data->position + 1]);
                        data->request.URI.push_back(data->buffer[data->position + 2]);

                        int value = std::stoi(hex, nullptr, 16);
                        std::string character;
                        character.push_back(static_cast<char>(value));
                        data->request.URL += character;
                        data->request.commands[data->request.nCommands - 1] += character;

                        data->position += 3;
                    }
                    else
                    {
                        data->request.URI.push_back(data->buffer[data->position]);
                        data->request.URL.push_back(data->buffer[data->position]);
                        data->request.commands[data->request.nCommands - 1].push_back(data->buffer[data->position]);

                        ++data->position;
                    }
                }
            }
            else
            {
                if (data->buffer[data->position] == '/')
                {
                    data->request.nCommands++;
                    data->request.commands.push_back("");
                }
                else
                {
                    data->request.commands[data->request.nCommands - 1].push_back(data->buffer[data->position]);
                }

                data->request.URI.push_back(data->buffer[data->position]);
                data->request.URL.push_back(data->buffer[data->position]);

                ++data->position;
            }

            if (data->buffer[data->position] == ' ') 
                return READ_PROTOCOL;
            else
                return READ_URI;

            break;
        case READ_PROTOCOL:
            if (data->buffer[data->position] == '\r' && data->buffer[data->position + 1] == '\n')
            {
                data->position += 2;

                return READ_HEADERS;
            }
            else
            {
                ++data->position;

                return READ_PROTOCOL;
            }

            break;
        case READ_HEADERS:
            if ((data->buffer[data->position] == 'H' || data->buffer[data->position] == 'h') && data->buffer[data->position + 1] == 'o' && data->buffer[data->position + 3] == 't')
            {
                //Host
                data->position += 6;

                data->request.Host = getValue(data->buffer, data->position, true);

                if (!_confs->Get(data->request.Host, data->Conf))
                {
                    data->response.status = 444;

                    return ERROR;
                }

                if (data->Conf.redirect != "")
                {
                    data->response.status = 303;
                    data->response.location = data->Conf.redirect + data->request.URI;

                    return ERROR;
                }

                return READ_HEADERS;
            }
            else if ((data->buffer[data->position] == 'A' || data->buffer[data->position] == 'a') && data->buffer[data->position + 13] == 'n' && data->buffer[data->position + 14] == 'g')
            {
                //Accept-Encoding: gzip, deflate, br
                data->position += 16;

                while (data->buffer[data->position] != '\r' && data->buffer[data->position + 1] != '\n')
                {
                    if (data->buffer[data->position] == 'b' && data->buffer[data->position + 1] == 'r')
                        data->request.br = true;

                    ++data->position;
                }

                data->position += 2; //Remove \r\n

                return READ_HEADERS;
            }
            else if (data->request.isPOST && (data->buffer[data->position] == 'C' || data->buffer[data->position] == 'c') && data->buffer[data->position + 7] == '-' && data->buffer[data->position + 8] == 'T' && data->buffer[data->position + 9] == 'y' && data->buffer[data->position + 10] == 'p' && data->buffer[data->position + 11] == 'e')
            {
                // Content-Type: application/x-www-form-urlencoded
                data->position += 14;

                /*
                0 : application/x-www-form-urlencoded
                1 : multipart/form-data
                2 : application/json
                3 : application/xml
                4 : application/pdf
                5 : application/octet-stream
                */

                // Content-Type: multipart/form-data; boundary=[boundary string]

                if (data->buffer[data->position] == 'm' && data->buffer[data->position + 18] == 'a')
                {
                    data->position += 30;

                    data->request.isMultipart = true;
                    data->request.boundary = "--";
                    data->request.boundary += getValue(data->buffer, data->position);
                }
                else
                {
                    data->request.contentType = getValue(data->buffer, data->position);

                    if (data->request.contentType.find("application/x-www-form-urlencoded") != 0)
                    {
                        data->request.isPOST = false;
                        data->response.status = 501; //not implemented                        

                        return ERROR;
                    }
                }

                return READ_HEADERS;
            }
            else if (data->request.isPOST && (data->buffer[data->position] == 'C' || data->buffer[data->position] == 'c') && data->buffer[data->position + 12] == 't' && data->buffer[data->position + 13] == 'h')
            {
                //Content-Length: 37    
                data->position += 16;

                data->request.contentLength = std::stoi(getValue(data->buffer, data->position));

                return READ_HEADERS;
            }
            else if ((data->buffer[data->position] == 'C' || data->buffer[data->position] == 'c') && data->buffer[data->position + 4] == 'i' && data->buffer[data->position + 5] == 'e')
            {
                //Cookie: session_id=abc123; user_id=1;
                data->position += 8;

                std::string value;
                std::string key;
                while (1)
                {
                    if (data->buffer[data->position] == '=')
                    {
                        data->request.cookies[value] = "";
                        key = value;
                        value = "";
                    }
                    else if (data->buffer[data->position] == ';' || data->buffer[data->position] == '\r')
                    {
                        data->request.cookies[key] = value;
                        value = "";

                        if (data->buffer[data->position] == '\r')
                            break;

                        data->position++; //jump the space?
                    }
                    else
                    {
                        value.push_back(data->buffer[data->position]);
                    }

                    data->position++;
                }

                return READ_HEADERS;
            }
            else if ((data->buffer[data->position] == 'O' || data->buffer[data->position] == 'o') && data->buffer[data->position + 1] == 'r' && data->buffer[data->position + 2] == 'i' && data->buffer[data->position + 3] == 'g' && data->buffer[data->position + 4] == 'i' && data->buffer[data->position + 6] == ':')
            {
                //Origin: http://localhost:8888
                data->position += 8;

                std::string url = getValue(data->buffer, data->position);
                std::string http = "http://";
                std::string https = "https://";

                if (url.find(http) == 0)
                    url.erase(0, http.length());
                else if (url.find(https) == 0)
                    url.erase(0, https.length());

                if (url.find(":") != std::string::npos)
                    url.erase(url.find(":"), 1);

                data->request.Origin = url;

                return READ_HEADERS;
            }
            else if ((data->buffer[data->position] == 'R' || data->buffer[data->position] == 'r') && data->buffer[data->position + 6] == 'r')
            {
                //Referer: http://localhost:8888/
                data->position += 9;

                data->request.Referer = getValue(data->buffer, data->position);

                return READ_HEADERS;
            }
            else if ((data->buffer[data->position] == 'I' || data->buffer[data->position] == 'i') && data->buffer[data->position + 2] == '-' && data->buffer[data->position + 11] == '-')
            {
                //If-Modified-Since
                data->position += 19;

                data->request.fileModifiedSince = getValue(data->buffer, data->position);

                return READ_HEADERS;
            }
            else if (data->buffer[data->position] == 'I' && data->buffer[data->position + 1] == 'P')
            {
                //IP
                data->position += 4;

                data->request.clientIP = getValue(data->buffer, data->position);

                return READ_HEADERS;
            }
            else if ((data->buffer[data->position] == 'U' || data->buffer[data->position] == 'u') && data->buffer[data->position + 4] == '-')
            {
                //User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/113.0.0.0 Safari/537.36
                data->position += 12;

                data->request.useragent = getValue(data->buffer, data->position);

                std::string useragent = data->request.useragent;
                std::transform(useragent.begin(), useragent.end(), useragent.begin(), ::tolower); // find() is case sensitive, check lower string instead

                if (useragent.find("bot") != std::string::npos || useragent.find("crawler") != std::string::npos || useragent.find("spider") != std::string::npos)
                    data->request.isBot = true;

                return READ_HEADERS;
            }
            else if (data->buffer[data->position] == '\r' && data->buffer[data->position + 1] == '\n' && data->buffer[data->position + 2] == '\r' && data->buffer[data->position + 3] == '\n')
            {
                data->position += 4;

                return DONE;
            }
            else
            {
                ++data->position;

                return READ_HEADERS;
            }

            break;
        case DONE:
            return DONE;

            break;
        }

        return ERROR;
    }

    void parser(std::unique_ptr<HTTPData>& data)
    {
        State current_state = READ_METHOD;
        while (data->position < data->buffer.size())
        {
            current_state = parse(current_state, data);

            if (current_state == DONE)
                break;
            else if (current_state == ERROR)
            {
                data->error = true;
                break;
            }
        }
    }

    bool getType(std::unique_ptr<HTTPData>& data)
    {
        //CUSTOM SITE MIME TYPES
        auto conf = data->Conf.mimetypes.find(data->response.content_type);
        if (conf != data->Conf.mimetypes.end())
        {
            data->response.content_type = conf->second;
            return true;
        }

        //ORPY DEFAULT MIME TYPES
        auto it = ContentTypeMapping.find(data->response.content_type);
        if (it == ContentTypeMapping.end())
            return false;

        data->response.content_type = it->second;

        return true;
    }

    void getPOST(std::unique_ptr<HTTPData>& data)
    {
        //formType=Form-Url-Encoded&name=&email=        
        if (data->position < data->buffer.size())
        {
            std::string key = "";

            while (data->buffer[data->position] != '\r' && data->buffer[data->position + 1] != '\n' && data->position + 1 < data->buffer.size())
            {
                if (data->buffer[data->position] == '=')
                {
                    ++data->position; //jump =

                    while (data->buffer[data->position] != '&' && data->buffer[data->position] != '\r' && data->position < data->buffer.size())
                    {
                        data->request.POST[key].push_back(data->buffer[data->position]);

                        ++data->position;
                    }

                    urlDecode(data->request.POST[key]);

                    key = "";
                }
                else
                    key.push_back(data->buffer[data->position]);

                ++data->position;
            }
        }
    }

    void elaborateMultipart(std::unique_ptr<HTTPData>& data)
    {
        bool work = true;
        MULTIPARTState state = HEADER;
        MP_DATA mpdata;
        std::string boundary = "";
        std::string line = "";
        int position = data->position;
        while (work)
        {
            switch (state)
            {
            case HEADER:
                boundary = getValue(data->buffer, data->position);

                if (boundary == data->request.boundary)
                {
                    state = DISPOSITION;
                }
                else
                    state = FINISH;
                break;
            case DISPOSITION:
                if (data->buffer[data->position] == 'C' && data->buffer[data->position + 18] == 'n')
                {
                    data->position += 21;

                    while (data->buffer[data->position] != ';')
                    {
                        mpdata.Disposition.push_back(data->buffer[data->position]);

                        ++data->position;
                    }

                    data->position += 2; //jump the space

                    state = NAME;
                }
                else
                    state = FINISH;
                break;
            case NAME:
                if (data->buffer[data->position] == 'n' && data->buffer[data->position + 3] == 'e')
                {
                    data->position += 6;

                    while (data->buffer[data->position] != '"')
                    {
                        mpdata.name.push_back(data->buffer[data->position]);

                        ++data->position;
                    }

                    data->position += 3; //jump the space

                    if (data->buffer[data->position] == '\r')
                    {
                        state = DATA;

                        data->position += 2; //jump \r\n
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
                if (data->buffer[data->position] == 'f' && data->buffer[data->position + 7] == 'e')
                {
                    data->position += 10;

                    while (data->buffer[data->position] != '"')
                    {
                        mpdata.filename.push_back(data->buffer[data->position]);

                        ++data->position;
                    }

                    if (mpdata.filename != "")
                        mpdata.isFile = true;

                    data->position += 3;

                    state = TYPE;
                }
                else
                    state = FINISH;
                break;
            case TYPE:
                if (data->buffer[data->position] == 'C' && data->buffer[data->position + 11] == 'e')
                {
                    data->position += 14;

                    mpdata.type = getValue(data->buffer, data->position);

                    data->position += 2; //jump \r\n

                    state = DATA;
                }
                else
                    state = FINISH;
                break;
            case DATA:
                while (true)
                {
                    line.push_back(data->buffer[data->position]);

                    if (data->buffer[data->position] == '\r')
                    {
                        line.push_back(data->buffer[data->position + 1]);

                        mpdata.data.insert(mpdata.data.end(), line.begin(), line.end());

                        line = "";

                        data->position += 2;

                        continue;
                    }

                    ++data->position;

                    if (line == boundary)
                    {
                        //REMOVE \r\n
                        mpdata.data.pop_back();
                        mpdata.data.pop_back();

                        break;
                    }
                }

                line = "";

                state = END;
                break;
            case END:
                line = getValue(data->buffer, data->position);

                data->request.MULTIPART[mpdata.name] = mpdata;
                mpdata = {};

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