/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkRTSTRUCTIO.h,v $
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
#ifndef __itkRTSTRUCTIO_h
#define __itkRTSTRUCTIO_h

#include "itkImageIOBase.h"
#include <fstream>
#include <string>

// Required for adding InsertDICOMSequence() method
#include "gdcmSeqEntry.h"

// The following string is used for distinguishing the encapsulation of an
// Item of DICOM sequence from the encapsulation of the DICOM sequence itself.
//
// XXX GORTHI: I am not sure whether this the right place to define this string
// considering the clear interface.
const std::string ITEM_ENCAPSULATE_STRING("DICOM_ITEM_ENCAPSULATE");


namespace itk
{

/** \class RTSTRUCTIO
 *
 *  \brief ImageIO class for reading and writing DICOM V3.0 and ACR/NEMA (V1.0 & V2.0) images
 *  This class is only an adaptor to the gdcm library (currently gdcm 1.2.x is used):
 *
 *  http://creatis-www.insa-lyon.fr/Public/Gdcm/
 *
 *  CREATIS INSA - Lyon 2003-2008
 *    http://www.creatis.insa-lyon.fr
 *
 *  \warning There are several restrictions to this current RTSTRUCTIO:
 *           *  Currently we always need a DICOM CT-image slice as an input
 *              to the DICOM image reader (not to RTSTRUCT reader!) to write
 *              a proper RTSTRUCT file. This needs to be modified.
 *
 *           *  RTSTRUCT Reader is yet to be implemented.
 *              All the functions of the RTSTRUCT Reader are currently dummy :-)
 *
 *           *  Currently RTSTRUCTIO is inheritted from ImageIOBase;
 *              this is does not seem to the ideal thing to do since 
 *              RTSTRUCT is not an image as such.
 *
 *           * The current interface of RTSTRUCT imitates "itkGDCMImageIO.h".
 *             But, modifications are surely required for some more functions.
 *
 *  \ingroup IOFilters
 *
 */
class InternalHeader;
class ITK_EXPORT RTSTRUCTIO : public ImageIOBase
{
public:
  /** Standard class typedefs. */
  typedef RTSTRUCTIO          Self;
  typedef ImageIOBase         Superclass;
  typedef SmartPointer<Self>  Pointer;
  
  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(RTSTRUCTIO, Superclass);

  /*-------- This part of the interface deals with reading data. ------ */

  /** Determine the file type. Returns true if this ImageIO can read the
   * file specified. */
  virtual bool CanReadFile(const char*);
  
  /** Set the spacing and dimesion information for the current filename. */
  virtual void ReadImageInformation();
  
  /** Reads the data from disk into the memory buffer provided. */
  virtual void Read(void* buffer);

  /*-------- This part of the interfaces deals with writing data. ----- */

  /** Determine the file type. Returns true if this ImageIO can write the
   * file specified. GDCM triggers on ".dcm" and ".dicom". */
  virtual bool CanWriteFile(const char*);

  /** Writes the spacing and dimentions of the image.
   * Assumes SetFileName has been called with a valid file name. */
  virtual void WriteImageInformation();
  
  /** Writes the data to disk from the memory buffer provided. Make sure
   * that the IORegion has been set properly. */
  virtual void Write(const void* buffer);
  
  /** Macro to access the DICOM UID prefix. By default this is the ITK
   *  root id. This default can be overriden if the exam is for example
   *  part of an existing study.
   */
  itkGetStringMacro(UIDPrefix);
  itkSetStringMacro(UIDPrefix);

  /** Access the generated DICOM UID's. */
  itkGetStringMacro(StudyInstanceUID);
  itkGetStringMacro(SeriesInstanceUID);
  itkGetStringMacro(FrameOfReferenceInstanceUID);

  /** Preserve the original DICOM UID of the input files
   */
  // XXX GORTHI Are these macros required for RTSTRUCT?
  itkSetMacro(KeepOriginalUID,bool);
  itkGetMacro(KeepOriginalUID,bool);
  itkBooleanMacro(KeepOriginalUID);

  /** Convenience methods to query patient information
   * These methods are here for compatibility with the
   * DICOMImageIO2 class. */
  //
  // GORTHI: Does not contain any scanner related options.
  // GORTHI: Do we have to add any other RTSTRUCT specific methdos?
  //
  void GetPatientName(char* name);
  void GetPatientID(char* id);
  void GetPatientSex(char* sex);
  void GetPatientAge(char* age);
  void GetStudyID(char* id);
  void GetPatientDOB(char* dob);
  void GetStudyDescription(char* desc);
  void GetStudyDate(char* date);
  void GetModality(char* modality);
  void GetManufacturer(char* manu);
  void GetInstitution(char* ins);
  void GetModel(char* model);

