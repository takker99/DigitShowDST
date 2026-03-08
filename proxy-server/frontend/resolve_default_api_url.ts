export const resolveDefaultApiUrl = () => {
  const hostname = globalThis?.location?.hostname ?? "";
  const isLocalhost = hostname === "localhost" ||
    hostname === "127.0.0.1" ||
    hostname === "::1" ||
    hostname === "[::1]";
  const isDenoNet = hostname.endsWith(".deno.net");

  return isLocalhost || isDenoNet
    ? globalThis.location.origin
    : "http://localhost:8080";
};
