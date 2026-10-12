#include "event.h"

#include <stdarg.h>

#include "dynarray.h"

void EventInitialize(Event* ev)  {
    ev->CurrentId = 0;
    M_DynArrayInit(ev->Identifiers);
    M_DynArrayInit(ev->Subscribers);
}

void EventFire(Event* ev, int argc, ...) {
    va_list args;
    va_start(args, argc);

    for (size_t i = 0; i < M_DynArrayGetSize(ev->Subscribers); i++) {
        va_list copy;
        va_copy(copy, args);
        (ev->Subscribers)[i](argc, copy);
        va_end(copy);
    }

    va_end(args);
}

size_t EventAdd(Event* ev, Subscriber* subscriber) {
    ev->CurrentId++;
    M_DynArrayPush(ev->Identifiers, ev->CurrentId);
    M_DynArrayPush(ev->Subscribers, subscriber);
    return ev->CurrentId;
}

void EventRemove(Event* ev, size_t handle) {
    int idx;
    M_DynArrayFind(ev->Identifiers, handle, idx);
    if(idx != M_DYNARRAY_NOT_FOUND) {
        M_DynArrayRemove(ev->Identifiers, idx);
        M_DynArrayRemove(ev->Subscribers, idx);
    }
}

void EventFree(Event* ev) {
    M_DynArrayFree(ev->Identifiers);
    M_DynArrayFree(ev->Subscribers);
}