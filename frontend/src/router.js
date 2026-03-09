import { createRouter, createWebHistory } from "vue-router";
import DashboardPage from "./views/DashboardPage.vue";
import MajorsPage from "./views/MajorsPage.vue";
import MajorDetailPage from "./views/MajorDetailPage.vue";
import JobsPage from "./views/JobsPage.vue";
import MatchPage from "./views/MatchPage.vue";
import LoginPage from "./views/LoginPage.vue";
import RegisterPage from "./views/RegisterPage.vue";
import PasswordPage from "./views/PasswordPage.vue";
import ProfilePage from "./views/ProfilePage.vue";
import { pinia } from "./stores";
import { useAuthStore } from "./stores/auth";

const routes = [
  { path: "/login", name: "login", component: LoginPage, meta: { requiresAuth: false } },
  { path: "/register", name: "register", component: RegisterPage, meta: { requiresAuth: false } },
  { path: "/password", name: "password", component: PasswordPage, meta: { requiresAuth: false } },
  { path: "/", redirect: "/majors" },
  { path: "/profile", name: "profile", component: ProfilePage, meta: { requiresAuth: true } },
  { path: "/match", name: "match", component: MatchPage, meta: { requiresAuth: true } },
  { path: "/majors", name: "majors", component: MajorsPage, meta: { requiresAuth: true } },
  { path: "/majors/:id", name: "major-detail", component: MajorDetailPage, meta: { requiresAuth: true } },
  { path: "/jobs", name: "jobs", component: JobsPage, meta: { requiresAuth: true } },
  { path: "/dashboard", name: "dashboard", component: DashboardPage, meta: { requiresAuth: true } },
  { path: "/students", redirect: "/match" }
];

const router = createRouter({
  history: createWebHistory(),
  routes
});

function getHomeRouteByRole(role) {
  if (role === "viewer") {
    return { name: "profile" };
  }
  return { name: "majors" };
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
