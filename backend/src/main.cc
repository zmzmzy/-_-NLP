#include <drogon/drogon.h>

#include "app_bootstrap.h"
#include "app_routes.h"

int main() {
    gm::configureApplication();
    gm::registerAllRoutes();

    drogon::app().run();
    return 0;
}
