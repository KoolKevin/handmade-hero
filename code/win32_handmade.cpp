#include <windows.h>
#include <stdint.h>

#define global_variable static
#define local_persistent static
#define internal static

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

// TODO: a global for now
global_variable bool running;
global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;
global_variable int bitmapWidth;
global_variable int bitmapHeight;
global_variable int bytesPerPixel = 4;

internal void
renderGradient(int XOffset, int YOffset)
{

    int width = bitmapWidth;

    // good way to write pixel loops. usiamo esplicitamente un
    // row pointer aggiuntivo dato che non è detto che il pixel
    // pointer sia allineato con la prossima riga alla fine del
    // loop interno
    int stride = width * bytesPerPixel;
    uint8 *row = (uint8 *)bitmapMemory;
    for (int Y = 0; Y < bitmapHeight; Y++)
    {
        uint32 *pixel = (uint32 *)row;
        for (int X = 0; X < bitmapWidth; X++)
        {
            uint8 red = (uint8)(X + XOffset);
            uint8 green = (uint8)(Y + YOffset);

            // BGR windows pixel layout
            *pixel = red << 16 | green << 8;
            pixel++;
        }

        row += stride;
    }
}

// DIB == DeviceIndipendentBitmap
//     == buffer in cui scrivere cosa disegnare
internal void
win32ResizeDIBSection(int width, int height)
{
    if (bitmapMemory)
    {
        VirtualFree(bitmapMemory, 0, MEM_RELEASE);
    }

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = -height; // origin in alto a sinistra
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    bitmapWidth = width;
    bitmapHeight = height;

    // 3 bytes of data + 1 for word alignment
    int bitmapMemorySize = 4 * width * height;
    bitmapMemory = VirtualAlloc(0, bitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // TODO: probably want to clear this to black
}

internal void
win32UpdateWindow(HDC deviceContext, RECT *windowRect, int X, int Y, int width, int height)
{
    int windowWidth = windowRect->right - windowRect->left;
    int windowHeight = windowRect->bottom - windowRect->top;
    // copia i bit da un buffer e li disegna nel DC
    // applicando opportuno stretch
    int res = StretchDIBits(
        deviceContext,
        // dirty window redraw
        // X, Y, width, height, // dst
        // X, Y, width, height, // src
        // full window redraw
        0, 0, windowWidth, windowHeight, // dst
        0, 0, bitmapWidth, bitmapHeight, // src
        bitmapMemory,
        &bitmapInfo,
        DIB_RGB_COLORS,
        SRCCOPY);
}

LRESULT CALLBACK win32MainWindowCallback(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam)
{
    LRESULT result = 0;
    // interessante usare blocchi con switch per
    // non far propagare variabili locali di un
    // case ad altri case
    switch (message)
    {
    case WM_SIZE:
    {
        OutputDebugStringA("WM_SIZE\n");

        RECT clientRect;
        GetClientRect(window, &clientRect);
        int width = clientRect.right - clientRect.left;
        int height = clientRect.bottom - clientRect.top;
        win32ResizeDIBSection(width, height);
    }
    break;

    case WM_DESTROY:
    {
        OutputDebugStringA("WM_DESTROY\n");
        running = false;
    }
    break;

    case WM_CLOSE:
    {
        OutputDebugStringA("WM_CLOSE\n");
        running = false;
    }
    break;

    case WM_ACTIVATEAPP:
    {
        OutputDebugStringA("WM_ACTIVATEAPP\n");
    }
    break;

    // messaggio inviato quando è necessario ridisegnare la
    // finestra (e.g. espansione, la spostiamo out-of-view, ...)
    case WM_PAINT:
    {
        OutputDebugStringA("WM_PAINT\n");

        // BeginPaint ci restituisce un oggetto importante: DeviceContext
        // It acts as a wrapper that combines:
        // - The Drawing Canvas (Where to draw: a window's display area,
        //   an off-screen bitmap, or even a printer page).
        // - The Drawing Attributes (How to draw: current brush color,
        //   pen thickness, font, clipping region, and background mode).
        //
        // Inoltre, la sottostruttura rcPaint rappresenta la sola zona dirty
        // che deve essere ridisegnata e non l'intera finestra (per questa
        // ha anche delle coordinate X,Y come punto d'inizio)
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        int X = paint.rcPaint.left;
        int Y = paint.rcPaint.top;
        int width = paint.rcPaint.right - paint.rcPaint.left;
        int height = paint.rcPaint.bottom - paint.rcPaint.top;

        // getClientRect(), diversametne da rcPaint, restituisce le dimensioni
        // totali dell'area interna della finestra (escludendo i bordi, la
        // barra del titolo e i menu) e non solo la zona dirty
        RECT clientRect;
        GetClientRect(window, &clientRect);
        win32UpdateWindow(deviceContext, &clientRect, X, Y, width, height);
        EndPaint(window, &paint);
    }
    break;
    default:
    {
        // callback di default di windows per gestire
        // messaggi che non mi interessano
        result = DefWindowProc(window, message, wParam, lParam);
    }
    }

    return result;
}

int CALLBACK WinMain(
    HINSTANCE instance,
    HINSTANCE prevInstance,
    LPSTR cmdLine,
    int showCmd)
{
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = win32MainWindowCallback;
    windowClass.hInstance = instance;
    // windowClass.hIcon;
    windowClass.lpszClassName = "myWindowClass";

    // registerClass() registra le informazioni sulla
    // finestra in uno stato conservato dal codice di
    // windows. Quest'ultimo può quindi, ad esempio,
    // invocare automaticamente la callback senza passare
    // per dispatchMessage() per messaggi "importanti".
    //
    // In sostanza windows può chiamare la mia callback
    // ogni volta che facciamo un windows call, di
    // conseguenza quest'ultima deve essere registrata
    // da qualche parte che conosce.
    if (RegisterClassA(&windowClass))
    {
        HWND windowHandle = CreateWindowExA(
            0,
            windowClass.lpszClassName,
            "handmade hero",
            WS_VISIBLE | WS_OVERLAPPEDWINDOW,
            // apri la finestra alle coordinate che preferisci
            // e con le dimensioni che preferisci
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            instance,
            0);

        if (windowHandle)
        {
            int XOffset = 0;
            int YOffset = 0;

            // ciclo che recupera i messaggi (eventi) associati alla
            // finestra da una message queue popolata da windows
            running = true;
            while (running)
            {
                // Se la funzione recupera un messaggio diverso da WM_QUIT,
                // il valore restituito è diverso da zero.  Se la funzione
                // recupera il messaggio WM_QUIT, il valore restituito è zero.
                // Se si verifica un errore, il valore restituito è -1.
                // NB: i messaggi non devono necessariamente essere associati
                // alla finestra (e.g. quitMessage)
                MSG message;
                while (PeekMessage(&message, 0, 0, 0, PM_REMOVE))
                {
                    if (message.message == WM_QUIT)
                        running = false;

                    TranslateMessage(&message); // for keyboard messages
                    DispatchMessage(&message);  // invoca la callback
                }

                renderGradient(XOffset, YOffset);

                HDC deviceContext = GetDC(windowHandle);
                RECT clientRect;
                GetClientRect(windowHandle, &clientRect);
                int width = clientRect.right - clientRect.left;
                int height = clientRect.bottom - clientRect.top;
                win32UpdateWindow(deviceContext, &clientRect, 0, 0, width, height);
                ReleaseDC(windowHandle, deviceContext);

                XOffset++;
                YOffset++;
            }
        }
        else
        {
            // TODO: error handling
        }
    }
    else
    {
        // TODO: error handling
    }

    return 0;
}