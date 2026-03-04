import { createRouter, createWebHistory } from "vue-router";
import DashboardPage from "./views/DashboardPage.vue";
import MajorsPage from "./views/MajorsPage.vue";
import JobsPage from "./views/JobsPage.vue";
import StudentsPage from "./views/StudentsPage.vue";
import MatchPage from "./views/MatchPage.vue";
import LoginPage from "./views/LoginPage.vue";
import { pinia } from "./stores";
import { useAuthStore } from "./stores/auth";

const routes = [
  { path: "/login", name: "login", component: LoginPage, meta: { requiresAuth: false } },
  { path: "/", name: "dashboard", component: DashboardPage, meta: { requiresAuth: true } },
  { path: "/majors", name: "majors", component: MajorsPage, meta: { requiresAuth: true } },
  { path: "/jobs", name: "jobs", component: JobsPage, meta: { requiresAuth: true } },
  { path: "/students", name: "students", component: StudentsPage, meta: { requiresAuth: true } },
  { path: "/match", name: "match", component: MatchPage, meta: { requiresAuth: true } }
];

const router = createRouter({
  history: createWebHistory(),
  routes
});

router.beforeEach(async (to) => {
  const authStore = useAuthStore(pinia);
  await authStore.bootstrap();

  const requiresAuth = to.matched.some((record) => record.meta.requiresAuth);
  if (requiresAuth && !authStore.isAuthenticated) {
    return { name: "login", query: { redirect: to.fullPath } };
  }
  if (to.name === "login" && authStore.isAuthenticated) {
    return { name: "dashboard" };
  }
  return true;
});

export default router;
