import { Hono } from "hono";
import { proxy } from "hono/proxy";
import { logger } from "hono/logger";
import { serveStatic } from "hono/deno";
import { fromFileUrl } from "@std/path";

const app = new Hono({ strict: false });

app.use(logger());

app.get(
  "/api/*",
  (c) => proxy(`http://localhost:8080${c.req.path}`),
);

app.on(
  "get",
  ["/", "/index"],
  (c) => c.redirect("/index.html"),
);

app.get(
  "/*",
  serveStatic({
    root: fromFileUrl(new URL("./dist", import.meta.url)),
    precompressed: true,
  }),
);

export default app satisfies Deno.ServeDefaultExport;
