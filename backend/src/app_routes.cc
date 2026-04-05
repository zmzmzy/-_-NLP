#include "app_routes.h"

namespace gm {

void registerAllRoutes() {
    registerSystemRoutes();
    registerEducationRoutes();
    registerJobsRoutes();
    registerStudentEmploymentRoutes();
    registerMatchingRoutes();
    registerDashboardInsightRoutes();
    registerProfileRoutes();
}

}  // namespace gm
