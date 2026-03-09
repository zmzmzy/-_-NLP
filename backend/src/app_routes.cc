#include "app_routes.h"

namespace gm {

void registerAllRoutes() {
    registerSystemRoutes();
    registerEducationRoutes();
    registerJobsRoutes();
    registerStudentEmploymentRoutes();
    registerMatchingRoutes();
    registerProfileRoutes();
}

}  // namespace gm
