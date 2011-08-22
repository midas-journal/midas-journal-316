#include <iostream>
#include <new>
#include <sstream>
#include <string>
#include <vector>

#include "itkGDCMImageIO.h" //for reading the input DICOM-CT-image-slice
#include "itkRTSTRUCTIO.h" //for writing the output RTSTRUCT

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkMetaDataObject.h"

using std::cerr;
using std::endl;
using std::ifstream;
using std::istream;
using std::string;
using std::stringstream;
using std::vector;

typedef itk::MetaDataDictionary       DictionaryType;
typedef itk::MetaDataObject< string > MetaDataStringType;
typedef itk::MetaDataObject< DictionaryType > MetaDataDictionaryObject;

// Following two values are fixed and are defined in the DICOM standards.
const string StudyComponentManagementSOPClass = "1.2.840.10008.3.1.2.3.2";
const string CTImageSOPClassUID = "1.2.840.10008.5.1.4.1.1.2";

const string roiGenAlgorithm = "ITK-GDCM based algorithm";

//-----------------------------------------------------------------------------
// Forward declaration of functions:
void readInputParameters(const char* parameterFileName);

bool IsTagPresent(DictionaryType::ConstIterator& tagItr,
                  DictionaryType::ConstIterator& end);

void ExitIfTagNotPresent(const string&                  entryId,
                         DictionaryType::ConstIterator& tagItr,
                         DictionaryType::ConstIterator& end);

string GetTagValue(DictionaryType::ConstIterator& tagItr);

void WriteCompulsoryTags(const vector<string>&  compulsoryModule,
                         const DictionaryType&  dictReader,
                         DictionaryType&        dictWriter);

void WriteOptionalTags(const vector<string>&   optionalModule,
                       const DictionaryType&   dictReader,
                       DictionaryType&         dictWriter);

string itos(int i); // convert int to string
string FixNumDigitsTo3( string str); 

void read_contour_data_file(char* );
void SkipWhiteSpace(istream& );


//-----------------------------------------------------------------------------
// Following two values are used for allocating the memory for
// temporarily storing the contours information later defined object.
//
// XXX Modify the following two values if they do not fit to your requirement!
//
#define MAX_NUM_CONTOURS 1000
#define MAX_NUM_ROIs 10

//precision with which the vertices are saved in the input text file.
#define PRECISION 16

#define MAX_CHAR 1024


typedef struct ContourObject_struct
{
  unsigned int totalContours;
  unsigned int sliceNumber[MAX_NUM_CONTOURS];
  unsigned int numOfPoints[MAX_NUM_CONTOURS];

  char* geometryType[MAX_NUM_CONTOURS];
  char* contourData[MAX_NUM_CONTOURS];

} *ContourObject_handle;

ContourObject_handle CONTOUR;

//---------------------------------------------------
// Definition of a structure for storing the parameters from
// the input <parameter-file>

typedef struct InputParameters_struct
{
  char* inputDCMFileName;
  char* outputFileName;
  char* prefixSOPInstUID;
  
  char* contourDataFileName[MAX_NUM_ROIs];
  char* roiName[MAX_NUM_ROIs];
  char* roiInterpretedType[MAX_NUM_ROIs];
  char* roiColor[MAX_NUM_ROIs];

  unsigned int NumSlicesInRTSTRUCT;
  unsigned int START_SLICE_NUM;
  unsigned int numOfROIs;
} *InputParameters_handle;

InputParameters_handle parameters;

//---------------------------------------------------
typedef signed short PixelType; //for Input CT slice
const unsigned int   Dimension = 2; //for Input CT slice

typedef itk::Image< PixelType, Dimension > ImageType;
typedef itk::ImageFileReader < ImageType > ReaderType;
typedef itk::ImageFileWriter < ImageType > WriterType;

typedef itk::GDCMImageIO ImageInType;
typedef itk::RTSTRUCTIO  STRUCTOutType;
//---------------------------------------------------


