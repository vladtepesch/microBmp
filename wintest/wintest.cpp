// 
//
// little test program for microBmp
//
// image data is displayed in a rather inefficient way via individual SetPixel calls 
// and also in the WM_PAINT event



#include <iostream>
#include <Windows.h>

#include "microBmp.h"

#define INCBIN_PREFIX g_
#include "incbin.h"



INCBIN(test32_c3, "test_26_d32_c3.bmp");  // 32bit RGB0 with set bitfield (standard pattern)
INCBIN(test24_c3, "test_38_d24_c3.bmp");  // 24bit RGB  with set bitfield (standard pattern)
INCBIN(test16_c3, "test_60_565.bmp");     // 16bit RGB565 with set bitfield (standard pattern)
INCBIN(test8, "test_66_d8_2.bmp");        // 8bit index (full palette)
INCBIN(test4, "test_10_d4.bmp");          // 4bit index 
INCBIN(test1, "test_47_d1.bmp");          // 1bit index

struct TestSet {
  const char* testCaseName;
  const uint8_t* imgData;
  uint32_t imgSize;
};

static const TestSet g_images[] = {
  { "32bit c3",  g_test32_c3Data, g_test32_c3Size},
  { "24bit c3",  g_test24_c3Data, g_test24_c3Size},
  { "16bit 565", g_test16_c3Data, g_test16_c3Size},
  { "8bit Pal",  g_test8Data, g_test8Size}, // will fail in with buffersize of 512 because palette alone would require 4*256 byte
  { "4bit Pal",  g_test4Data, g_test4Size},
  { "1bit Pal",  g_test1Data, g_test1Size},
  {0,0,0}
};


const char* s_errorStr[] = {
  "ok",
  "cache_buffer_too_small",
  "unsupported_file_type",
  "unsupported_bmp_format"
};

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void readData(void* o_buffer, uint32_t i_numBytes, uint32_t i_offset, void* io_userData)
{
  memcpy(o_buffer, ((uint8_t*)io_userData) + i_offset, i_numBytes);
}


