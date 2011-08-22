# Before compiling the mask2contour.cxx file present in this directory,
# make sure that ITK is compiled with "ITK_USE_REVIEW" option
# enabled.
#
# For compiling mask2contour.cxx,
#  you may copy the following to a CamkeLists.txt file
#  and then compile using CMake:

#=========================================================
#=========================================================
# This project is designed to convert the masks to contours.
PROJECT(Mask2Contour)

# Find ITK Library
FIND_PACKAGE(ITK)
IF(ITK_FOUND)
  INCLUDE(${ITK_USE_FILE})
ELSE(ITK_FOUND)
  MESSAGE(FATAL_ERROR
          "Cannot build without ITK.  Please set ITK_DIR.")
ENDIF(ITK_FOUND)

ADD_EXECUTABLE(mask2contour mask2contour.cxx)

TARGET_LINK_LIBRARIES(mask2contour ITKCommon ITKIO ITKIOReview)
# If older versions of ITK are used, ITKIOReview may have to be replaced
#  with ITKReview.
#=========================================================
#=========================================================
