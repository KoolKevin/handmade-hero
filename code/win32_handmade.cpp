#include <windows.h>
#include <stdint.h>
#include <dsound.h>
#include <math.h>

#define global_variable static
#define local_persistent static
#define internal static
#define PI 3.1415926f

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

struct win32OffscreenBuffer {
    BITMAPINFO info;
    void* memory;
    int width;
    int height;
    int bytesPerPixel;
    int stride;
};

struct win32WindowDimension {
    int width;
    int height;
};

struct win32SoundOutput {
    int seconds;
    int sampleRate;
    int bytesPerSample;
    int secondaryBufferSize;
    uint32 runningSampleIndex;
    int toneHz;
    int16 toneVolume;
    int wavePeriod;
};


// globals aren't that much of a problem if
// - you understand why you're using a global
// - the global should stay global
// - there is only one of them
global_variable win32OffscreenBuffer globalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER secondaryBuffer;
global_variable bool running;
global_variable int XOffset;
global_variable int YOffset;

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter);
typedef DIRECT_SOUND_CREATE(direct_sound_create);
#define DirectSoundCreate DirectSoundCreate_

internal void win32InitDSound(HWND window, int32 samplesPerSec, int32 bufferSize) {
    // carichiamo la libreria DirectSound dinamicamente. In questo modo, se un
    // utente non ha la libreria possiamo gestire la situazione e permettergli
    // di giocare comunque al gioco
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");
    if (DSoundLibrary) {
        direct_sound_create* DirectSoundCreate =
            (direct_sound_create*)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        LPDIRECTSOUND DirectSound;
        if (DirectSoundCreate &&  SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
            WAVEFORMATEX waveFormat = {};
            waveFormat.wFormatTag = WAVE_FORMAT_PCM;
            waveFormat.nChannels = 2; // le halfword risiederanno in memoria così: L R L R ...
            waveFormat.nSamplesPerSec = samplesPerSec;
            waveFormat.wBitsPerSample = 16;
            waveFormat.nBlockAlign = (waveFormat.nChannels * waveFormat.wBitsPerSample) / 8;
            waveFormat.nAvgBytesPerSec = waveFormat.nSamplesPerSec * waveFormat.nBlockAlign;
            waveFormat.cbSize = 0;

            // creiamo due buffer perchè "windows is weird". Il primo è più una handle
            // alla scheda audio per configurazione (setFormat()). Il secondo è il buffer
            // vero e proprio in cui scriviamo i dati da suonare
            if(SUCCEEDED(DirectSound->SetCooperativeLevel(window, DSSCL_PRIORITY))) {
                // create primary buffer (bugia, non è un buffer, solo una handle
                // per configurare la scheda audio specificando il sound format
                // del audio che vogliamo suonare)
                DSBUFFERDESC bufferDescription = {};
                bufferDescription.dwSize = sizeof(bufferDescription);
                // special-secret buffer that's really a handle
                bufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER; 
                LPDIRECTSOUNDBUFFER primaryBuffer;
                if(SUCCEEDED(DirectSound->CreateSoundBuffer(&bufferDescription, &primaryBuffer, 0))) {
                    if(SUCCEEDED(primaryBuffer->SetFormat(&waveFormat))) {
                        // abbiamo finalmente creato il primary buffer!
                        OutputDebugStringA("settato il formato del primary buffer\n");
                    } else {
                        // TODO: error
                    }
                } else {
                    // TODO: error
                }
            } else {
                // TODO: error
            }

            // create secondary buffer, the one where we actually write the
            // stuff we want to play
            DSBUFFERDESC bufferDescription = {};
            bufferDescription.dwSize = sizeof(bufferDescription);
            bufferDescription.dwFlags = 0;
            bufferDescription.dwBufferBytes = bufferSize;
            bufferDescription.lpwfxFormat = &waveFormat;
            if (SUCCEEDED(DirectSound->CreateSoundBuffer(&bufferDescription, &secondaryBuffer, 0))) {
                OutputDebugStringA("creato il secondary buffer\n");
            }
            else
            {
                // TODO: error
            }
        } else {
            // TODO: error
        }
    } else {
        // TODO: error
    }
}