  /** More general method to retrieve an arbitrary DICOM value based
   * on a DICOM Tag (eg "0123|4567").
   * WARNING: You need to use the lower case for hex 0x[a-f], for instance:
   * "0020|000d" instead of "0020|000D" (the latter won't work)
   */
  bool GetValueFromTag(const std::string & tag, std::string & value);

  /** Method for consulting the DICOM dictionary and recovering the text
   * description of a field using its numeric tag represented as a string.  If
   * the tagkey is not found in the dictionary then this static method return
   * false and the value "Unknown " in the labelId. If the tagkey is found then
   * this static method returns true and the actual string descriptor of the
   * tagkey is returned in the variable labelId. */
  static bool GetLabelFromTag( const std::string & tag, 
                               std::string & labelId );

  /** A DICOM file can contains multiple binary stream that can be very long
   * For example an Overlay on the image. Most of the time user do not want to load
   * this binary structure in memory since it can consume lot of memory. Therefore
   * any field that is bigger than the default value 0xfff is discarded and just seek'd 
   * This method allow advanced user to force the reading of such field
   */
  itkSetMacro(MaxSizeLoadEntry, long);

  /** Parse any sequences in the DICOM file. Defaults to the value of
   *  LoadSequencesDefault. Loading DICOM files is faster when
   *  sequences are not needed.
   */
  itkSetMacro(LoadSequences, bool);
  itkGetMacro(LoadSequences, bool);
  itkBooleanMacro(LoadSequences);

  /** Parse any private tags in the DICOM file. Defaults to the value
   * of LoadPrivateTagsDefault. Loading DICOM files is faster when
   * private tags are not needed.
   */
  itkSetMacro(LoadPrivateTags, bool);
  itkGetMacro(LoadPrivateTags, bool);
  itkBooleanMacro(LoadPrivateTags);  

  /** Global method to define the default value for
   * LoadSequences. When instances of RTSTRUCTIO are created, the
   * ivar LoadSequences is initialized to the value of
   * LoadSequencesDefault.  This method is useful when relying on the
   * IO factory mechanism to load images rather than specifying a
   * particular ImageIO object on the readers. Default is false. */
  static void SetLoadSequencesDefault(bool b)
    { m_LoadSequencesDefault = b; }
  static void LoadSequencesDefaultOn()
    { m_LoadSequencesDefault = true; }
  static void LoadSequencesDefaultOff()
    { m_LoadSequencesDefault = false; }
  static bool GetLoadSequencesDefault()
    { return m_LoadSequencesDefault; }

  /** Global method to define the default value for
   * LoadPrivateTags. When instances of RTSTRUCTIO are created, the
   * ivar LoadPrivateTags is initialized to the value of
   * LoadPrivateTagsDefault.  This method is useful when relying on the
   * IO factory mechanism to load images rather than specifying a
   * particular ImageIO object on the readers. Default is false. */
  static void SetLoadPrivateTagsDefault(bool b)
    { m_LoadPrivateTagsDefault = b; }
  static void LoadPrivateTagsDefaultOn()
    { m_LoadPrivateTagsDefault = true; }
  static void LoadPrivateTagsDefaultOff()
    { m_LoadPrivateTagsDefault = false; }
  static bool GetLoadPrivateTagsDefault()
    { return m_LoadPrivateTagsDefault; }

protected:
  RTSTRUCTIO();
  ~RTSTRUCTIO();
  void PrintSelf(std::ostream& os, Indent indent) const;

  bool OpenGDCMFileForReading(std::ifstream& os, const char* filename);
  bool OpenGDCMFileForWriting(std::ofstream& os, const char* filename);
  void InternalReadImageInformation(std::ifstream& file);

  // GORTHI: Added a method to recurively enter sequence data
  bool InsertDICOMSequence(MetaDataDictionary &sub_dict,
                           const unsigned int itemDepthLevel,
                           gdcm::SeqEntry     *seqEntry);

  std::string m_UIDPrefix;
  std::string m_StudyInstanceUID;
  std::string m_SeriesInstanceUID;
  std::string m_FrameOfReferenceInstanceUID;
  bool        m_KeepOriginalUID;
  long        m_MaxSizeLoadEntry;

private:
  RTSTRUCTIO(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

  std::string m_PatientName;
  std::string m_PatientID;
  std::string m_PatientDOB;
  std::string m_StudyID;   
  std::string m_StudyDescription;
  std::string m_PatientSex;
  std::string m_PatientAge;
  std::string m_StudyDate;
  std::string m_Modality;
  std::string m_Manufacturer;
  std::string m_Institution;
  std::string m_Model;

  bool        m_LoadSequences;
  bool        m_LoadPrivateTags;
  static bool m_LoadSequencesDefault;
  static bool m_LoadPrivateTagsDefault;
  
  InternalHeader * m_DICOMHeader;
};

} // end namespace itk

#endif // __itkRTSTRUCTIO_h
