#include "morgutest.h"

#include "unitDynarray.h"
#include "unitEvent.h"

int main(void) {
    // DynArray tests
    M_RunTest(TestDynarrayAlloc);
    M_RunTest(TestDynarrayRealloc);
    M_RunTest(TestDynarrayPush);
    M_RunTest(TestDynarrayRemove);
    M_RunTest(TestDynarrayFind);

    // Event tests
    M_RunTest(TestEventInit);
    M_RunTest(TestEventFire);
    M_RunTest(TestEventRemove);

    return M_Report();
}