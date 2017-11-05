/*
** dynarray.cpp
**
** internal data types for dynamic arrays
**
**---------------------------------------------------------------------------
** Copyright 2016-2017 Christoph Oelckers
** All rights reserved.
**
** Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions
** are met:
**
** 1. Redistributions of source code must retain the above copyright
**    notice, this list of conditions and the following disclaimer.
** 2. Redistributions in binary form must reproduce the above copyright
**    notice, this list of conditions and the following disclaimer in the
**    documentation and/or other materials provided with the distribution.
** 3. The name of the author may not be used to endorse or promote products
**    derived from this software without specific prior written permission.
** 4. When not used as part of ZDoom or a ZDoom derivative, this code will be
**    covered by the terms of the GNU General Public License as published by
**    the Free Software Foundation; either version 2 of the License, or (at
**    your option) any later version.
**
** THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
** IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
** OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
** IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
** INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
** NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
** THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**---------------------------------------------------------------------------
**
*/

#include "vm.h"

// We need one specific type for each of the 7 integral VM types and instantiate the needed functions for each of them.
// Dynamic arrays cannot hold structs because for every type there'd need to be an internal implementation which is impossible.

typedef TArray<uint8_t> FDynArray_I8;
typedef TArray<uint16_t> FDynArray_I16;
typedef TArray<uint32_t> FDynArray_I32;
typedef TArray<float> FDynArray_F32;
typedef TArray<double> FDynArray_F64;
typedef TArray<void*> FDynArray_Ptr;
typedef TArray<FString> FDynArray_String;

#define DEFINE_DYNARRAY_COPY_FUNCTION(cls) \
	DEFINE_ACTION_FUNCTION(cls, Copy)      \
	{                                      \
		PARAM_SELF_STRUCT_PROLOGUE(cls);   \
		PARAM_POINTER(other, cls);         \
		*self = *other;                    \
		return 0;                          \
	}

#define DEFINE_DYNARRAY_MOVE_FUNCTION(cls) \
	DEFINE_ACTION_FUNCTION(cls, Move)      \
	{                                      \
		PARAM_SELF_STRUCT_PROLOGUE(cls);   \
		PARAM_POINTER(other, cls);         \
		*self = std::move(*other);         \
		return 0;                          \
	}

#define DEFINE_DYNARRAY_POP_FUNCTION(cls) \
	DEFINE_ACTION_FUNCTION(cls, Pop)      \
	{                                     \
		PARAM_SELF_STRUCT_PROLOGUE(cls);  \
		ACTION_RETURN_BOOL(self->Pop());  \
	}

#define DEFINE_DYNARRAY_DELETE_FUNCTION(cls) \
	DEFINE_ACTION_FUNCTION(cls, Delete)      \
	{                                        \
		PARAM_SELF_STRUCT_PROLOGUE(cls);     \
		PARAM_INT(index);                    \
		PARAM_INT_DEF(count);                \
		self->Delete(index, count);          \
		return 0;                            \
	}

#define DEFINE_DYNARRAY_SHRINK_FUNCTION(cls) \
	DEFINE_ACTION_FUNCTION(cls, ShrinkToFit) \
	{                                        \
		PARAM_SELF_STRUCT_PROLOGUE(cls);     \
		self->ShrinkToFit();                 \
		return 0;                            \
	}

#define DEFINE_DYNARRAY_GROW_FUNCTION(cls)   \
	DEFINE_ACTION_FUNCTION(cls, Grow)        \
	{                                        \
		PARAM_SELF_STRUCT_PROLOGUE(cls);     \
		PARAM_INT(count);                    \
		self->Grow(count);                   \
		return 0;                            \
	}

#define DEFINE_DYNARRAY_RESIZE_FUNCTION(cls) \
	DEFINE_ACTION_FUNCTION(cls, Resize)      \
	{                                        \
		PARAM_SELF_STRUCT_PROLOGUE(cls);     \
		PARAM_INT(count);                    \
		self->Resize(count);                 \
		return 0;                            \
	}

#define DEFINE_DYNARRAY_RESERVE_FUNCTION(cls)    \
	DEFINE_ACTION_FUNCTION(cls, Reserve)         \
	{                                            \
		PARAM_SELF_STRUCT_PROLOGUE(cls);         \
		PARAM_INT(count);                        \
		ACTION_RETURN_INT(self->Reserve(count)); \
	}

