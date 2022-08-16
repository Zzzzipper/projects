<template>
  <LayoutDefault>
    
    <div>Im license</div>
  </LayoutDefault>
</template>

<script lang="ts">
import { defineComponent } from "vue";
import { LayoutDefault } from "../../layouts/layout-default";
import { LogoutOutlined } from "@ant-design/icons-vue";

export default defineComponent({
  name: "PageLicense",

  components: {
    LayoutDefault,
    LogoutOutlined,
  },
});
</script>