internal void win32FillSoundBuffer(win32SoundOutput* soundOutput, DWORD byteToLock, DWORD bytesToWrite) {
    VOID *region1;
    DWORD region1Size;
    VOID *region2;
    DWORD region2Size;
    if (SUCCEEDED(secondaryBuffer->Lock(
            byteToLock, bytesToWrite,
            &region1, &region1Size,
            &region2, &region2Size,
            0)))
    {
        int16 *sampleOut = (int16 *)region1;
        DWORD region1Samplecount = region1Size / soundOutput->bytesPerSample;
        for (DWORD sampleIndex = 0; sampleIndex < region1Samplecount; sampleIndex++)
        {
            float t = 2.0f * PI *
                      ((float)soundOutput->runningSampleIndex / (float)soundOutput->wavePeriod);
            float sineValue = sinf(t);
            int16 sampleValue = (int16)(sineValue * soundOutput->toneVolume);

            *sampleOut = sampleValue; // left
            sampleOut++;
            *sampleOut = sampleValue; // right
            sampleOut++;

            soundOutput->runningSampleIndex++;
        }

        sampleOut = (int16 *)region2;
        DWORD region2Samplecount = region2Size / soundOutput->bytesPerSample;
        for (DWORD sampleIndex = 0; sampleIndex < region2Samplecount; sampleIndex++)
        {
            float t = 2.0f * PI *
                      ((float)soundOutput->runningSampleIndex / (float)soundOutput->wavePeriod);
            float sineValue = sinf(t);
            int16 sampleValue = (int16)(sineValue * soundOutput->toneVolume);

            *sampleOut = sampleValue; // left
            sampleOut++;
            *sampleOut = sampleValue; // right
            sampleOut++;

            soundOutput->runningSampleIndex++;
        }

        secondaryBuffer->Unlock(region1, region1Size, region2, region2Size);
    }
}

internal win32WindowDimension win32GetWindowDimension(HWND window) {
    win32WindowDimension dims;

    // getClientRect(), diversametne da rcPaint, restituisce le dimensioni
    // totali dell'area interna della finestra (escludendo i bordi, la
    // barra del titolo e i menu) e non solo la zona dirty
    RECT clientRect;
    GetClientRect(window, &clientRect);
    dims.width = clientRect.right - clientRect.left;
    dims.height = clientRect.bottom - clientRect.top;

    return dims;
}

internal void
renderGradient(win32OffscreenBuffer buffer, int XOffset, int YOffset)
{
    // good way to write pixel loops. usiamo esplicitamente un
    // row pointer aggiuntivo dato che non è detto che il pixel
    // pointer sia allineato con la prossima riga alla fine del
    // loop interno
    uint8 *row = (uint8 *)buffer.memory;
    for (int Y = 0; Y < buffer.height; Y++)
    {
        uint32 *pixel = (uint32 *)row;
        for (int X = 0; X < buffer.width; X++)
        {
            uint8 red = (uint8)(X + XOffset);
            uint8 green = (uint8)(Y + YOffset);

            // BGR windows pixel layout
            *pixel = red << 16 | green << 8;
            pixel++;
        }

        row += buffer.stride;
    }
}

