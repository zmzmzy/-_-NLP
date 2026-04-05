import { createRouter, createWebHistory } from "vue-router";
import DashboardPage from "./views/DashboardPage.vue";
import MajorDetailPage from "./views/MajorDetailPage.vue";
import LoginPage from "./views/LoginPage.vue";
import RegisterPage from "./views/RegisterPage.vue";
import PasswordPage from "./views/PasswordPage.vue";
import ProfilePage from "./views/ProfilePage.vue";
import MajorCenterLayout from "./views/major-center/MajorCenterLayout.vue";
import MajorCenterOverviewPage from "./views/major-center/MajorCenterOverviewPage.vue";
import MajorCenterCollegesPage from "./views/major-center/MajorCenterCollegesPage.vue";
import MajorCenterMajorsPage from "./views/major-center/MajorCenterMajorsPage.vue";
import MajorCenterStudentsPage from "./views/major-center/MajorCenterStudentsPage.vue";
import MajorCenterImportsPage from "./views/major-center/MajorCenterImportsPage.vue";
import JobCenterLayout from "./views/job-center/JobCenterLayout.vue";
import JobCenterOverviewPage from "./views/job-center/JobCenterOverviewPage.vue";
import JobCenterCompaniesPage from "./views/job-center/JobCenterCompaniesPage.vue";
import JobCenterJobsPage from "./views/job-center/JobCenterJobsPage.vue";
import JobCenterImportsPage from "./views/job-center/JobCenterImportsPage.vue";
import MatchCenterLayout from "./views/match-center/MatchCenterLayout.vue";
import MatchCenterOverviewPage from "./views/match-center/MatchCenterOverviewPage.vue";
import MatchCenterSinglePage from "./views/match-center/MatchCenterSinglePage.vue";
import MatchCenterMultiPage from "./views/match-center/MatchCenterMultiPage.vue";
import MatchCenterBatchPage from "./views/match-center/MatchCenterBatchPage.vue";
import MatchCenterHistoryPage from "./views/match-center/MatchCenterHistoryPage.vue";
import DashboardCenterLayout from "./views/dashboard-center/DashboardCenterLayout.vue";
import DashboardCenterOverviewPage from "./views/dashboard-center/DashboardCenterOverviewPage.vue";
import DashboardCenterRankingPage from "./views/dashboard-center/DashboardCenterRankingPage.vue";
import DashboardCenterGapsPage from "./views/dashboard-center/DashboardCenterGapsPage.vue";
import DashboardCenterInsightPage from "./views/dashboard-center/DashboardCenterInsightPage.vue";
import DashboardCenterHistoryPage from "./views/dashboard-center/DashboardCenterHistoryPage.vue";
import { pinia } from "./stores";
import { useAuthStore } from "./stores/auth";

const routes = [
  { path: "/login", name: "login", component: LoginPage, meta: { requiresAuth: false } },
  { path: "/register", name: "register", component: RegisterPage, meta: { requiresAuth: false } },
  { path: "/password", name: "password", component: PasswordPage, meta: { requiresAuth: false } },
  { path: "/", redirect: "/major-center/overview" },
  { path: "/profile", name: "profile", component: ProfilePage, meta: { requiresAuth: true } },
  {
    path: "/match",
    component: MatchCenterLayout,
    meta: { requiresAuth: true },
    children: [
      { path: "", redirect: { name: "match-center-overview" } },
      { path: "overview", name: "match-center-overview", component: MatchCenterOverviewPage },
      { path: "single", name: "match-center-single", component: MatchCenterSinglePage },
      { path: "multi", name: "match-center-multi", component: MatchCenterMultiPage },
      { path: "batch", name: "match-center-batch", component: MatchCenterBatchPage },
      { path: "history", name: "match-center-history", component: MatchCenterHistoryPage }
    ]
  },
  {
    path: "/major-center",
    component: MajorCenterLayout,
    meta: { requiresAuth: true },
    children: [
      { path: "", redirect: { name: "major-center-overview" } },
      { path: "overview", name: "major-center-overview", component: MajorCenterOverviewPage },
      { path: "colleges", name: "major-center-colleges", component: MajorCenterCollegesPage },
      { path: "majors", name: "major-center-majors", component: MajorCenterMajorsPage },
      { path: "students", name: "major-center-students", component: MajorCenterStudentsPage },
      { path: "imports", name: "major-center-imports", component: MajorCenterImportsPage }
    ]
  },
  {
    path: "/job-center",
    component: JobCenterLayout,
    meta: { requiresAuth: true },
    children: [
      { path: "", redirect: { name: "job-center-overview" } },
      { path: "overview", name: "job-center-overview", component: JobCenterOverviewPage },
      { path: "companies", name: "job-center-companies", component: JobCenterCompaniesPage },
      { path: "jobs", name: "job-center-jobs", component: JobCenterJobsPage },
      { path: "imports", name: "job-center-imports", component: JobCenterImportsPage }
    ]
  },
  { path: "/majors", redirect: "/major-center/overview" },
  { path: "/majors/:id", name: "major-detail", component: MajorDetailPage, meta: { requiresAuth: true } },
  { path: "/jobs", redirect: "/job-center/overview" },
  {
    path: "/dashboard",
    component: DashboardCenterLayout,
    meta: { requiresAuth: true },
    children: [
      { path: "", redirect: { name: "dashboard-center-overview" } },
      { path: "overview", name: "dashboard-center-overview", component: DashboardCenterOverviewPage },
      { path: "ranking", name: "dashboard-center-ranking", component: DashboardCenterRankingPage },
      { path: "gaps", name: "dashboard-center-gaps", component: DashboardCenterGapsPage },
      { path: "insight", name: "dashboard-center-insight", component: DashboardCenterInsightPage },
      { path: "history", name: "dashboard-center-history", component: DashboardCenterHistoryPage },
      { path: "legacy", name: "dashboard-legacy", component: DashboardPage }
    ]
  },
  { path: "/students", redirect: "/major-center/students" }
];

const router = createRouter({
  history: createWebHistory(),
  routes
});

function getHomeRouteByRole(role) {
  if (role === "viewer") {
    return { name: "profile" };
  }
  return { name: "major-center-overview" };
}

router.beforeEach(async (to) => {
  const authStore = useAuthStore(pinia);
  await authStore.bootstrap();

  const requiresAuth = to.matched.some((record) => record.meta.requiresAuth);
  if (requiresAuth && !authStore.isAuthenticated) {
    return { name: "login", query: { redirect: to.fullPath } };
  }
  if (authStore.isAuthenticated && (to.name === "login" || to.name === "register" || to.name === "password")) {
    return getHomeRouteByRole(authStore.user?.role);
  }
  if (authStore.isAuthenticated && authStore.user?.role === "viewer" && to.name !== "profile") {
    return { name: "profile" };
  }
  return true;
});

export default router;
