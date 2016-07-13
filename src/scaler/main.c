//Main function for testing the scaler class

#include <stdlib.h>
#include <stdio.h>

#include "scaler.h"

#define IN_Y_W 8
#define IN_Y_H 4
#define in_cb_width 4
#define in_cb_height 2

#define OUT_Y_W 4
#define OUT_Y_H 2

void printPicBuffer(pic_buffer_t* buffer)
{
  for (int i = 0; i < buffer->height; i++) {
    for (int j = 0; j < buffer->width; j++) {
      printf("%i ", buffer->data[buffer->width*i + j]);
    }
    printf("\n");
  }
}

void printout( yuv_buffer_t* buffer )
{
  printf("Y:\n");
  printPicBuffer(buffer->y);
  printf("Cb:\n");
  printPicBuffer(buffer->u);
  printf("Cr:\n");
  printPicBuffer(buffer->v);
}

void test1()
{
  //Create a simple "picture" to debug scaler

  uint8_t y_data[IN_Y_W*IN_Y_H] = {
    25, 64, 240, 40, 16, 250, 0, 42,
    125, 164, 240, 140, 16, 250, 3, 12,
    /*25, 164, 20, 40 , 16, 250, 0, 4,
    25, 14, 140, 50 , 16, 205, 234, 44,
    57, 82, 34, 90, 65, 90, 44, 22,
    89, 92, 0, 71, 61, 78, 109, 100,*/
    0, 124, 78, 56, 29, 0, 4, 8,
    4, 7, 56, 12, 49, 7, 2, 0
  };

  uint8_t cb_data[in_cb_width*in_cb_height] = {
    240, 40, 16, 7,
    /*164, 240, 16, 7,
    16, 16, 16, 7,*/
    7, 35, 79, 5
  };

  uint8_t cr_data[in_cb_width*in_cb_height] = {
    40, 140, 16, 7,
    /*135, 40 , 16, 6,
    16, 16, 16, 5,*/
    8, 54, 21, 4
  };


  int is_420 = IN_Y_W != in_cb_width ? 1 : 0;
  yuv_buffer_t* pic = newYuvBuffer_uint8(y_data, cb_data, cr_data, IN_Y_W, IN_Y_H, is_420);
  scaling_parameter_t param = newScalingParameters(IN_Y_W, IN_Y_H, OUT_Y_W, OUT_Y_H);

  yuv_buffer_t* scaled = yuvDownscaling(pic, &param, is_420);
  printout(scaled);

  //Free memory
  deallocateYuvBuffer(pic);
  deallocateYuvBuffer(scaled);
}


//Copy data to a output array
void copyBack(pic_data_t* dst, uint8_t* src, int size)
{
  for (int i = 0; i < size; i++) {
    dst[i] = (pic_data_t)src[i];
  }
}

/**
* \brief Read a single frame from a file.
*
* Read luma and chroma values from file. Extend pixels if the image buffer
* is larger than the input image.
*
* \param file          input file
* \param input_width   width of the input video in pixels
* \param input_height  height of the input video in pixels
* \param img_out       image buffer
*
* \return              1 on success, 0 on failure
*/
int yuv_io_read(FILE* file,
  unsigned input_width, unsigned input_height,
  yuv_buffer_t *img_out)
{
  
  const unsigned y_size = input_width * input_height;
  const unsigned uv_input_width = input_width / 2;
  const unsigned uv_input_height = input_height / 2;
  const unsigned uv_size = uv_input_width * uv_input_height;

  const unsigned uv_array_width = img_out->y->width / 2;
  const unsigned uv_array_height = img_out->y->height / 2;

  if (input_width == img_out->y->width) {
    // No need to extend pixels.
    const size_t pixel_size = sizeof(unsigned char);
    unsigned char* buffer = malloc(sizeof(unsigned char)*y_size + pixel_size);
    int read = fread(buffer, pixel_size, y_size, file);
    if (fread(buffer, pixel_size, y_size, file) != y_size) {
      free(buffer); return 0;
    }
    copyBack(img_out->y->data, buffer, y_size);
    if (fread(buffer, pixel_size, uv_size, file) != uv_size) {
      free(buffer);  return 0;
    }
    copyBack(img_out->u->data, buffer, uv_size);
    if (fread(buffer, pixel_size, uv_size, file) != uv_size) {
      free(buffer);  return 0;
    }
    copyBack(img_out->v->data, buffer, uv_size);
  }


  return 1;
}

/**
* \brief Write a single frame to a file.
*
* \param file           output file
* \param img            image to output
* \param output_width   width of the output in pixels
* \param output_height  height of the output in pixels
*
* \return              1 on success, 0 on failure
*/
int yuv_io_write(FILE* file,
  const yuv_buffer_t *img,
  unsigned output_width, unsigned output_height)
{
  const int width = img->y->width;
  for (int y = 0; y < output_height; ++y) {
    fwrite(&img->y[y * width], sizeof(*img->y), output_width, file);
    // TODO: Check that fwrite succeeded.
  }
  for (int y = 0; y < output_height / 2; ++y) {
    fwrite(&img->u[y * width / 2], sizeof(*img->u), output_width / 2, file);
  }
  for (int y = 0; y < output_height / 2; ++y) {
    fwrite(&img->v[y * width / 2], sizeof(*img->v), output_width / 2, file);
  }

  return 1;
}

/**
* \brief Downscale given kvz_picture. Use sizes in kvz_picture for scaling
*/
void kvzDownscaling(yuv_buffer_t* in, yuv_buffer_t* out)
{
  //Create picture buffers based on given kvz_pictures
  int32_t in_y_width = in->y->width;
  int32_t in_y_height = in->y->height;
  int32_t out_y_width = out->y->width;
  int32_t out_y_height = out->y->height;

  //assumes 420
  int is_420 = in->y->width != in->u->width ? 1 : 0;
  scaling_parameter_t param = newScalingParameters(in_y_width, in_y_height, out_y_width, out_y_height);

  yuv_buffer_t* scaled = scale(in, &param, is_420);

  //copy data to out
  pic_data_t* tmp = out->y->data;
  out->y->data = scaled->y->data;
  scaled->y->data = tmp;

  tmp = out->u->data;
  out->u->data = scaled->u->data;
  scaled->u->data = tmp;

  tmp = out->v->data;
  out->v->data = scaled->v->data;
  scaled->v->data = tmp;

  //Free memory
  deallocateYuvBuffer(scaled);
}

void test2()
{
  int32_t in_width = 600;
  int32_t in_height = 600;

  FILE* file = fopen("Kimono1_600x600_24.yuv","r");
  if (file == NULL) {
    perror("FIle open failed");
  }
  
  yuv_buffer_t* data = newYuvBuffer_uint8(NULL, NULL, NULL, in_width, in_height, 1);
  yuv_buffer_t* out = newYuvBuffer_uint8(NULL, NULL, NULL, in_width / 2 + 1, in_height / 2, 1);

  if (yuv_io_read(file, in_width, in_height, data)) {
    
    kvzDownscaling(data, out);
    
    FILE* out_file = fopen("Kimono_301x300.yuv", "w");
    yuv_io_write(out_file, out, out->y->width, out->y->height);
    fclose(out_file);
  }
  else {
    perror("Read fail");
  }

  deallocateYuvBuffer(data);
  deallocateYuvBuffer(out);
  fclose(file);
}

int main()
{
  test2();

  system("Pause");
  return EXIT_SUCCESS;
}