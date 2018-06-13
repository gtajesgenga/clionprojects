#ifndef VTKPLUGIN_LIBRARY_H
#define VTKPLUGIN_LIBRARY_H

#include "OrthancCPlugin.h"
#include <jsoncpp/json/value.h>
#include <jsoncpp/json/reader.h>
#include <boost/noncopyable.hpp>
#include <string>

namespace OrthancPlugins {
    typedef void (*RestCallback)(OrthancPluginRestOutput *output,
                                 const char *url,
                                 const OrthancPluginHttpRequest *request);


    class PluginException {
    private:
        OrthancPluginErrorCode code_;

    public:
        PluginException(OrthancPluginErrorCode code) : code_(code) {
        }

        OrthancPluginErrorCode GetErrorCode() const {
            return code_;
        }

        const char *GetErrorDescription(OrthancPluginContext *context) const;
    };

    class MemoryBuffer : public boost::noncopyable
    {
    private:
        OrthancPluginContext*      context_;
        OrthancPluginMemoryBuffer  buffer_;

    public:
        MemoryBuffer(OrthancPluginContext* context) :
                context_(context)
        {
            buffer_.data = NULL;
            buffer_.size = 0;
        }

        ~MemoryBuffer()
        {
            Clear();
        }

        OrthancPluginMemoryBuffer* operator*()
        {
            return &buffer_;
        }

        // This transfers ownership
        void Assign(OrthancPluginMemoryBuffer& other)
        {
            Clear();

            buffer_.data = other.data;
            buffer_.size = other.size;

            other.data = NULL;
            other.size = 0;
        }

        const char* GetData() const
        {
            if (buffer_.size > 0)
            {
                return reinterpret_cast<const char*>(buffer_.data);
            }
            else
            {
                return NULL;
            }
        }

        size_t GetSize() const
        {
            return buffer_.size;
        }

        void Clear()
        {
            if (buffer_.data != NULL)
            {
                OrthancPluginFreeMemoryBuffer(context_, &buffer_);
                buffer_.data = NULL;
                buffer_.size = 0;
            }
        }

        void ToString(std::string& target) const
        {
            if (buffer_.size == 0)
            {
                target.clear();
            }
            else
            {
                target.assign(reinterpret_cast<const char*>(buffer_.data), buffer_.size);
            }
        }

        void ToJson(Json::Value& target) const
        {
            if (buffer_.data == NULL ||
                buffer_.size == 0)
            {
                throw PluginException(OrthancPluginErrorCode_InternalError);
            }

            const char* tmp = reinterpret_cast<const char*>(buffer_.data);

            Json::Reader reader;
            if (!reader.parse(tmp, tmp + buffer_.size, target))
            {
                OrthancPluginLogError(context_, "Cannot convert some memory buffer to JSON");
                throw PluginException(OrthancPluginErrorCode_BadFileFormat);
            }
        }


        bool RestApiGet(const std::string& uri,
                        bool applyPlugins)
        {
            Clear();

            OrthancPluginErrorCode error;

            if (applyPlugins)
            {
                error = OrthancPluginRestApiGetAfterPlugins(context_, &buffer_, uri.c_str());
            }
            else
            {
                error = OrthancPluginRestApiGet(context_, &buffer_, uri.c_str());
            }

            if (error == OrthancPluginErrorCode_Success)
            {
                return true;
            }
            else if (error == OrthancPluginErrorCode_UnknownResource ||
                     error == OrthancPluginErrorCode_InexistentItem)
            {
                return false;
            }
            else
            {
                throw PluginException(error);
            }
        }

        bool RestApiPost(const std::string& uri,
                         const char* body,
                         size_t bodySize,
                         bool applyPlugins)
        {
            Clear();

            OrthancPluginErrorCode error;

            if (applyPlugins)
            {
                error = OrthancPluginRestApiPostAfterPlugins(context_, &buffer_, uri.c_str(), body, bodySize);
            }
            else
            {
                error = OrthancPluginRestApiPost(context_, &buffer_, uri.c_str(), body, bodySize);
            }

            if (error == OrthancPluginErrorCode_Success)
            {
                return true;
            }
            else if (error == OrthancPluginErrorCode_UnknownResource ||
                     error == OrthancPluginErrorCode_InexistentItem)
            {
                return false;
            }
            else
            {
                throw PluginException(error);
            }
        }


