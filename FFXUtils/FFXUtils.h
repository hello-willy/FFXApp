#pragma once

#include <QtCore/qglobal.h>

#ifndef BUILD_STATIC
# if defined(FFXUTILS_LIB)
#  define FFXUTILS_EXPORT Q_DECL_EXPORT
# else
#  define FFXUTILS_EXPORT Q_DECL_IMPORT
# endif
#else
# define FFXUTILS_EXPORT
#endif