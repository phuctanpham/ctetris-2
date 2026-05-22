// =============================================================================
// api/src/index.ts -- cTetris Cloudflare Worker API
// =============================================================================
// Routes:
//   GET  /leaderboard?limit=N   → top N rows from sync_records (default 50, max 100)
//   POST /record                → best-only upsert of a game record: keeps one
//                                 row per (user, story, chapter), overwriting
//                                 only when the new score is higher.
//                                 Requires Authorization header.
//   GET  /health                → {ok: true}
//
// Auth: POST /record requires "Authorization: Bearer <token>" header.
// Token is issued by 3rd-party OTP service — Worker only checks it is non-empty.
// All other validation is on the payload fields.
// =============================================================================

export interface Env {
	DB: D1Database;
}

// ---------------------------------------------------------------------------
// CORS headers -- allow WASM origin (GitHub Pages) + localhost dev
// ---------------------------------------------------------------------------
const CORS_HEADERS: Record<string, string> = {
	"Access-Control-Allow-Origin":  "*",
	"Access-Control-Allow-Methods": "GET, POST, OPTIONS",
	"Access-Control-Allow-Headers": "Content-Type, Authorization",
	"Access-Control-Max-Age":       "86400",
};

function jsonResponse(body: unknown, status = 200): Response {
	return new Response(JSON.stringify(body), {
		status,
		headers: { "Content-Type": "application/json", ...CORS_HEADERS },
	});
}

function errorResponse(message: string, status = 400): Response {
	return jsonResponse({ ok: false, error: message }, status);
}

// ---------------------------------------------------------------------------
// Route: GET /leaderboard
// ---------------------------------------------------------------------------
async function handleLeaderboard(request: Request, env: Env): Promise<Response> {
	const url    = new URL(request.url);
	let   limit  = parseInt(url.searchParams.get("limit") ?? "50", 10);
	if (isNaN(limit) || limit < 1)   limit = 50;
	if (limit > 100)                  limit = 100;

	const { results } = await env.DB.prepare(
		`SELECT name_user, total_score, total_seconds, avg_speed, end_ts,
		        id_story, id_chapter
		 FROM   sync_records
		 ORDER  BY total_score DESC
		 LIMIT  ?`
	).bind(limit).all();

	return jsonResponse({ ok: true, data: results });
}

// ---------------------------------------------------------------------------
// Route: POST /record
// Body: { nameUser, totalScore, totalSeconds, avgSpeed, endTs, idStory, idChapter }
// ---------------------------------------------------------------------------
interface RecordBody {
	nameUser:     string;
	totalScore:   number;
	totalSeconds: number;
	avgSpeed:     number;
	endTs:        number;
	idStory:      number;
	idChapter:    number;
}

async function handlePostRecord(request: Request, env: Env): Promise<Response> {
	// Auth: 3rd-party OTP service issues the token; we just require it present.
	const auth  = request.headers.get("Authorization") ?? "";
	const token = auth.startsWith("Bearer ") ? auth.slice(7).trim() : "";
	if (!token) {
		return errorResponse("Missing or invalid Authorization header", 401);
	}

	let body: RecordBody;
	try {
		body = await request.json() as RecordBody;
	} catch {
		return errorResponse("Invalid JSON body");
	}

	// Validate required fields
	const { nameUser, totalScore, totalSeconds, avgSpeed, endTs, idStory, idChapter } = body;

	if (!nameUser || typeof nameUser !== "string" || nameUser.trim().length === 0) {
		return errorResponse("nameUser is required");
	}
	if (typeof totalScore   !== "number" || totalScore   < 0) return errorResponse("invalid totalScore");
	if (typeof totalSeconds !== "number" || totalSeconds < 0) return errorResponse("invalid totalSeconds");
	if (typeof avgSpeed     !== "number" || avgSpeed     < 0) return errorResponse("invalid avgSpeed");
	if (typeof endTs        !== "number" || endTs        <= 0) return errorResponse("invalid endTs");
	if (typeof idStory      !== "number") return errorResponse("invalid idStory");
	if (typeof idChapter    !== "number") return errorResponse("invalid idChapter");

	// Cap score to match client-side cap of 99999
	const score = Math.min(Math.round(totalScore), 99999);

	// Best-only upsert: keep a single row per (user, story, chapter) and only
	// overwrite when the new score beats the stored best. The WHERE on the
	// DO UPDATE makes a not-better submission a no-op. Relies on the unique
	// index from migrations/best-only-upsert.sql.
	const result = await env.DB.prepare(
		`INSERT INTO sync_records
		   (name_user, total_score, total_seconds, avg_speed, end_ts, id_story, id_chapter)
		 VALUES (?, ?, ?, ?, ?, ?, ?)
		 ON CONFLICT(name_user, id_story, id_chapter) DO UPDATE SET
		   total_score   = excluded.total_score,
		   total_seconds = excluded.total_seconds,
		   avg_speed     = excluded.avg_speed,
		   end_ts        = excluded.end_ts
		 WHERE excluded.total_score > sync_records.total_score`
	).bind(
		nameUser.trim().slice(0, 32),   // max 32 chars
		score,
		Math.round(totalSeconds),
		Math.round(avgSpeed * 100) / 100,
		Math.round(endTs),
		Math.round(idStory),
		Math.round(idChapter),
	).run();

	// changes > 0 => inserted or improved; 0 => existing best kept.
	const improved = (result.meta?.changes ?? 0) > 0;
	return jsonResponse({ ok: true, improved }, 201);
}

// ---------------------------------------------------------------------------
// Main fetch handler
// ---------------------------------------------------------------------------
export default {
	async fetch(request: Request, env: Env): Promise<Response> {
		const url    = new URL(request.url);
		const method = request.method.toUpperCase();

		// Preflight
		if (method === "OPTIONS") {
			return new Response(null, { status: 204, headers: CORS_HEADERS });
		}

		if (method === "GET" && url.pathname === "/health") {
			return jsonResponse({ ok: true });
		}

		if (method === "GET" && url.pathname === "/leaderboard") {
			return handleLeaderboard(request, env);
		}

		if (method === "POST" && url.pathname === "/record") {
			return handlePostRecord(request, env);
		}

		return errorResponse("Not found", 404);
	},
} satisfies ExportedHandler<Env>;
