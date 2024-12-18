/******************************************************************************/
/*
/*    NT Service Class
/*
/*    copyright 2003  , Stefan Voitel - Berlin / Germany
/*
/*    stefan.voitel@winways.de	
/******************************************************************************/

#ifndef TSERVICE_CLASS_INCLUDED
#define TSERVICE_CLASS_INCLUDED

#if ! defined (_WIN32_WINNT)
  #define _WIN32_WINNT  0x0400
#endif
  
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include "TService.h"
#include "TService.cpp"

#endif // TSERVICE_CLASS_INCLUDED
