/*=========================================================================

  Program:   Insight Segmentation & Registration Toolkit
  Module:    $RCSfile: itkRTSTRUCTIOFactory.h,v $
  Language:  C++
  Date:      
  Version:   

  Copyright (c) Insight Software Consortium. All rights reserved.
  See ITKCopyright.txt or http://www.itk.org/HTML/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notices for more information.

=========================================================================*/
#ifndef __itkRTSTRUCTIOFactory_h
#define __itkRTSTRUCTIOFactory_h

#include "itkObjectFactoryBase.h"
#include "itkImageIOBase.h"

namespace itk
{
/** \class RTSTRUCTIOFactory
 * \brief Create instances of RTSTRUCTIO objects using an object factory.
 */
class ITK_EXPORT RTSTRUCTIOFactory : public ObjectFactoryBase
{
public:  
  /** Standard class typedefs. */
  typedef RTSTRUCTIOFactory        Self;
  typedef ObjectFactoryBase        Superclass;
  typedef SmartPointer<Self>       Pointer;
  typedef SmartPointer<const Self> ConstPointer;
  
  /** Class methods used to interface with the registered factories. */
  virtual const char* GetITKSourceVersion() const;
  virtual const char* GetDescription() const;
  
  /** Method for class instantiation. */
  itkFactorylessNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(RTSTRUCTIOFactory, ObjectFactoryBase);

  /** Register one factory of this type  */
  static void RegisterOneFactory()
    {
    RTSTRUCTIOFactory::Pointer gdcmFactory = RTSTRUCTIOFactory::New();
    ObjectFactoryBase::RegisterFactory(gdcmFactory);
    }

protected:
  RTSTRUCTIOFactory();
  ~RTSTRUCTIOFactory();

private:
  RTSTRUCTIOFactory(const Self&); //purposely not implemented
  void operator=(const Self&); //purposely not implemented

};
  
  
} // end namespace itk

#endif
