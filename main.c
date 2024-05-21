/*  this is connected to the image segmentation project I'm working on from the 
*   "modern C" book I'm going through. The project is,
*
*   "to segment a grayscale image into connected regions that are 'similar' in some 
*   sense or another. Such a segmentation forms a partition in the set of pixels ...
*   you should use a Union-Find structure to represent regions, one per pixel 
*   at the start.
* 
*   Can you implement a statistics function that computes a statistic for all regions?
*   This should be another array ... that for each root holds the number of pixels and
*   sum of all values.
* 
*   Can you implement a merge criterion for regions? Test whether the mean values of 
*   two regions are not too far apart: say, no more than 5 gray values.
* 
*   Can you implement a Line by Line merge strategy that, for each pixel on a line of the 
*   image tests whether its region should be merged to the left and/or to the top?
* 
*   Can you iterate line by line until there are no mmore changes: that is, such that the
*   resulting regions/sets are all test negatively with their respective neighboring
*   regions?
* 
*   Now that you have a complete function for image segmentation, try it on images with
*   assorted subjects and sizes.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include <png.h>
typedef struct
{
  png_uint_32 height;
  png_uint_32 width;
  int bit_depth;
  int color_type;
  int interlace_method;
  int compression_method;
  int filter_method;
}IHDR_data;


typedef struct
{
  double whiteX;
  double whiteY;
  double redX;
  double redY;
  double greenX;
  double greenY;
  double blueX;
  double blueY;
}XYcHRM;

typedef struct
{
  char * name;
  int compression_type;
  png_bytep profile;
  png_uint_32 proflen;
}iCCP_data;

typedef struct
{
  png_bytep trans_alpha;
  int num_trans;
  png_color_16p trans_color;
}tRNS_data;

typedef struct
{
  png_int_32 offset_x;
  png_int_32 offset_y;
  int unit_type;
}oFFs_data;

typedef struct
{
  png_uint_32 res_x;
  png_uint_32 res_y;
  int unit_type;
}pHYS_data;

typedef struct
{
  int unit;
  double width;
  double height;
}sCAL_data;


png_bytepp convert_to_grayscale(png_structp png_ptr, png_infop info_ptr);
char * pngInterlaceTypeString(png_byte interlace_type);

int main (void)
{

  /*
  *   rewrite so the program reads data from the image and makes informed changes to image
  *   data
  */


#ifndef PNG_STDIO_SUPPORTED
  puts("This program only works if the current version of LIBPNG supports \
        stdio FILE * reading, which this version does not.\n Terminating program.");
  return EXIT_FAILURE;
