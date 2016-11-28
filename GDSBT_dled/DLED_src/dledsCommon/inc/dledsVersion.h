/************************************************************************/
/*  (C) COPYRIGHT 2008 by Bombardier Transportation                     */
/*                                                                      */
/*  Bombardier Transportation Switzerland Dep. PPC/EUT                  */
/************************************************************************/
/*                                                                      */
/*  PROJECT:      Remote download                                       */
/*                                                                      */
/*  MODULE:       dleds                                                 */
/*                                                                      */
/*  ABSTRACT:     This header file builds the DLEDS SW version string.  */
/*                                                                      */
/*                                                                      */
/*                                                                      */
/*  REMARKS:                                                            */
/*                                                                      */
/*  DEPENDENCIES: See include list                                      */
/*                                                                      */
/*  ACCEPTED:                                                           */
/*                                                                      */
/************************************************************************/
/*                                                                      */
/*  HISTORY:                                                            */
/*                                                                      */
/*  version  yy-mm-dd  name       depart.  ref   status                 */
/*  -------  --------  ---------  -------  ----  ---------              */
/*    1.0    10-04-06  S.Eriksson PPC/TET1S   --   created              */
/*                                                                      */
/************************************************************************/
#ifndef DLEDSVERSION_H
#define DLEDSVERSION_H

#define INTERN_STRINGIFY(x)         #x
#define STRINGIFY(x)                INTERN_STRINGIFY(x)

#define SW_VERSION_STRING           STRINGIFY(DLEDS_VER)

#endif /* DLEDSVERSION_H*/
