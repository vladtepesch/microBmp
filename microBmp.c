#include <stdlib.h>
#include <stdbool.h>
#include "microBmp.h"

typedef struct bmp_RGB {
  uint8_t r;
  uint8_t g;
  uint8_t b;
}bmp_RGB;

inline static uint8_t trailingZeros(uint32_t v)
{
  unsigned int c = 32; // c will be the number of zero bits on the right
  v &= -((int32_t)(v));
  if (v) c--;
  if (v & 0x0000FFFF) c -= 16;
  if (v & 0x00FF00FF) c -= 8;
  if (v & 0x0F0F0F0F) c -= 4;
  if (v & 0x33333333) c -= 2;
  if (v & 0x55555555) c -= 1;
  return (uint8_t)c;
}


inline static uint8_t popcount(uint32_t v) 
{
  v = v - ((v >> 1) & 0x55555555);                    // reuse input as temporary
  v = (v & 0x33333333) + ((v >> 2) & 0x33333333);     // temp
  return  ((v + (v >> 4) & 0xF0F0F0F) * 0x1010101) >> 24; // count
}


inline static uint8_t stretchTo8bit(uint8_t v, uint8_t mask)
{
  // 0001'1111  | pop = 5
  // 1111'1000  | << 8-5
  // (bits - (8-bits)) = bits - 8 + bits = 2*bits -8

  // 0000'0011  2
  // 1100'0000  6
  //     

  uint8_t bits = popcount(mask);
  if (bits < 8) {
    v = (v << (8 - bits));
    //if (bits| (v >> (8 - ())
    // todo: bill empty bits
  }
  return v;
}
inline size_t calc_row_size(const microBmp_BmpInfo * dibHeader) {
  /* Weird formula because BMP row sizes are padded up to a multiple of 4 bytes. */
  return (((dibHeader->bitsPerPixel * dibHeader->imageWidth) + 31) / 32) * 4;
}

static bool  microBmp_checkSupportedCompression(const microBmp_BmpInfo* dibHeader) {

  return     (dibHeader->compressionMethod == 0)
          || (    (dibHeader->compressionMethod == 3) 
               && (dibHeader->bitsPerPixel == 16))
          || (    (dibHeader->compressionMethod == 3) 
               && ((dibHeader->bitsPerPixel == 32) || (dibHeader->bitsPerPixel == 24))
               && (dibHeader->maskR == 0x00FF0000)
               && (dibHeader->maskG == 0x0000FF00)
               && (dibHeader->maskB == 0x000000FF)
            )
  ;
}

microBmpStatus microBmp_init(microBmp_State* o_this, uint8_t* io_buffer, size_t i_buffersize, microBmp_loadDataFunc i_loadDataFunc, void* i_userData)
{

  if (i_buffersize < sizeof(microBmp_FileMetaData)){
    return MBMP_STATUS_CACHE_BUFFER_TOO_SMALL;
  }

  o_this->loadDataFunc = i_loadDataFunc;
  o_this->loadDataUserData = i_userData;

  /* load BMP meta data */
  if (i_loadDataFunc) {
    i_loadDataFunc(io_buffer, sizeof(microBmp_FileMetaData), 0, i_userData);
  }

  const microBmp_FileHeader* fileheader = &((microBmp_FileMetaData*)io_buffer)->fileHeader;
  const microBmp_BmpInfo*    dibHeader  = &((microBmp_FileMetaData*)io_buffer)->bmpInfo;
  uint32_t imgDataOffset = fileheader->imageDataOffset;
  if (fileheader->fileIdentifier != 19778) {
    return MBMP_STATUS_UNSUPPORTED_FILE_TYPE;
  }

  if (    (dibHeader->bitsPerPixel == 1)
       || !microBmp_checkSupportedCompression(dibHeader)
       || (dibHeader->colorPlanes != 1)
     )
  {
    return MBMP_STATUS_UNSUPPORTED_BMP_FORMAT;
  }

  o_this->imageWidth = (uint16_t)dibHeader->imageWidth;
  o_this->imageHeight = (uint16_t)dibHeader->imageHeight;
  o_this->bitsPerPixel     = (uint8_t)dibHeader->bitsPerPixel;
  o_this->bytesPerPixel    = o_this->bitsPerPixel / 8; 
  o_this->colorsInPalette = (uint16_t)dibHeader->colorsInPalette;
  o_this->palette = NULL;
  o_this->bytesPerRow = calc_row_size(dibHeader);
  o_this->endOfImage = (imgDataOffset + (uint32_t)o_this->bytesPerRow * dibHeader->imageHeight);


  /* Calculating file constants */
  if (dibHeader->bitsPerPixel == 16) {
    if (dibHeader->compressionMethod == 3) {
      o_this->shiftR = trailingZeros(dibHeader->maskR);
      o_this->shiftG = trailingZeros(dibHeader->maskG);
      o_this->shiftB = trailingZeros(dibHeader->maskB);
      o_this->maskR  = (uint8_t)(dibHeader->maskR >> o_this->shiftR);
      o_this->maskG  = (uint8_t)(dibHeader->maskG >> o_this->shiftG);
      o_this->maskB  = (uint8_t)(dibHeader->maskB >> o_this->shiftB);
    } else {
      o_this->shiftR = 10;
      o_this->shiftG =  5;
      o_this->shiftB =  0;
      o_this->maskR = 0x1F;
      o_this->maskG = 0x1F;
      o_this->maskB = 0x1F;
    }
  }
  o_this->currentRow = 0;

  if (dibHeader->bitsPerPixel <= 8) {                      // palette image use part of buffer as palette buffer and rest as data cache
    uint32_t paletteSize = dibHeader->colorsInPalette * 4;
    uint32_t reqMinBuffersize = paletteSize + o_this->bytesPerRow;
    if (reqMinBuffersize <= i_buffersize) {
      if (i_loadDataFunc) {
        o_this->palette = io_buffer;
        io_buffer     += paletteSize;
        i_buffersize -= paletteSize;
        i_loadDataFunc(o_this->palette, paletteSize, sizeof(microBmp_FileHeader) + dibHeader->headerSize, i_userData);
      } else {
        o_this->palette = io_buffer + sizeof(microBmp_FileHeader) + dibHeader->headerSize;
      }

    } else {
      return MBMP_STATUS_CACHE_BUFFER_TOO_SMALL;
    }
  }

  o_this->cachedRows = 0;
  o_this->cacheSizeRows = (uint16_t)( i_buffersize/o_this->bytesPerRow);
  o_this->cacheSizeBytes = o_this->cacheSizeRows * o_this->bytesPerRow;
  o_this->imageData = io_buffer;

  if (o_this->cacheSizeRows == 0 || o_this->imageData == NULL) {
    return MBMP_STATUS_CACHE_BUFFER_TOO_SMALL;
  }

  return MBMP_STATUS_OK;
}


