<template>
  <a-spin :indicator="indicator" />
</template>

<script lang="ts">
import { LoadingOutlined } from '@ant-design/icons-vue';
import { defineComponent, h } from 'vue';

export default defineComponent({
  setup () {
    const indicator = h(LoadingOutlined, {
      style: {
        fontSize: '24px'
      },
      spin: true
    });
    return {
      indicator
    };
  }
});
</script>
