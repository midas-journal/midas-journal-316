#include "itkImage.h"
#include "itkImageFileReader.h"

//To extract axial slices of the 3D image
#include "itkExtractImageFilter.h"

//To extract contours from axial slices
#include "itkContourExtractor2DImageFilter.h"

#include <cmath> // for using fabs()
#include <fstream>
#include <iostream>
#include <iomanip> //format manipulation
#include <stdio.h> // for deleting a temporary text file
#include <vector>

using std::cerr;
using std::endl;
using std::ios;
using std::ifstream;
using std::ofstream;
using std::string;


// Definitions of variables used by the program.
// -------------------------------------------------------------
const unsigned int ImageDimension = 3;
const unsigned int SliceDimension = 2;

// XXX It is assumed that the pixel-type is unsigned char.
typedef unsigned char PixelType;

// This defines the types of the contours:
const string OPEN_PLANAR( "OPEN_PLANAR" );
const string CLOSED_PLANAR( "CLOSED_PLANAR" );

// Any deviation below the FLOAT_EPSILON value is neglected.
const float FLOAT_EPSILON = 0.0001;

// The following is the precision used while writing
// the coordinates of the contours to a text file.
const unsigned int precision = 16;

//Set the contour value to be extracted. 
const double contourValue = 100.0;

// Global variable to count the total number of contours
unsigned int TOTAL_NUMBER_OF_CONTOURS = 0;

// -------------------------------------------------------------

// -------------------------------------------------------------
typedef itk::Image<PixelType, ImageDimension>  InputImageType;
typedef itk::Image<PixelType, SliceDimension>  ImageSliceType;

typedef itk::ExtractImageFilter<InputImageType, ImageSliceType> SliceExtractorType;
typedef itk::ContourExtractor2DImageFilter<ImageSliceType>      ContourExtractorType;

typedef ContourExtractorType::VertexType VertexType;

typedef itk::ImageFileReader<InputImageType> ImageReaderType;
// -------------------------------------------------------------

// Forward declaration of the functions.
// -------------------------------------------------------------
void WriteContourVertices(ofstream&                     file1,
                          ContourExtractorType::Pointer contourExtractFilter,
                          const unsigned int            currentSlice,
                          const int                     offset_index[],
                          const double                  spacing[]);

void WriteCommonData(const unsigned int sliceNumber,
                     const unsigned int numContourPoints,
                     const string       geometricType,
                     ofstream&          file1);

void WriteVertexCoordinates(const        VertexType vertex, 
                            const int    offset_index[],
                            const double spacing[],
                            const double zValue,
                            ofstream&    file1);

void WriteLastVertexCoordinates(const        VertexType vertex, 
                                const int    offset_index[],
                                const double spacing[],
                                const double zValue,
                                ofstream&    file1);
void AppendTextFile(const char* file1, const char* outputFile);
// -------------------------------------------------------------

int main(int argc, char *argv[])
{
  if( argc < 6 )
  {
    cerr << "Missing Parameters... " << endl;
    cerr << "Usage: " << argv[0];
    cerr << " <input-image>  <output-file>";
    cerr << " <x-offset-index>  <y-offset-index> <z-offset-index>" << endl;
    return EXIT_FAILURE;
  }

  const char* inputFileName   = argv[1];
  const char* outputFileName  = argv[2];
  const int   offset_index[3] = { atoi(argv[3]), atoi(argv[4]), atoi(argv[5]) };

  const char* tempFileName   = "temp_junk_file.txt";

  // Make sure that the <output-file> can be opened. 
  ofstream file1(tempFileName, ios::trunc);
  if ( ! file1.is_open() )
  {
    cerr << "Unable to open the text file:  "
         << outputFileName << endl;
    return EXIT_FAILURE;
  }

  ImageReaderType::Pointer reader = ImageReaderType::New();
  reader->SetFileName(inputFileName);

  try 
  { 
    reader->Update();
  } 
  catch( itk::ExceptionObject & err ) 
  { 
    cerr << "ExceptionObject caught !" << endl; 
    cerr << err << endl; 
    return EXIT_FAILURE;
  } 

  const InputImageType::SpacingType& spacing = reader->GetOutput()->GetSpacing();

  //"space" will be latter used as multiplication factor to coordinates of vertices.
  //It is found that the vertices returned by the contour extractor are in terms of index.
  //Hence those values are multiplied with spacing while writing to output file.
  const double space[] = {spacing[0], spacing[1], spacing[2]};

  ContourExtractorType::Pointer contourExtractFilter = ContourExtractorType::New();
  contourExtractFilter->SetContourValue(contourValue);
  contourExtractFilter->ReverseContourOrientationOn();

  InputImageType::RegionType inputRegion = 
                        reader->GetOutput()->GetLargestPossibleRegion();
  InputImageType::SizeType size = inputRegion.GetSize();
  const unsigned int numberOfSlices = size[2];
  size[2] = 0; //only one axial slice at a time is considered;

  InputImageType::RegionType sliceRegion;
  sliceRegion.SetSize(size);
  InputImageType::IndexType start = inputRegion.GetIndex();

  SliceExtractorType::Pointer sliceExtractFilter = SliceExtractorType::New();

  for ( unsigned int currentSlice = 0;
        currentSlice < numberOfSlices;
        currentSlice++ )
  {
    start[0] = 0;
    start[1] = 0;
    start[2] = currentSlice;

    sliceRegion.SetIndex(start);

    sliceExtractFilter->SetExtractionRegion(sliceRegion);
    sliceExtractFilter->SetInput(reader->GetOutput());
    contourExtractFilter->SetInput(sliceExtractFilter->GetOutput());

    try 
    { 
      contourExtractFilter->Update();
    } 
    catch( itk::ExceptionObject & err ) 
    { 
      cerr << "ExceptionObject caught!" << endl; 
      cerr << err << endl; 
      return EXIT_FAILURE;
    }
    WriteContourVertices(file1, contourExtractFilter, 
                         currentSlice, offset_index, space);
  } 
  file1.close();

  AppendTextFile(tempFileName, outputFileName);
  return EXIT_SUCCESS;
}
// -------------------------------------------------------------


