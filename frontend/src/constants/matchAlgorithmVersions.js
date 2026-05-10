export const MATCH_ALGORITHM_OPTIONS = [
  { value: "v0.1-baseline", label: "v0.1-baseline（基线）" },
  { value: "v0.2-major-job", label: "v0.2-major-job（主流程）" },
  { value: "v1.0-hybrid-idf", label: "v1.0-hybrid-idf（IDF增强）" },
  { value: "v1.1-hybrid-emb", label: "v1.1-hybrid-emb（Embedding增强）" },
  { value: "v0.4-major-student", label: "v0.4-major-student（历史兼容）" }
];

export const MATCH_ALGORITHM_FILTER_OPTIONS = [
  { value: "", label: "默认（latest）" },
  ...MATCH_ALGORITHM_OPTIONS
];

export const DEFAULT_MAJOR_JOB_ALGORITHM = "v0.2-major-job";
export const DEFAULT_STUDENT_JOB_ALGORITHM = "v0.4-major-student";

const ALLOWED_ALGORITHM_SET = new Set(MATCH_ALGORITHM_OPTIONS.map((item) => item.value));

export function normalizeMatchAlgorithmVersion(value, fallback = DEFAULT_MAJOR_JOB_ALGORITHM) {
  const normalized = String(value || "").trim();
  if (ALLOWED_ALGORITHM_SET.has(normalized)) {
    return normalized;
  }
  return fallback;
}
