
#include "bad_pixels.h"


bool BadPixels::writeToFile( std::map<int, int> &bad_pixels ){
  bool retval = true;

  std::ofstream outfile; 
  std::map<int, int>::iterator iter = bad_pixels.begin();
  outfile.open( filename_.c_str(),  std::ios::trunc | std::ios::binary );

  if( outfile.is_open() ){
    while( iter != bad_pixels.end() ){
      int value = iter->first;
      outfile.write( (char*) ( &value ), sizeof( value ) );
      ++iter;
    }
  }else{
     retval = false;
  }

   return retval;
}
  

bool BadPixels::writeToFile( std::vector<int> &bad_pixels ){
  bool retval = true;
  std::ofstream outfile;
  std::vector<int>::iterator iter = bad_pixels.begin();
  outfile.open( filename_.c_str(), std::ios::trunc | std::ios::binary );
  if( outfile.is_open() ){
    while( iter != bad_pixels.end() ){
      int value = *iter;
      outfile.write( (char*)( &value ), sizeof( value ) );
      ++iter;
    }
  } else {
    retval = false;
  }

  return retval;
}


bool BadPixels::getValuesFromFile( std::vector<int> & bad_pixels ){
   bool retval = true; 
   std::ifstream infile;
   infile.open( filename_.c_str(), std::ios::binary );
   if( infile.is_open() ){
      while( !infile.eof() ){
        int m = 0;
        infile.read( (char*)( &m ), sizeof( m ) );
        bad_pixels.push_back( m ); 
      }
   } else {
     retval = false;
   }  
 
   return retval; 
}



bool BadPixels::findInDataBySector( const ProcessParameters &params, Pedestal &ped, 
                                  std::map<int,int> &threshold, std::map<int,int> &bad_pixels ){
   bool retval = true; 
   CDSImage ped_data;
   ped.getValues( ped_data );

   DirectoryEntry entry = params.getDirectoryEntry();
   EMImage em_image = EMImage();
  
   for(int frame_num = params.getMinFrame(); frame_num <= params.getMaxFrame(); frame_num++){
      CDSImage cds_image;
      if( em_image.getCDSImage( entry.start_address, frame_num, cds_image ) ){
        for( int pixel = 0; pixel < dimensions::num_rows*dimensions::num_columns; pixel++ ){
           int column_val = pixel % dimensions::num_columns;
           int sector = column_val/hardware::num_sector_pixels;
        
             std::map<int,int>::iterator threshold_iter = threshold.find( sector );
             int pixel_val = cds_image.frame[pixel] - ped_data.frame[pixel];
             if( std::abs(pixel_val) > threshold_iter->second ){ 
               ++bad_pixels[pixel]; 
             } 
         
        }
      } else {
         retval = false;
      }
   }

   return retval; 
} 


bool BadPixels::correctInImage( std::vector<int> &bad_pixels, CDSImage &image ){
  
  bool retval = true;
  std::map<int,int> pixel_values;

  //take image vector and map to pixel locations  
  for( int pixel_location = 0; pixel_location < dimensions::num_rows*dimensions::num_columns; pixel_location++  ){
    pixel_values.insert( std::make_pair( pixel_location, image.frame[pixel_location] ) );
  }
 
  //Find bad pixels and replace their values with the average of the values around the 
  //bad pixel. If bad pixels are adjacent, discount that bad pixel value from average.
  std::map<int,int>::iterator pixel_iter = pixel_values.begin();
  std::map<int,int>::iterator result;
  std::vector<int>::iterator bad_pixel_iter = bad_pixels.begin();
  int counter = 0;
  while( bad_pixel_iter != bad_pixels.end() ){
    result = pixel_values.find( *bad_pixel_iter );
    if( result != pixel_values.end() ){ //found a bad pixel so find average value around it
      std::vector<int> pixel_locations;
      int pixel_loc = *bad_pixel_iter; 
      getMatrixPixelLocations( pixel_loc, 3, pixel_locations );
      removeBadMatrixPixels( bad_pixels, pixel_locations );
      std::vector<int>::iterator loc_iter = pixel_locations.begin();
      int total = 0;
      while( loc_iter != pixel_locations.end() ){
      	std::map<int,int>::iterator sum_iter = pixel_values.find( *loc_iter );
      	total = total + sum_iter->second;
      	++loc_iter;
      } 
      int average = 0;
      if( pixel_locations.size() > 0 ){
         average = total/pixel_locations.size();
       }

      counter++;
      //Remove bad key value pair and replace with new one.
      pixel_values.erase( pixel_loc );
      pixel_values.insert( std::make_pair( pixel_loc, average ) ); 
    }
    ++bad_pixel_iter;
  }
  
  //replace interpolated value in bad pixel location   
  pixel_iter = pixel_values.begin();
  while( pixel_iter != pixel_values.end() ){
    image.frame[ pixel_iter->first] = pixel_iter->second;
    ++pixel_iter;
  }
    
  return retval;
}



bool BadPixels::appendPixels( std::vector<int> &new_bad_pixels ){
  bool retval = true;
  std::vector<int> old_bad_pixels;
  std::vector<int> result_bp;
  std::vector<int>::iterator result_bp_iter = result_bp.begin();
  std::vector<int>::iterator result_end_iter;

  getValuesFromFile( old_bad_pixels )
  sort( new_bad_pixels.begin(), new_bad_pixels.end() );
  sort( old_bad_pixels.begin(), old_bad_pixels.end() );
  result_end_iter = unique( result_bp.begin(), result_bp.end() );
  result_bp.erase( result_end_iter, result_bp.end() );

  retval = writeToFile( result_bp );

  return retval; 
}


bool BadPixels::getMatrixPixelLocations( const int pixel , const int matrix_size, std::vector<int> &pixel_locations ){
      bool retval = true;
      //divide up the pixel location into its row and column indices
      int row_val = pixel / dimensions::num_rows;
      int col_val = pixel % dimensions::num_columns;

      int shift_size = (matrix_size + 1)/2;

      for( int row_shift = 1; row_shift <= matrix_size; row_shift++ ){
         for( int col_shift = 1; col_shift <= matrix_size; col_shift++ ){
            int shifted_row = row_val - shift_size + row_shift;
            int shifted_col = col_val - shift_size + col_shift;
            if( shifted_row >= 0 && shifted_row < dimensions::num_rows
               && shifted_col >= 0 && shifted_col < dimensions::num_columns ){
               int shifted_pixel = shifted_col + (shifted_row * dimensions::num_rows );
               pixel_locations.push_back( shifted_pixel );
            } else {
               retval = false;
            }
         }
      }

      //remove middle pixel location from vector
      std::vector<int>::iterator result;
      result = find( pixel_locations.begin(), pixel_locations.end(), pixel );
      if( result != pixel_locations.end() ){
        pixel_locations.erase( result );
      } else {
	std::cout << "Can't erase middle of matrix" << std::endl;
	retval = false;
      }
      std::sort( pixel_locations.begin(), pixel_locations.end() );

      return retval;
}

void BadPixels::removeBadMatrixPixels( std::vector<int> &bad_pixels, std::vector<int> &pixel_locations ){
  //average only the matrix values that are not bad pixels
  std::vector<int>::iterator location_iter = pixel_locations.begin();
  while( location_iter != pixel_locations.end() ){
    std::vector<int>::iterator result;
    result = find( bad_pixels.begin(), bad_pixels.end(), *location_iter );
    if( result != bad_pixels.end() ){
      location_iter = pixel_locations.erase( result );
    } else {
    ++location_iter;
    }
  }
}
