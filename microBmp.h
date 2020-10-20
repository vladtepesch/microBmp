/**
 * small lib for reading image data fom bmp image files.
 * designed not to use dynmic memory and to only work with externally provided buffers and load data row wise
 * 
 */

#ifndef BMP_IMAGE_HEADER
#define BMP_IMAGE_HEADER

#include <stdint.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif



/**
 *  user provided function that should load the given amount of bytes from the offset and copy it into the given buffer
 *
 *  \param[out]    o_buffer      Buffer to load data into
 *  \param[in]     i_numBytes    size of requested data
 *  \param[in]     i_offset      "file" (or whatever source the bmp data has) offset of the requested data
 *  \param[in,out] io_userData   pointer to user data that was passed to init
 *
 */
typedef void (*microBmp_loadDataFunc)(void* o_buffer, uint32_t i_numBytes, uint32_t i_offset, void* io_userData);

typedef enum {
  MBMP_STATUS_OK=0, 
  MBMP_STATUS_CACHE_BUFFER_TOO_SMALL, 
  MBMP_STATUS_UNSUPPORTED_FILE_TYPE,
  MBMP_STATUS_UNSUPPORTED_BMP_FORMAT
} microBmpStatus; 


#ifdef _MSC_VER
#  define BMP_STRUCTPACK_ATT 
#  pragma pack(push,1)
#else
#  define BMP_STRUCTPACK_ATT __attribute__ ((packed))
#endif

typedef struct BMP_STRUCTPACK_ATT {
  uint16_t fileIdentifier;
  uint32_t fileSize;
  uint16_t reserved1;
  uint16_t reserved2;
  uint32_t imageDataOffset;
} microBmp_FileHeader;

typedef struct BMP_STRUCTPACK_ATT {
  uint32_t headerSize;
  int32_t imageWidth;
  int32_t imageHeight;
  uint16_t colorPlanes;
  uint16_t bitsPerPixel;
  uint32_t compressionMethod;
  uint32_t imageSize; /*Size of raw bitmap data*/
  int32_t horizontalRes;
  int32_t verticalRes;
  uint32_t colorsInPalette;
  uint32_t numImportantColors;
  uint32_t maskR;
  uint32_t maskG;
  uint32_t maskB;
} microBmp_BmpInfo;

typedef struct BMP_STRUCTPACK_ATT {
  microBmp_FileHeader fileHeader;
  microBmp_BmpInfo    bmpInfo;
}microBmp_FileMetaData;

#ifdef _MSC_VER
#  pragma pack(pop)
#endif


typedef struct {
  uint16_t imageWidth;
  uint16_t imageHeight;
  uint32_t bytesPerRow;      /**< Size of a row in bytes */
  uint8_t  bytesPerPixel;    /**< bytes per pixel (may be 0 if multiple pixels stored in a byte)*/
  uint8_t  bitsPerPixel;     /**< bits per pixel */
  uint16_t colorsInPalette;  /**< number of colors in the palette if image is indexed  */
  uint8_t  shiftR;           /**< lowest bit offset of r color if 16bit image */
  uint8_t  shiftG;           /**< lowest bit offset of g color if 16bit image */
  uint8_t  shiftB;           /**< lowest bit offset of b color if 16bit image */
  uint8_t  maskR;            /**< bit mask of r color after shifting if 16bit image */
  uint8_t  maskG;            /**< bit mask of g color after shifting if 16bit image */
  uint8_t  maskB;            /**< bit mask of b color after shifting if 16bit image */

  uint32_t endOfImage;       /**< End of the image data in the "file" */
  uint16_t currentRow;       /**< Current row, starting at 0 */



  uint16_t cachedRows;       /**< Number of rows currently cached */
  uint16_t cacheSizeRows;    /**< Cache Size in rows */
  uint32_t cacheSizeBytes;   /**< Cache Size in bytes */

  const uint8_t * rowData;         /**< Current row data */
  uint8_t * imageData;       /**< Loaded image data */
  uint8_t * palette;
  microBmp_loadDataFunc loadDataFunc;
  void*                 loadDataUserData;
} microBmp_State;

/**
 * Initialises the image loader and loads in BMP files headers.
 * 
 * @param[out] o_this               Image loader in which to store information.
 * @param[out] io_buffer            If i_loadDataFunc is non-NULL this buffer that is used internally as cache for read image data 
 *                                  - so it will get overwritten  
 *                                  If i_loadDataFunc is NULL the io_buffer has to contain the whole image data. 
 *                                  In this case the buffer is handled read only. So it should be okay to pass a const pointer.
 * @param[in]  i_buffersize         sizeof the buffer
 * @param[in]  i_loadDataFunc       Function to load in image data. This may be NULL, if the io_buffer does contain the whole image
 * @param[in]  i_userData           optional pointer to user specific data thats just gets passed to dataRetrievalFunc as io_userData
 */
microBmpStatus microBmp_init(microBmp_State* o_this, uint8_t* io_buffer, size_t i_buffersize, microBmp_loadDataFunc i_loadDataFunc, void* i_userData);

/**
 * deinitializes the object - should be called after object is not needed anymore
 * Currently does not do anything (and probably never will).
 */
static inline void microBmp_deinit(microBmp_State* io_this) {
  (void)io_this; // currently does nothing
}


/**
 * sets the row that is read with microBmp_getNextRow
 */
void microBmp_setNextRow(microBmp_State* io_this, uint16_t row);


/**
 * returns pointer to the image data of the next row 
 * loads data if required via the loadDataFunc 
 * 
 * \returns pointer to raw image data. this can be an index to palette or BGR or BGRA tuples
 *          use one of the microBmp_convertRowTo* functions to get actual image data 
 */
const uint8_t* microBmp_getNextRow(microBmp_State * io_this);

/** returns the bitmap data of the current row from pixel [x1, x2[ into rgb and writes the data into o_targetbuf */
void microBmp_convertRowToRGB(const microBmp_State* i_this, uint8_t* o_targetBuf, uint16_t x1, uint16_t x2);

/** returns the bitmap data of the current row from pixel [x1, x2[ into 16bit RGB565 and writes the data into o_targetbuf */
void microBmp_convertRowTo565(const microBmp_State* i_this, uint16_t* o_targetBuf, uint16_t x1, uint16_t x2);



#ifdef __cplusplus
}
#endif


#endif
