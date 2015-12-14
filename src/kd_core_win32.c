#include <KD/kd.h>
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4668 )
#pragma warning( disable: 4820 )
#pragma warning( disable: 4255 )
#pragma warning( disable: 4820 )
#endif /* _MSC_VER */
#include <tchar.h>
#include <windows.h>
#include <stdlib.h>
#ifdef _MSC_VER
#pragma warning( pop )

#pragma warning( disable: 4710 )
#endif /* _MSC_VER */

struct KDWindow {
    struct KDWindow *next;
    HWND hwindow;
    EGLDisplay display;
    EGLConfig config;
    int width; /**< Width of window's client area */
    int height; /**< Height of window's client area */
    const char *caption;
    int visible;
    int focused;
    void *userptr;
};

static KDWindow *window_list = NULL;

static const WCHAR *MainWindowClassName = L"KD Window Class";
static HINSTANCE AppInstance;

#define MESSAGE_QUEUE_SIZE 200
static KDEvent message_queue[MESSAGE_QUEUE_SIZE] = {0};
static unsigned int queue_head = 0;
static unsigned int queue_tail = 0;
static unsigned int queue_active = 0;

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam);

int CALLBACK WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine, int nCmdShow)
{
    KDint result;
    WNDCLASSEX wc = {0};
    AppInstance = hInstance;
    wc.cbSize = sizeof (WNDCLASSEX);
    wc.hCursor = LoadCursor (NULL, IDC_ARROW);
    wc.hInstance = AppInstance;
    wc.lpfnWndProc = WindowProcedure;
    wc.lpszClassName = MainWindowClassName;
    wc.style = CS_OWNDC;
    if ( RegisterClassExW (&wc) == 0 ) {
        return 1;
    }
    result = kdMain (__argc, (const KDchar * const *)__argv);
    UnregisterClass (MainWindowClassName, AppInstance);
    return result;
}

LPWSTR get_unicode (const KDchar *string)
{
    LPWSTR UnicodeString = NULL;
    int UnicodeStringLength = MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS,
                              string, -1, UnicodeString, 0);
    UnicodeString = (LPWSTR) kdMalloc (sizeof (WCHAR) * UnicodeStringLength);
    MultiByteToWideChar (CP_UTF8, MB_ERR_INVALID_CHARS, string, -1, UnicodeString,
                         UnicodeStringLength);
    return UnicodeString;
}

KD_API void KD_APIENTRY kdLogMessage (const KDchar *string)
{
    LPWSTR UnicodeString = get_unicode (string);
    OutputDebugStringW (UnicodeString);
    OutputDebugStringW (L"\n");
    kdFree (UnicodeString);
}

KD_API void KD_APIENTRY kdSetError (KDint error)
{
}

KD_API void *KD_APIENTRY kdMalloc (KDsize size)
{
    return malloc (size);
}

KD_API void KD_APIENTRY kdFree (void *ptr)
{
    free (ptr);
}

KD_API void *KD_APIENTRY kdMemcpy (void *buf, const void *src, KDsize len)
{
    return memcpy (buf, src, len);
}

KD_API KDust KD_APIENTRY kdGetTimeUST (void)
{
    return -1;
}

KD_API KDWindow *KD_APIENTRY kdCreateWindow (EGLDisplay display,
        EGLConfig config, void *eventuserptr)
{
    KDWindow *window = (KDWindow *) kdMalloc (sizeof (KDWindow));
    if (window == NULL) {
        kdSetError (KD_ENOMEM);
        return KD_NULL;
    }
    window->userptr = (eventuserptr) ? eventuserptr : window;
    window->width = CW_USEDEFAULT;
    window->height = CW_USEDEFAULT;
    window->caption = NULL;
    window->visible = 1;
    window->focused = 1;
    window->hwindow = NULL;
    window->display = display;
    window->config = config;
    window->next = window_list;
    window_list = window;
    return window;
}