const uint8_t* microBmp_getNextRow(microBmp_State * io_this) 
{
  if (io_this->currentRow == io_this->imageHeight) {
    return NULL;
  }
  if(io_this->cachedRows == 0)
  {
    if (io_this->loadDataFunc) {
      /* BMP stores image data backwards, counting back to the row we want to start the read at. */
      uint32_t offset = io_this->endOfImage - (uint32_t)io_this->bytesPerRow * (uint32_t)(io_this->currentRow + io_this->cacheSizeRows);
      io_this->loadDataFunc(io_this->imageData, io_this->cacheSizeBytes, offset, io_this->loadDataUserData);
      io_this->cachedRows = io_this->cacheSizeRows;
      /* Move the row pointer forward to the start of the last row **/
      io_this->rowData = io_this->imageData + (io_this->bytesPerRow) * (io_this->cacheSizeRows - 1);
    } else {
      uint32_t offset = io_this->endOfImage - (uint32_t)io_this->bytesPerRow * (uint32_t)(io_this->currentRow+1);
      io_this->rowData = io_this->imageData + offset;
      io_this->cachedRows = 1;
    }

  }
  else
  {
    /* Moving down a row (which is backwards in memory)
       RowData is in 2 byte chunks (uint16), rowSize in bytes. Divide by 2. */
    io_this->rowData -= io_this->bytesPerRow;
  }
  --io_this->cachedRows;
  io_this->currentRow += 1;
  return  io_this->rowData;
}

void microBmp_setNextRow(microBmp_State* io_this, uint16_t row)
{
  /** \todo do not invalidate all cash rows if not necessary */
  io_this->currentRow = row;
  io_this->cachedRows = 0;
}


static bmp_RGB microBmp_getColorAt(const microBmp_State* i_this, uint16_t x)
{
  bmp_RGB col;
  const uint8_t* coldata;
  if (i_this->palette) {
    uint32_t bitOff  = x * i_this->bitsPerPixel;
    uint32_t byteOff = bitOff / 8;
    uint32_t idx = i_this->rowData[byteOff];
    if (i_this->bitsPerPixel == 4) { // multiple pixel per byte - refine index
      if (x & 1) {
        idx = idx & 0xf;
      }else{
        idx = idx >> 4;
      }
    } 
    coldata = &i_this->palette[idx * 4];
    col.b = coldata[0];
    col.g = coldata[1];
    col.r = coldata[2];
  } else if(i_this->bytesPerPixel ==2) {
    const uint16_t* u16Row = (const uint16_t*)i_this->rowData;
    uint16_t c16 = u16Row[x];
    col.r = stretchTo8bit((c16 >> i_this->shiftR)& i_this->maskR, i_this->maskR);
    col.g = stretchTo8bit((c16 >> i_this->shiftG)& i_this->maskG, i_this->maskG);
    col.b = stretchTo8bit((c16 >> i_this->shiftB)& i_this->maskB, i_this->maskB);

  } else {
    uint32_t byteOff = x * i_this->bytesPerPixel;
    coldata = &i_this->rowData[byteOff];
    col.b = coldata[0];
    col.g = coldata[1];
    col.r = coldata[2];
  }
  return col;
}

/** returns the bitmap data of the current row from pixel [x1, x2[ into rgb and writes the data into o_targetbuf */
void microBmp_convertRowToRGB(const microBmp_State* i_this, uint8_t* o_targetBuf, uint16_t x1, uint16_t x2) {
  while (x1 < x2) {
    bmp_RGB c = microBmp_getColorAt(i_this, x1);
    o_targetBuf[0] = c.r;
    o_targetBuf[1] = c.g;
    o_targetBuf[2] = c.b;
    o_targetBuf += 3;
    ++x1;
  }
}