void WriteContourVertices(ofstream&                     file1,
                          ContourExtractorType::Pointer contourExtractFilter,
                          const unsigned int            currentSlice,
                          const int                     offset_index[],
                          const double                  spacing[])
{
  unsigned int numOutputs = contourExtractFilter->GetNumberOfOutputs();

  // update the "TOTAL_NUMBER_OF_CONTOURS" value
  TOTAL_NUMBER_OF_CONTOURS += numOutputs;

  unsigned int numVertices;
  VertexType   firstVertex;
  VertexType   lastVertex;

  // Compute the z-value for this planar contour.
  const double zValue = ( currentSlice - offset_index[2] ) * (spacing[2]);


  for (unsigned int i = 0; i < numOutputs; i++)
  {
    ContourExtractorType::VertexListConstPointer vertices =
                          contourExtractFilter->GetOutput(i)->GetVertexList();

    numVertices = vertices->Size();

    firstVertex = vertices->ElementAt(0);
    lastVertex  = vertices->ElementAt(numVertices-1);

    if ( (fabs(firstVertex[0] - lastVertex[0]) < FLOAT_EPSILON ) &&
         (fabs(firstVertex[1] - lastVertex[1]) < FLOAT_EPSILON ) )
    {
      // It's a closed contour.
      // So, the last vertex won't be written as it is same as the 1st vertex.
      // Further, the last but one vertex is seperately handled to avoid the
      // "\" symbol at the end.
      //
      WriteCommonData(currentSlice, numVertices-1, CLOSED_PLANAR, file1);

      for ( unsigned int j = 0; j < ( numVertices-2 ); j++ )
      {
              WriteVertexCoordinates( vertices->ElementAt(j), offset_index,
                                      spacing, zValue, file1 );
      }

      // In order to avoid "\" symbol at the end of the vertices string,
      // the last verex is specially handled here.....
      WriteLastVertexCoordinates( vertices->ElementAt( numVertices-2 ),
                                 offset_index, spacing, zValue, file1 );
      file1 << endl << endl;
    } else
    {
      // "The contour is a open planar one.
      WriteCommonData(currentSlice, numVertices, OPEN_PLANAR, file1);

      for ( unsigned int j = 0; j < ( numVertices-1 ); j++ )
      {
              WriteVertexCoordinates( vertices->ElementAt(j), offset_index,
                                      spacing, zValue, file1 );
      }

      // In order to avoid "\" symbol at the end of the vertices string,
      // the last verex is specially handled here.....
      WriteLastVertexCoordinates( vertices->ElementAt( numVertices-1 ),
                                  offset_index, spacing, zValue, file1 );
      file1 << endl << endl;
    }
  }
}


void WriteCommonData(const unsigned int sliceNumber,
                     const unsigned int numContourPoints,
                     const string       geometricType,
                     ofstream&          file1)
{
  file1 << "[Slice Number]" << endl;
  file1 << sliceNumber << endl << endl;

  file1 << "[Geometric Type]" << endl;
  file1 << geometricType << endl << endl;

  file1 << "[Number of Contour Points]" << endl;
  file1 << numContourPoints << endl << endl;

  file1 << "[Contour Data]" << endl;
}


void WriteVertexCoordinates(const        VertexType vertex, 
                            const int    offset_index[],
                            const double spacing[],
                            const double zValue,
                            ofstream&    file1)
{
  file1 << std::setprecision(precision)
        << ( (vertex[0]-offset_index[0]) * spacing[0] ) << "\\"
        << ( (vertex[1]-offset_index[1]) * spacing[1] ) << "\\"
        << zValue << "\\";
}


void WriteLastVertexCoordinates(const        VertexType vertex, 
                                const int    offset_index[],
                                const double spacing[],
                                const double zValue,
                                ofstream&    file1)
{
  file1 << std::setprecision(precision)
        << ( (vertex[0]-offset_index[0]) * spacing[0] ) << "\\"
        << ( (vertex[1]-offset_index[1]) * spacing[1] ) << "\\"
        << zValue;
}


void AppendTextFile(const char* fileName1, const char* outputFileName)
{
  ifstream file1(fileName1);
  ofstream outputFile(outputFileName, ios::trunc);

  if ( ! file1.is_open() )
    {
      cerr << "Unable to open the text file:  "
           << fileName1 << endl;
      exit(1);
    }

  if ( ! outputFile.is_open() )
    {
      cerr << "Unable to open the text file:  "
           << outputFileName << endl;
      exit(1);
    }

  outputFile << "[Total Number of Contours]" << endl;
  outputFile << TOTAL_NUMBER_OF_CONTOURS << endl << endl;

  char ch;
  while (file1.get(ch)) // copies all characters (including newline chars)
    outputFile.put(ch);
  
  file1.close();
  outputFile.close();

  // Finally remove the 1st file.
  if ( remove(fileName1) != 0 )
  {
    cerr <<"Unable to remove the temporary file" <<endl;
    exit(1);
  }
  
}
