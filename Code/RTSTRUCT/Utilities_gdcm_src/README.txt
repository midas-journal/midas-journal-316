# List of files modified in ITK/Utilities/gdcm/src/ directory:
# 1. gdcmFile.cxx
# 2. gdcmFileHelper.h
# 3. gdcmFileHelper.cxx
#
#
# The modifications can be compared with respect to
#   ITK Release 3.10.1 (patch 1) (December 8 2008),
#   using the old files present in bkp_old_files/ directory. 
#   Those are marked with "*_10_old.cxx" naming.
#
#
# For using this code, replace the original files in 
#  ITK/Utilities/gdcm/src/ directory with above files
#  and compile ITK code again.
#
#
#--------------------------------
# 1. Modifications made in gdcmFile.cxx:
#--------------------------------
# Line number: 1716
# In bool File::Write(std::string fileName, FileType writetype)
# {
#   Insertion of (GrPixel, NumPixel) element is done only when the
#    modality is not RTSTRUCT.
# }
#
#
#--------------------------------
# 2. Modifications made in gdcmFileHelper.h:
#--------------------------------
# Added a new public function:
#   void CheckRTSTRUCTMandatoryElements();
#
#
#--------------------------------
# 3. Modifications made in gdcmFileHelper.cxx:
#--------------------------------
#
# RTSTRUCT Condion is added on the following functions:
#   bool FileHelper::Write(std::string const &fileName)
#   void FileHelper::RestoreWrite()
#   void FileHelper::RestoreWriteOfLibido()
#
# Code for the following new function is written:
#   void FileHelper::CheckRTSTRUCTMandatoryElements()
#
#
