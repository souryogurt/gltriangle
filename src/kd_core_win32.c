#include <KD/kd.h>
#ifdef _MSC_VER
#pragma warning( push )
#pragma warning( disable: 4668 )
#pragma warning( disable: 4820 )
#pragma warning( disable: 4255 )
#endif /* _MSC_VER */
#include <tchar.h>
#include <windows.h>
#include <stdlib.h>
#ifdef _MSC_VER
#pragma warning( pop )

#pragma warning( disable: 4710 )
#endif /* _MSC_VER */

int CALLBACK WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
                      LPSTR lpCmdLine, int nCmdShow)
{
    return kdMain (__argc, (const KDchar * const *)__argv);
}

KD_API void KD_APIENTRY kdLogMessage (const KDchar *string)
{
}

KD_API void KD_APIENTRY kdSetError (KDint error)
{
}

KD_API void *KD_APIENTRY kdMalloc (KDsize size)
{
    return KD_NULL;
}

KD_API void KD_APIENTRY kdFree (void *ptr)
{
}

KD_API KDust KD_APIENTRY kdGetTimeUST (void)
{
    return -1;
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
