import {createGraphQLHandler} from '@platform/graphql';

export const POST = createGraphQLHandler({useOAuthToken: true});
