#pragma once

#include <stdarg.h>
#include <stddef.h>

typedef void Subscriber(int, va_list);

typedef struct Event {
    size_t CurrentId;
    size_t* Identifiers;
    Subscriber** Subscribers;
} Event;

void EventInitialize(Event* ev);
size_t EventAdd(Event* ev, Subscriber* subscriber);
void EventFire(Event* ev, int argc, ...);
void EventRemove(Event* ev, size_t handle);
void EventFree(Event* ev);