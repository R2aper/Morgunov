#include "unitEvent.h"

#include <stddef.h>
#include <stdarg.h>

#include "src/common/event.h"
#include "src/common/dynarray.h"
#include "morgutest.h"

static int _TestGlobalVariable = 0;

static void _TestFunc(int argc, va_list ls) {
    _TestGlobalVariable = va_arg(ls, int);
}

void TestEventInit(void) {
    Event event;
    EventInitialize(&event);

    M_Assert(M_DynArrayGetSize(event.Subscribers) == 0);

    EventFree(&event);
}


void TestEventFire(void) {
    Event event;
    EventInitialize(&event);

    EventAdd(&event, &_TestFunc);
    M_Assert(M_DynArrayGetSize(event.Subscribers) == 1);
    EventFire(&event, 1, 1);
    M_Assert(_TestGlobalVariable == 1);

    EventFree(&event);
}

void TestEventRemove(void) {
    Event event;
    EventInitialize(&event);

    size_t id = EventAdd(&event, &_TestFunc);
    M_Assert(M_DynArrayGetSize(event.Subscribers) == 1);

    EventRemove(&event, id);
    M_Assert(M_DynArrayGetSize(event.Subscribers) == 0);

    EventFree(&event);
}