/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkRTSTRUCTIO.cxx,v $
  Language:  C++
  Date:      
  Version:   

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

  Portions of this code are covered under the VTK copyright.
  See VTKCopyright.txt or http://www.kitware.com/VTKCopyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "itkRTSTRUCTIO.h"
#include "itkIOCommon.h"
#include "itkPoint.h"
#include "itkArray.h"
#include "itkMatrix.h"
#include <vnl/vnl_vector.h>
#include <vnl/vnl_matrix.h>
#include <vnl/vnl_cross.h>

#include "itkMetaDataObject.h"

#include <itksys/SystemTools.hxx>
#include <itksys/Base64.h>
#include "gdcmValEntry.h" //internal of gdcm
#include "gdcmBinEntry.h" //internal of gdcm
#include "gdcmSQItem.h"

#include "gdcmFile.h"
#include "gdcmFileHelper.h"
#include "gdcmUtil.h"
#include "gdcmGlobal.h"   // access to dictionary
#include "gdcmDictSet.h"  // access to dictionary

#include <fstream>
#include <math.h>   //for fabs on SGI
#include <itksys/ios/sstream>

namespace itk
{
class InternalHeader
{
public:
  InternalHeader() : m_Header(0) {}
  gdcm::File *m_Header;
};

bool RTSTRUCTIO::m_LoadSequencesDefault = true;
bool RTSTRUCTIO::m_LoadPrivateTagsDefault = true;


RTSTRUCTIO::RTSTRUCTIO()
  : m_LoadSequences( m_LoadSequencesDefault ),
    m_LoadPrivateTags( m_LoadPrivateTagsDefault )
{
  this->m_DICOMHeader = new InternalHeader;
  m_ByteOrder = LittleEndian; //default
  m_FileType = Binary;  //default...always true
  // UIDPrefix is the ITK root id tacked with a ".1"
  // allowing to designate a subspace of the id space for ITK generated DICOM
  m_UIDPrefix = "1.2.826.0.1.3680043.2.1125." "1";

  // Purely internal use, no user access:
  m_StudyInstanceUID = "";
  m_SeriesInstanceUID = "";
  m_FrameOfReferenceInstanceUID = "";

  m_KeepOriginalUID = false;
  m_MaxSizeLoadEntry = 0xfff;
}

RTSTRUCTIO::~RTSTRUCTIO()
{
  if (this->m_DICOMHeader->m_Header)
    {
    delete this->m_DICOMHeader->m_Header;
    }
  delete this->m_DICOMHeader;
}

bool RTSTRUCTIO::OpenGDCMFileForReading(std::ifstream& os,
                                         const char* filename)
{
  //XXX NOT YET IMPLEMENTED.
  return false;
}


bool RTSTRUCTIO::OpenGDCMFileForWriting(std::ofstream& os,
                                         const char* filename)
{
  // Make sure that we have a file to
  if ( *filename == 0 )
    {
    itkExceptionMacro(<<"A FileName must be specified.");
    return false;
    }

  // Close file from any previous image
  if ( os.is_open() )
    {
    os.close();
    }

  // Open the new file for writing
  itkDebugMacro(<< "Initialize: opening file " << filename);

#ifdef __sgi
  // Create the file. This is required on some older sgi's
  std::ofstream tFile(filename,std::ios::out);
  tFile.close();
#endif

  // Actually open the file
  os.open( filename, std::ios::out | std::ios::binary );

  if( os.fail() )
    {
    itkExceptionMacro(<< "Could not open file: "
                      << filename << " for writing."
                      << std::endl
                      << "Reason: "
                      << itksys::SystemTools::GetLastSystemError());
    return false;
    }
  return true;
}


// This method will only test if the header looks like a
// GDCM image file.
bool RTSTRUCTIO::CanReadFile(const char* filename)
{
  //XXX NOT YET IMPLEMENTED.
  return false;
}

void RTSTRUCTIO::Read(void* buffer)
{
  //XXX NOT YET IMPLEMENTED.
}


void RTSTRUCTIO::InternalReadImageInformation(std::ifstream& file)
{
  //XXX NOT YET IMPLEMENTED.
}

void RTSTRUCTIO::ReadImageInformation()
{
  //XXX NOT YET IMPLEMENTED.
}


bool RTSTRUCTIO::CanWriteFile(const char* name)
{
  std::string filename = name;

  if(  filename == "" )
    {
    itkDebugMacro(<<"No filename specified.");
    return false;
    }

  std::string::size_type dcmPos = filename.rfind(".dcm");
  if ( (dcmPos != std::string::npos)
       && (dcmPos == filename.length() - 4) )
    {
    return true;
    }

  dcmPos = filename.rfind(".DCM");
  if ( (dcmPos != std::string::npos)
       && (dcmPos == filename.length() - 4) )
    {
    return true;
    }

  std::string::size_type dicomPos = filename.rfind(".dicom");
  if ( (dicomPos != std::string::npos)
       && (dicomPos == filename.length() - 6) )
    {
    return true;
    }

  dicomPos = filename.rfind(".DICOM");
  if ( (dicomPos != std::string::npos)
       && (dicomPos == filename.length() - 6) )
    {
    return true;
    }

  return false;
}

void RTSTRUCTIO::WriteImageInformation()
{
  //XXX NOT YET IMPLEMENTED.
}

void RTSTRUCTIO::Write(const void* buffer)
{
  std::ofstream file;
  if ( !this->OpenGDCMFileForWriting(file, m_FileName.c_str()) )
    {
    return;
    }
  file.close();

  gdcm::File *header = new gdcm::File();
  gdcm::FileHelper *gfile = new gdcm::FileHelper( header );

  std::string value;
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
#if defined(_MSC_VER) && _MSC_VER < 1300
  // Not using real iterators, but instead the GetKeys() method
  // since VS6 is broken and does not export properly iterators
  // GetKeys will duplicate the entire DICOM header
  std::vector<std::string> keys = dict.GetKeys();
  for( std::vector<std::string>::const_iterator it = keys.begin();
       it != keys.end(); ++it )
    {
    const std::string &key = *it; //Needed for bcc32
#else
  //Smarter approach using real iterators
  itk::MetaDataDictionary::ConstIterator itr = dict.Begin();
  itk::MetaDataDictionary::ConstIterator end = dict.End();

  while(itr != end)
  {
    const std::string &key = itr->first; //Needed for bcc32
#endif
    
    // Convert DICOM name to DICOM (group,element)
    gdcm::DictEntry *dictEntry =
      header->GetPubDict()->GetEntry(key);
    // Anything that has been changed in the MetaData Dict will be pushed
    // into the DICOM header:
    if (dictEntry)
      {
      // Seperately handle the case when the Value is a sub-dictionary....
      if( typeid(dict).name() == (dict[key].GetPointer())->GetMetaDataObjectTypeName() )
      {
        gdcm::SeqEntry *seqEntry = header->InsertSeqEntry(
                                              dictEntry->GetGroup(),
                                              dictEntry->GetElement());  
        MetaDataDictionary sub_dict;
        ExposeMetaData<itk::MetaDataDictionary>(dict, key, sub_dict);

        // the depth asscoiated with 1st layer of items of this sequence = 1
        InsertDICOMSequence(sub_dict, 1, seqEntry);
      } else
      {
      ExposeMetaData<std::string>(dict, key, value);

      if (dictEntry->GetVR() != "OB" && dictEntry->GetVR() != "OW")
        {
        if(dictEntry->GetElement() != 0) // Get rid of group length, they are not useful
          {
          header->InsertValEntry( value,
                                  dictEntry->GetGroup(),
                                  dictEntry->GetElement());
          }
        }

      } // end dict entry with string as value
    } 
   
    #if !(defined(_MSC_VER) && _MSC_VER < 1300)
      ++itr;
    #endif
  }

  // Write Explicit for both 1 and 3 components images:
  gfile->SetWriteTypeToDcmExplVR();


  // UID generation part:

  // We only create *ONE* Study/Series.Frame of Reference Instance UID
  if( m_StudyInstanceUID.empty() )
    {
    // As long as user maintain there gdcmIO they will keep the same
    // Study/Series instance UID.
    m_StudyInstanceUID = gdcm::Util::CreateUniqueUID( m_UIDPrefix );
    m_SeriesInstanceUID = gdcm::Util::CreateUniqueUID( m_UIDPrefix );
    m_FrameOfReferenceInstanceUID = gdcm::Util::CreateUniqueUID( m_UIDPrefix );
    }
  std::string uid = gdcm::Util::CreateUniqueUID( m_UIDPrefix );

  header->InsertValEntry( uid, 0x0008, 0x0018); //[SOP Instance UID]
  header->InsertValEntry( uid, 0x0002, 0x0003); //[Media Stored SOP Instance UID]

  header->InsertValEntry( m_SeriesInstanceUID, 0x0020, 0x000e); //[Series Instance UID]

  if( ! gfile->Write( m_FileName ) )
  {
    itkExceptionMacro(<< "Cannot write the requested file:"
                      << m_FileName
                      << std::endl
                      << "Reason: "
                      << itksys::SystemTools::GetLastSystemError());
  }

  // Clean up
  delete gfile;
  delete header;
}

// Convenience methods to query patient information. These
// methods are here for compatibility with the DICOMImageIO2 class.
void RTSTRUCTIO::GetPatientName( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0010|0010", m_PatientName);
  strcpy (name, m_PatientName.c_str());
}

void RTSTRUCTIO::GetPatientID( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0010|0020", m_PatientID);
  strcpy (name, m_PatientID.c_str());
}

void RTSTRUCTIO::GetPatientSex( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0010|0040", m_PatientSex);
  strcpy (name, m_PatientSex.c_str());
}

void RTSTRUCTIO::GetPatientAge( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0010|1010", m_PatientAge);
  strcpy (name, m_PatientAge.c_str());
}

void RTSTRUCTIO::GetStudyID( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0020|0010", m_StudyID);
  strcpy (name, m_StudyID.c_str());
}

void RTSTRUCTIO::GetPatientDOB( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0010|0030", m_PatientDOB);
  strcpy (name, m_PatientDOB.c_str());
}

void RTSTRUCTIO::GetStudyDescription( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0008|1030", m_StudyDescription);
  strcpy (name, m_StudyDescription.c_str());
}

void RTSTRUCTIO::GetStudyDate( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0008|0020", m_StudyDate);
  strcpy (name, m_StudyDate.c_str());
}

void RTSTRUCTIO::GetModality( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0008|0060", m_Modality);
  strcpy (name, m_Modality.c_str());
}

void RTSTRUCTIO::GetManufacturer( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0008|0070", m_Manufacturer);
  strcpy (name, m_Manufacturer.c_str());
}

void RTSTRUCTIO::GetInstitution( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0008|0080", m_Institution);
  strcpy (name, m_Institution.c_str());
}

void RTSTRUCTIO::GetModel( char *name)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  ExposeMetaData<std::string>(dict, "0008|1090", m_Model);
  strcpy (name, m_Model.c_str());
}

bool RTSTRUCTIO::GetValueFromTag(const std::string & tag, std::string & value)
{
  MetaDataDictionary & dict = this->GetMetaDataDictionary();
  return ExposeMetaData<std::string>(dict, tag, value);
}

bool RTSTRUCTIO::GetLabelFromTag( const std::string & tagkey,
                                   std::string & labelId )
{
  gdcm::Dict *pubDict = gdcm::Global::GetDicts()->GetDefaultPubDict();
  
  gdcm::DictEntry *dictentry = pubDict->GetEntry( tagkey );

  bool found;

  // If tagkey was found (ie DICOM tag from public dictionary),
  // then return the name:
  if( dictentry )
    {
    labelId = dictentry->GetName();
    found = true;
    }
  else
    {
    labelId = "Unknown";
    found = false;
    }
  return found;
}


void RTSTRUCTIO::PrintSelf(std::ostream& os, Indent indent) const
{
  //XXX NOT YET IMPLEMENTED.
}


bool RTSTRUCTIO::InsertDICOMSequence(MetaDataDictionary &sub_dict,
                                     const unsigned int itemDepthLevel,
                                     gdcm::SeqEntry     *seqEntry)

{
  gdcm::SQItem *sqItem = NULL;

  itk::MetaDataDictionary::ConstIterator itr = sub_dict.Begin();
  itk::MetaDataDictionary::ConstIterator end = sub_dict.End();

  while(itr != end)
  {
    // Get the key
    const std::string &inside_key = itr->first;

    // To separate the items from each other, each item is
    // encapsulated inside a dictionary and to distinguish them against sub-sequences,
    // special-predefined-strings are associated with those dictionaries.
    // 
    // In such cases, inside sub-dictionary associated with those
    // special-predefined-strings is passed to this self-calling function.
 
    if (ITEM_ENCAPSULATE_STRING == inside_key.substr(0, ITEM_ENCAPSULATE_STRING.length()))
    {
      // Get the dictionary associted with that string...
      itk::MetaDataDictionary actual_sub_dict;
      ExposeMetaData<itk::MetaDataDictionary>(sub_dict, inside_key, actual_sub_dict);

      // Call this function on that sub-dictionary
      InsertDICOMSequence(actual_sub_dict, itemDepthLevel, seqEntry);
    } else
    {
      if ( itr == sub_dict.Begin() )
      {
        sqItem = new gdcm::SQItem(itemDepthLevel);
      }

      assert( sqItem != NULL );

      // if the value associated with this key is again a dictionary,
      // recursively call this function to insert a sub-sequence:
      if( typeid(sub_dict).name() == (sub_dict[inside_key].GetPointer())->GetMetaDataObjectTypeName() )
      {
        itk::MetaDataDictionary inside_sub_dict;
        ExposeMetaData<itk::MetaDataDictionary>(sub_dict, inside_key, inside_sub_dict);

        // if the attached sub-dictinary is a sequence, then.......
        gdcm::SeqEntry *sub_seqEntry = new gdcm::SeqEntry(
                        gdcm::Global::GetDicts()->GetDefaultPubDict()->GetEntry(inside_key));
        InsertDICOMSequence(inside_sub_dict, itemDepthLevel+1, sub_seqEntry);

        sqItem->AddEntry(sub_seqEntry);

      } else // if the value associated with the key is a string, directly add it to the sequence
      {
        std::string inside_value;
        ExposeMetaData<std::string>(sub_dict, inside_key, inside_value);

        gdcm::ValEntry *inside_valEntry = new gdcm::ValEntry(
                        gdcm::Global::GetDicts()->GetDefaultPubDict()->GetEntry(inside_key) );
        inside_valEntry->SetValue(inside_value);

        sqItem->AddEntry(inside_valEntry);
      }
    }
    ++itr; 
  }
  if (sqItem != NULL)
  {
    // 2nd argument: ordinal number of the seq-item
    // seqEntry->AddSQItem(sqItem, seqEntry->GetNumberOfSQItems()+1);
    seqEntry->AddSQItem(sqItem, seqEntry->GetNumberOfSQItems());
  }
  return true;
}

} // end namespace itk
