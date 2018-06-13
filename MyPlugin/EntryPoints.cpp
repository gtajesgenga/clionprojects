//
// Created by gus on 18/04/18.
//

#include <assert.h>
#include "OrthancContext.h"
#include "MyPluginPlugin.h"

extern "C"
{
    ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext *context) {
        /* Check the version of the Orthanc core */
        if (OrthancPluginCheckVersion(context) == 0)
        {
            char info[1024];
            sprintf(info, "Your version of Orthanc (%s) must be above %d.%d.%d to run this plugin",
                    context->orthancVersion,
                    ORTHANC_PLUGINS_MINIMAL_MAJOR_NUMBER,
                    ORTHANC_PLUGINS_MINIMAL_MINOR_NUMBER,
                    ORTHANC_PLUGINS_MINIMAL_REVISION_NUMBER);
            OrthancPluginLogError(context, info);
            return -1;
        }

        try
        {
            // Read the configuration
            OrthancPlugins::Configuration::Initialize(context);

            // Configure the DICOMweb callbacks
            if (OrthancPlugins::Configuration::GetBooleanValue("Enable", true))
            {
                std::string root = OrthancPlugins::Configuration::GetRoot();
                assert(!root.empty() && root[root.size() - 1] == '/');

                OrthancPlugins::Configuration::LogWarning("URI to the My Plugin REST API: " + root);

                OrthancPlugins::RegisterRestCallback<MyPlugin>(context, root + "myplugin", true);
            }
            else
            {
                OrthancPlugins::Configuration::LogWarning("My Plugin support is disabled");
            }
        }
        catch (OrthancPlugins::PluginException& e)
        {
            OrthancPlugins::Configuration::LogError("Exception while initializing the My Plugin plugin: " +
                                                    std::string(e.GetErrorDescription(context)));
            return -1;
        }
        catch (...)
        {
            OrthancPlugins::Configuration::LogError("Exception while initializing the My Plugin plugin");
            return -1;
        }

        return 0;

}

    ORTHANC_PLUGINS_API void OrthancPluginFinalize() {
        OrthancContext::GetInstance().LogWarning("Finalizing My Plugin sample");
        OrthancContext::GetInstance().Finalize();
    }

    ORTHANC_PLUGINS_API const char *OrthancPluginGetName() {
        return "myplugin-sample";
    }

    ORTHANC_PLUGINS_API const char *OrthancPluginGetVersion() {
        return "1.0";
    }
}
