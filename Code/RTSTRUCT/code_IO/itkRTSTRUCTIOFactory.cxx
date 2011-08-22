/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkRTSTRUCTIOFactory.cxx,v $
  Language:  C++
  Date:      
  Version:   

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#include "itkRTSTRUCTIOFactory.h"
#include "itkCreateObjectFunction.h"
#include "itkRTSTRUCTIO.h"
#include "itkVersion.h"

  
namespace itk
{
RTSTRUCTIOFactory::RTSTRUCTIOFactory()
{
  this->RegisterOverride("itkImageIOBase",
                         "itkRTSTRUCTIO",
                         "GDCM Image IO",
                         1,
                         CreateObjectFunction<RTSTRUCTIO>::New());
}
  
RTSTRUCTIOFactory::~RTSTRUCTIOFactory()
{
}

const char* RTSTRUCTIOFactory::GetITKSourceVersion() const
{
  return ITK_SOURCE_VERSION;
}

const char* RTSTRUCTIOFactory::GetDescription() const
{
  return "GDCM ImageIO Factory, allows the loading of DICOM images into Insight";
}

} // end namespace itk
