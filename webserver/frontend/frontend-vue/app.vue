<template>
  <RouterView>
    <template #default="{ Component }">
      <Suspense :timeout="0">
        <template #default>
          <AuthChecker>
            <component :is="Component" />
          </AuthChecker>
        </template>
        <template #fallback> Im loading </template>
      </Suspense>
    </template>
  </RouterView>
</template>

<script lang="ts">
import { defineComponent, defineAsyncComponent, h, VNode } from "vue";
import { RouterView, useRoute, useRouter } from "vue-router";
import BackendApiService from "./src/services/backend-api-service";
import config from "./src/config";

const backendApiService = new BackendApiService(config.backendApiUrl);

const AuthChecker = defineAsyncComponent(async () => {
  const route = useRoute();
  const router = useRouter();
  await router.isReady();
  const isAuthenticated = await backendApiService.getAuthenticationStatus();

  if (isAuthenticated && route.fullPath.startsWith("/authentication")) {
    await router.replace("/");
  }

  if (!isAuthenticated && !route.fullPath.startsWith("/authentication")) {
    await router.replace("/authentication");
  }

  return {
    render(): VNode {
      return h(this.$slots.default);
    },
  };
});

export default defineComponent({
  components: {
    RouterView,
    AuthChecker,
  },
});
</script>
