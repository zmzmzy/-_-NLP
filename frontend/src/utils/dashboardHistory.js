const STORAGE_KEY = "gm_dashboard_history_v1";
const MAX_ITEMS = 200;

function safeParse(raw) {
  if (!raw) {
    return [];
  }
  try {
    const parsed = JSON.parse(raw);
    if (Array.isArray(parsed)) {
      return parsed;
    }
  } catch (_err) {
    // ignore parse errors and fallback to empty list
  }
  return [];
}

function readList() {
  return safeParse(localStorage.getItem(STORAGE_KEY));
}

function writeList(list) {
  localStorage.setItem(STORAGE_KEY, JSON.stringify(list));
}

function nextId() {
  return Date.now() * 1000 + Math.floor(Math.random() * 1000);
}

export function listDashboardHistory() {
  const list = readList();
  return [...list].sort((a, b) => Number(b.id || 0) - Number(a.id || 0));
}

export function addDashboardHistory(entry) {
  const row = {
    id: nextId(),
    type: String(entry?.type || "overview"),
    title: String(entry?.title || "看板操作"),
    status: String(entry?.status || "success"),
    summary: String(entry?.summary || ""),
    params: entry?.params || {},
    created_at: new Date().toISOString()
  };
  const list = readList();
  list.unshift(row);
  if (list.length > MAX_ITEMS) {
    list.length = MAX_ITEMS;
  }
  writeList(list);
  return row;
}

export function removeDashboardHistory(id) {
  const target = Number(id || 0);
  const list = readList().filter((item) => Number(item.id || 0) !== target);
  writeList(list);
}

export function clearDashboardHistory() {
  writeList([]);
}