int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
  (void)pCmdLine;
  // Register the window class.
  const wchar_t CLASS_NAME[] = L"Sample Window Class";

  WNDCLASS wc = { };

  wc.lpfnWndProc = WindowProc;
  wc.hInstance = hInstance;
  wc.lpszClassName = CLASS_NAME;

  RegisterClass(&wc);

  // Create the window.

  HWND hwnd = CreateWindowEx(
    0,                              // Optional window styles.
    CLASS_NAME,                     // Window class
    L"Learn to Program Windows",    // Window text
    WS_OVERLAPPEDWINDOW,            // Window style

    // Size and position
    CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,

    NULL,       // Parent window    
    NULL,       // Menu
    hInstance,  // Instance handle
    NULL        // Additional application data
  );

  if (hwnd == NULL)
  {
    return 0;
  }

  ShowWindow(hwnd, nCmdShow);

  // Run the message loop.

  MSG msg = { };
  while (GetMessage(&msg, NULL, 0, 0))
  {
    TranslateMessage(&msg);
    DispatchMessage(&msg);
  }

  return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  switch (uMsg)
  {
  case WM_DESTROY:
    PostQuitMessage(0);
    return 0;

  case WM_PAINT:
  {
    PAINTSTRUCT ps;
    HDC hdc = BeginPaint(hwnd, &ps);

    FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW ));
    
    int xoffset = 5;
    int xsteps = 150;
    int ySteps = 70;
    int yoffset = 5;

    {
      RECT r = { xoffset, yoffset + ySteps/2, xoffset + 500, yoffset + 500 };
      const char* text = "test";
      DrawTextA(hdc, text, strlen(text), &r, 0);

      for (int i = 0; g_images[i].testCaseName; ++i) {
        yoffset  += ySteps;
        r.top    += ySteps;
        r.bottom += ySteps;
        text = "test";
        DrawTextA(hdc, g_images[i].testCaseName, strlen(g_images[i].testCaseName), &r, 0);
      }

    }

    // draw image via load call back
    yoffset = 5;
    xoffset += xsteps;
    {
      RECT r = { xoffset, yoffset + ySteps/2, xoffset + 500, yoffset + 500 };
      const char* text = "with callback";
      DrawTextA(hdc, text, strlen(text), &r, 0);
      yoffset += ySteps;
      r.top += ySteps;
      r.bottom += ySteps;

      for(int i=0; g_images[i].testCaseName; ++i)
      {
        int y = yoffset;
        microBmp_State imload;
        uint8_t imgbuff[512];
        microBmpStatus status = microBmp_init(&imload, imgbuff, sizeof(imgbuff), &readData, (void*)g_images[i].imgData);
        if (MBMP_STATUS_OK == status)
        {
          while (const uint8_t*  row = microBmp_getNextRow(&imload)) {
            uint8_t rowRGB[1024];
            microBmp_convertRowToRGB(&imload, rowRGB, 0, imload.imageWidth);
            for (int x = 0; x < imload.imageWidth; ++x) {
              uint8_t* rgb = &rowRGB[x * 3];
              SetPixel(hdc, xoffset + x, y, RGB(rgb[0], rgb[1], rgb[2]));
            }
            y++;
          }
        }else{
          const char*  errormessage = s_errorStr[status];
          DrawTextA(hdc, errormessage, strlen(errormessage), &r, 0);
        }
        yoffset += ySteps;
        r.top    += ySteps;
        r.bottom += ySteps;
        microBmp_deinit(&imload);
      }
    }

    // draw image directly from img buffer
    yoffset = 5;
    xoffset += xsteps;
    {
      RECT r = { xoffset, yoffset + ySteps / 2, xoffset + 500, yoffset + 500 };
      const char* text = "direct from buffer";
      DrawTextA(hdc, text, strlen(text), &r, 0);
      yoffset += ySteps;
      r.top += ySteps;
      r.bottom += ySteps;

      for (int i = 0; g_images[i].testCaseName; ++i)
      {
        int y = yoffset;
        microBmp_State imload;
        microBmpStatus status = microBmp_init(&imload, (uint8_t*)g_images[i].imgData, g_images[i].imgSize, NULL, NULL);
        if (MBMP_STATUS_OK == status)
        {
          while (const uint8_t* row = microBmp_getNextRow(&imload)) {
            uint8_t rowRGB[1024];
            microBmp_convertRowToRGB(&imload, rowRGB, 0, imload.imageWidth);
            for (int x = 0; x < imload.imageWidth; ++x) {
              uint8_t* rgb = &rowRGB[x * 3];
              SetPixel(hdc, xoffset + x, y, RGB(rgb[0], rgb[1], rgb[2]));
            }
            y++;
          }
        } else {
          const char* errormessage = s_errorStr[status];
          DrawTextA(hdc, errormessage, strlen(errormessage), &r, 0);
        }
        yoffset += ySteps;
        r.top += ySteps;
        r.bottom += ySteps;
        microBmp_deinit(&imload);
      }
    }

    // draw image directly from img buffer
    yoffset = 5;
    xoffset += xsteps;
    {
      RECT r = { xoffset, yoffset + ySteps / 2, xoffset + 500, yoffset + 500 };
      const char* text = "16bit output";
      DrawTextA(hdc, text, strlen(text), &r, 0);
      yoffset += ySteps;
      r.top += ySteps;
      r.bottom += ySteps;

      for (int i = 0; g_images[i].testCaseName; ++i)
      {
        int y = yoffset;
        microBmp_State imload;
        microBmpStatus status = microBmp_init(&imload, (uint8_t*)g_images[i].imgData, g_images[i].imgSize, NULL, NULL);
        if (MBMP_STATUS_OK == status)
        {
          while (const uint8_t* row = microBmp_getNextRow(&imload)) {
            uint16_t rowRGB[1024];
            microBmp_convertRowTo565(&imload, rowRGB, 0, imload.imageWidth);
            for (int x = 0; x < imload.imageWidth; ++x) {
              uint16_t c = rowRGB[x];
              SetPixel(hdc, xoffset + x, y, RGB((c>>8) & 0xf8, (c >> 3) & 0xfC, (c << 3)&0xfF));
            }
            y++;
          }
        }
        else {
          const char* errormessage = s_errorStr[status];
          DrawTextA(hdc, errormessage, strlen(errormessage), &r, 0);
        }
        yoffset += ySteps;
        r.top += ySteps;
        r.bottom += ySteps;
        microBmp_deinit(&imload);
      }
    }

    EndPaint(hwnd, &ps);
  }
  return 0;

  }
  return DefWindowProc(hwnd, uMsg, wParam, lParam);
}
