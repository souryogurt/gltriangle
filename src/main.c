#include <KD/kd.h>
#include <EGL/egl.h>

KDint kdMain (KDint argc, const KDchar *const *argv)
{
    const EGLint attributes[] = {
        EGL_RED_SIZE, 8,
        EGL_GREEN_SIZE, 8,
        EGL_BLUE_SIZE, 8,
        EGL_ALPHA_SIZE, 8,
        EGL_DEPTH_SIZE, 16,
        EGL_STENCIL_SIZE, 8,
        EGL_CONFORMANT, EGL_OPENGL_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_BIT,
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_NONE
    };
    EGLDisplay display;
    EGLint egl_major, egl_minor;
    EGLBoolean err;
    EGLConfig config;
    EGLint n_configs;
    EGLContext context;
    KDWindow *window = KD_NULL;
    KDint32 win_size[] = {640, 480};
    EGLNativeWindowType nativewindow;
    EGLSurface window_surface;
    const KDEvent *event;

    display = eglGetDisplay (EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        kdLogMessage ("No matching EGL display is available");
        goto _EXIT_APP;
    }
    if (eglInitialize (display, &egl_major, &egl_minor) != EGL_TRUE) {
        kdLogMessage ("Can't initialize EGL on a display");
        goto _EXIT_APP;
    }
    err = eglChooseConfig (display, attributes, &config, 1, &n_configs);
    if ((err != EGL_TRUE) || (n_configs == 0)) {
        kdLogMessage ("Can't retrieve a framebuffer config");
        goto _EXIT_APP;
    }
    err = eglBindAPI (EGL_OPENGL_API);
    if (err != EGL_TRUE) {
        kdLogMessage ("Can't bind OpenGL API");
        goto _EXIT_APP;
    }
    context = eglCreateContext (display, config, EGL_NO_CONTEXT, NULL);
    if (context == EGL_NO_CONTEXT) {
        kdLogMessage ("Can't create OpenGL context");
        goto _EXIT_APP;
    }
    window = kdCreateWindow (display, config, KD_NULL);
    if (window == KD_NULL) {
        kdLogMessage ("Can't create window");
        goto _EXIT_APP;
    }

    kdSetWindowPropertycv (window, KD_WINDOWPROPERTY_CAPTION, "OpenGL window");
    kdSetWindowPropertyiv (window, KD_WINDOWPROPERTY_SIZE, win_size);

    if (kdRealizeWindow (window, &nativewindow) != 0) {
        kdLogMessage ("Can't realize the window");
        goto _EXIT_APP;
    }

    window_surface = eglCreateWindowSurface (display, config, nativewindow, NULL);
    if (window_surface == EGL_NO_SURFACE) {
        kdLogMessage ("Can't create rendering surface");
        goto _EXIT_APP;
    }

    err = eglMakeCurrent (display, window_surface, window_surface, context);
    if (err == EGL_FALSE) {
        kdLogMessage ("Can't make OpenGL context be current");
        goto _EXIT_APP;
    }

    while ((event = kdWaitEvent (0)) != 0) {
        kdDefaultEvent (event);
        eglSwapBuffers (display, window_surface);
    }

    eglMakeCurrent (display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
_EXIT_APP:
    eglDestroySurface (display, window_surface);
    kdDestroyWindow (window);
    eglDestroyContext (display, context);
    eglTerminate (display);
    return 0;
}
