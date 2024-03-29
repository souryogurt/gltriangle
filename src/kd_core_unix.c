#include <KD/kd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

struct KDWindow {
    struct KDWindow *next;
    Atom wm_delete_window; /**< Atom to receive "window closed" message */
    Window xwindow; /**< Native X11 window */
    EGLDisplay display;
    EGLConfig config;
    int width; /**< Width of window's client area */
    int height; /**< Height of window's client area */
    const char *caption;
    int visible;
    int focused;
    void *userptr;
};

/** The name the program was run with */
static const char *program_name;

static Display *x11_display = NULL;

static KDWindow *window_list = NULL;

#define MESSAGE_QUEUE_SIZE 200
static KDEvent message_queue[MESSAGE_QUEUE_SIZE];
static unsigned int queue_head = 0;
static unsigned int queue_tail = 0;
static unsigned int queue_active = 0;

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
    KDWindow *current = window_list;
    while (current != NULL && current != window) {
        current = current->next;
    }
    if (current == NULL || nativewindow == NULL) {
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
    KDWindow *current = window_list;
    while (current != NULL && current != window) {
        current = current->next;
    }
    if (current != NULL && pname == KD_WINDOWPROPERTY_SIZE) {
        window->width = param[0];
        window->height = param[1];
        if (window->xwindow) {
            XResizeWindow (x11_display, window->xwindow,
                           (unsigned int) window->width,
                           (unsigned int) window->height);
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
        if (window->xwindow) {
            XStoreName (x11_display, window->xwindow, window->caption);
        }
        return 0;
    }
    kdSetError (KD_EINVAL);
    return -1;
}

static void enqueue_event (KDEvent *new_event)
{
    kdMemcpy (&message_queue[queue_tail], new_event, sizeof (KDEvent));
    queue_tail = (queue_tail + 1) % MESSAGE_QUEUE_SIZE;
    if (queue_active < MESSAGE_QUEUE_SIZE) {
        queue_active++;
    } else {
        queue_head = (queue_head + 1) % MESSAGE_QUEUE_SIZE;
    }
}

static KDEvent *dequeue_event (void)
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

KD_API void KD_APIENTRY kdDefaultEvent (const KDEvent *event)
{
}

static void window_procedure (const XEvent *event)
{
    XConfigureEvent xce;
    KDWindow *window = window_list;
    KDEvent ev = {0};
    while (window != NULL && window->xwindow != event->xany.window) {
        window = window->next;
    }
    if (window == NULL) {
        return;
    }
    ev.timestamp = kdGetTimeUST();
    ev.userptr = window->userptr;
    switch (event->type) {
        case ClientMessage:
            if ((Atom)event->xclient.data.l[0] == window->wm_delete_window) {
                ev.type = KD_EVENT_WINDOW_CLOSE;
            } else {
                return;
            }
            break;
        case ConfigureNotify:
            xce = event->xconfigure;
            if ((xce.width != window->width) || (xce.height != window->height)) {
                window->width = xce.width;
                window->height = xce.height;
                ev.type = KD_EVENT_WINDOWPROPERTY_CHANGE;
                ev.data.windowproperty.pname = KD_WINDOWPROPERTY_SIZE;
            } else {
                return;
            }
            break;
        default:
            return;
    }
    /* TODO: call callback if set and return. */
    /* if no callbacks are set, then enquee */
    enqueue_event (&ev);
}

KD_API const KDEvent *KD_APIENTRY kdWaitEvent (KDust timeout)
{
    while (1) {
        KDEvent *ev;
        XEvent event;
        int n_events = XPending (x11_display);
        while (n_events > 0) {
            XNextEvent (x11_display, &event);
            window_procedure (&event);
            n_events--;
        }
        ev = dequeue_event();
        if (ev != NULL) {
            return ev;
        }
        if (timeout == 0) {
            kdSetError (KD_EAGAIN);
            return KD_NULL;
        }
        /* Wait for event */
        XPeekEvent (x11_display, &event);
    }
    kdSetError (KD_EAGAIN);
    return KD_NULL;
}
