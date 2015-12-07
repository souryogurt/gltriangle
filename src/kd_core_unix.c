#include <KD/kd.h>
#include <stdlib.h>
#include <stdio.h>

struct KDWindow {
    Atom wm_delete_window; /**< Atom to receive "window closed" message */
    Window xwindow; /**< Native X11 window */
    int width; /**< Width of window's client area */
    int height; /**< Height of window's client area */
};

/** The name the program was run with */
static const char *program_name;

static Display *x11_display = NULL;

static KDEvent default_event;

int main (int argc, char *argv[])
{
    program_name = argv[0];
    return kdMain (argc, (const KDchar * const *)argv);
}

KD_API void KD_APIENTRY kdLogMessage (const KDchar *string)
{
    fprintf (stderr, "%s: %s\n", program_name, string);
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

KD_API KDust KD_APIENTRY kdGetTimeUST (void)
{
    return -1;
}

KD_API KDWindow *KD_APIENTRY kdCreateWindow (EGLDisplay display,
        EGLConfig config, void *eventuserptr)
{
    EGLBoolean err;
    VisualID visual_id;
    int n_visuals;
    XVisualInfo *info = NULL;
    KDWindow *window = KD_NULL;
    XVisualInfo info_template;
    if (x11_display == NULL) {
        x11_display = XOpenDisplay (NULL);
        if (x11_display == NULL ) {
            kdSetError (KD_EPERM);
            return KD_NULL;
        }
    }
    err = eglGetConfigAttrib (display, config, EGL_NATIVE_VISUAL_ID,
                              (EGLint *)&visual_id);
    if (err == EGL_FALSE) {
        kdSetError (KD_EPERM);
        return KD_NULL;
    }
    info_template.visualid = visual_id;
    info = XGetVisualInfo (x11_display, VisualIDMask, &info_template, &n_visuals);
    if (info == NULL) {
        kdSetError (KD_EPERM);
        return KD_NULL;
    }
    window = (KDWindow *) kdMalloc (sizeof (KDWindow));
    if (window != KD_NULL) {
        const unsigned long attributes_mask = CWBorderPixel | CWColormap |
                                              CWEventMask;
        XSetWindowAttributes window_attributes;
        Window root = RootWindow (x11_display, info->screen);
        window->width = 0;
        window->height = 0;
        window_attributes.colormap = XCreateColormap (x11_display, root,
                                     info->visual, AllocNone);
        window_attributes.background_pixmap = None;
        window_attributes.border_pixel = 0;
        window_attributes.event_mask = StructureNotifyMask;
        window->xwindow = XCreateWindow (x11_display, root, 0, 0, 640, 480,
                                         0, info->depth, InputOutput,
                                         info->visual, attributes_mask,
                                         &window_attributes);
        window->wm_delete_window = XInternAtom (x11_display, "WM_DELETE_WINDOW",
                                                False);
        XSetWMProtocols (x11_display, window->xwindow,
                         &window->wm_delete_window, 1);
    }
    XFree (info);
    return window;
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

KD_API KDint KD_APIENTRY kdRealizeWindow (KDWindow *window,
        EGLNativeWindowType *nativewindow)
{
    if (window != NULL) {
        XMapWindow (x11_display, window->xwindow);
        *nativewindow = window->xwindow;
        return 0;
    }
    return -1;
}

KD_API void KD_APIENTRY kdDefaultEvent (const KDEvent *event)
{
}

KD_API const KDEvent *KD_APIENTRY kdWaitEvent (KDust timeout)
{
    return KD_NULL;
}

KD_API KDint KD_APIENTRY kdDestroyWindow (KDWindow *window)
{
    if (window != NULL) {
        XDestroyWindow (x11_display, window->xwindow);
        kdFree (window);
        return 0;
    }
    return -1;
}
