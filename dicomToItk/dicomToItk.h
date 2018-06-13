#ifndef DICOMITKLIBRARY_LIBRARY_H
#define DICOMITKLIBRARY_LIBRARY_H

#include <string>

using byte = unsigned char;

class VtkGenerator {
private:
    std::string directory;
    std::string outputFile;

public:
    VtkGenerator(std::string directory, std::string outputfile);

    virtual ~VtkGenerator();

    bool generate();

};

#endif