
#ifndef ITKIOMesh_EXPORT_H
#define ITKIOMesh_EXPORT_H

#ifdef ITK_STATIC
#  define ITKIOMesh_EXPORT
#  define ITKIOMesh_TEMPLATE_EXPORT
#  define ITKIOMesh_HIDDEN
#else
#  ifndef ITKIOMesh_EXPORT
#    ifdef ITKIOMesh_EXPORTS
        /* We are building this library */
#      define ITKIOMesh_EXPORT 
#    else
        /* We are using this library */
#      define ITKIOMesh_EXPORT 
#    endif
#  endif

#  ifndef ITKIOMesh_TEMPLATE_EXPORT
#    ifdef ITKIOMesh_EXPORTS
        /* We are building this library */
#      define ITKIOMesh_TEMPLATE_EXPORT 
#    else
        /* We are using this library */
#      define ITKIOMesh_TEMPLATE_EXPORT 
#    endif
#  endif

#  ifndef ITKIOMesh_HIDDEN
#    define ITKIOMesh_HIDDEN 
#  endif
#endif

#ifndef ITKIOMESH_DEPRECATED
#  define ITKIOMESH_DEPRECATED __declspec(deprecated)
#endif

#ifndef ITKIOMESH_DEPRECATED_EXPORT
#  define ITKIOMESH_DEPRECATED_EXPORT ITKIOMesh_EXPORT ITKIOMESH_DEPRECATED
#endif

#ifndef ITKIOMESH_DEPRECATED_NO_EXPORT
#  define ITKIOMESH_DEPRECATED_NO_EXPORT ITKIOMesh_HIDDEN ITKIOMESH_DEPRECATED
#endif

#define DEFINE_NO_DEPRECATED 0
#if DEFINE_NO_DEPRECATED
# define ITKIOMESH_NO_DEPRECATED
#endif

#endif