KD_API KDint KD_APIENTRY kdSetWindowPropertyiv (KDWindow *window, KDint pname,
        const KDint32 *param)
{
    KDWindow *current = window_list;
    while (current != NULL && current != window) {
        current = current->next;
    }
    if (current != NULL && pname == KD_WINDOWPROPERTY_SIZE) {
        window->width = param[0];
        window->height = param[1];
        if (window->hwindow) {
            RECT window_size = {0};
            DWORD dwStyle = GetWindowLong (window->hwindow, GWL_STYLE);
            window_size.right = window->width;
            window_size.bottom = window->height;
            AdjustWindowRect (&window_size, dwStyle, FALSE);
            SetWindowPos (window->hwindow, 0, 0, 0,
                          window_size.right - window_size.left,
                          window_size.bottom - window_size.top,
                          SWP_NOZORDER | SWP_NOMOVE | SWP_NOACTIVATE);
        }
        return 0;
    }
    kdSetError (KD_EINVAL);
    return -1;
}

KD_API KDint KD_APIENTRY kdSetWindowPropertycv (KDWindow *window, KDint pname,
        const KDchar *param)
{
    KDWindow *current = window_list;
    while (current != NULL && current != window) {
        current = current->next;
    }
    if (current != NULL && pname == KD_WINDOWPROPERTY_CAPTION) {
        window->caption = param;
        if (window->hwindow) {
            LPWSTR caption = get_unicode (window->caption);
            SetWindowTextW (window->hwindow, caption);
            kdFree (caption);
        }
        return 0;
    }
    kdSetError (KD_EINVAL);
    return -1;
}

void enqueue_event (KDEvent *new_event)
{
    kdMemcpy (&message_queue[queue_tail], new_event, sizeof (KDEvent));
    queue_tail = (queue_tail + 1) % MESSAGE_QUEUE_SIZE;
    if (queue_active < MESSAGE_QUEUE_SIZE) {
        queue_active++;
    } else {
        queue_head = (queue_head + 1) % MESSAGE_QUEUE_SIZE;
    }
}

KDEvent *dequeue_event (void)
{
    KDEvent *ev;
    if (!queue_active) {
        return NULL;
    }
    ev = &message_queue[queue_head];
    queue_head = (queue_head + 1) % MESSAGE_QUEUE_SIZE;
    queue_active--;
    return ev;
}

LRESULT CALLBACK WindowProcedure (HWND hwnd, UINT uMsg, WPARAM wParam,
                                  LPARAM lParam)
{
    KDWindow *window = window_list;
    KDEvent ev = {0};
    LRESULT result = DefWindowProc (hwnd, uMsg, wParam, lParam);
    while (window != NULL && window->hwindow != hwnd) {
        window = window->next;
    }
    if (window == NULL) {
        return result;
    }
    ev.timestamp = kdGetTimeUST();
    ev.userptr = window->userptr;
    switch (uMsg) {
        case WM_CLOSE:
            /* KD_EVENT_WINDOW_CLOSE */
            ev.type = KD_EVENT_WINDOW_CLOSE;
            break;
        case WM_SIZE:
            /* KD_EVENT_WINDOWPROPERTY_CHANGE size*/
            ev.type = KD_EVENT_WINDOWPROPERTY_CHANGE;
            window->width = LOWORD (lParam);
            window->height = HIWORD (lParam);
            ev.data.windowproperty.pname = KD_WINDOWPROPERTY_SIZE;
            break;
        case WM_SETTEXT:
            /* KD_EVENT_WINDOWPROPERTY_CHANGE caption*/
            ev.type = KD_EVENT_WINDOWPROPERTY_CHANGE;
            ev.data.windowproperty.pname = KD_WINDOWPROPERTY_CAPTION;
            break;
        case WM_PAINT:
            /* KD_EVENT_WINDOW_REDRAW */
            ev.type = KD_EVENT_WINDOW_REDRAW;
            break;
        case WM_KILLFOCUS:
            /* KD_EVENT_WINDOW_FOCUS */
            ev.type = KD_EVENT_WINDOW_FOCUS;
            ev.data.windowfocus.focusstate = 0;
            break;
        case WM_SETFOCUS:
            /* KD_EVENT_WINDOW_FOCUS */
            ev.type = KD_EVENT_WINDOW_FOCUS;
            ev.data.windowfocus.focusstate = 1;
            break;
    }
    /* TODO: call callback if set and return. */
    /* if no callbacks are set, then enquee */
    enqueue_event (&ev);
    return result;
}

