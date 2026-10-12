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
 * OSS GraphQL adapter.
 *
 * There is no public Intern GraphQL backend, and the app executes no GraphQL
 * queries today. This returns 501 so the /api/graphql route exists and the Relay
 * network layer fails loudly if a query is ever added. The internal adapter
 * re-exports @nest/interngraph's createGraphQLHandler.
 */

import {NextResponse} from 'next/server';

export function createGraphQLHandler(_config?: {
  // Accepted for API parity with @nest/interngraph's createGraphQLHandler.
  // The OSS build has no GraphQL backend, so auth options are ignored here.
  useOAuthToken?: boolean;
}) {
  return async function POST(): Promise<NextResponse> {
    return NextResponse.json(
      {errors: [{message: 'GraphQL is not available in the OSS build.'}]},
      {status: 501},
    );
  };
}
