//
// Created by gus on 18/04/18.
//

#include "MyPluginPlugin.h"
#include "OrthancContext.h"
#include <fstream>
#include <iostream>

using namespace std;

// Look for some argument in the HTTP GET request
static bool GetArgument(std::string& value,
                        const OrthancContext::Arguments& arguments,
                        const std::string& key)
{
    OrthancContext::Arguments::const_iterator it = arguments.find(key);

    if (it == arguments.end())
    {
        return false;
    }

    value = it->second;
    return true;
}

ORTHANC_PLUGINS_API void MyPlugin(OrthancPluginRestOutput *output, const char *url, const OrthancPluginHttpRequest *request) {
    OrthancContext::GetInstance().LogInfo("Processing a MyPlugin request");

    // Extract the GET arguments of the WADO request
    OrthancContext::Arguments arguments;
    OrthancContext::GetInstance().ExtractGetArguments(arguments, *request);

    // Dispatch according to the requested content type
    std::string contentType = "application/octet-stream";   // By default, JPEG image will be returned
    GetArgument(contentType, arguments, "contentType");

    if (contentType == "application/octet-stream")
    {
        ifstream file ("test.vtk", std::ios::binary);
        if (file.is_open())
        {
            streampos size = file.tellg();
            char* memblock = new char[size];
            file.seekg(0, ios::beg);
            file.read(memblock, size);
            file.close();
            OrthancContext::GetInstance().AnswerBuffer(output, memblock, contentType);
        }
    }
    else
    {
        OrthancContext::GetInstance().LogError("Unsupported My Plugin content type: " + contentType);
    }

}