#ifndef DICOMITKLIBRARY_LIBRARY_H
#define DICOMITKLIBRARY_LIBRARY_H

#include <string>

using byte = unsigned char;

class VtkGenerator {
private:
    const char* directory;
    const char* outputFile;

public:
    VtkGenerator(const char* directory, const char* outputfile);

    virtual ~VtkGenerator();

    bool generate();

};

#endif