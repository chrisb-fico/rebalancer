/**
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/**
 * @format
 *
 * OSS auth middleware adapter — pass-through (no SSO enforcement).
 *
 * Kept in a separate, non-"use client" module from auth.tsx because Next.js
 * middleware (proxy.ts) runs in the edge runtime and must not import a client
 * module. The internal counterpart re-exports @nest/intern-auth's
 * createAuthMiddleware.
 */

import {NextResponse, type NextRequest} from 'next/server';

export function createAuthMiddleware(_options?: {
  // Accepted for API parity with @nest/intern-auth's createAuthMiddleware.
  // The OSS build enforces no SSO, so OIDC options are ignored here.
  useOIDC?: true;
  publicRoutes?: string[];
}) {
  return function middleware(_request: NextRequest): NextResponse {
    return NextResponse.next();
  };
}
