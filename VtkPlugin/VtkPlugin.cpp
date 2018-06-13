#include "VtkPlugin.h"
#include <map>
#include <vector>
#include <cassert>
#include <algorithm>
#include <boost/regex.hpp>
#include <boost/filesystem.hpp>
#include <OrthancCPlugin.h>
#include <fstream>

using namespace boost::filesystem;


void ToLowerCase(std::string& s)
{
    std::transform(s.begin(), s.end(), s.begin(), tolower);
}

std::string StripSpaces(const std::string& source)
{
    size_t first = 0;

    while (first < source.length() &&
           isspace(source[first]))
    {
        first++;
    }

    if (first == source.length())
    {
        // String containing only spaces
        return "";
    }

    size_t last = source.length();
    while (last > first &&
           isspace(source[last - 1]))
    {
        last--;
    }

    assert(first <= last);
    return source.substr(first, last - first);
}

void TokenizeString(std::vector<std::string> &result,
                    const std::string &value,
                    char separator) {
    result.clear();

    std::string currentItem;

    for (char i : value) {
        if (i == separator) {
            result.push_back(currentItem);
            currentItem.clear();
        } else {
            currentItem.push_back(i);
        }
    }

result.push_back(currentItem);
}

void ParseContentType(std::string& application,
                      std::map<std::string, std::string>& attributes,
                      const std::string& header)
{
    application.clear();
    attributes.clear();

    std::vector<std::string> tokens;
    TokenizeString(tokens, header, ';');

    assert(!tokens.empty());
    application = tokens[0];
    StripSpaces(application);
    ToLowerCase(application);

    boost::regex pattern(R"(\s*([^=]+)\s*=\s*([^=]+)\s*)");

    for (size_t i = 1; i < tokens.size(); i++)
    {
        boost::cmatch what;
        if (boost::regex_match(tokens[i].c_str(), what, pattern))
        {
            std::string key(what[1]);
            std::string value(what[2]);
            ToLowerCase(key);
            attributes[key] = value;
        }
    }
}

void LogError(const std::string& message)
{
    OrthancPluginLogError(context_, message.c_str());
}

void LogInfo(const std::string& message)
{
    OrthancPluginLogInfo(context_, message.c_str());
}

extern "C"
{
    ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext *context) {

        context_ = context;

        OrthancPlugins::RegisterRestCallback<GetVtk>(context, "/vtk/studies/([^/]*)/series/([^/]*)", true);

        LogInfo("URI to VTK  API: /vtk/");

        return 0;
    }

    ORTHANC_PLUGINS_API void OrthancPluginFinalize() {
    }


    ORTHANC_PLUGINS_API const char *OrthancPluginGetName() {
        return "vtk";
    }


    ORTHANC_PLUGINS_API const char *OrthancPluginGetVersion() {
        return "1.0";
    }
}

static bool LocateSeries(OrthancPluginRestOutput* output,
                         std::string& uri,
                         const OrthancPluginHttpRequest* request) {

    if (request->method != OrthancPluginHttpMethod_Get) {
        OrthancPluginSendMethodNotAllowed(context_, output, "GET");
        return false;
    }

    std::string id;

    {
        char *tmp = OrthancPluginLookupSeries(context_, request->groups[1]);
        if (tmp == NULL) {
            LogError(
                    "Accessing an inexistent series with WADO-RS: " + std::string(request->groups[1]));
            OrthancPluginSendHttpStatusCode(context_, output, 404);
            return false;
        }

        id.assign(tmp);
        OrthancPluginFreeString(context_, tmp);
    }

    Json::Value study;
    if (!OrthancPlugins::RestApiGetJson(study, context_, "/series/" + id + "/study", false))
    {
        OrthancPluginSendHttpStatusCode(context_, output, 404);
        return false;
    }

    if (study["MainDicomTags"]["StudyInstanceUID"].asString() != std::string(request->groups[0]))
    {
        LogError("No series " + std::string(request->groups[1]) +
                                                " in study " + std::string(request->groups[0]));
        OrthancPluginSendHttpStatusCode(context_, output, 404);
        return false;
    }

    uri = "/series/" + id;
    return true;
}

