#include "dicomToItk.h"

#include <iostream>
#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkMeshFileWriter.h>
#include <itkBinaryMask3DMeshSource.h>
#include "itkMesh.h"


VtkGenerator::VtkGenerator(const char* directory, const char* outputfile)  : directory(std::move(directory)), outputFile(std::move(outputfile)) {}

VtkGenerator::~VtkGenerator() {
    directory = nullptr;
    outputFile = nullptr;
}

bool VtkGenerator::generate() {
    using PixelType = unsigned short;
    constexpr unsigned int Dimension = 3;
    using ImageType = itk::Image< PixelType, Dimension >;

    using ReaderType = itk::ImageSeriesReader< ImageType >;
    ReaderType::Pointer reader = ReaderType::New();

    using ImageIOType = itk::GDCMImageIO;
    ImageIOType::Pointer dicomIO = ImageIOType::New();
    reader->SetImageIO( dicomIO );

    using NamesGeneratorType = itk::GDCMSeriesFileNames;
    NamesGeneratorType::Pointer nameGenerator = NamesGeneratorType::New();
    nameGenerator->SetUseSeriesDetails( true );
    nameGenerator->AddSeriesRestriction("0008|0021" );
    nameGenerator->SetDirectory( directory );

    using SeriesIdContainer = std::vector< std::string >;
    const SeriesIdContainer & seriesUID = nameGenerator->GetSeriesUIDs();
    auto seriesItr = seriesUID.begin();
    auto seriesEnd = seriesUID.end();
    std::cout << std::endl << "The directory: " << std::endl;
    std::cout << std::endl << directory << std::endl << std::endl;
    std::cout << "Contains the following DICOM Series: ";
    std::cout << std::endl << std::endl;
    while( seriesItr != seriesEnd )
    {
        std::cout << seriesItr->c_str() << std::endl;
        ++seriesItr;
    }

    std::string seriesIdentifier = seriesUID.begin()->c_str();

    std::cout << std::endl << std::endl;
    std::cout << "Now reading series: " << std::endl << std::endl;
    std::cout << seriesIdentifier << std::endl;
    std::cout << std::endl << std::endl;

    using FileNamesContainer = std::vector< std::string >;
    FileNamesContainer fileNames;
    fileNames = nameGenerator->GetFileNames( seriesIdentifier );

    std::cout << std::endl << std::endl;
    std::cout << "List of filenames: " << std::endl << std::endl;

    for (std::string file : fileNames) {
        std::cout << file << std::endl;
        std::cout << std::endl;
    }

    reader->SetFileNames( fileNames );

    try {
        reader->Update();
    } catch (itk::ExceptionObject &ex) {
        std::cout << ex << std::endl;
        return false;
    }

    using MeshType = itk::Mesh< double, Dimension >;
    using WriterType = itk::MeshFileWriter< MeshType >;

    using FilterType = itk::BinaryMask3DMeshSource< ImageType, MeshType >;
    FilterType::Pointer filter = FilterType::New();
    filter->SetInput( reader->GetOutput() );
    filter->SetObjectValue( 255 );

    WriterType::Pointer writer = WriterType::New();
    char * res = new char();
    strcpy(res, directory);
    strcat(res, outputFile);

    std::cout << "Using output filename:" << std::endl;
    std::cout << res << std::endl;

    writer->SetFileName( res );
    writer->SetInput( filter->GetOutput() );

    try {
        writer->Update();
    } catch (itk::ExceptionObject &ex) {
        std::cout << ex << std::endl;
        return false;
    }

    return true;
}