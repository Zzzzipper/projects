<template>
  <div class="layout-default">
    <ALayout :has-sider="true">
      <ALayoutSider breakpoint="lg" collapsed-width="0">
        <AMenu
          v-if="isMenuLoaded"
          v-model:selectedKeys="selectedKeys"
          v-model:openKeys="openKeys"
          theme="dark"
          mode="inline"
          @openChange="onOpenChange"
        >
          <ASubMenu key="retrospective">
            <template #title>
              <span>
                <LineChartOutlined />
                <span>Ретроспектива</span>
              </span>
            </template>

            <template #default>
              <AMenuItem
                v-for="retrospectiveItem in menu.retrospective?.items"
                :key="retrospectiveItem.fileId"
              >
                <RouterLink
                  :to="`/retrospective/?fileId=${retrospectiveItem.fileId}`"
                >
                  {{ retrospectiveItem.name }}
                </RouterLink>
              </AMenuItem>
            </template>
          </ASubMenu>

          <AMenuItem key="liсense">
            <RouterLink :to="`/license`"> </RouterLink>
            <SafetyCertificateOutlined />
            <span>Лицензии</span>
          </AMenuItem>

          <AMenuItem key="logout" @click="logout">
            <LogoutOutlined />
            <span>Выйти</span>
          </AMenuItem>
        </AMenu>

        <ASkeleton v-else active :style="{ padding: '0 20px' }" />
      </ALayoutSider>

      <ALayout>
        <ALayoutContent :style="{ display: 'flex', flexDirection: 'column' }">
          <slot />
        </ALayoutContent>
      </ALayout>
    </ALayout>
  </div>
</template>

<script lang="ts">
import {
  LineChartOutlined,
  LogoutOutlined,
  SafetyCertificateOutlined,
} from "@ant-design/icons-vue";
import { defineComponent, toRefs, reactive, ref } from "vue";
import { RouterLink } from "vue-router";
import { message } from "ant-design-vue";
import BackendApiService from "../../services/backend-api-service";
import config from "../../config";

const backendApiService = new BackendApiService(config.backendApiUrl);
export default defineComponent({
  components: {
    LineChartOutlined,
    LogoutOutlined,
    SafetyCertificateOutlined,
    RouterLink,
  },

  setup() {
    const isMenuLoaded = ref(false);
    const menu = reactive({});

    backendApiService.loadModule().then((response) => {
      backendApiService.getMenuTest();
      backendApiService.getRetrospectiveConfiguration('')
    });

    // @todo: add error handling
    // backendApiService.getMenu().then((menuEntity) => {
    //   Object.assign(menu, menuEntity);
    //   isMenuLoaded.value = true;
    //   console.log(menu)
    // });

    const state = reactive({
      rootSubmenuKeys: [],
      openKeys: [],
      selectedKeys: [],
    });

    const onOpenChange = (openKeys: string[]) => {
      const latestOpenKey = openKeys.find(
        (key) => !state.openKeys.includes(key)
      );
      if (!state.rootSubmenuKeys.includes(latestOpenKey!)) {
        state.openKeys = openKeys;
      } else {
        state.openKeys = latestOpenKey ? [latestOpenKey] : [];
      }
    };

    return {
      ...toRefs(state),
      onOpenChange,
      isMenuLoaded,
      menu,
    };
  },

  methods: {
    async logout() {
      try {
        await backendApiService.logout();
        await this.$router.push("/authentication");
      } catch (e) {
        message.error("Произошла неизвестная ошибка при выходе");
      }
    },
  },
});
</script>

<style>
.layout-default {
  flex-grow: 1;
  display: flex;
  flex-direction: column;
}
</style>
