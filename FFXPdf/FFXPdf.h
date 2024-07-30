#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(FFXPDF_LIB)
#  define FFXPDF_EXPORT Q_DECL_EXPORT
# else
#  define FFXPDF_EXPORT Q_DECL_IMPORT
# endif
#else
# define FFXPDF_EXPORT
#endif