int main(int argc, char* argv[])
{
  // Verify the number of parameters in the command line.
  if( argc < 2 )
  {
    cerr << "Usage: " << endl;
    cerr << argv[0] << " <parameter-file>" << endl;
    return EXIT_FAILURE;
  }

  // Read the input parameters.
  parameters = new InputParameters_struct;
  readInputParameters(argv[1]);

  ImageInType::Pointer gdcmIO1 = ImageInType::New(); // for DICOM reader
  const DictionaryType& dictReader = gdcmIO1->GetMetaDataDictionary();

  // the tags and keys of interest are read from the input DICOM file
  // and are written to the output RTSTRUCT DICOM file.
  ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName(parameters->inputDCMFileName);
  reader->SetImageIO(gdcmIO1);

  try
  {
    reader->Update();
  }
  catch (itk::ExceptionObject &ex)
  {
    cerr << "Exception caught while reading the i/p DICOM file: " << endl;
    cerr << ex << endl;
  }

  //       |------------------------------|
  //       | RT STRUCTURE SET IOD MODULES |
  //       |------------------------------|
  //
  // Only the mandatory(M) IOD modules of RTSTRUCT are created by this program.
  // Those manadatory modules are as follows:
  //
  // |--------------------|----------------------|
  // | Information-Entity |  Mandatory Modules   |
  // |--------------------|----------------------|
  // |                    |                      |
  // | Patient            | Patient              |
  // |                    |                      |
  // |--------------------|----------------------|
  // |                    |                      |
  // | Study              | General Study        |
  // |                    |                      |
  // |--------------------|----------------------|
  // |                    |                      |
  // | Series             | RT Series            |
  // |                    |                      |
  // |--------------------|----------------------|
  // |                    |                      |
  // | Equipment          | General Equipment    |
  // |                    |                      |
  // |--------------------|----------------------|
  // |                    |                      |
  // | Structure Set      | Structure Set        |
  // |                    | ROI Contour          |
  // |                    | RT ROI Observations  |
  // |                    | SOP Common           |
  // |                    |                      |
  // |--------------------|----------------------|
  //
  // Each individual module is described in detail below....
  //
  
  
  STRUCTOutType::Pointer gdcmIO2 = STRUCTOutType::New(); // for DICOM writer
  DictionaryType& dictWriter = gdcmIO2->GetMetaDataDictionary();

  vector<string> mandatoryModule;
  vector<string> optioalModule;

  DictionaryType::ConstIterator  tagItr;
  DictionaryType::ConstIterator  end = dictReader.End();
  string tagValue;

  //                  |--------------------|
  //                  | Patient Module (M) |
  //                  |--------------------|
  //
  // Only the following tags of this module are written!
  //
  // |-------|----------------------|-------------|------|
  // | S.No. | Attribute Name       | Tag         | Type |
  // |-------|----------------------|-------------|------|
  // | 1     | Patient's Name       | (0010,0010) | 2    |
  // | 2     | Patient ID           | (0010,0020) | 2    |
  // | 3     | Patient's Birth Date | (0010,0030) | 2    |
  // | 4     | Patient's Sex        | (0010,0040) | 2    |
  // |-------|----------------------|-------------|------|
  //
  // This information is extracted from the input CT-Slice.

  mandatoryModule.push_back("0010|0010");
  mandatoryModule.push_back("0010|0020");
  mandatoryModule.push_back("0010|0030");
  mandatoryModule.push_back("0010|0040");


  //                  |--------------------------|
  //                  | General Study Module (M) |
  //                  |--------------------------|
  //
  // Only the following tags of this module are written!
  // 
  //
  // |-------|--------------------------------|-------------|------|
  // | S.No. | Attribute Name                 | Tag         | Type |
  // |-------|--------------------------------|-------------|------|
  // | 1     | Study Inst. UID                | (0020|000d) | 1    | 
  // | 2     | Study Date                     | (0008|0020) | 2    |
  // | 3     | Study Time                     | (0008|0030) | 2    |
  // | 4     | Ref. Phys. Name                | (0008|0090) | 2    |
  // | 5     | Study ID                       | (0020|0010) | 2    |
  // | 6     | Accession Number               | (0008|0050) | 2    |
  // |-------|--------------------------------|-------------|------|
  // | 7     | Study Description              | (0008|1030) | 3    |
  // | 8     | Physician(s) of Record         | (0008|1048) | 3    |
  // |-------|--------------------------------|-------------|------|
  // | 9     | Referenced Study Sequence      | (0008|1110) | 3    |
  // | 9.1   |  > Referenced SOP Class UID    | (0008|1150) | 1C   |
  // | 9.2   |  > Referenced SOP Instance UID | (0008|1155) | 1C   |
  // |-------|--------------------------------|-------------|------|
  //
  // This information is extracted from the input CT-Slice.

  mandatoryModule.push_back("0020|000d");
  mandatoryModule.push_back("0008|0020");
  mandatoryModule.push_back("0008|0030");
  mandatoryModule.push_back("0008|0090");
  mandatoryModule.push_back("0020|0010");
  mandatoryModule.push_back("0008|0050");

  optioalModule.push_back("0008|1030");
  optioalModule.push_back("0008|1048");

  STRUCTOutType::Pointer refStudySeq = STRUCTOutType::New();
  DictionaryType & refStudyDict = refStudySeq->GetMetaDataDictionary();

  // StudyComponentManagementSOPClass = 1.2.840.10008.3.1.2.3.2
  itk::EncapsulateMetaData<string>(refStudyDict, "0008|1150", StudyComponentManagementSOPClass);

  // Reference SOP instance UID:
  // This value is same as that of (0020|000d)
  // The same value will also be used latter in Structure-Set module.

  string sopInstUID;
  tagItr = dictReader.Find( "0020|000d" );

  if (IsTagPresent( tagItr, end ) )
  {
    sopInstUID = GetTagValue( tagItr );
  } else
  {
    cerr << "Unable to find the (0020,000d) tag in the input CT image!" << endl;
  }

  itk::EncapsulateMetaData<string>(refStudyDict, "0008|1155", sopInstUID);

  itk::EncapsulateMetaData<DictionaryType>(dictWriter, "0008|1110", refStudyDict);


  //                  |----------------------|
  //                  | RT Series Module (M) |
  //                  |----------------------|
  //
  // Only the following tags of this module are written!
  //
  // |-------|--------------------|-------------|------|
  // | S.No. | Attribute Name     | Tag         | Type |
  // |-------|--------------------|-------------|------|
  // | 1     | Modality           | (0008|0060) | 1    | 
  // | 2     | Series Inst. UID   | (0020|000e) | 1    |
  // | 3     | Series Number      | (0020|0011) | 2    |
  // |-------|--------------------|-------------|------|
  // | 4     | Series Description | (0008|103e) | 3    |
  // |-------|------------------  |-------------|------|
  //

  // Modality: "RTSTRUCT"
  itk::EncapsulateMetaData<string>(dictWriter, "0008|0060", "RTSTRUCT");

  // Series Inst. UID: The default value assigned by GDCM is used!!!

  // Series Number: A number that identifies this series
  // No value is set for this tag at this moment
  itk::EncapsulateMetaData<string>(dictWriter, "0020|0011", "");

  itk::EncapsulateMetaData<string>(dictWriter, "0008|103e", "ITK/GDCM ROI");



  //             |------------------------------|
  //             | General Equipment Module (M) |
  //             |------------------------------|
  //
  // Only the following tags of this module are written!
  //
  // |-------|------------------|-------------|------|
  // | S.No. | Attribute Name   | Tag         | Type |
  // |-------|------------------|-------------|------|
  // | 1     | Manufacturer     | (0008|0070) | 2    | 
  // |-------|------------------|-------------|------|
  // | 2     | Station Name     | (0008|1010) | 3    |
  // |-------|------------------|-------------|------|


  // The default manufacturer name for this output is: "GDCM Factory"
  // and the same name is retained.
  //

  // (0x0008,0x1010) SH Station Name
  itk::EncapsulateMetaData<string>(dictWriter, "0008|1010", "ITK/GDCM PC");

  //                 |------------------------|
  //                 |  SOP Common Module (M) |
  //                 |------------------------|
  //
  // Only the following tags of this module are written!
  //
  // |-------|--------------------------|-------------|------|
  // | S.No. | Attribute Name           | Tag         | Type |
  // |-------|--------------------------|-------------|------|
  // | 1     | SOP Class UID            | (0008|0016) | --   |
  // | 2     | SOP Instance UID         | (0008|0018) | --   |
  // | 3     | Instance Creation Date   | (0008|0012) | 3    |
  // | 4     | Instance Creation Time   | (0008|0013) | 3    |
  // | 5     | Timezone Offset From UTC | (0008|0201) | 3    |
  // |-------|--------------------------|-------------|------|
  //

  // SOP Class UID is written in gdcmFileHelper.cxx
  // SOP Instance UID is written in gdcmFileHelper.cxx
  // Instance Creation Date is Set in gdcmFileHelper.cxx
  // Instance Creation Time is Set in gdcmFileHelper.cxx
  //

  optioalModule.push_back("0008|0201");

  // (0x0002,0x0016) AE Source Application Entity Title
  optioalModule.push_back("0002|0016");

  WriteCompulsoryTags(mandatoryModule, dictReader, dictWriter);
  WriteOptionalTags(optioalModule, dictReader, dictWriter);


  //                 |--------------------------|
  //                 | Structure Set Module (M) |
  //                 |--------------------------|
  //
  // Only the following tags of this module are written!
  //
  // A structure set defines a set of areas of significance.
  // Each area can be associated with a Frame of Reference and zero or more
  // images. Information which can be transferred with each region of
  // interest (ROI) includes geometrical and display parameters, 
  // and generation technique.
  //
  // We will write only those attributes listed in the following table.
  //
  //
  // |-----------|--------------------------------------|-------------|------|
  // | S.No.     |           Attribute Name             |    Tag      | Type |
  // |-----------|--------------------------------------|-------------|------|
  // | 1         | Structure Set Label                  | (3006|0002) | 1    |
  // |-----------|--------------------------------------|-------------|------|
  // | 2         | Structure Set Name                   | (3006|0004) | 3    |
  // |-----------|--------------------------------------|-------------|------|
  // | 3         | Structure Set Date                   | (3006|0008) | 2    |
  // |-----------|--------------------------------------|-------------|------|
  // | 4         | Structure Set Time                   | (3006|0009) | 2    |
  // |-----------|--------------------------------------|-------------|------|
  // | 5         | Referenced frame of Ref. Sequence    | (3006|0010) | 3    |
  // | 5.1       |  > Frame of Ref. UID                 | (0020|0052) | 1C   |
  // |           |                                      |             |      |
  // | 5.2       |  > RT Referenced Study Sequence      | (3006|0012) | 3    |
  // | 5.2.1     |    >> Referenced SOP Class UID       | (0008|1150) | 1C   |
  // | 5.2.2     |    >> Referenced SOP Instance UID    | (0008|1155) | 1C   |
  // |           |                                      |             |      |
  // | 5.2.3     |    >> RT Referenced Series Sequence  | (3006|0014) | 1C   |
  // | 5.2.3.1   |       >>> Series Instance UID        | (0020|000E) | 1C   |
  // |           |                                      |             |      |
  // | 5.2.3.2   |       >>> Contour Image Sequence     | (3006|0016) | 1C   |
  // | 5.2.3.2.1 |           >>>> Ref. SOP Class UID    | (0008|1150) | 1C   |
  // | 5.2.3.2.2 |           >>>> Ref. SOP Instance UID | (0008|1155) | 1C   |
  // |-----------|--------------------------------------|-------------|------|
  // | 6         | Structure Set ROI Sequence           | (3006|0020) | 3    |
  // | 6.1       |  > ROI Number                        | (3006|0022) | 1C   |
  // | 6.2       |  > Referenced Frame of ref. UID      | (3006|0024) | 1C   |
  // | 6.3       |  > ROI Name                          | (3006|0026) | 2C   |
  // | 6.4       |  > ROI Generation Algorithm          | (3006|0036) | 2C   |
  // |-----------|--------------------------------------|-------------|------|
  //
  //

  // 1
  // Structure Set Label = Study Description name (0008,1030)
  // If study description name does not exist, set it to "test-structure"
  tagItr = dictReader.Find( "0008|1030" );
  tagValue = "test-structure";
  if (IsTagPresent( tagItr, end ) )
    tagValue = GetTagValue( tagItr );
  itk::EncapsulateMetaData<string>(dictWriter, "3006|0002", tagValue);

  // 2
  // Structure Set Name = "ROI"
  itk::EncapsulateMetaData<string>(dictWriter, "3006|0004", "ROI");

  // 3
  // Structure Set Time = Initially set it's value to an empty string;
  // set it's value correctly, latter in "gdcmFileHelper.cxx"
  itk::EncapsulateMetaData<string>(dictWriter, "3006|0008", "");

  // 4
  // Structure Set Date = Initially set it's value to an empty string;
  // set it's value correctly, latter in "gdcmFileHelper.cxx"
  itk::EncapsulateMetaData<string>(dictWriter, "3006|0009", "");

  // Create all the required Dictionaries.....
  // ------------------------------------------------------
  // Dictionary for including "Referenced frame of Reference"
  STRUCTOutType::Pointer gdcmRefFrameSeq = STRUCTOutType::New();
  DictionaryType refFrameDict = gdcmRefFrameSeq->GetMetaDataDictionary();

  // Dictionary for including "RT Referenced Study Sequence"
  STRUCTOutType::Pointer gdcmRtRefStudySeq = STRUCTOutType::New();
  DictionaryType rtRefStudyDict = gdcmRtRefStudySeq->GetMetaDataDictionary();

  // Dictionary for including "RT Referenced Series Sequence"
  STRUCTOutType::Pointer gdcmRtRefSerSeq = STRUCTOutType::New();
  DictionaryType rtRefSerDict = gdcmRtRefSerSeq->GetMetaDataDictionary();

  // Dictionary for including "Contour Image Sequence"
  STRUCTOutType::Pointer gdcmContourImageSeq = STRUCTOutType::New();
  DictionaryType contourImageDict = gdcmContourImageSeq->GetMetaDataDictionary();
  // ------------------------------------------------------

  // The Frame of Reference UID (0020,0052) shall be used to uniquely identify
  // a frame of reference for a series. Each series shall have 
  // a single Frame of Reference UID. However, multiple Series
  // within a Study may share a Frame of Reference UID.
  // All images in a Series that share the same Frame of Reference UID
  // shall be spatially related to each other.
  // 
  // We assume here that there is a single frame of reference and,
  // we will extract that value from the CT-DICOM image.
  //
  string frameOfReferenceUID;
  tagItr = dictReader.Find( "0020|0052" );

  if (IsTagPresent( tagItr, end ) )
  {
    frameOfReferenceUID = GetTagValue( tagItr );
  } else
  {
    cerr << "Unable to find the Frame of Reference UID in the CT image!"
         << endl;
  }
  // 5.1
  itk::EncapsulateMetaData<string>(refFrameDict, "0020|0052", frameOfReferenceUID);

  // -----------------------------------------------------------------------
  // Bottom-up approach is followed when there are recursive sequences....
  // -----------------------------------------------------------------------
  
  // 5.2.1
  // Referenced SOP class UID = StudyComponentManagementSOPClass
  itk::EncapsulateMetaData<string>(rtRefStudyDict, "0008|1150", StudyComponentManagementSOPClass);

  // 5.2.2
  // Referenced SOP Instance UID: Already available = sopInstUID;
  itk::EncapsulateMetaData<string>(rtRefStudyDict, "0008|1155", sopInstUID);

  // Series Instance UID: Unique Identifier of the series containing the image
  //It's value = (0020,000e) tag value of the CT image
  string refSeriesInstUID;
  tagItr = dictReader.Find( "0020|000e" );

  if (IsTagPresent( tagItr, end ) )
  {
    refSeriesInstUID = GetTagValue( tagItr );
  } else
  {
    cerr << "Unable to find (0020|000e) tag in the CT image!" << endl;
  }
  // 5.2.3.1
  itk::EncapsulateMetaData<string>(rtRefSerDict, "0020|000e", refSeriesInstUID);


  string tempSOPInstUID;
  string tempString;
  unsigned int startNum = parameters->START_SLICE_NUM;

  for ( int count = 1; count <= parameters->NumSlicesInRTSTRUCT; count++ )
  {
    tempString = ITEM_ENCAPSULATE_STRING + FixNumDigitsTo3( itos( count ) );
    tempSOPInstUID = (parameters->prefixSOPInstUID) + itos( startNum );

    STRUCTOutType::Pointer gdcmIOItem = STRUCTOutType::New();
    DictionaryType &itemDict = gdcmIOItem->GetMetaDataDictionary();

    // 5.2.3.2.1
    // Referenced SOP Class UID:
    // Uniquely identifies the referenced image SOP instance
    // It's value is constant = CTImageSOPClassUID
    itk::EncapsulateMetaData<string>(itemDict, "0008|1150", CTImageSOPClassUID);

    // 5.2.3.2.2
    // Referenced SOP Instance UID
    itk::EncapsulateMetaData<string>(itemDict, "0008|1155", tempSOPInstUID);

    // item encapsulation
    itk::EncapsulateMetaData<DictionaryType>(contourImageDict, tempString, itemDict);

    ++startNum;
  }

  // 5.2.3.2
  itk::EncapsulateMetaData<DictionaryType>(rtRefSerDict, "3006|0016", contourImageDict);

  // 5.2.3
  itk::EncapsulateMetaData<DictionaryType>(rtRefStudyDict, "3006|0014", rtRefSerDict);

  // 5.2
  itk::EncapsulateMetaData<DictionaryType>(refFrameDict, "3006|0012", rtRefStudyDict);

  // 5
  itk::EncapsulateMetaData<DictionaryType>(dictWriter, "3006|0010", refFrameDict);
  //-------------------------------------------------------------------------------


  STRUCTOutType::Pointer gdcmIOStructureSetRoi = STRUCTOutType::New();
  DictionaryType &structureSetRoiSeq = gdcmIOStructureSetRoi->GetMetaDataDictionary();

  // 6.2
  // Referenced Frame of ref. UID = frameOfReferenceUID
  //
  // Uniquely identifies Frame of Reference in which ROI is defined, specified by Frame
  // of Reference UID (0020,0052) in Referenced Frame of Reference Sequence
  // (3006,0010). Required if Structure Set ROI Sequence (3006,0020) is sent.
   
  for (unsigned int roiNumber = 1; roiNumber <= parameters->numOfROIs; roiNumber++)
  {
    tempString = ITEM_ENCAPSULATE_STRING + FixNumDigitsTo3( itos( roiNumber ) );

    STRUCTOutType::Pointer gdcmIOItem = STRUCTOutType::New();
    DictionaryType &itemDict = gdcmIOItem->GetMetaDataDictionary();

    itk::EncapsulateMetaData<string>(itemDict, "3006|0022", itos(roiNumber));
    itk::EncapsulateMetaData<string>(itemDict, "3006|0024", frameOfReferenceUID);
    itk::EncapsulateMetaData<string>(itemDict, "3006|0026", parameters->roiName[roiNumber-1]);
    itk::EncapsulateMetaData<string>(itemDict, "3006|0036", roiGenAlgorithm);

    itk::EncapsulateMetaData<DictionaryType>(structureSetRoiSeq, tempString, itemDict);
  }

  // 6
  itk::EncapsulateMetaData<DictionaryType>(dictWriter, "3006|0020", structureSetRoiSeq);



  //                       |------------------------|
  //                       | ROI Contour Module (M) |
  //                       |------------------------|
  //
  // Only the following tags of this module are written!
  //
  // This module is used to define the ROI as a set of contours.
  // Each ROI contains a sequence of one or more contours, where a contour
  // is either a single point (for a point ROI) or more than one point
  // (representing an open or closed polygon).
  // 
  //
  // We will write only those attributes listed in the following table.
  //
  //
  // |---------|---------------------------------------|-------------|------|
  // | S.No.   |           Attribute Name              |    Tag      | Type |
  // |---------|---------------------------------------|-------------|------|
  // | 1       | ROI Contour Sequence                  | (3006|0039) | 1    |
  // | 1.1     |  > Referenced ROI Number              | (3006|0084) | 1    |
  // | 1.2     |  > ROI Display Color                  | (3006|002a) | 3    |
  // |         |                                       |             |      |
  // | 1.3     |  > Contour Sequence                   | (3006|0040) | 3    |
  // |         |                                       |             |      |
  // | 1.3.1   |    >> Contour Image Sequence          | (3006|0016) | 3    |
  // | 1.3.1.1 |       >>> Referenced SOP Class UID    | (0008|1150) | 1C   |
  // | 1.3.1.2 |       >>> Referenced SOP Instance UID | (0008|1155) | 1C   |
  // |         |                                       |             |      |
  // | 1.3.2   |    >> Contour Geometric Type          | (3006|0042) | 1C   |
  // | 1.3.3   |    >> Number of Contour Points        | (3006|0046) | 1C   |
  // | 1.3.4   |    >> Contour Data                    | (3006|0050) | 1C   |
  // |---------|---------------------------------------|-------------|------|

  STRUCTOutType::Pointer gdcmIORoiContour = STRUCTOutType::New();
  DictionaryType &roiContourSeq = gdcmIORoiContour->GetMetaDataDictionary();

  for (unsigned int contourItem = 1; contourItem <= parameters->numOfROIs; contourItem++)
  {
    // Create a temporary string for encapsulating the item:
    string tempStringHigh = ITEM_ENCAPSULATE_STRING + FixNumDigitsTo3( itos( contourItem ) );

    // Create an item-dictionary for each contour
    STRUCTOutType::Pointer gdcmIOItem = STRUCTOutType::New();
    DictionaryType &itemDict = gdcmIOItem->GetMetaDataDictionary();

    // Create an item-dictionary for each contour

    //1.1 Referenced ROI Number
    //ROIs are successively numberd starting from 1
    itk::EncapsulateMetaData<string>(itemDict, "3006|0084", itos(contourItem));

    //1.2 ROI Display Color
    itk::EncapsulateMetaData<string>(itemDict, "3006|002a", parameters->roiColor[contourItem-1]);

    //1.3 Contour Sequence Declaration
    STRUCTOutType::Pointer gdcmIOContour = STRUCTOutType::New();
    DictionaryType &contourSeq = gdcmIOContour->GetMetaDataDictionary();

    CONTOUR = new ContourObject_struct;
    read_contour_data_file(parameters->contourDataFileName[contourItem-1]); 

    unsigned int sliceNumber;

    for (unsigned int count = 1; count <= CONTOUR->totalContours; count++)
    {
      // Create an item-dictionary for each contour
      STRUCTOutType::Pointer gdcmIOSubItem = STRUCTOutType::New();
      DictionaryType &subItemDict = gdcmIOSubItem->GetMetaDataDictionary();

      // Creating a name for encapsulating the item....
      tempString = ITEM_ENCAPSULATE_STRING + FixNumDigitsTo3( itos( count ) );

      // 1.3.1 Declaration
      // Dictionary for including "Contour Image Sequence"
      STRUCTOutType::Pointer gdcmContourImageSeq2 = STRUCTOutType::New();
      DictionaryType contourImageDict2 = gdcmContourImageSeq2->GetMetaDataDictionary();

      // 1.3.1.1
      // Referenced SOP Class UID:
      // Uniquely identifies the referenced image SOP instance
      // It's value is constant = CTImageSOPClassUID
      itk::EncapsulateMetaData<string>(contourImageDict2, "0008|1150", CTImageSOPClassUID); 

      // 1.3.1.2
      // Finding the Referenced SOP Instance UID, from the slice Number
      sliceNumber = CONTOUR->sliceNumber[count-1];
      tempSOPInstUID = (parameters->prefixSOPInstUID) + itos( parameters->START_SLICE_NUM + sliceNumber );
      itk::EncapsulateMetaData<string>(contourImageDict2, "0008|1155", tempSOPInstUID);

      // 1.3.1 Encapsulation
      // Add Contour Image Sequence to Item Dictionary
      itk::EncapsulateMetaData<DictionaryType>(subItemDict, "3006|0016", contourImageDict2);

      //1.3.2
      // Add Contour Geometric Type
      itk::EncapsulateMetaData<string>(subItemDict, "3006|0042", CONTOUR->geometryType[count-1]);

      //1.3.3
      //Add Number of Contour Points
      itk::EncapsulateMetaData<string>(subItemDict, "3006|0046",  itos( CONTOUR->numOfPoints[count-1] ) );


      //1.3.4
      //Add Contour Data
      itk::EncapsulateMetaData<string>(subItemDict, "3006|0050", CONTOUR->contourData[count-1]);

      // Add this sub-item to Contour Sequence
      itk::EncapsulateMetaData<DictionaryType>(contourSeq, tempString, subItemDict);
    }
  
    itk::EncapsulateMetaData<DictionaryType>(itemDict, "3006|0040", contourSeq);
    itk::EncapsulateMetaData<DictionaryType>(roiContourSeq, tempStringHigh, itemDict);
  }
  
  // Add ROI Contour Sequence to Final Dictonary
  itk::EncapsulateMetaData<DictionaryType>(dictWriter, "3006|0039", roiContourSeq);

  // Clean up
  delete CONTOUR;



  //                 |--------------------------------|
  //                 | RT ROI Observations Module (M) |
  //                 |--------------------------------|
  //
  // Only the following tags of this module are written!
  //
  // The RT ROI Observations module specifies the identification and
  // interpretation of an ROI specified in the Structure Set and ROI 
  // Contour modules.
  //
  //
  // We will write only those attributes listed in the following table.
  //

  // |-------|------------------------------|-------------|------|
  // | S.No. |           Attribute Name     |    Tag      | Type |
  // |-------|------------------------------|-------------|------|
  // | 1     | RT ROI Observations Sequence | (3006|0080) | 1    |
  // | 1.1   |  > Observation Number        | (3006|0082) | 1    |
  // | 1.2   |  > Referenced ROI Number     | (3006|0084) | 1    |
  // | 1.3   |  > RT ROI Interpreted Type   | (3006|00a4) | 2    |
  // | 1.4   |  > ROI Interpreter           | (3006|00a6) | 2    |
  // |-------|------------------------------|-------------|------|


  STRUCTOutType::Pointer gdcmIORoiObservations = STRUCTOutType::New();
  DictionaryType &roiObservationsSeq = gdcmIORoiObservations->GetMetaDataDictionary();

  for (unsigned int observeItem = 1; observeItem <= parameters->numOfROIs; observeItem++)
  {
    // Create a temporary string for encapsulating the item:
    tempString = ITEM_ENCAPSULATE_STRING + FixNumDigitsTo3( itos( observeItem ) );

    // Create an item-dictionary for each observation
    STRUCTOutType::Pointer gdcmIOItem = STRUCTOutType::New();
    DictionaryType &itemDict = gdcmIOItem->GetMetaDataDictionary();

    // Referenced roi number itself is used as the observation number

    //1.1 Observation Number
    itk::EncapsulateMetaData<string>(itemDict, "3006|0082", itos(observeItem));

    //1.2 Referenced ROI Number
    itk::EncapsulateMetaData<string>(itemDict, "3006|0084", itos(observeItem));

    //1.3 RT ROI Interpreted Type
    itk::EncapsulateMetaData<string>(itemDict, "3006|00a4", parameters->roiInterpretedType[observeItem-1]);

    //1.4 XXX Currently an empty string is used for ROI Interpreter
    itk::EncapsulateMetaData<string>(itemDict, "3006|00a6", "");

    //Add the it
    itk::EncapsulateMetaData<DictionaryType>(roiObservationsSeq, tempString, itemDict);
  }

  // Add ROI Observations Sequence to the Final Dictonary
  itk::EncapsulateMetaData<DictionaryType>(dictWriter, "3006|0080", roiObservationsSeq);
  //-------------------------------------------------------------------------------

  WriterType::Pointer writer = WriterType::New();

  // We now need to not only explicitly set the proper image IO (GDCMImageIO), but also
  // we must tell the ImageFileWriter NOT to use the MetaDataDictionary from the
  // input but from the GDCMImageIO since this is the one that contains the DICOM
  // specific information
  //
  writer->UseInputMetaDataDictionaryOff ();

  writer->SetFileName(parameters->outputFileName);

  writer->SetInput(reader->GetOutput());

  writer->SetImageIO(gdcmIO2);

  try
  {
      writer->Update();
  }
  catch (itk::ExceptionObject & excp)
  {
      cerr << "Exception is thrown while writing the DICOM file: " << endl;
      cerr << excp << endl;
  }


  // Clean up
  delete parameters;

  return EXIT_SUCCESS;   
}


