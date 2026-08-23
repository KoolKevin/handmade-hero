#include <windows.h>

LRESULT CALLBACK MainWindowCallback(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam
) {
    LRESULT result = 0;
    // interessante usare blocchi con switch per
    // non far propagare variabili locali di un
    // case ad altri case
    switch(message) {
        case WM_SIZE: {
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_DESTROY: {
            OutputDebugStringA("WM_DESTROY\n");
        } break;

        case WM_CLOSE: {
            OutputDebugStringA("WM_CLOSE\n");
        } break;

        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        // messaggio inviato quando è necessario ridisegnare la 
        // finestra (e.g. espansione, la spostiamo out-of-view, ...)
        case WM_PAINT: {
            OutputDebugStringA("WM_PAINT\n");

            PAINTSTRUCT paint; 
            HDC DeviceContext = BeginPaint(window, &paint);
            // disegnamo un rettangolo
            int X = paint.rcPaint.left;
            int Y = paint.rcPaint.top;
            int width = paint.rcPaint.right - paint.rcPaint.left; 
            int height = paint.rcPaint.bottom - paint.rcPaint.top; 
            PatBlt(DeviceContext, X, Y, width, height, WHITENESS);
            EndPaint(window, &paint);
        } break;
        default: {
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
    LPSTR     cmdLine,
    int       showCmd
) {
    WNDCLASSA windowClass = {};
    windowClass.lpfnWndProc = MainWindowCallback;
    windowClass.hInstance = instance;
    // windowClass.hIcon;
    windowClass.lpszClassName = "myWindowClass";

    // registerClass() registra le informazioni sulla 
    // finestra in uno stato conservato dal codice di
    // windows. Quest'ultimo può quindi, ad esempio,
    // invocare automaticamente la callback senza passare
    // per il nostro codice che invoca dispatchMessage()
    // per messaggi "importanti". 
    //
    // In sostanza windows può chiamare la tua callback 
    // quando gli pare e quindi quest'ultima deve essere
    // registrata da qualche parte che conosce.
    if(RegisterClassA(&windowClass)) {
        HWND windowHandle = CreateWindowExA(
            0,
            windowClass.lpszClassName,
            "handmade hero",
            WS_VISIBLE|WS_OVERLAPPEDWINDOW,
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
        
        if(windowHandle) {
            // ciclo che recupera i messaggi (eventi) associati alla
            // finestra da una message queue popolata da windows
            MSG message;
            for(;;) {
                BOOL messageResult = GetMessage(&message, 0, 0, 0);
                if (messageResult) {
                    TranslateMessage(&message); // for keyboard messages
                    DispatchMessage(&message); // invoca la callback
                } else {
                    break;
                }
            }
        } else {
            // error handling
        }
    } else {
        // error handling
    }

    return 0;
}