#define DEFINE_DYNARRAY_MAX_FUNCTION(cls) \
	DEFINE_ACTION_FUNCTION(cls, Max)      \
	{                                     \
		PARAM_SELF_STRUCT_PROLOGUE(cls);  \
		ACTION_RETURN_INT(self->Max());   \
	}

#define DEFINE_DYNARRAY_CLEAR_FUNCTION(cls) \
	DEFINE_ACTION_FUNCTION(cls, Clear)      \
	{                                       \
		PARAM_SELF_STRUCT_PROLOGUE(cls);    \
		self->Clear();                      \
		return 0;                           \
	}

#define DEFINE_DYNARRAY_COMMON_FUNCTIONS(cls) \
	DEFINE_DYNARRAY_COPY_FUNCTION(cls)        \
	DEFINE_DYNARRAY_MOVE_FUNCTION(cls)        \
	DEFINE_DYNARRAY_POP_FUNCTION(cls)         \
	DEFINE_DYNARRAY_DELETE_FUNCTION(cls)      \
	DEFINE_DYNARRAY_SHRINK_FUNCTION(cls)      \
	DEFINE_DYNARRAY_GROW_FUNCTION(cls)        \
	DEFINE_DYNARRAY_RESIZE_FUNCTION(cls)      \
	DEFINE_DYNARRAY_RESERVE_FUNCTION(cls)     \
	DEFINE_DYNARRAY_MAX_FUNCTION(cls)         \
	DEFINE_DYNARRAY_CLEAR_FUNCTION(cls)

#define DEFINE_DYNARRAY_FIND_FUNCTION(cls, type) \
	DEFINE_ACTION_FUNCTION(cls, Find)            \
	{                                            \
		PARAM_SELF_STRUCT_PROLOGUE(cls);         \
		PARAM_##type(val);                       \
		ACTION_RETURN_INT(self->Find(val));      \
	}

#define DEFINE_DYNARRAY_PUSH_FUNCTION(cls, type) \
	DEFINE_ACTION_FUNCTION(cls, Push)            \
	{                                            \
		PARAM_SELF_STRUCT_PROLOGUE(cls);         \
		PARAM_##type(val);                       \
		ACTION_RETURN_INT(self->Push(val));      \
	}                                            \

#define DEFINE_DYNARRAY_INSERT_FUNCTION(cls, type) \
	DEFINE_ACTION_FUNCTION(cls, Insert)            \
	{                                              \
		PARAM_SELF_STRUCT_PROLOGUE(cls);           \
		PARAM_INT(index);                          \
		PARAM_##type(val);                         \
		self->Insert(index, val);                  \
		return 0;                                  \
	}

#define DEFINE_DYNARRAY_TYPED_FUNCTIONS(cls, type) \
	DEFINE_DYNARRAY_FIND_FUNCTION(cls, type)       \
	DEFINE_DYNARRAY_PUSH_FUNCTION(cls, type)       \
	DEFINE_DYNARRAY_INSERT_FUNCTION(cls, type)

#define DEFINE_DYNARRAY_FUNCTIONS(cls, type)   \
	DEFINE_DYNARRAY_COMMON_FUNCTIONS(cls)      \
	DEFINE_DYNARRAY_TYPED_FUNCTIONS(cls, type)

#define DEFINE_DYNARRAY(cls, type)                 \
	DEFINE_DYNARRAY_FUNCTIONS(F##cls, type)        \
	DEFINE_FIELD_NAMED_X(cls, FArray, Count, Size)

#define PARAM_VOIDPTR(x) PARAM_POINTER(x, void)

DEFINE_DYNARRAY(DynArray_I8, INT)
DEFINE_DYNARRAY(DynArray_I16, INT)
DEFINE_DYNARRAY(DynArray_I32, INT)
DEFINE_DYNARRAY(DynArray_F32, FLOAT)
DEFINE_DYNARRAY(DynArray_F64, FLOAT)
DEFINE_DYNARRAY(DynArray_Ptr, VOIDPTR)
DEFINE_DYNARRAY(DynArray_String, STRING)
