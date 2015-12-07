#include <KD/kd.h>
#include <stdlib.h>
#include <stdio.h>

/** The name the program was run with */
static const char *program_name;

int main (int argc, char *argv[])
{
    program_name = argv[0];
    return kdMain (argc, (const KDchar * const *)argv);
}

KD_API void KD_APIENTRY kdLogMessage (const KDchar *string)
{
    fprintf (stderr, "%s: %s\n", program_name, string);
}

KD_API KDWindow *KD_APIENTRY kdCreateWindow (EGLDisplay display,
        EGLConfig config, void *eventuserptr)
{
    return KD_NULL;
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
    return -1;
}