void ExitIfTagNotPresent(const string&                  entryId,
                         DictionaryType::ConstIterator& tagItr,
                         DictionaryType::ConstIterator& end)
{
  if (IsTagPresent(tagItr, end) == false)
  {
    cerr << "Tag: " << entryId;
    cerr << " not found in the DICOM header" << endl;
    EXIT_FAILURE;
  }
}


bool IsTagPresent(DictionaryType::ConstIterator& tagItr,
                  DictionaryType::ConstIterator& end)
{
    if( tagItr == end)
      return false;
    
    return true;
}


string GetTagValue(DictionaryType::ConstIterator& tagItr)
{
  MetaDataStringType::ConstPointer entryValue;
  entryValue =
           dynamic_cast<const MetaDataStringType *>
           ( tagItr->second.GetPointer() );

  if( !entryValue )
  {
    cerr << "Entry was not of string type" << endl;
    EXIT_FAILURE;
  }
  return entryValue->GetMetaDataObjectValue();
}


void WriteCompulsoryTags(const vector<string>& mandatoryModule,
                        const DictionaryType& dictReader,
                        DictionaryType&       dictWriter)
{
  string tagID;
  string tagValue;

  vector<string>::const_iterator stringItr;
  DictionaryType::ConstIterator  tagItr;
  DictionaryType::ConstIterator  end = dictReader.End();

  for (stringItr = mandatoryModule.begin(); stringItr != mandatoryModule.end(); stringItr++)
  {
    tagID  = *stringItr;

    tagItr = dictReader.Find( tagID );
    ExitIfTagNotPresent( tagID, tagItr, end );

    tagValue = GetTagValue( tagItr );

    itk::EncapsulateMetaData<string>(dictWriter, tagID, tagValue);
  }
}


