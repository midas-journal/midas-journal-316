# Please refer to the README.txt files present in
#  Code_IO/ and Utilities_gdcm_src/ directories
#  for details.
#
# For compiling the export2RTSTRUCT.cxx,
#  you may copy the following to a CamkeLists.txt file
#  and then compile using CMake:

#=========================================================
#=========================================================
# This project is designed to export the contours to RTSTRUCT.
PROJECT(Export2RTSTRUCT)

# Find ITK Library
FIND_PACKAGE(ITK)
IF(ITK_FOUND)
  INCLUDE(${ITK_USE_FILE})
ELSE(ITK_FOUND)
  MESSAGE(FATAL_ERROR
          "Cannot build without ITK.  Please set ITK_DIR.")
ENDIF(ITK_FOUND)

ADD_EXECUTABLE(export2RTSTRUCT export2RTSTRUCT.cxx )

TARGET_LINK_LIBRARIES(export2RTSTRUCT ITKCommon ITKIO)
#=========================================================
#=========================================================