#endif

  /*
  *   using both read and write APIs at the same time is adding a lot of stupid complexity.
  *   I'm going to separate everything out now
  */

  char * my_png_read_file_name = "sprout_agony_png.png";
  char * my_png_write_file_name = "gray_sprout.png";
  FILE * my_png_readFP = fopen(my_png_read_file_name, "rb");
  
  /*-------------------------------PRE-READING INITIALIZATION-----------------------------*/

  /*  Image has 32 bits per pixel with RGBA format
  */
  printf("Initializing read data structs... ");
  if (!my_png_readFP)
  {
    printf("ERROR: Failed to open file \"%s\". Program terminating", my_png_read_file_name);
    return EXIT_FAILURE;
  }

  unsigned int bytes_checked = 8;
  png_byte header[bytes_checked];

  if (fread(header, 1, bytes_checked, my_png_readFP) != bytes_checked)
  {
    printf("ERROR: Failed to read %u bytes from file \"%s\". Program terminating", bytes_checked, my_png_read_file_name);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }
  
  bool is_png = !png_sig_cmp(header, 0, bytes_checked);
  if (!is_png)
  {
    printf("ERROR: File \"%s\" is not a PNG file", my_png_read_file_name);
  }
  
  png_structp rpng_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!rpng_ptr)
  {
    printf("ERROR: Failed to create png read struct for file %s. Program terminating", my_png_read_file_name);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }
  
  png_infop rinfo_ptr = png_create_info_struct(rpng_ptr);

  if (!rinfo_ptr)
  {
    printf("ERROR: Failed to create png info struct for file %s. Program terminating", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, NULL, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }
  printf("Done initializing read data structs\n");
  if (setjmp(png_jmpbuf(rpng_ptr)))
  {
    printf("ERROR occured reading file %s. Program terminating", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }
  
  printf("Initializing read I/O... ");
  png_init_io(rpng_ptr, my_png_readFP);
  printf("Done initializing I/O\n");

  /*  If any of these png_set_... functions throw an error, move them to the input transformation
  *   section
  */
  png_set_sig_bytes(rpng_ptr, bytes_checked);

  /*  These next 2 API calls were recommended by libpng documentation
  */
  png_set_crc_action(rpng_ptr, PNG_CRC_ERROR_QUIT, PNG_CRC_WARN_DISCARD);

  png_color_16 gamma_screen_color;
  //  Not sure what to do with the other fields of this besides zero it out
  gamma_screen_color.blue = 0;
  gamma_screen_color.green = 0;
  gamma_screen_color.red = 0;
  gamma_screen_color.index = 0;

  gamma_screen_color.gray = 256 /*  might bug out, idk how the API reads this*/;
  png_set_background(rpng_ptr, &gamma_screen_color, PNG_BACKGROUND_GAMMA_SCREEN, 0, 1);

  /*  Reading with the low level interface because the transforms I need I 
  *   can't pass to the high level API
  */
  printf("Reading data into the info struct... ");
  fflush(stdout);
  png_read_info(rpng_ptr, rinfo_ptr);
  puts("Done");

  /*--------------------------READING THE FILE FROM THE API-------------------------*/

  
  
  IHDR_data image_IHDR;
  if (!png_get_IHDR(rpng_ptr, rinfo_ptr, &image_IHDR.width, &image_IHDR.height, 
              &image_IHDR.bit_depth, &image_IHDR.color_type, &image_IHDR.interlace_method, 
              &image_IHDR.compression_method, &image_IHDR.filter_method))
  {
    printf("ERROR: Failed to collect height or width or bit depth information from image %s.Program terminating.", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }

  printf("color type: %d\n", image_IHDR.color_type);
  png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
  fclose(my_png_readFP);
  return EXIT_SUCCESS;
  png_byte channels = png_get_channels(rpng_ptr, rinfo_ptr);

  if (!channels)
  {
    printf("ERROR: Failed to retrieve channel data from image %s. Program terminating.", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }
  
  size_t row_bytes = png_get_rowbytes(rpng_ptr, rinfo_ptr);

  if (!row_bytes)
  {
    printf("ERROR: failed to get row bytes from image %s. Program terminating.", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }
  
  png_colorp palette;
  int num_palette = 0;
  if (!png_get_PLTE(rpng_ptr, rinfo_ptr, &palette, &num_palette))
  {
    printf("ERROR: failed to retrieve palette data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }

  double gamma = 0;
  if (!png_get_gAMA(rpng_ptr, rinfo_ptr, &gamma))
  {
    printf("ERROR: failed to retrieve gamma data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }

  /*  Because chromaticity, or in libpng cHRM as referencd by API functions, is a new 
  *   concept to me, here is a brief explanation of why this is included in PNG images,
  *   as well as a link to the documentation which explains it in more detail:
  * 
  *   "The cHRM chunk is used, together with the gAMA chunk, to convey precise color 
  *   information so that a PNG image can be displayed or printed with better color 
  *   fidelity than is possible without this information."
  * 
  *   Source: http://www.libpng.org/pub/png/spec/1.2/PNG-ColorAppendix.html
  */
  XYcHRM file_cHRM;
  if (!png_get_cHRM(rpng_ptr, rinfo_ptr, &file_cHRM.whiteX, &file_cHRM.whiteY, 
                &file_cHRM.redX, &file_cHRM.redY, &file_cHRM.greenX,
                &file_cHRM.greenY, &file_cHRM.blueX, &file_cHRM.blueY))
  {
    printf("ERROR: failed to retrieve cHRM data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }
  
  int sRGB_intent;
  if (!png_get_sRGB(rpng_ptr, rinfo_ptr, &sRGB_intent))
  {
    printf("ERROR: failed to retrieve sRGB intent data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }

  iCCP_data image_iCCP;
  if (!png_get_iCCP(rpng_ptr, rinfo_ptr, &image_iCCP.name, &image_iCCP.compression_type,
                    &image_iCCP.profile, &image_iCCP.proflen))
  {
    printf("ERROR: failed to retrieve iCCP data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }
  
  png_color_8p sig_bit;
  if (!png_get_sBIT(rpng_ptr, rinfo_ptr, &sig_bit))
  {
    printf("ERROR: failed to retrieve significant bit number data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }
  
  
  tRNS_data image_tRNS;
  if (!png_get_tRNS(rpng_ptr, rinfo_ptr, &image_tRNS.trans_alpha, &image_tRNS.num_trans,
                    &image_tRNS.trans_color))
  {
    printf("ERROR: failed to retrieve transparency data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }

  png_uint_32 num_exif;
  png_bytep exif;
  if (!png_get_eXIf_1(rpng_ptr, rinfo_ptr, &num_exif, &exif))
  {
    printf("ERROR: failed to retrieve exif data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }
  
  png_uint_16p hist;
  if (!png_get_hIST(rpng_ptr, rinfo_ptr, &hist))
  {
    printf("ERROR: failed to retrieve palette histogram data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }

  /*  Background was already set by an earlier function, so it's known.
  *   Grab it with gamma_screen_color.gray
  */  
  
  /*  For the particular reason that I am writing this program, or rather 
  *   as a consequence of it, I know the comments that will be left on the image (I wrote
  *   them myself :^) ) so I'm going to skip grabbing the comments too.
  */

  png_sPLT_tp entries;
  int num_spalettes = png_get_sPLT(rpng_ptr, rinfo_ptr, &entries);
  if (!entries)
  {
    printf("ERROR: failed to retrieve palette structure entries data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }

  
  oFFs_data image_oFFs;
  if(!png_get_oFFs(rpng_ptr, rinfo_ptr, &image_oFFs.offset_x, &image_oFFs.offset_y, &image_oFFs.unit_type))
  {
    printf("ERROR: failed to retrieve image offset data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }

  pHYS_data image_pHYS;
  if (!png_get_pHYs(rpng_ptr, rinfo_ptr, &image_pHYS.res_x, &image_pHYS.res_y, &image_pHYS.unit_type))
  {
    printf("ERROR: failed to retrieve physical resolution data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }
  
  sCAL_data image_sCAL;
  if (!png_get_sCAL(rpng_ptr, rinfo_ptr, &image_sCAL.unit, &image_sCAL.width, &image_sCAL.height))
  {
    printf("ERROR: failed to retrieve physical scale data from image %s", my_png_read_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }

  png_unknown_chunkp unknown_chunk_entries = NULL;
  int num_unknown_chunks = png_get_unknown_chunks(rpng_ptr, rinfo_ptr, &unknown_chunk_entries);

  /*--------------------------------INPUT TRANSFORMATIONS------------------------------*/


  //  required because I fiddled with the background because I don't want to deal with alpha
  //  the image is 32 bits per pixel with 8 bits per channel before transformations
  png_set_expand(rpng_ptr);

  png_set_scale_16(rpng_ptr);

  
  //  apparently this is a built-in way to transform the image... we'll see

  printf("Running RGB to gray transformation... ");
  png_set_rgb_to_gray(rpng_ptr, PNG_ERROR_ACTION_WARN, -1, -1);
  printf("Done running RGB to gray transformation\n");


  
  /*---------------------------------PRE-WRITING INITIALIZATION---------------------------*/

  /*
  *   Initialiing the writing part
  */

  printf("Initializing new file writing... ");
  FILE * my_png_writeFP = fopen(my_png_write_file_name, "wb");

  if (!my_png_writeFP)
  {
    printf("ERROR: Failed to open file %s. Program Terminating.", my_png_write_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    return EXIT_FAILURE;
  }

  png_structp wpng_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);

  if (!wpng_ptr)
  {
    printf("ERROR: failed to create png write struct for file %s. Program terminating.", my_png_write_file_name);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    fclose(my_png_writeFP);
    return EXIT_FAILURE;
  }
  
  png_infop winfo_ptr = png_create_info_struct(wpng_ptr);

  if (!winfo_ptr)
  {
    printf("ERROR: failed to create png write info struct for file %s. Program terminating.", my_png_write_file_name);
    png_destroy_write_struct(&wpng_ptr, NULL);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    fclose(my_png_writeFP);
    return EXIT_FAILURE;
  }

  printf("Done initializing new file writing.\n New PNG file created named %s\n", my_png_write_file_name);

  if (setjmp(png_jmpbuf(wpng_ptr)))
  {
    printf("ERROR writing file %s. Program terminating.", my_png_write_file_name);
    png_destroy_write_struct(&wpng_ptr, &winfo_ptr);
    png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
    fclose(my_png_readFP);
    fclose(my_png_writeFP);
    return EXIT_FAILURE;
  }
  
  printf("Initializing write file I/O for PNG file %s... ", my_png_write_file_name);
  png_init_io(wpng_ptr, my_png_writeFP);
  printf("Done\n");


  /*
  *   Pass in the information from the source PNG
  */
  printf("Writing to new file header... \n");
  png_set_IHDR(wpng_ptr, winfo_ptr, image_IHDR.width, image_IHDR.height, image_IHDR.bit_depth,
                image_IHDR.color_type, image_IHDR.interlace_method, image_IHDR.compression_method,
                image_IHDR.filter_method);



  /*
  *   The documentation recommended that I set the gamma in the written image.
  */
   
  png_text text_ptr[3];

  char key0[] = "Title";
  char text0[] = "Grayscale sprout";
  text_ptr[0].key = key0;
  text_ptr[0].text = text0;
  text_ptr[0].compression = PNG_TEXT_COMPRESSION_NONE;
  text_ptr[0].itxt_length = 0;
  text_ptr[0].lang = NULL;
  text_ptr[0].lang_key = NULL;

  char key1[] = "Author";
  char text1[] = "coding creep";
  text_ptr[1].key = key1;
  text_ptr[1].text = text1;
  text_ptr[1].compression = PNG_TEXT_COMPRESSION_NONE;
  text_ptr[1].itxt_length = 0;
  text_ptr[1].lang = NULL;
  text_ptr[1].lang_key = NULL;

  char key2[] = "Description";
  char text2[] = "The first of many great and world-shaking journalistic pieces documented by none other than the coding creep. An image of another \
similarly creep creeper creepy trapped in the gray world, her color siphoned off as she was tossed in to \
create red 35, yellow 78, and blue 150, three of the primary coloring agents used in Jake Paul and KSI's PRIME Energy \
Drink(tm), used to fuel the lacrosse games and fortnite dances of ADHD amphetamine addled tweens and soothe the hangovers \
of beer-swilling state college fraternity Tier 3 KSI subbed dudebros much the same. Aerojet Rocketdyne SEW-R SKWUBBEWS \
paid for by the gofundme set up to revitalize her chrominance have already been deployed in arena sports facilities \
across the nation to catch the not-really-digested food coloring passed by the staff and preteen customers alike, more \
information to follow in future releases.";
  text_ptr[2].key = key2;
  text_ptr[2].text = text2;
  text_ptr[2].compression = PNG_TEXT_COMPRESSION_zTXt;
  text_ptr[2].itxt_length = 0;
  text_ptr[2].lang = NULL;
  text_ptr[2].lang_key = NULL;

  /*  Write the header information into memory
  */
  printf("Writing text to struct... ");
  png_set_text(wpng_ptr, winfo_ptr, text_ptr, 3);


  /*  Cleanup
  */

  png_read_end(rpng_ptr, rinfo_ptr);
  png_destroy_read_struct(&rpng_ptr, &rinfo_ptr, NULL);
  png_destroy_write_struct(&wpng_ptr, &winfo_ptr);
  fclose(my_png_writeFP);
  fclose(my_png_readFP);
  puts("Done\nProgram completed successfully!");
  return EXIT_SUCCESS;
}

png_bytepp convert_to_grayscale(png_structp png_ptr, png_infop info_ptr) 
{
  /*
  *   This was generated by chatgpt. I'll try the version in the documentation if this 
  *   doesn't work.
  */
  int width = png_get_image_width(png_ptr, info_ptr);
  int height = png_get_image_height(png_ptr, info_ptr);
  //png_byte color_type = png_get_color_type(png_ptr, info_ptr);
  //png_byte bit_depth = png_get_bit_depth(png_ptr, info_ptr);
  png_bytep *row_pointers = png_get_rows(png_ptr, info_ptr);

  if (width == 0 || height == 0 )
  {
    printf("error: width = %d and height = %d", width, height);
    return (png_bytepp) NULL;
  }
  
  if (row_pointers == NULL)
  {
    puts("error: row_pointers not allocated");
    return (png_bytepp) NULL;
  }
  

  for (int y = 0; y < height; y++) 
  {
    png_bytep row = row_pointers[y];
    for (int x = 0; x < width; x++) 
    {
      png_bytep px = &(row[x * 3]);  // Assume RGB for simplicity
      // Convert to grayscale using the luminance formula
      float gray = 0.299 * px[0] + 0.587 * px[1] + 0.114 * px[2];
      px[0] = px[1] = px[2] = (png_byte)gray;
    }
  }

  return row_pointers;
}

// Main function and other libpng setup/teardown code would go here

char * pngInterlaceTypeString(png_byte interlace_type)
{
  if (interlace_type == PNG_INTERLACE_ADAM7)
  {
    return "Adam7";
  }
  else if (interlace_type == PNG_INTERLACE_ADAM7_PASSES)
  {
    return "Adam7 passes";
  }
  else if (interlace_type == PNG_INTERLACE_LAST)
  {
    return "Last";
  }
  else if (interlace_type == PNG_INTERLACE_NONE)
  {
    return "None";
  }
  else
  {
    return "Unknown/unsupported";
  }
  
}