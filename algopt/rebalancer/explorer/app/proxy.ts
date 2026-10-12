import {createAuthMiddleware} from '@platform/auth-middleware';

export default createAuthMiddleware({
  useOIDC: true,
  publicRoutes: [
    '/',
    '/api/health',
    '/api/console-logs',
    '/api/console-bridge-script',
    '/login',
    '/logged-out',
  ],
});

export const config = {
  matcher: [
    '/((?!_next/static|_next/image|favicon.ico|.*\\.(?:svg|png|jpg|jpeg|gif|webp)$).*)',
  ],
};