static void AnswerListOfDicomInstances(OrthancPluginRestOutput* output,
                                       const std::string& resource)
{

    Json::Value instances;
    if (!OrthancPlugins::RestApiGetJson(instances, context_, resource + "/instances", false))
    {
        // Internal error
        OrthancPluginSendHttpStatusCode(context_, output, 400);
        return;
    }

    if (OrthancPluginStartMultipartAnswer(context_, output, "related", "application/dicom"))
    {
        throw OrthancPlugins::PluginException(OrthancPluginErrorCode_NetworkProtocol);
    }

    for (Json::Value::ArrayIndex i = 0; i < instances.size(); i++)
    {
        std::string uri = "/instances/" + instances[i]["ID"].asString() + "/file";

        LogInfo("Call to: " + uri);

        OrthancPlugins::MemoryBuffer dicom(context_);
        if (dicom.RestApiGet(uri, false) &&
            OrthancPluginSendMultipartItem(context_, output, dicom.GetData(), dicom.GetSize()) != 0)
        {
            throw OrthancPlugins::PluginException(OrthancPluginErrorCode_InternalError);
        }
    }
}

void GetVtk(OrthancPluginRestOutput *output, const char *url, const OrthancPluginHttpRequest *request) {

    LogInfo("Processing a VTK request");
    std::string application;
    std::string accept;
    std::map<std::string, std::string> attributes;
    ParseContentType(application, attributes, accept);
    // Dispatch according to the requested content type
    std::string returnContentType = "application/octet-stream";   // By default, JPEG image will be returned
    if (!accept.empty())
    {
        returnContentType = accept;
    }

    std::string uri;
    if (LocateSeries(output, uri, request))
    {
        //AnswerListOfDicomInstances(output, uri);
        Json::Value seriesResponse;

        if (!OrthancPlugins::RestApiGetJson(seriesResponse, context_, uri, false))
        {
            OrthancPluginSendHttpStatusCode(context_, output, 404);
            return;
        }

        if (seriesResponse["Instances"].empty())
        {
            OrthancPluginSendHttpStatusCode(context_, output, 204);
            return;
        }

        path ph ( std::tmpnam(nullptr) );
        if (!exists(ph)) {
            create_directories(ph);
            LogInfo("Temp directory '" + ph.string() + "' created");
        }
        LogInfo("Using temp directory: '" + ph.string() + "' to store dicom files");

        Json::Value instances = seriesResponse["Instances"];
        for (int i = 0; i < instances.size(); ++i) {
            OrthancPluginMemoryBuffer response;
            OrthancPluginRestApiGet(context_, &response, std::string("/instances/" + instances[i].asString() + "/file").c_str());
            std::string outName (ph.string() + "/" + instances[i].asString() + ".dicom");
            std::ofstream outFile(outName, std::ofstream::binary);
            outFile.write(reinterpret_cast<const char *>(response.data), response.size);
            LogInfo(outName + " writed");
            outFile.close();
            OrthancPluginFreeMemoryBuffer(context_, &response);
        }

        return;
    }
    else
    {

        LogError("File not found: ");
        throw OrthancPlugins::PluginException(OrthancPluginErrorCode_UnknownResource);
    }


//    std::string application;
//    std::string accept;
//    std::map<std::string, std::string> attributes;
//    ParseContentType(application, attributes, accept);
//
//    // Dispatch according to the requested content type
//    std::string returnContentType = "application/octet-stream";   // By default, JPEG image will be returned
//
//    if (!accept.empty())
//    {
//        returnContentType = accept;
//    }
//
//    if (returnContentType == "application/octet-stream")
//    {
//        //ifstream file ("test.vtk", std::ios::binary);
//        FILE * pFile;
//        long size;
//        char * buffer;
//        size_t result;
//
//        if ((pFile = fopen("test.vtk", "rb")) != nullptr)
//        {
//            fseek (pFile, 0, SEEK_END);   // non-portable
//            size = ftell (pFile);
//            printf ("Size of myfile.txt: %ld bytes.\n",size);
//            rewind (pFile);
//            buffer = (char*) malloc (sizeof(char)*size);
//
//            if (buffer == nullptr) { fputs ("Memory error",stderr); exit (2); }
//
//            result = fread (buffer, 1, static_cast<size_t>(size), pFile);
//
//            if (result != size) {fputs ("Reading error",stderr); exit (3);}
//
//            OrthancPluginAnswerBuffer(context_, output, buffer, static_cast<uint32_t>(size), returnContentType.c_str());
//            fclose (pFile);
//            free (buffer);
//
//        }
//        else
//        {
//
//            LogError("File not found: ");
//            throw OrthancPlugins::PluginException(OrthancPluginErrorCode_UnknownResource);
//        }
//    }
//    else
//    {
//        LogError("Unsupported VTK content type: " + returnContentType);
//        throw OrthancPlugins::PluginException(OrthancPluginErrorCode_BadRequest);
//    }
}
