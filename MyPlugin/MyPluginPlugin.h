#pragma once

#include "Resources/OrthancCPlugin.h"
#include "Resources/OrthancPluginCppWrapper.h"
#include "Resources/Configuration.h"

/**
 * This is the main callback to handle a WADO request.
 **/
ORTHANC_PLUGINS_API void MyPlugin(OrthancPluginRestOutput* output,
                                 const char* url,
                                 const OrthancPluginHttpRequest* request);