// DIB == DeviceIndipendentBitmap
//     == buffer in cui scrivere cosa disegnare
internal void
win32ResizeDIBSection(win32OffscreenBuffer* buffer, int width, int height)
{
    if (buffer->memory)
    {
        VirtualFree(buffer->memory, 0, MEM_RELEASE);
    }

    buffer->info.bmiHeader.biSize = sizeof(buffer->info.bmiHeader);
    buffer->info.bmiHeader.biWidth = width;
    buffer->info.bmiHeader.biHeight = -height; // origin in alto a sinistra
    buffer->info.bmiHeader.biPlanes = 1;
    buffer->info.bmiHeader.biBitCount = 32;
    buffer->info.bmiHeader.biCompression = BI_RGB;

    buffer->width = width;
    buffer->height = height;

    // 3 bytes of data + 1 for word alignment
    buffer->bytesPerPixel = 4;
    int bitmapMemorySize = buffer->bytesPerPixel * width * height;
    buffer->memory = VirtualAlloc(0, bitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
    buffer->stride = width * buffer->bytesPerPixel;

    // TODO: probably want to clear this to black
}

internal void
win32CopyBufferToWindow(HDC deviceContext, win32OffscreenBuffer buffer, int windowWidth, int windowHeight)
{
    // TODO: aspect ratio correction
    // TODO: lo stretch fa un po' schifo

    // copia i bit da un buffer e li disegna nel DC
    // applicando opportuno stretch
    int res = StretchDIBits(
        deviceContext,
        0, 0, windowWidth, windowHeight, // dst
        0, 0, buffer.width, buffer.height, // src
        buffer.memory,
        &buffer.info,
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
    switch (message) {
        case WM_SIZE:
        {
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_SYSKEYDOWN:
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP:
        {
            uint32 VKCode = wParam; // VK == Virtual Key
            bool32 wasDown = ((lParam & (1 << 30)) != 0);
            bool32 isDown = ((lParam & (1 << 31)) == 0);
            if (!isDown) {
                break;
            }

            switch(VKCode) {
                case 'W': {
                    YOffset+=10;
                } break;

                case 'A': {
                    XOffset+=10;
                } break;

                case 'S': {
                    YOffset-=10;
                } break;

                case 'D': {
                    XOffset-=10;
                } break;

                case 'Q': {
                } break;

                case 'E': {
                } break;

                case VK_UP: {
                } break;

                case VK_LEFT: {
                } break;

                case VK_DOWN: {
                } break;

                case VK_RIGHT: {
                } break;

                case VK_ESCAPE: {
                } break;

                case VK_SPACE: {
                } break;
            }

            bool32 altWasDown = ((lParam & (1 << 29)));
            if(VKCode == VK_F4 && altWasDown) {
                running = false;
            }
        } break;

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
            win32WindowDimension dims = win32GetWindowDimension(window);
            win32CopyBufferToWindow(deviceContext, globalBackbuffer, dims.width, dims.height);
            EndPaint(window, &paint);
        } break;

        case WM_DESTROY:
        {
            OutputDebugStringA("WM_DESTROY\n");
            running = false;
        } break;

        case WM_CLOSE:
        {
            OutputDebugStringA("WM_CLOSE\n");
            running = false;
        } break;

        case WM_ACTIVATEAPP:
        {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

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
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
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
        HWND window = CreateWindowExA(
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

        if (window)
        {
            // creiamo il nostro backbuffer fisso
            win32ResizeDIBSection(&globalBackbuffer, 1280, 720);

            XOffset = 0;
            YOffset = 0;

            win32SoundOutput soundOutput = {};
            soundOutput.seconds = 1;
            soundOutput.sampleRate = 48000;
            soundOutput.bytesPerSample = sizeof(int16) * 2;
            soundOutput.secondaryBufferSize = soundOutput.sampleRate * soundOutput.bytesPerSample * soundOutput.seconds;
            soundOutput.runningSampleIndex = 0;
            soundOutput.toneHz = 440;
            soundOutput.toneVolume = 6000;
            soundOutput.wavePeriod = soundOutput.sampleRate / soundOutput.toneHz;

            win32InitDSound(window, soundOutput.sampleRate, soundOutput.secondaryBufferSize);
            win32FillSoundBuffer(&soundOutput, 0, soundOutput.secondaryBufferSize);
            secondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);


            // ciclo che recupera i messaggi (eventi) associati alla
            // finestra da una message queue popolata da windows
            running = true;
            while (running) {
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

                renderGradient(globalBackbuffer, XOffset, YOffset);

                HDC deviceContext = GetDC(window);
                win32WindowDimension dims = win32GetWindowDimension(window);
                win32CopyBufferToWindow(deviceContext, globalBackbuffer, dims.width, dims.height);
                ReleaseDC(window, deviceContext);

                // test per directsound output
                DWORD playCursor;
                DWORD writeCursor;
                if (SUCCEEDED(secondaryBuffer->GetCurrentPosition(&playCursor, &writeCursor))) {
                    // write pointer
                    DWORD byteToLock = 
                        (soundOutput.runningSampleIndex*soundOutput.bytesPerSample) % soundOutput.secondaryBufferSize;
                    // we want to write until (and not over) the play cursor
                    DWORD bytesToWrite;
                    if (byteToLock == playCursor)
                        bytesToWrite = 0;
                    else if (byteToLock > playCursor)
                        bytesToWrite = (soundOutput.secondaryBufferSize - byteToLock) + playCursor;
                    else
                        bytesToWrite = playCursor - byteToLock;

                    win32FillSoundBuffer(&soundOutput, byteToLock, bytesToWrite);
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