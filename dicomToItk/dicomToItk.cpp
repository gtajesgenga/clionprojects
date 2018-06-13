#include "dicomToItk.h"

#include <iostream>
#include <itkImage.h>
#include <itkImageSeriesReader.h>
#include <itkGDCMImageIO.h>
#include <itkGDCMSeriesFileNames.h>
#include <itkMeshFileWriter.h>
#include <itkBinaryMask3DMeshSource.h>
#include "itkMesh.h"


VtkGenerator::VtkGenerator(std::string directory, std::string outputfile)  : directory(std::move(directory)), outputFile(std::move(outputfile)) {}

VtkGenerator::~VtkGenerator() {
    directory.clear();
    outputFile.clear();
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

    std::string seriesIdentifier = seriesUID.begin()->c_str();

    using FileNamesContainer = std::vector< std::string >;
    FileNamesContainer fileNames;
    fileNames = nameGenerator->GetFileNames( seriesIdentifier );

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
    writer->SetFileName( directory + outputFile );
    writer->SetInput( filter->GetOutput() );

    try {
        writer->Update();
    } catch (itk::ExceptionObject &ex) {
        std::cout << ex << std::endl;
        return false;
    }

    return true;
}