void WriteOptionalTags(const vector<string>& optionalModule,
                       const DictionaryType& dictReader,
                       DictionaryType&       dictWriter)
{
  string tagID;
  string tagValue;

  vector<string>::const_iterator stringItr;
  DictionaryType::ConstIterator  tagItr;
  DictionaryType::ConstIterator  end = dictReader.End();

  for (stringItr = optionalModule.begin(); stringItr != optionalModule.end(); stringItr++)
  {
    tagID  = *stringItr;

    tagItr = dictReader.Find( tagID );

    if (IsTagPresent( tagItr, end ) )
    {
      tagValue = GetTagValue( tagItr );
      itk::EncapsulateMetaData<string>(dictWriter, tagID, tagValue);
    }
  }
}


string itos(int i)	// convert int to string
{
  stringstream s;
  s << i;
  return s.str();
}


// Add zeros before the stringto make # of digits = 3
// It assumes that number of digits <= 3
string FixNumDigitsTo3( string str) 
{
  unsigned length = str.length();
  
  assert( length != 0 );
  assert( length <= 3 );

  if (length == 1)
  {
   str = "00" + str;
  } else if (length == 2)
  {
    str = "0" +str;
  }
  return str;
}


// "[" is also considered as white-space/comment and is ignored
void SkipWhiteSpace(istream& f)
{
  while(f && !f.eof() && (std::ws(f).peek())=='[')
  {
    // XXX Gorthi Hard Coded;
    // I am not sure how this works :(
    f.ignore(150,'\n');
  }
}