        bool RestApiPut(const std::string& uri,
                        const char* body,
                        size_t bodySize,
                        bool applyPlugins)
        {
            Clear();

            OrthancPluginErrorCode error;

            if (applyPlugins)
            {
                error = OrthancPluginRestApiPutAfterPlugins(context_, &buffer_, uri.c_str(), body, bodySize);
            }
            else
            {
                error = OrthancPluginRestApiPut(context_, &buffer_, uri.c_str(), body, bodySize);
            }

            if (error == OrthancPluginErrorCode_Success)
            {
                return true;
            }
            else if (error == OrthancPluginErrorCode_UnknownResource ||
                     error == OrthancPluginErrorCode_InexistentItem)
            {
                return false;
            }
            else
            {
                throw PluginException(error);
            }
        }


        bool RestApiPost(const std::string& uri,
                         const std::string& body,
                         bool applyPlugins)
        {
            return RestApiPost(uri, body.empty() ? NULL : body.c_str(), body.size(), applyPlugins);
        }

        bool RestApiPut(const std::string& uri,
                        const std::string& body,
                        bool applyPlugins)
        {
            return RestApiPut(uri, body.empty() ? NULL : body.c_str(), body.size(), applyPlugins);
        }
    };


    namespace Internals
    {
        template <RestCallback Callback>
        OrthancPluginErrorCode Protect(OrthancPluginRestOutput* output,
                                       const char* url,
                                       const OrthancPluginHttpRequest* request)
        {
            try
            {
                Callback(output, url, request);
                return OrthancPluginErrorCode_Success;
            }
            catch (OrthancPlugins::PluginException& e)
            {
                return e.GetErrorCode();
            }
#if HAS_ORTHANC_EXCEPTION == 1
                catch (Orthanc::OrthancException& e)
      {
        return static_cast<OrthancPluginErrorCode>(e.GetErrorCode());
      }
#endif
            catch (...)
            {
                return OrthancPluginErrorCode_Plugin;
            }
        }
    }

    namespace Orthanc
    {
        // Specific Character Sets
        // http://dicom.nema.org/medical/dicom/current/output/html/part03.html#sect_C.12.1.1.2
        enum Encoding
        {
            Encoding_Ascii,
            Encoding_Utf8,
            Encoding_Latin1,
            Encoding_Latin2,
            Encoding_Latin3,
            Encoding_Latin4,
            Encoding_Latin5,                        // Turkish
            Encoding_Cyrillic,
            Encoding_Windows1251,                   // Windows-1251 (commonly used for Cyrillic)
            Encoding_Arabic,
            Encoding_Greek,
            Encoding_Hebrew,
            Encoding_Thai,                          // TIS 620-2533
            Encoding_Japanese,                      // JIS X 0201 (Shift JIS): Katakana
            Encoding_Chinese                        // GB18030 - Chinese simplified
            //Encoding_JapaneseKanji,               // Multibyte - JIS X 0208: Kanji
            //Encoding_JapaneseSupplementaryKanji,  // Multibyte - JIS X 0212: Supplementary Kanji set
            //Encoding_Korean,                      // Multibyte - KS X 1001: Hangul and Hanja
        };
    }

    template <RestCallback Callback>
    void RegisterRestCallback(OrthancPluginContext *context, const std::string &uri, bool isThreadSafe) {

        if (isThreadSafe)
        {
            OrthancPluginRegisterRestCallbackNoLock(context, uri.c_str(), Internals::Protect<Callback>);
        }
        else
        {
            OrthancPluginRegisterRestCallback(context, uri.c_str(), Internals::Protect<Callback>);
        }
    }

    bool RestApiGetJson(Json::Value& result,
                        OrthancPluginContext* context,
                        const std::string& uri,
                        bool applyPlugins)
    {
        MemoryBuffer answer(context);
        if (!answer.RestApiGet(uri, applyPlugins))
        {
            return false;
        }
        else
        {
            answer.ToJson(result);
            return true;
        }
    }
}

OrthancPluginContext*  context_;

extern "C"
{
    ORTHANC_PLUGINS_API int32_t OrthancPluginInitialize(OrthancPluginContext *context);

    ORTHANC_PLUGINS_API void OrthancPluginFinalize();

    ORTHANC_PLUGINS_API const char *OrthancPluginGetName();

    ORTHANC_PLUGINS_API const char *OrthancPluginGetVersion();
}

void GetVtk(OrthancPluginRestOutput *output, const char *url, const OrthancPluginHttpRequest *request);

#endif