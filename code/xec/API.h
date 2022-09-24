// Copyright (c) Borislav Stanimirov
// SPDX-License-Identifier: MIT
//
#pragma once
#include <splat/symbol_export.h>

#if XEC_SHARED
#   if BUILDING_XEC
#       define XEC_API SYMBOL_EXPORT
#   else
#       define XEC_API SYMBOL_IMPORT
#   endif
#else
#   define XEC_API
#endif