void read_contour_data_file(char* config_file)
{
    ifstream f;
    char geomBuffer[MAX_CHAR] = {'\0'};

    f.open(config_file);

    if (f)
    {
      SkipWhiteSpace(f);            
     f >> CONTOUR->totalContours;

     for ( unsigned int i=0; i < CONTOUR->totalContours; i++ )
     {
       // Slice Number
       SkipWhiteSpace(f);
       f >> CONTOUR->sliceNumber[i];

       // Geometry-Type
       SkipWhiteSpace(f);
       if( (CONTOUR->geometryType[i] = new char[MAX_CHAR]) == NULL ) exit(1);
       f >> geomBuffer;
       strcpy(CONTOUR->geometryType[i], geomBuffer);

       // Number of Points
       SkipWhiteSpace(f);
       f >> CONTOUR->numOfPoints[i];

       // Contour Data
       SkipWhiteSpace(f);

       // 3 coordinates: (x,y,z) for each point are to be stored.
       // "\\" is inserted in between the cooordinates => 4 additional bytes
       // are required for each point. 
       // 6 more bytes are used for accomidating the digits
       // before the decimal point. =>
       // In addition to precision bytes, (6+4=) 10 additional bytes are used. 
       //
       unsigned int numOfBytes = (PRECISION+10) * 3 * CONTOUR->numOfPoints[i];
       char* contourData = new char[numOfBytes];

       f >> contourData;
       if( (CONTOUR->contourData[i] = new char[numOfBytes]) == NULL ) exit(1);
       strcpy(CONTOUR->contourData[i], contourData);

       delete [] contourData;
     }
     f.close();
    }else
    {
      cerr << "No contour data file specified...quitting.\n";
      exit(1);
    }
}


