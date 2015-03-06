
#ifndef BADPIXELS_H
#define BADPIXELS_H

#include <string>
#include <vector>
#include <iostream>
#include <fstream>
#include <map>
#include <algorithm>
#include <iterator>

#include "../../constants/em4_constants.h"
#include "../../constants/em4_data_types.h"
#include "../../data_acquisition/streamstor_handler.h"
#include "../../input/process_parameters.h"
#include "../dark_current/pedestal.h"
#include "../streamstor_image/em_image.h"

//This class identifies and handles the bad pixels in the CMOS sensor.
//To identify bad pixels, a large number of frames in the dark are acquired
//and any pixels with values about a certain threshold, which varies by sector
//of chip, are considered bad.

//Bad pixel locations are fairly stable over time so the results are written
//to a file and are used to correct an image in a post-processing step. 
//Since radiation damage over time creates new bad pixels, the procedure 
//to identify and append new bad pixels should be performed periodically. 
 

class BadPixels {
	public:
	  //The constructor for this class requires a filename where the bad pixel 
	  //locations are or will be recorded.
          BadPixels::BadPixels( const std::string &filename ){ filename_ = filename; }
	 
          //Both writeToFile methods write out the location of bad pixels to a file.   
	  bool writeToFile( std::map<int,int> &bad_pixels );
	  bool writeToFile( std::vector<int> &bad_pixels );

	  //Method getValuesFromFile reads the bad pixel locations from the file
          //holding these values. 
          bool getValuesFromFile( std::vector<int> &bad_pixels );
 

          //The findInDataBySector method takes as input the parameter class holding the 
	  //information about the data acquisition in the dark to search for bad pixels,
	  //a dark current pedestal file, and a map of thresholds for bad pixels that 
          //vary by sector on the CMOS chip. The values generated from this method is 
          //a map of bad pixel locations plus frequency. Useful for diagnostics of chip
          //behavior. 

          bool findInDataBySector( const ProcessParameters &params, Pedestal &ped, std::map<int,int> &threshold, std::map<int, int> &bad_pixels );
      
          // This method has inputs of a vector of bad pixel locations and
          // a correlated double sampled corrected image object. The bad pixel values 
          // in the image are replaced by an interpolated value from the surrounding
          // pixels, excepting adjacent bad pixels. 
       
           bool correctInImage( std::vector<int> &bad_pixels, CDSImage &image );
         
           //The appendPixels method has an input of a vector of new bad pixels. The
           //new set of bad pixels will be appended to the current bad pixel file without
           //duplicate entries. 

           bool appendPixels( std::vector<int> &new_bad_pixels );


	private:
	   
           //Input to getMatrixPixelLocations is the bad pixel location and the size of 
           //the surrounding pixels to return. This method returns the pixel locations 
           //around the central bad pixel. 
	   bool getMatrixPixelLocations( const int pixel, const int matrix_size, 
                                         std::vector<int> &pixel_locations );

           //Method to identify bad pixels in adjacent pixels to a bad pixel and remove
           //their contribution to the interpolation. 
	   void removeBadMatrixPixels( std::vector<int> &bad_pixels, std::vector<int> &pixel_locations);


	   
           std::string filename_;
	   std::vector<int> bad_pixels_;
};
#endif
