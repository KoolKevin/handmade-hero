#include <windows.h>

#define global_variable static
#define local_persistent static
#define internal static

// TODO: a global for now
global_variable bool running;
global_variable BITMAPINFO bitmapInfo;
global_variable void *bitmapMemory;
global_variable HBITMAP bitmapHandle;
global_variable HDC bitmapDeviceContext;
// DIB == DeviceIndipendentBitmap
//     == buffer in cui scrivere cosa disegnare
internal void
win32ResizeDIBSection(int width, int height)
{
    if (bitmapHandle)
    {
        DeleteObject(bitmapHandle);
    }

    if (!bitmapDeviceContext)
    {
        // passando 0 recuperiamo un DC compatibile con
        // il mio schermo. Non disegneremo sullo schermo
        // ma sulla bitmap in memoria
        bitmapDeviceContext = CreateCompatibleDC(0);
    }

    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = width;
    bitmapInfo.bmiHeader.biHeight = height;
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    // recuperiamo la memoria per la bitmap
    bitmapHandle = CreateDIBSection(
        bitmapDeviceContext,
        &bitmapInfo,
        DIB_RGB_COLORS,
        &bitmapMemory,
        0, 0);
}

internal void
win32UpdateWindow(HDC deviceContext, int X, int Y, int width, int height)
{
    int res = StretchDIBits(
        deviceContext,
        X, Y, width, height,
        X, Y, width, height,
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
        PAINTSTRUCT paint;
        HDC deviceContext = BeginPaint(window, &paint);
        int X = paint.rcPaint.left;
        int Y = paint.rcPaint.top;
        int width = paint.rcPaint.right - paint.rcPaint.left;
        int height = paint.rcPaint.bottom - paint.rcPaint.top;
        win32UpdateWindow(deviceContext, X, Y, width, height);
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
            // ciclo che recupera i messaggi (eventi) associati alla
            // finestra da una message queue popolata da windows
            running = true;
            while (running)
            {
                MSG message;
                // Se la funzione recupera un messaggio diverso da WM_QUIT,
                // il valore restituito è diverso da zero.  Se la funzione
                // recupera il messaggio WM_QUIT, il valore restituito è zero.
                // Se si verifica un errore, il valore restituito è -1.
                // NB: i messaggi non devono necessariamente essere associati
                // alla finestra (e.g. quitMessage)
                BOOL messageResult = GetMessage(&message, 0, 0, 0);
                if (messageResult > 0)
                {
                    TranslateMessage(&message); // for keyboard messages
                    DispatchMessage(&message);  // invoca la callback
                }
                else
                {
                    break;
                }
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