void readInputParameters( const char* parameterFileName)
{
    char* nameBuffer = new char[MAX_CHAR];

    ifstream f;
    f.open(parameterFileName);

    if (f)
    {
      // Input DICOM Image Slice Name with Complete Path
      SkipWhiteSpace(f);
      if ( (parameters->inputDCMFileName = new char[MAX_CHAR]) == NULL )
        exit(1);
      f >> nameBuffer;
      strcpy(parameters->inputDCMFileName, nameBuffer);

      // Output RTSTRUCT File Name
      SkipWhiteSpace(f);
      if ( (parameters->outputFileName = new char[MAX_CHAR]) == NULL )
        exit(1);
      f >> nameBuffer;
      strcpy(parameters->outputFileName, nameBuffer);

      // Number of Image Slices in the Segmented Image
      SkipWhiteSpace(f);
      f >> parameters->NumSlicesInRTSTRUCT;

      // Starting Slice Number for the Segmented with respect to original DICOM file
      SkipWhiteSpace(f);
      f >> parameters->START_SLICE_NUM;

      // Common SOP Instance UID Prefix for the DICOM Series
      SkipWhiteSpace(f);
      if ( (parameters->prefixSOPInstUID = new char[MAX_CHAR]) == NULL )
        exit(1);
      f >> nameBuffer;
      strcpy(parameters->prefixSOPInstUID, nameBuffer);

      // Number of ROIs to be written to RTSTRUCT
      SkipWhiteSpace(f);
      f >> parameters->numOfROIs;

      // Names of the text files containing Contour Data
      for (unsigned int num = 0; num < parameters->numOfROIs; num++)
      {
        SkipWhiteSpace(f);
        if ((parameters->contourDataFileName[num] = new char[MAX_CHAR]) == NULL )
          exit(1);
        f >> nameBuffer;
        strcpy(parameters->contourDataFileName[num], nameBuffer);
      }

      // Names to be assigned to the ROIs
      for (unsigned int num = 0; num < parameters->numOfROIs; num++)
      {
        SkipWhiteSpace(f);
        if ((parameters->roiName[num] = new char[MAX_CHAR]) == NULL )
          exit(1);

        // Since there can be spaces in the name, getline() is used instead of <<
        f.getline(parameters->roiName[num], MAX_CHAR);
      }


      // ROI Interpretted Types for Each ROI
      for (unsigned int num = 0; num < parameters->numOfROIs; num++)
      {
        SkipWhiteSpace(f);
        if ((parameters->roiInterpretedType[num] = new char[MAX_CHAR]) == NULL )
          exit(1);
        f >> nameBuffer;
        strcpy(parameters->roiInterpretedType[num], nameBuffer);
      }


      // Colors to be Assigned for each ROI
      for (unsigned int num = 0; num < parameters->numOfROIs; num++)
      {
        SkipWhiteSpace(f);
        if ((parameters->roiColor[num] = new char[MAX_CHAR]) == NULL )
          exit(1);
        f >> nameBuffer;
        strcpy(parameters->roiColor[num], nameBuffer);
      }

      // Clean up
      delete [] nameBuffer;

      f.close();
    } else {
      cerr << "No Input Parameter file specified...quitting.\n";
      exit(1);
    }
}
/************************************************************************/