KD_API KDint KD_APIENTRY kdRealizeWindow (KDWindow *window,
        EGLNativeWindowType *nativewindow)
{

    DWORD dwStyle;
    DWORD width, height;
    EGLint err;
    LPWSTR caption;
    EGLint pixel_format;
    HDC hDC;
    PIXELFORMATDESCRIPTOR pfd = {0};
    KDWindow *current = window_list;
    while (current != NULL && current != window) {
        current = current->next;
    }
    if (current == NULL || nativewindow == NULL) {
        kdSetError (KD_EINVAL);
        return -1;
    }
    if (window->hwindow) {
        kdSetError (KD_EPERM);
        return -1;
    }

    dwStyle = WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN;
    if (window->width != CW_USEDEFAULT && window->height != CW_USEDEFAULT) {
        RECT window_size = {0};
        window_size.right = window->width;
        window_size.bottom = window->height;
        AdjustWindowRect (&window_size, dwStyle & ~WS_OVERLAPPED, FALSE);
        width = window_size.right - window_size.left;
        height = window_size.bottom - window_size.top;
    } else {
        width = window->width;
        height = window->height;
    }
    if (window->caption != NULL) {
        caption = get_unicode (window->caption);
    } else {
        caption = (LPWSTR) window->caption;
    }

    window->hwindow = CreateWindowEx (0, MainWindowClassName, caption,
                                      dwStyle, CW_USEDEFAULT, CW_USEDEFAULT,
                                      width, height, 0, 0, AppInstance, 0);
    kdFree (caption);
    if (window->visible) {
        ShowWindow (window->hwindow, SW_SHOWNORMAL);
        window->focused = 1;
    }

    err = eglGetConfigAttrib (window->display, window->config, EGL_NATIVE_VISUAL_ID,
                              &pixel_format);
    if (err == EGL_FALSE) {
        DestroyWindow (window->hwindow);
        kdSetError (KD_ENOMEM);
        return -1;
    }
    hDC = GetDC (window->hwindow);
    DescribePixelFormat (hDC, pixel_format, sizeof (PIXELFORMATDESCRIPTOR), &pfd);
    if (!SetPixelFormat (hDC, pixel_format, &pfd)) {
        ReleaseDC (window->hwindow, hDC);
        DestroyWindow (window->hwindow);
        kdSetError (KD_ENOMEM);
        return -1;
    }
    ReleaseDC (window->hwindow, hDC);
    *nativewindow = window->hwindow;
    return 0;
}

KD_API void KD_APIENTRY kdDefaultEvent (const KDEvent *event)
{
}

KD_API const KDEvent *KD_APIENTRY kdWaitEvent (KDust timeout)
{
    KDEvent *ev;
    MSG msg;
    do {
        while (PeekMessage (&msg, 0, 0, 0, PM_REMOVE)) {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }
        ev = dequeue_event();
        if (ev != NULL) {
            return ev;
        }
        if (timeout == 0) {
            kdSetError (KD_EAGAIN);
            return KD_NULL;
        }
    } while (WaitMessage());
    kdSetError (KD_EAGAIN);
    return KD_NULL;
}

KD_API KDint KD_APIENTRY kdDestroyWindow (KDWindow *window)
{
    KDWindow **current;
    for (current = &window_list; *current; current = & (*current)->next) {
        if (*current == window && (window->hwindow != NULL)) {
            *current = window->next;
            DestroyWindow (window->hwindow);
            kdFree (window);
            return 0;
        }
    }
    kdSetError (KD_EINVAL);
    return -1;
}
