import { Hono } from "hono";
import { proxy } from "hono/proxy";
import { logger } from "hono/logger";
import { serveStatic } from "hono/deno";

const app = new Hono({ strict: false });

app.use(logger());

app.get(
  "/api/*",
  (c) => proxy(`http://localhost:8080${c.req.path}`),
);

app.on(
  "get",
  ["/", "/sample-viewer", "/sample-viewer.html"],
  serveStatic({ path: "./scripts/sample-viewer.html" }),
);

export default app satisfies Deno.ServeDefaultExport;
