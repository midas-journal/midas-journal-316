# A brief description of the files:
#
#----------------
# Input Files:
#----------------
# 1. DICOM-CT-Image/ directory: Contains the DICOM CT image slices.
#
# 2. modified_image.*:          A cropped and preprocessed image
#                               of the original DICOM file.
#
#
# 3. mask1.*:                   Mask of the External-Contour.
#
# 4. mask2.*:                   Mask of the Bones.
#
#----------------
# Output Files:
#----------------
# 5. contour1.txt:              Contour Data of mask1.*
#
# 6. contour2,txt:              Contour Data of mask2.*
#
# 7. output_RTSTRUCT.dcm:       Final RTSTRUCT file.


# The following is an example of how to call to call the executables:
mask2contour.exe mask1.mhd contour1.txt 256 256 0
mask2contour.exe mask2.mhd contour2.txt 256 256 0
export2RTSTRUCT.exe parameter_file.txt
