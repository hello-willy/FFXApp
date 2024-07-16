#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(FFXCORE_LIB)
#  define FFXCORE_EXPORT Q_DECL_EXPORT
# else
#  define FFXCORE_EXPORT Q_DECL_IMPORT
# endif
#else
# define FFXCORE_EXPORT
#endif