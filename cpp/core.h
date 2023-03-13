#ifndef CORE_H__23_09_13__11_39
#define CORE_H__23_09_13__11_39

#ifdef CPU_SAME53

#define	CORETYPE_SAME53
#define CORTEX_M4

#include "same53.h"

#elif defined(CPU_XMC48)

#define	CORETYPE_XMC4800
#define CORTEX_M4

#include "XMC4800.h"

#elif defined(CPU_LPC824)

#define	CORETYPE_LPC82X

#define	CORETYPE_LPC82X
#define CORTEX_M0

#include "lpc82x.h"

#elif defined(CPU_LPC8XX)

#define	CORETYPE_LPC82X

#define	CORETYPE_LPC82X
#define CORTEX_M0

#include "lpc82x.h"

#endif

#endif // CORE_H__23_09_13__11_39
