#include <KD/kd.h>
#include <stdlib.h>
#include <stdio.h>

struct KDWindow {
    struct KDWindow *next;
    Atom wm_delete_window; /**< Atom to receive "window closed" message */
    Window xwindow; /**< Native X11 window */
    EGLDisplay display;
    EGLConfig config;
    int width; /**< Width of window's client area */
    int height; /**< Height of window's client area */
    char *caption;
    int visible;
    int focused;
    void *userptr;
};

/** The name the program was run with */
static const char *program_name;

static Display *x11_display = NULL;

static KDWindow *window_list = NULL;

static KDEvent last_event;

static KDint last_error;

int main (int argc, char *argv[])
{
    KDint result;
    program_name = argv[0];
    x11_display = XOpenDisplay (NULL);
    result = kdMain (argc, (const KDchar * const *)argv);
    XCloseDisplay (x11_display);
    return result;
}

KD_API void KD_APIENTRY kdLogMessage (const KDchar *string)
{
    fprintf (stderr, "%s: %s\n", program_name, string);
}

KD_API void KD_APIENTRY kdSetError (KDint error)
{
    last_error = error;
}

KD_API void *KD_APIENTRY kdMalloc (KDsize size)
{
    return malloc (size);
}
KD_API void KD_APIENTRY kdFree (void *ptr)
{
    free (ptr);
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
    window->width = 640;
    window->height = 480;
    window->caption = NULL;
    window->visible = 1;
    window->focused = 1;
    window->xwindow = 0;
    window->wm_delete_window = 0;
    window->display = display;
    window->config = config;
    window->next = window_list;
    window_list = window;
    return window;
}

static XVisualInfo *get_visualinfo (EGLDisplay display, EGLConfig config)
{
    EGLBoolean err;
    EGLint visual_id;
    XVisualInfo *info = KD_NULL;
    err = eglGetConfigAttrib (display, config, EGL_NATIVE_VISUAL_ID, &visual_id);
    if (err == EGL_TRUE) {
        XVisualInfo info_template;
        int n_visuals;
        info_template.visualid = (VisualID)visual_id;
        info = XGetVisualInfo (x11_display, VisualIDMask, &info_template, &n_visuals);
    }
    return info;
}

KD_API KDint KD_APIENTRY kdRealizeWindow (KDWindow *window,
        EGLNativeWindowType *nativewindow)
{
    XSetWindowAttributes window_attributes;
    XVisualInfo *info;
    Window root;
    const unsigned long attributes_mask = CWBorderPixel | CWColormap |
                                          CWEventMask;
    if (window == NULL || nativewindow == NULL) {
        kdSetError (KD_EINVAL);
        return -1;
    }
    if (window->xwindow) {
        kdSetError (KD_EPERM);
        return -1;
    }
    if ((info = get_visualinfo (window->display, window->config)) == NULL) {
        kdSetError (KD_ENOMEM);
        return -1;
    }
    root = RootWindow (x11_display, info->screen);
    window_attributes.colormap = XCreateColormap (x11_display, root,
                                 info->visual, AllocNone);
    window_attributes.background_pixmap = None;
    window_attributes.border_pixel = 0;
    window_attributes.event_mask = StructureNotifyMask;
    window->xwindow = XCreateWindow (x11_display, root, 0, 0, 640, 480,
                                     0, info->depth, InputOutput,
                                     info->visual, attributes_mask,
                                     &window_attributes);
    if (window->caption != NULL) {
        XStoreName (x11_display, window->xwindow, window->caption);
    }
    if (window->visible) {
        XMapWindow (x11_display, window->xwindow);
    }
    window->wm_delete_window = XInternAtom (x11_display, "WM_DELETE_WINDOW",
                                            False);
    XSetWMProtocols (x11_display, window->xwindow,
                     &window->wm_delete_window, 1);
    *nativewindow = window->xwindow;
    XFree (info);
    return 0;
}

KD_API KDint KD_APIENTRY kdDestroyWindow (KDWindow *window)
{
    KDWindow **current;
    for (current = &window_list; *current; current = & (*current)->next) {
        if (*current == window && (window->xwindow != 0)) {
            *current = window->next;
            XDestroyWindow (x11_display, window->xwindow);
            kdFree (window);
            return 0;
        }
    }
    kdSetError (KD_EINVAL);
    return -1;
}

KD_API KDint KD_APIENTRY kdSetWindowPropertyiv (KDWindow *window, KDint pname,
        const KDint32 *param)
{
    return -1;
}

KD_API KDint KD_APIENTRY kdSetWindowPropertycv (KDWindow *window, KDint pname,
        const KDchar *param)
{
    return -1;
}

KD_API void KD_APIENTRY kdDefaultEvent (const KDEvent *event)
{
}

KD_API const KDEvent *KD_APIENTRY kdWaitEvent (KDust timeout)
{
    int n_events = XPending (x11_display);
    while (((timeout == 0) && (n_events > 0)) || (timeout == -1)) {
        XEvent event;
        KDWindow *window;
        XNextEvent (x11_display, &event);
        for (window = window_list; window != NULL; window = window->next) {
            if (event.xany.window == window->xwindow) {
                last_event.timestamp = kdGetTimeUST ();
                if (event.type == ClientMessage) {
                    if ((Atom)event.xclient.data.l[0] == window->wm_delete_window) {
                        last_event.type = KD_EVENT_WINDOW_CLOSE;
                        last_event.userptr = window->userptr;
                        return &last_event;
                    }
                } else if (event.type == ConfigureNotify) {
                    XConfigureEvent xce;
                    xce = event.xconfigure;
                    if ((xce.width != window->width) || (xce.height != window->height)) {
                        window->width = xce.width;
                        window->height = xce.height;
                        last_event.type = KD_EVENT_WINDOWPROPERTY_CHANGE;
                        last_event.userptr = window->userptr;
                        last_event.data.windowproperty.pname = KD_WINDOWPROPERTY_SIZE;
                        return &last_event;
                    }
                }
            }
        }
        n_events--;
    }
    kdSetError (KD_EAGAIN);
    return KD_NULL;
}
