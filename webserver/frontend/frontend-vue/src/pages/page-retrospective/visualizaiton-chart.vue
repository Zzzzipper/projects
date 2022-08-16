<template>
  <div ref="rootRef" class="page-retrospective-charts">
    <template v-for="storeChart in storeCharts">
      <canvas
        :key="storeChart.id"
        :ref="(el) => el ? chartRefs[storeChart.id]= el : undefined"
        @mouseenter="enableCrosshairs(storeChart.id)"
        @mouseleave="disableCrosshairs(storeChart.id)"
        @mousemove="setCrosshairsCoordinates($event, storeChart.id)"
      />
    </template>
  </div>
</template>

<script lang="ts">
import { defineComponent, onMounted, ref } from 'vue';
import { useStore } from '../../services/store-service';
import { assert } from '../../utils';

export default defineComponent({
  name: 'VisualizationChart',

  emits: {
    canvasesReady: (_: {
      containerWidth: number,
      containerHeight: number,
      canvases: {[id: string]: HTMLCanvasElement}
    }) => true,
    crosshairsEnable: (_: string) => true,
    crosshairsDisable: (_: string) => true,
    crosshairsSetCoordinates: (_:{ x: number, y: number, chartId: string }) => true
  },

  setup (_, context) {
    // Data binds
    const dataBinds = {
      chartRefs: ref<{[id: string]: HTMLCanvasElement}>([]),
      rootRef: ref<HTMLDivElement>()
    };

    // Crosshair Binds
    const crosshairBinds = {
      enableCrosshairs: (chartId: string) => context.emit('crosshairsEnable', chartId),
      disableCrosshairs: (chartId: string) => context.emit('crosshairsDisable', chartId),
      setCrosshairsCoordinates: (event: MouseEvent, chartId: string) =>
        context.emit('crosshairsSetCoordinates', { x: event.layerX, y: event.layerY, chartId })
    };

    // Store binds
    const store = useStore();
    const storeBinds = {
      storeCharts: store.state.retrospective.charts
    };

    onMounted(async () => {
      assert(dataBinds.rootRef.value !== undefined, 'rootRef is undefined');
      const containerWidth = dataBinds.rootRef.value?.clientWidth as number;
      const containerHeight = dataBinds.rootRef.value?.clientHeight as number;

      context.emit('canvasesReady', {
        containerWidth,
        containerHeight,
        canvases: dataBinds.chartRefs.value
      });
    });

    return {
      ...crosshairBinds,
      ...storeBinds,
      ...dataBinds
    };
  }
});
</script>

<style>
.page-retrospective-charts {
  flex-grow: 1;
}
</style>
